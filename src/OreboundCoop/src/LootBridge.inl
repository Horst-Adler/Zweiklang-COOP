// Native inventories stay on the game thread. Only explicit IDs/counts travel
// over the reliable channel; no engine pointers or raw native structs do.
namespace lo=loot;
struct LootSource {
    zCVob* vob=nullptr;oCNpc* corpse=nullptr;oCMobContainer* chest=nullptr;lo::Identity identity;
};
struct LootItemRef {oCItem* item=nullptr;lo::Entry entry;};
lo::Ledger lootLedger;lo::Snapshot lootSnapshot;
LootSource guestLootSource,clientLootSource;
std::vector<LootItemRef> guestLootItems,clientLootItems;
std::deque<co::Bytes> lootOutbox;
std::map<std::uint32_t,lo::Message> lootApplied;
std::uint32_t lootRequest=0,clientOpenRequest=0,clientLease=0,clientLootRevision=0,clientPendingTake=0;
unsigned long long lootClientWait=0,lootLastKeepAlive=0,lootLastTraffic=0;
bool lootNativeBypass=false,clientLootShown=false,clientLootClosing=false;
bool clientQuickLoot=false;

void ReleaseLootSource(LootSource& source){if(source.identity.kind==lo::Source::Merchant&&source.corpse){auto it=npcByPointer.find(source.corpse);if(it!=npcByPointer.end()){auto* model=zDYNAMIC_CAST<zCModel>(source.corpse->GetVisual());if(model)model->aniTransScale=it->second->rootScale;}}if(source.vob)source.vob->Release();source=LootSource{};}
void ReleaseLootItems(std::vector<LootItemRef>& items){for(auto& i:items)if(i.item)i.item->Release();items.clear();}
void LootLog(const char* event,const std::string& detail){diagnostics.Write(1,event,detail);}
bool LootJournal(const char* event,const lo::Message& m){
    const auto logPath=diagnostics.Path();const auto slash=logPath.find_last_of("/\\");
    if(logPath.empty()||slash==std::string::npos)return false;
    const auto path=logPath.substr(0,slash+1)+"loot-transactions.jsonl";
    HANDLE file=CreateFileA(path.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)return false;
    SYSTEMTIME t;GetSystemTime(&t);char date[40];sprintf_s(date,"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
    const auto name=diagnostics.Path().substr(diagnostics.Path().find_last_of("/\\")+1);
    const auto& source=HostCombat()?guestLootSource.identity:clientLootSource.identity;
    std::ostringstream s;s<<"{\"time_utc\":\""<<date<<"\",\"event\":\""<<event<<"\",\"session\":\""<<Diagnostics::Escape(name)<<"\",\"pid\":"<<GetCurrentProcessId()<<",\"request\":"<<m.request<<",\"lease\":"<<m.lease<<",\"revision\":"<<m.revision<<",\"token\":"<<m.item.token<<",\"instance\":"<<m.item.instance<<",\"amount\":"<<m.item.amount<<",\"status\":"<<static_cast<unsigned>(m.status)
      <<",\"source_kind\":"<<static_cast<unsigned>(source.kind)<<",\"source_npc\":"<<source.npc<<",\"source_name\":\""<<Diagnostics::Escape(source.name)<<"\",\"source_position\":["<<source.position[0]<<","<<source.position[1]<<","<<source.position[2]<<"]}\n";
    const auto line=s.str();DWORD written=0;const bool ok=WriteFile(file,line.data(),static_cast<DWORD>(line.size()),&written,nullptr)&&written==line.size()&&FlushFileBuffers(file);CloseHandle(file);return ok;
}
std::uint32_t NextLootRequest(){if(lootRequest==UINT32_MAX){combatFailure="Loot request sequence exhausted";return 0;}return ++lootRequest;}
void QueueLoot(const lo::Message& m){if(lootOutbox.size()>=128){combatFailure="Loot send queue full";return;}lootOutbox.push_back(lo::Encode(m));}
std::string LootStatusText(lo::Status s){
    switch(s){case lo::Status::Busy:return Ui("ui_ffc80ffb0bf3");case lo::Status::Locked:return Ui("ui_3b0cdc376883");case lo::Status::Range:return Ui("ui_fd1e4c4de9e2");case lo::Status::Empty:return Ui("ui_66712cecbfd2");case lo::Status::Stale:return Ui("ui_9af3a5d36bb9");default:return Ui("ui_5ae5f9cf43e0");}
}
static_assert(sizeof(oCItemContainer)==0xA0&&sizeof(oCNpcContainer)==0xA4&&sizeof(oCNpcInventory)==0xCC,"Loot native inventory ABI mismatch");
oCNpcContainer* NativeCorpseLootUI(){
#if defined(__G2A)
    // Verified against OpenDeadNpc in the supported 2.6 executable. The SDK's
    // stealcontainer at AB27DC is a DIFFERENT window (pickpocketing).
    return *reinterpret_cast<oCNpcContainer**>(0x00AB27E0);
#else
    return nullptr;
#endif
}
oCItemContainer* LootUI(const LootSource& source){
    if(source.identity.kind==lo::Source::Merchant)return WorldTradeUI(source.corpse);
    if(source.chest)return source.chest->items;
    auto* corpseUI=NativeCorpseLootUI();
    if(source.corpse&&corpseUI&&corpseUI->GetOwner()==source.corpse)return corpseUI;
    return nullptr;
}
bool LootUIOpen(const LootSource& source){auto* ui=LootUI(source);return ui&&ui->IsOpen();}
bool LootEligible(oCItem* item){return item&&item->GetInstance()>0&&item->amount>0&&!item->HasFlag(0x10)&&!item->HasFlag(0x40000000);}
std::vector<oCItem*> LootNativeItems(const LootSource& source){
    std::vector<oCItem*> out;
    auto* list=source.corpse?&source.corpse->inventory2.inventory:(source.chest?&source.chest->containList:nullptr);
    if(list)for(auto* it=list->GetNextInList();it;it=it->GetNextInList()){auto* item=it->GetData();if(LootEligible(item))out.push_back(item);}
    return out;
}
bool LootRemove(const LootSource& source,oCItem* item,std::uint32_t amount){
    if(!item||!amount||item->amount<static_cast<int>(amount))return false;
    const auto contents=LootNativeItems(source);if(std::find(contents.begin(),contents.end(),item)==contents.end())return false;
    if(item->amount>static_cast<int>(amount)){item->amount-=static_cast<int>(amount);return true;}
    // Every item passed here has an explicit lease reference. Native list/UI
    // removal may release its own references without invalidating our pointer.
    if(source.corpse){
        if(auto* ui=LootUI(source))if(ui->contents!=&source.corpse->inventory2.inventory){const bool owns=ui->ownList!=0;ui->Remove(item);if(owns){item->amount=0;return true;}}
        source.corpse->inventory2.Remove(item);
    }else if(source.chest){
        if(source.chest->items)source.chest->items->Remove(item);else source.chest->Remove(item);
    }else return false;
    item->amount=0;return true;
}
oCItem* CreateLootItem(const lo::Entry& entry){
    if(!parser||entry.instance>=static_cast<std::uint32_t>(parser->symtab.GetNumInList()))return nullptr;
    auto* type=parser->GetSymbol(zSTRING("C_ITEM"));auto* symbol=parser->GetSymbol(static_cast<int>(entry.instance));int depth=0;
    while(symbol&&symbol!=type&&depth++<64)symbol=symbol->parent;
    if(!type||symbol!=type)return nullptr;
    auto* item=zDYNAMIC_CAST<oCItem>(ogame->GetGameWorld()->CreateVob(zVOB_TYPE_ITEM,static_cast<int>(entry.instance)));
    if(item)item->amount=static_cast<int>(entry.amount);return item;
}
void CloseClientLootUI(){
    lootNativeBypass=true;
    if(clientLootSource.identity.kind==lo::Source::Merchant){WorldTradeClose();lootNativeBypass=false;clientLootShown=false;return;}
    if(player){
        // A denied async chest request must also release the interaction even
        // though the inventory window was never opened.
        if(clientLootSource.chest&&(LootUIOpen(clientLootSource)||player->interactMob==clientLootSource.chest))clientLootSource.chest->Close(player);
        else if(clientLootSource.corpse&&clientLootShown&&LootUIOpen(clientLootSource)){if(auto* ui=LootUI(clientLootSource))ui->Close();oCNpc::game_mode=0;player->CloseInventory();}
    }
    lootNativeBypass=false;clientLootShown=false;
}
void ClearClientLoot(){CloseClientLootUI();ReleaseLootItems(clientLootItems);ReleaseLootSource(clientLootSource);clientOpenRequest=clientLease=clientLootRevision=clientPendingTake=0;clientLootClosing=false;clientQuickLoot=false;lootClientWait=0;lootSnapshot=lo::Snapshot{};}
void LootReset(){
    RecoverySessionReset();
    if(clientPendingTake||lootLedger.HasUnapplied())LootLog("loot_recovery_pending","Session ended with transfer in flight; durable transaction decision and save checkpoint will be reconciled on resume.");
    ClearClientLoot();ReleaseLootItems(guestLootItems);ReleaseLootSource(guestLootSource);lootLedger=lo::Ledger{};lootOutbox.clear();lootApplied.clear();lootRequest=0;lootLastKeepAlive=lootLastTraffic=0;
}
lo::Identity LootIdentity(oCNpc* npc){
    lo::Identity id;id.kind=lo::Source::Corpse;const auto found=npcByPointer.find(npc);if(found!=npcByPointer.end())id.npc=found->second->id;return id;
}
lo::Identity LootIdentity(oCMobContainer* chest){
    lo::Identity id;id.kind=lo::Source::Chest;id.name=chest->GetObjectName().ToChar();const auto p=chest->GetPositionWorld();id.position={p[0],p[1],p[2]};return id;
}
bool RequestClientLoot(LootSource source){
    if(clientPendingTake||clientOpenRequest){notice=Ui("ui_73217e8c9ce7");return true;}
    if(!source.vob||(source.corpse&&!source.identity.npc)||source.identity.name.size()>63){notice=Ui("ui_c1729fcf850a");return true;}
    source.vob->AddRef();clientLootSource=source;
    lo::Message m;m.op=clientQuickLoot?lo::Op::QuickOpen:lo::Op::Open;m.request=clientOpenRequest=NextLootRequest();m.source=source.identity;QueueLoot(m);lootClientWait=Now();notice=Ui("ui_84ab6893f077");
    LootLog("loot_open_request","request="+std::to_string(m.request)+" kind="+std::to_string(static_cast<unsigned>(m.source.kind))+" npc="+std::to_string(m.source.npc));return true;
}
bool LootBeforeCorpseOpen(){
    if(!lootNativeBypass&&!RecoveryCanLoot())return true;
    if(lootNativeBypass||!CombatConnected()||!player)return false;
    auto* targetNpc=player->GetFocusNpc();if(!targetNpc)return false;
    if(targetNpc==PartnerNpc()){notice=Ui("ui_63cda7a7c0f6");return true;}
    if(HostCombat()){if(guestLootSource.vob==targetNpc){notice=Ui("ui_cd7ea0cb58b3");return true;}return false;}
    if(!targetNpc->IsDead()&&!targetNpc->IsUnconscious())return true;
    LootSource s;s.vob=s.corpse=targetNpc;s.identity=LootIdentity(targetNpc);return RequestClientLoot(s);
}
bool LootBeforeChestOpen(oCMobContainer* chest,oCNpc* who){
    if(who==player&&!lootNativeBypass&&!RecoveryCanLoot())return true;
    if(lootNativeBypass||!CombatConnected()||who!=player)return false;
    if(HostCombat()){if(guestLootSource.vob==chest){notice=Ui("ui_7b2d83e77b85");return true;}return false;}
    if(chest->locked)return false;
    LootSource s;s.vob=s.chest=chest;s.identity=LootIdentity(chest);return RequestClientLoot(s);
}
lo::Status ResolveHostLoot(const lo::Identity& id,LootSource& source){
    source.identity=id;
    if(id.kind==lo::Source::Corpse||id.kind==lo::Source::Merchant){auto it=npcById.find(id.npc);if(it==npcById.end())return lo::Status::Missing;source.vob=source.corpse=it->second->npc;if(id.kind==lo::Source::Merchant?(source.corpse->IsDead()||source.corpse->IsUnconscious()):(!source.corpse->IsDead()&&!source.corpse->IsUnconscious()))return lo::Status::BadRequest;}
    else {
        auto* world=ogame->GetGameWorld();int matches=0;
        if(world->voblist)for(auto* it=world->voblist->GetNextInList();it;it=it->GetNextInList()){
            auto* c=zDYNAMIC_CAST<oCMobContainer>(it->GetData());if(!c||id.name!=c->GetObjectName().ToChar())continue;
            if(Distance2(c->GetPositionWorld(),zVEC3(id.position[0],id.position[1],id.position[2]))<=2500){source.vob=source.chest=c;++matches;}
        }
        if(matches!=1)return lo::Status::Missing;if(source.chest->locked)return lo::Status::Locked;
    }
    if(!source.vob||source.vob==PartnerNpc()||source.vob==player||source.vob->homeWorld!=ogame->GetWorld())return lo::Status::BadRequest;
    if(!haveTarget||Now()-lastSnapshotAt>1000||Distance2(source.vob->GetPositionWorld(),zVEC3(target.x,target.y,target.z))>250000)return lo::Status::Range;
    if(LootUIOpen(source))return lo::Status::Busy;
    return lo::Status::Ok;
}
void DenyLoot(const lo::Message& request,lo::Status status){auto denied=request;denied.op=lo::Op::Denied;denied.status=status;QueueLoot(denied);LootLog("loot_deny","request="+std::to_string(request.request)+" status="+std::to_string(static_cast<unsigned>(status)));}
void OpenHostLoot(const lo::Message& request){
    if(lootLedger.Lease()||lootLedger.HasUnapplied()){DenyLoot(request,lo::Status::Busy);return;}
    LootSource s;auto status=ResolveHostLoot(request.source,s);if(status!=lo::Status::Ok){DenyLoot(request,status);return;}
    if(request.op==lo::Op::QuickOpen&&s.chest&&(s.chest->IsOccupied()||s.chest->conditionFunc.Length()||s.chest->onStateFuncName.Length()||s.chest->useWithItem.Length())){DenyLoot(request,lo::Status::BadRequest);return;}
    if(s.identity.kind==lo::Source::Merchant)WorldPrepareMerchant(s.corpse);
    else if(s.corpse&&!QolPrepareCorpse(s.corpse)){DenyLoot(request,lo::Status::BadRequest);return;}
    if(s.corpse)s.corpse->inventory2.UnpackAllItems();
    const auto items=LootNativeItems(s);if(items.size()>lo::MaxEntries){DenyLoot(request,lo::Status::Limit);return;}
    std::vector<lo::Entry> entries;
    for(auto* item:items){lo::Entry e{static_cast<std::uint32_t>(entries.size()+1),static_cast<std::uint32_t>(item->GetInstance()),static_cast<std::uint32_t>(item->amount)};if(!lo::Valid(e)){DenyLoot(request,lo::Status::BadRequest);return;}entries.push_back(e);}
    const auto lease=lootLedger.Open(entries);if(!lease){DenyLoot(request,lo::Status::Busy);return;}
    s.vob->AddRef();guestLootSource=s;ReleaseLootItems(guestLootItems);
    for(std::size_t i=0;i<items.size();++i){items[i]->AddRef();guestLootItems.push_back({items[i],entries[i]});}
    lootLastTraffic=Now();lo::Message response;response.op=lo::Op::Begin;response.request=request.request;response.lease=lease;response.revision=1;response.total=static_cast<std::uint32_t>(entries.size());QueueLoot(response);
    for(std::size_t index=0;index<entries.size();index+=lo::PageEntries){response.op=lo::Op::Page;response.index=static_cast<std::uint32_t>(index);response.entries.assign(entries.begin()+index,entries.begin()+(std::min)(entries.size(),index+lo::PageEntries));QueueLoot(response);}
    LootLog("loot_lease","lease="+std::to_string(lease)+" entries="+std::to_string(entries.size()));
}
bool ApplyLootSnapshot(){
    if(!clientLootSource.vob||clientLootSource.vob->homeWorld!=ogame->GetWorld()||(clientLootSource.identity.kind!=lo::Source::Merchant&&LootUIOpen(clientLootSource))||Distance2(clientLootSource.vob->GetPositionWorld(),player->GetPositionWorld())>250000)return false;
    std::vector<LootItemRef> prepared;
    for(const auto& entry:lootSnapshot.Entries()){
        auto* item=CreateLootItem(entry);if(!item){ReleaseLootItems(prepared);return false;}prepared.push_back({item,entry});
    }
    if(clientLootSource.identity.kind==lo::Source::Merchant)WorldTradeSnapshot(true);
    if(clientLootSource.corpse)clientLootSource.corpse->inventory2.UnpackAllItems();
    for(auto* item:LootNativeItems(clientLootSource)){item->AddRef();const auto amount=item->amount;const bool removed=LootRemove(clientLootSource,item,static_cast<unsigned>(amount));item->Release();if(!removed){ReleaseLootItems(prepared);return false;}}
    for(auto& item:prepared){
        if(clientLootSource.corpse){auto* stored=clientLootSource.corpse->PutInInv(item.item);if(!stored){ReleaseLootItems(prepared);return false;}stored->AddRef();item.item->Release();item.item=stored;}
        else clientLootSource.chest->Insert(item.item);
    }
    clientLootItems=std::move(prepared);
    if(clientQuickLoot){clientLootShown=true;lootClientWait=0;LootLog("quickloot_snapshot","entries="+std::to_string(clientLootItems.size()));return true;}
    lootNativeBypass=true;
    if(clientLootSource.identity.kind==lo::Source::Merchant)WorldTradeSnapshot(false);
    else if(clientLootSource.corpse){player->SetFocusVob(clientLootSource.corpse);player->OpenDeadNpc();}
    else clientLootSource.chest->Open(player);
    lootNativeBypass=false;clientLootShown=LootUIOpen(clientLootSource);lootClientWait=0;
    notice=clientLootShown?Ui("ui_479fa1f13962"):Ui("ui_e81e3f83e8b8");
    return clientLootShown;
}
bool LootInterceptTransfer(oCItemContainer* container,int,int amount){
    if(!lootNativeBypass&&RecoveryTransfer(container,amount))return true;
    if(lootNativeBypass||!CombatConnected())return false;
    if(HostCombat()){if(guestLootSource.vob&&(container==LootUI(guestLootSource))){notice=Ui("ui_8c60f10d6461");return true;}return false;}
    if(!clientLootSource.vob)return false;
    auto* sourceUI=LootUI(clientLootSource);
    if(container!=sourceUI){if(container==&player->inventory2){notice=Ui("ui_23f7e3fcb536");return true;}return false;}
    if(clientPendingTake||!clientLease||!clientLootShown)return true;
    auto* selected=container->GetSelectedItem();if(!selected||amount<=0)return true;
    for(const auto& i:clientLootItems)if(i.item==selected&&i.entry.amount){
        lo::Message request;request.op=lo::Op::Take;request.request=clientPendingTake=NextLootRequest();request.lease=clientLease;request.revision=clientLootRevision;request.item=i.entry;
        request.item.amount=(std::min)(i.entry.amount,static_cast<unsigned>(container->CanTransferMoreThanOneItem()?amount:1));QueueLoot(request);lootClientWait=Now();
        LootLog("loot_take_request","request="+std::to_string(request.request)+" token="+std::to_string(request.item.token)+" amount="+std::to_string(request.item.amount));return true;
    }
    notice=Ui("ui_2f084074674d");return true;
}
void LootReceive(const std::vector<std::uint8_t>& bytes){
    lo::Message m;if(!lo::Decode(bytes,m)){LootLog("loot_invalid","Malformed loot message");return;}
    if(m.op==lo::Op::Take||m.op==lo::Op::Result){LootLog("recovery_required","Legacy inventory mutations disabled: persistent transaction ID required");return;}
    if(HostCombat()){
        switch(m.op){
        case lo::Op::QuickOpen:case lo::Op::Open:OpenHostLoot(m);return;
        case lo::Op::Take:{
            auto result=lootLedger.Take(m,[&](const lo::Entry& item,std::uint32_t amount){
                if(!guestLootSource.vob||item.token>guestLootItems.size()||!haveTarget||Now()-lastSnapshotAt>1000||Distance2(guestLootSource.vob->GetPositionWorld(),zVEC3(target.x,target.y,target.z))>250000)return false;
                if(guestLootSource.vob->homeWorld!=ogame->GetWorld()||!havePeerCombat||peerCombat.stats.attributes[0]<=0||(guestLootSource.corpse&&!guestLootSource.corpse->IsDead()&&!guestLootSource.corpse->IsUnconscious())||(guestLootSource.chest&&guestLootSource.chest->locked))return false;
                auto& native=guestLootItems[item.token-1];if(native.entry.instance!=item.instance||native.item->amount!=static_cast<int>(item.amount))return false;
                if(!LootJournal("host_prepare",m))return false;
                if(!LootRemove(guestLootSource,native.item,amount))return false;
                LootJournal("host_commit",m);return true;
            });
            QueueLoot(result);lootLastTraffic=Now();LootLog("loot_result","request="+std::to_string(m.request)+" lease="+std::to_string(m.lease)+" amount="+std::to_string(m.item.amount)+" status="+std::to_string(static_cast<unsigned>(result.status)));return;
        }
        case lo::Op::Applied:if(lootLedger.Applied(m.request,m.lease)){LootJournal("host_receipt",m);lootLastTraffic=Now();}return;
        case lo::Op::KeepAlive:if(m.lease==lootLedger.Lease())lootLastTraffic=Now();return;
        case lo::Op::Close:if(lootLedger.Close(m.lease)){ReleaseLootItems(guestLootItems);ReleaseLootSource(guestLootSource);LootLog("loot_release","lease="+std::to_string(m.lease));}return;
        default:LootLog("loot_invalid","Wrong guest message direction");return;
        }
    }
    if(m.op==lo::Op::Denied){if(m.request==clientOpenRequest){notice=LootStatusText(m.status);ClearClientLoot();}return;}
    if(m.op==lo::Op::Begin){if(m.request!=clientOpenRequest||clientLease||!lootSnapshot.Begin(m)){combatFailure="Invalid loot snapshot begin";return;}clientLease=m.lease;clientLootRevision=m.revision;if(lootSnapshot.Complete()&&!ApplyLootSnapshot())clientLootClosing=true;return;}
    if(m.op==lo::Op::Page){if(!lootSnapshot.Page(m)){combatFailure="Incomplete/out-of-order loot snapshot";return;}if(lootSnapshot.Complete()&&!ApplyLootSnapshot())clientLootClosing=true;return;}
    if(m.op!=lo::Op::Result){LootLog("loot_invalid","Wrong host message direction");return;}
    auto old=lootApplied.find(m.request);
    if(old!=lootApplied.end()){
        if(old->second.lease!=m.lease||old->second.revision!=m.revision||old->second.status!=m.status||old->second.item.token!=m.item.token||old->second.item.amount!=m.item.amount||old->second.item.instance!=m.item.instance){combatFailure="Conflicting loot grant";return;}
        m.op=lo::Op::Applied;QueueLoot(m);return;
    }
    if(m.request!=clientPendingTake||m.lease!=clientLease){combatFailure="Unsolicited loot result";return;}
    if(m.status!=lo::Status::Ok){notice=LootStatusText(m.status);clientPendingTake=0;lootClientWait=0;clientLootClosing=true;return;}
    if(lootApplied.size()>=lo::MaxTransactions||m.revision!=clientLootRevision+1||m.item.token>clientLootItems.size()){combatFailure="Loot grant revision/limit mismatch";return;}
    auto& entry=clientLootItems[m.item.token-1];
    if(entry.entry.instance!=m.item.instance||entry.entry.amount<m.item.amount||!LootJournal("client_prepare",m)){combatFailure="Loot grant mismatch/journal unavailable";return;}
    auto* item=CreateLootItem(m.item);if(!item){combatFailure="Loot grant item unavailable; see loot journal";return;}
    auto* stored=player->PutInInv(item);item->Release();
    if(!stored){combatFailure="Loot owner insertion failed; see loot journal";return;}
    // The grant has now been applied. Never replay it automatically if the
    // mirror/UI update or durable receipt fails afterwards.
    lootApplied.emplace(m.request,m);
    if(!LootRemove(clientLootSource,entry.item,m.item.amount)){combatFailure="Loot applied but source mirror failed; see journal";return;}
    entry.entry.amount-=m.item.amount;clientLootRevision=m.revision;clientPendingTake=0;lootClientWait=0;
    if(auto* ui=LootUI(clientLootSource))ui->CheckSelectedItem();
    if(!LootJournal("client_applied",m)){combatFailure="Loot applied but receipt journal failed";return;}
    m.op=lo::Op::Applied;QueueLoot(m);LootLog("loot_applied","request="+std::to_string(m.request)+" instance="+std::to_string(m.item.instance)+" amount="+std::to_string(m.item.amount));
}
void LootFrame(unsigned long long now){
    if(!CombatConnected())return;
    if(!HostCombat()&&clientLootSource.vob){
        if(clientLootShown&&!clientQuickLoot&&!LootUIOpen(clientLootSource))clientLootClosing=true;
        if(lootClientWait&&now-lootClientWait>10000){combatFailure="Loot operation timed out; inspect inventory journal before retry";return;}
        if(clientLootClosing&&!clientPendingTake&&clientLease){lo::Message close;close.op=lo::Op::Close;close.request=NextLootRequest();close.lease=clientLease;QueueLoot(close);ClearClientLoot();}
        else if(clientLease&&now-lootLastKeepAlive>=1000){lootLastKeepAlive=now;lo::Message keep;keep.op=lo::Op::KeepAlive;keep.request=NextLootRequest();keep.lease=clientLease;QueueLoot(keep);}
    }
    if(HostCombat()&&lootLedger.Lease()&&now-lootLastTraffic>15000){combatFailure="Guest loot lease timed out; no automatic item rollback";return;}
    for(int i=0;i<8&&!lootOutbox.empty()&&network.PendingEvents()<48;++i){if(!network.SendEvent(lootOutbox.front(),now)){combatFailure="Loot reliable channel unavailable";break;}lootOutbox.pop_front();}
}
