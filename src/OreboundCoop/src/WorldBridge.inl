namespace ws=worldsync;
struct GroundBinding {oCItem* item=nullptr;ws::Ground state;bool owned=false,bound=false,probed=false;unsigned long long lastSent=0;std::array<float,3> sentPosition{};};
std::vector<std::unique_ptr<GroundBinding>> groundBindings;
std::map<oCItem*,GroundBinding*> groundByPointer;
std::map<unsigned,GroundBinding*> groundById;
std::map<std::string,unsigned long long> npcProbes;
std::map<unsigned,NpcBinding*> npcProbeTargets;unsigned npcProbeSerial=0;
unsigned worldDualPasses=0,worldTimersRestored=0,worldGuestTicks=0;
std::deque<co::Bytes> worldOutbox;
unsigned nextGroundId=1;std::size_t groundCursor=0;
unsigned long long lastWorldScan=0,lastWorldSend=0,lastWorldLog=0,worldConnectedAt=0;
bool worldConstructing=false,worldCulling=false,worldGuestPass=false,worldNativeDropPass=false;
oCInformationManager* worldTradeManager=nullptr;
bool worldTradeReady=false;
std::string worldGroundPending;
ws::TalkLease worldTalkLease;

void QueueWorld(co::Bytes bytes){if(bytes.size()>net::MaximumDatagramBytes-net::ProtocolHeaderBytes||worldOutbox.size()>=8192){combatFailure="World lifecycle queue limit reached";return;}worldOutbox.push_back(std::move(bytes));}
void WorldReset(){
    worldConstructing=true;
    worldTalkLease.Clear();
    for(auto& entry:groundBindings){if(entry->owned&&entry->item->homeWorld)entry->item->RemoveVobFromWorld();entry->item->Release();}
    worldOutbox.clear();groundBindings.clear();groundByPointer.clear();groundById.clear();npcProbes.clear();npcProbeTargets.clear();npcProbeSerial=0;nextGroundId=1;groundCursor=0;
    worldDualPasses=worldTimersRestored=worldGuestTicks=0;lastWorldScan=lastWorldSend=lastWorldLog=worldConnectedAt=0;worldGroundPending.clear();worldTradeManager=nullptr;worldTradeReady=false;worldConstructing=false;
}
bool WorldLocalTalksTo(oCNpc* npc){
    if(!npc||!player)return false;
    if(player->talkOther==npc||npc->talkOther==player)return true;
    auto& info=oCInformationManager::GetInformationManager();if(!info.IsDone&&info.Npc==npc&&info.Player==player)return true;
    const auto binding=npcByPointer.find(npc);
    return binding!=npcByPointer.end()&&worldTalkLease.Allows(binding->second->id,Now());
}
bool WorldBeginTalk(oCNpc* npc,oCNpc* other){
    if(!CombatConnected()||HostCombat()||other!=player||!npc||npc==player||npc==PartnerNpc()||npc->IsDead()||npc->IsUnconscious())return false;
    const auto it=npcByPointer.find(npc);if(it==npcByPointer.end()||!it->second->id)return false;
    auto& entry=*it->second;worldTalkLease.Begin(entry.id,Now());
    if(auto* model=npc->GetModel()){model->aniTransScale=entry.rootScale;model->doVobRot=entry.originalRotation;}
    npc->SetSleepingMode(zVOB_AWAKE);
    CombatLog("dialog_request","npc="+std::string(npc->GetObjectName().ToChar())+" id="+std::to_string(entry.id)+" guest_ai_lease_ms=3000");return true;
}
unsigned WorldLocalInteraction(){if(!player)return 0;auto* npc=player->talkOther;auto& info=oCInformationManager::GetInformationManager();if(!npc&&!info.IsDone&&info.Player==player)npc=info.Npc;auto it=npcByPointer.find(npc);if(it!=npcByPointer.end())return it->second->id;return worldTalkLease.Allows(worldTalkLease.npc,Now())?worldTalkLease.npc:0;}
bool WorldTradePaused(oCNpc* npc){
    if(!HostCombat()||!npc||WorldLocalTalksTo(npc))return false;
    if(guestLootSource.corpse==npc&&guestLootSource.identity.kind==lo::Source::Merchant)return true;
    auto it=npcByPointer.find(npc);return havePeerCombat&&it!=npcByPointer.end()&&peerCombat.interaction==it->second->id&&haveTarget&&Distance2(npc->GetPositionWorld(),zVEC3(target.x,target.y,target.z))<250000;
}
oCItemContainer* WorldTradeUI(oCNpc* npc){
    if(worldTradeManager&&worldTradeManager->Npc==npc&&worldTradeManager->DlgTrade&&worldTradeManager->DlgTrade->DlgInventoryNpc)return worldTradeManager->DlgTrade->DlgInventoryNpc->StealContainer;
    auto& info=oCInformationManager::GetInformationManager();
    if(info.Mode==oCInformationManager::INFO_MGR_MODE_TRADE&&info.Npc==npc&&info.DlgTrade&&info.DlgTrade->DlgInventoryNpc)return info.DlgTrade->DlgInventoryNpc->StealContainer;
    return nullptr;
}
bool WorldTradeBusy(oCNpc* npc){auto* ui=WorldTradeUI(npc);return ui&&ui->IsOpen();}
void WorldTradeClose(){if(worldTradeManager&&worldTradeManager->Mode==oCInformationManager::INFO_MGR_MODE_TRADE)worldTradeManager->OnTradeEnd();worldTradeManager=nullptr;worldTradeReady=false;}
void WorldTradeSnapshot(bool begin){
    if(!worldTradeManager||!worldTradeManager->DlgTrade)return;
    auto* dialog=worldTradeManager->DlgTrade;
    if(begin){dialog->AllDialogsStop();return;}
    dialog->SetNpcLeft(clientLootSource.corpse);dialog->SetNpcRight(player);dialog->StartSelection();worldTradeReady=true;
}
void WorldPrepareMerchant(oCNpc* npc){
    if(!npc||!parser)return;
    auto* self=parser->GetSymbol(zSTRING("SELF"));auto* other=parser->GetSymbol(zSTRING("OTHER"));
    auto* scriptItem=parser->GetSymbol(zSTRING("ITEM"));auto* victim=parser->GetSymbol(zSTRING("VICTIM"));
    void* oldSelf=self?self->GetInstanceAdr():nullptr;void* oldOther=other?other->GetInstanceAdr():nullptr;
    void* oldItem=scriptItem?scriptItem->GetInstanceAdr():nullptr;void* oldVictim=victim?victim->GetInstanceAdr():nullptr;
    struct RestoreTradeBindings {zCParser* p;void* s;void* o;void* item;void* victim;bool hasItem;bool hasVictim;~RestoreTradeBindings(){if(p!=parser)return;p->SetInstance(zSTRING("SELF"),s);p->SetInstance(zSTRING("OTHER"),o);if(hasItem)p->SetInstance(zSTRING("ITEM"),item);if(hasVictim)p->SetInstance(zSTRING("VICTIM"),victim);}} restore{parser,oldSelf,oldOther,oldItem,oldVictim,scriptItem!=nullptr,victim!=nullptr};
    const int helper=parser->GetIndex(zSTRING("ZK_COOP_PREPARE_TRADE"));
    if(helper<0){combatFailure="Trade preparation script missing";return;}
    parser->SetInstance(zSTRING("SELF"),npc);parser->SetInstance(zSTRING("OTHER"),npc);
    ExtensionCall(extensions::Command::PrepareTrade);
    npc->inventory2.UnpackAllItems();
    LootLog("trade_prepare","npc="+std::string(npc->GetObjectName().ToChar()));
}
void WorldTradeBegin(oCInformationManager* manager){
    if(!CombatConnected()||HostCombat()||!manager||manager->Player!=player||!manager->Npc)return;
    if(!RecoveryCanLoot()||clientPendingTake||clientOpenRequest||clientLootShown){manager->OnTradeEnd();notice=Ui("ui_eb5b307ed781");return;}
    worldTradeManager=manager;worldTradeReady=false;
    LootSource source;source.vob=source.corpse=manager->Npc;source.identity=LootIdentity(manager->Npc);source.identity.kind=lo::Source::Merchant;
    if(!source.identity.npc){WorldTradeClose();notice=Ui("ui_679d06ba430a");return;}
    if(manager->DlgTrade)manager->DlgTrade->AllDialogsDisable();
    RequestClientLoot(source);
}
void WorldNpcVisual(co::Npc& s,oCNpc* npc){
    s.sprint=QolSprint(npc);s.model=npc->mds_name.ToChar();s.body=npc->body_visualName.ToChar();s.head=npc->head_visualName.ToChar();
    if(s.model.size()>63)s.model.clear();if(s.body.size()>63)s.body.clear();if(s.head.size()>63)s.head.clear();
    s.bodyTex=npc->body_TexVarNr;s.headTex=npc->head_TexVarNr;s.bodyColor=npc->body_TexColorNr;
    s.headColor=0;auto* armor=npc->GetEquippedArmor();s.armor=armor?armor->GetInstance():-1;CharacterNpcCapture(s,npc);
}
co::Npc WorldNpcState(NpcBinding& e){
    co::Npc s=e.desired;s.id=e.id;s.generation=e.generation;s.life=e.life;s.revision=++combatRevision;s.instance=e.npc->GetInstance();s.anchor={e.anchor[0],e.anchor[1],e.anchor[2]};
    if(e.life!=3){const auto p=e.npc->GetPositionWorld();const auto d=e.npc->GetAtVectorWorld();s.pose={p[0],p[1],p[2],std::atan2(d[0],d[2])*57.295779513f};s.stats=ReadCombatStats(e.npc);s.animation=CombatAnimation(zDYNAMIC_CAST<zCModel>(e.npc->GetVisual()));WorldNpcVisual(s,e.npc);}
    e.desired=s;return s;
}
void WorldLife(oCNpc* npc,unsigned life){
    if(worldConstructing||!CombatConnected()||!HostCombat()||!npc||npc==player||npc==PartnerNpc())return;
    auto* entry=RegisterCombatNpc(npc);if(!entry)return;
    if(life==1&&entry->life==3){if(entry->generation==UINT32_MAX){combatFailure="NPC generation exhausted";return;}++entry->generation;}
    if(entry->life==life&&entry->last)return;
    // Capture before Disable/Delete unloads the native model; a newly enabled
    // generation must use its fresh native state rather than the tombstone.
    if(life==1)entry->life=1;
    auto state=WorldNpcState(*entry);entry->life=life;state.life=life;entry->desired=state;entry->last=Now();
    auto bytes=co::Encode(state);bytes[0]=49;QueueWorld(std::move(bytes));
    LootLog("npc_lifecycle","id="+std::to_string(entry->id)+" generation="+std::to_string(entry->generation)+" life="+std::to_string(life));
}
NpcBinding* WorldCreateNpc(const co::Npc& message){
    if(worldConstructing)return nullptr;worldConstructing=true;
    auto* npc=zDYNAMIC_CAST<oCNpc>(ogame->GetGameWorld()->CreateVob(zVOB_TYPE_NSC,message.instance));
    if(!npc){worldConstructing=false;combatFailure="Cannot construct authoritative NPC instance";return nullptr;}
    // Keep native dialogue perception on real world NPCs. Mirror hooks suppress
    // autonomous AI, while a requested guest conversation needs AssessTalk_S.
    npc->dontWriteIntoArchive=TRUE;
    auto* entry=RegisterCombatNpc(npc);if(!entry){npc->Release();worldConstructing=false;return nullptr;}
    entry->owned=entry->mirror=true;entry->anchor=zVEC3(message.anchor[0],message.anchor[1],message.anchor[2]);
    zVEC3 position(message.pose.x,message.pose.y,message.pose.z);npc->Enable(position);if(!npc->homeWorld)ogame->GetWorld()->AddVob(npc);
    npc->Release();worldConstructing=false;
    LootLog("npc_created","instance="+std::to_string(message.instance)+" id="+std::to_string(message.id));return entry;
}
void WorldApplyNpc(NpcBinding& entry,const co::Npc& state){
    auto* npc=entry.npc;if(CharacterLocal(npc))return;CharacterNpcApply(entry,state);
    if(state.life!=1){if(player){if(player->GetFocusVob()==npc)player->SetFocusVob(nullptr);if(player->enemy==npc)player->SetEnemy(nullptr);}if(!entry.hidden){npc->Disable();entry.hidden=true;}return;}
    if(!npc->homeWorld){zVEC3 p(state.pose.x,state.pose.y,state.pose.z);npc->Enable(p);if(!npc->homeWorld)ogame->GetWorld()->AddVob(npc);}
    entry.hidden=false;
    const auto key=co::AppearanceKey(state);
    if(key!=entry.visualKey){
        if(!state.model.empty()&&state.model!=npc->mds_name.ToChar())static_cast<zCVob*>(npc)->SetVisual(zSTRING(state.model.c_str()));
        if(!state.body.empty()&&!state.head.empty()){zSTRING body(state.body.c_str()),head(state.head.c_str());npc->SetAdditionalVisuals(body,state.bodyTex,state.bodyColor,head,state.headTex,state.headColor,state.armor);}
        entry.visualKey=key;
    }
    QolApplySprint(npc,state.sprint);
    const bool talking=WorldLocalTalksTo(npc);
    if(auto* model=npc->GetModel()){if(!entry.modelDefaultsCaptured){entry.rootScale=model->aniTransScale;entry.originalRotation=model->doVobRot;entry.modelDefaultsCaptured=true;}model->aniTransScale=talking?entry.rootScale:zVEC3(0,0,0);model->doVobRot=talking?entry.originalRotation:FALSE;}
    npc->SetPhysicsEnabled(talking?entry.originalPhysics:FALSE);npc->SetSleepingMode(zVOB_AWAKE);
}
GroundBinding* RegisterGround(oCItem* item){
    if(!item||!item->homeWorld||item->homeWorld!=ogame->GetWorld()||item->amount<=0||item->GetInstance()<=0)return nullptr;
    auto found=groundByPointer.find(item);if(found!=groundByPointer.end())return found->second;
    if(groundBindings.size()>=8192){combatFailure="Ground registry limit reached";return nullptr;}
    auto entry=std::make_unique<GroundBinding>();entry->item=item;item->AddRef();const auto p=item->GetPositionWorld();
    entry->state.instance=item->GetInstance();entry->state.amount=item->amount;entry->state.present=true;entry->state.anchor=entry->state.position={p[0],p[1],p[2]};entry->state.name=item->GetObjectName().ToChar();if(entry->state.name.size()>63)entry->state.name.clear();
    if(HostCombat()){entry->state.id=nextGroundId++;groundById[entry->state.id]=entry.get();}
    auto* result=entry.get();groundByPointer[item]=result;groundBindings.push_back(std::move(entry));return result;
}
void WorldGroundChanged(oCItem* item){
    if(!CombatConnected()||!HostCombat()||worldConstructing)return;
    const auto it=groundByPointer.find(item);if(it==groundByPointer.end())return;
    auto& g=it->second->state;g.present=item->homeWorld==ogame->GetWorld()&&item->amount>0;if(g.present){const auto p=item->GetPositionWorld();g.position={p[0],p[1],p[2]};g.amount=item->amount;}
    g.name=item->GetObjectName().ToChar();if(g.name.size()>63)g.name.clear();g.revision=++combatRevision;it->second->lastSent=Now();it->second->sentPosition=g.position;QueueWorld(ws::Encode(g));
    LootLog("ground_changed","id="+std::to_string(g.id)+" present="+std::to_string(g.present)+" amount="+std::to_string(g.amount));
}
oCItem* WorldGroundFor(const rc::Tx& tx){
    std::vector<oCItem*> candidates;std::vector<float> distances;auto* world=ogame->GetGameWorld();
    const bool tagged=tx.source.name.rfind("ZK_DROP_",0)==0;
    if(world->voblist_items)for(auto* it=world->voblist_items->GetNextInList();it;it=it->GetNextInList()){
        auto* item=it->GetData();if(!item||item->homeWorld!=world||static_cast<unsigned>(item->GetInstance())!=tx.item)continue;
        if(tagged&&tx.source.name!=item->GetObjectName().ToChar())continue;
        const auto p=item->GetPositionWorld();candidates.push_back(item);distances.push_back(Distance2(p,zVEC3(tx.source.position[0],tx.source.position[1],tx.source.position[2])));
    }
    const int resolved=ws::MatchGround(distances,tagged);return resolved>=0?candidates[resolved]:nullptr;
}
bool WorldRecoveryEffect(const rc::Record& record,bool desired,bool& handled){
    const auto& tx=record.tx;
    handled=record.role==rc::Role::Host&&(tx.action==rc::Action::Drop||tx.action==rc::Action::GroundTake);
    if(!handled)return false;if(tx.world!=CurrentWorld())return false;
    if(tx.action==rc::Action::Drop){
        const auto tag="ZK_DROP_"+tx.id;auto* existing=ogame->GetWorld()->SearchVobByName(zSTRING(tag.c_str()));
        if(desired){if(existing)return zDYNAMIC_CAST<oCItem>(existing)!=nullptr;
            lo::Entry e{1,tx.item,tx.amount};auto* item=CreateLootItem(e);if(!item)return false;
            item->SetObjectName(zSTRING(tag.c_str()));item->SetPositionWorld(zVEC3(tx.source.position[0],tx.source.position[1],tx.source.position[2]));
            ogame->GetWorld()->AddVob(item);item->SetCollDetStat(TRUE);item->SetCollDetDyn(TRUE);item->SetPhysicsEnabled(TRUE);RegisterGround(item);item->Release();return true;
        }
        if(existing)existing->RemoveVobFromWorld();return true;
    }
    auto* item=WorldGroundFor(tx);if(!desired)return false;
    if(!item||item->amount<static_cast<int>(tx.amount))return false;
    LootLog("ground_effect_begin","tx="+tx.id+" item="+std::to_string(tx.item)+" amount="+std::to_string(tx.amount));
    RegisterGround(item);CharacterGroundTheft(item,tx.ownerTimeline);if(item->amount>static_cast<int>(tx.amount))item->amount-=tx.amount;else item->RemoveVobFromWorld();WorldGroundChanged(item);LootLog("ground_effect_end","tx="+tx.id);return true;
}
bool WorldHostRequest(rc::Message& m){
    if(m.tx.action!=rc::Action::GroundTake&&m.tx.action!=rc::Action::Drop)return false;
    bool allowed=m.op==rc::Op::Request&&recoveryReady&&m.tx.hostTimeline==recoveryTimeline&&m.tx.world==CurrentWorld()&&haveTarget&&Now()-lastSnapshotAt<=1000;
    allowed=allowed&&Distance2(zVEC3(m.tx.source.position[0],m.tx.source.position[1],m.tx.source.position[2]),zVEC3(target.x,target.y,target.z))<250000;
    if(m.tx.action==rc::Action::GroundTake){auto* item=WorldGroundFor(m.tx);allowed=allowed&&item&&item->amount>=static_cast<int>(m.tx.amount);}
    else allowed=allowed&&m.tx.source.name=="ZK_DROP_"+m.tx.id;
    auto tx=m.tx;tx.decision=allowed?rc::Decision::Commit:rc::Decision::Abort;
    LootLog("ground_decision_begin","tx="+tx.id+" decision="+std::to_string(static_cast<unsigned>(tx.decision))+" item="+std::to_string(tx.item));
    if(!recoveryStore.Put(tx,rc::Role::Host))throw std::runtime_error("Cannot persist ground decision");
    if(!RecoveryReconcile(*recoveryStore.Find(tx.id)))throw std::runtime_error("Ground decision replay deferred");
    m.tx=tx;m.op=rc::Op::Result;recoveryResponses[tx.id]=m;QueueRecovery(m);LootLog("ground_decision","tx="+tx.id+" action="+std::to_string(static_cast<unsigned>(tx.action))+" decision="+std::to_string(static_cast<unsigned>(tx.decision)));return true;
}
bool WorldGroundAction(oCNpc* actor,zCVob* vob,bool drop){
    if(worldConstructing||worldNativeDropPass||actor!=player||!CombatConnected())return false;
    auto* item=zDYNAMIC_CAST<oCItem>(vob);if(!item)return false;
    if(!recoveryReady||!RecoveryCanLoot()){notice=Ui("ui_999289eaa19c");LootLog("ground_blocked","item="+std::string(item->GetObjectName().ToChar())+" ready="+std::to_string(recoveryReady)+" reason="+RecoveryBlockReason());return true;}
    for(int i=0;i<8&&!worldOutbox.empty()&&lootOutbox.size()<48;++i){lootOutbox.push_back(std::move(worldOutbox.front()));worldOutbox.pop_front();}
    if(!worldGroundPending.empty()){notice=Ui("ui_69b8143492ba");return true;}
    if(drop&&(!LootEligible(item)||item->HasFlag(1<<12))){notice=Ui("ui_d83a0327940b");return true;}
    if(!drop&&HostCombat()){RegisterGround(item);return false;}
    try{
        if(!drop&&recoveryPeerTimeline.empty())return true;
        rc::Tx tx;tx.id=rc::NewGuid();tx.hostTimeline=HostCombat()?recoveryTimeline:recoveryPeerTimeline;tx.ownerTimeline=recoveryTimeline;tx.world=CurrentWorld();tx.action=drop?rc::Action::Drop:rc::Action::GroundTake;tx.source.kind=lo::Source::Ground;tx.item=item->GetInstance();tx.amount=static_cast<unsigned>(item->amount);
        const auto p=drop?player->GetPositionWorld()+player->GetAtVectorWorld()*100.0f:item->GetPositionWorld();tx.source.position={p[0],p[1]+(drop?75.0f:0.0f),p[2]};tx.source.name=drop?"ZK_DROP_"+tx.id:item->GetObjectName().ToChar();
        const auto role=HostCombat()?rc::Role::Local:rc::Role::Owner;
        LootLog("ground_request_begin","tx="+tx.id+" action="+std::to_string(static_cast<unsigned>(tx.action))+" item="+std::to_string(tx.item)+" amount="+std::to_string(tx.amount));
        if(!recoveryStore.Put(tx,role))throw std::runtime_error("Cannot prepare ground operation");
        if(drop){
            std::set<oCItem*> before;auto* gameWorld=ogame->GetGameWorld();if(gameWorld->voblist_items)for(auto* it=gameWorld->voblist_items->GetNextInList();it;it=it->GetNextInList())before.insert(it->GetData());
            const std::string originalName=item->GetObjectName().ToChar();item->SetObjectName(zSTRING(tx.source.name.c_str()));
            worldNativeDropPass=true;const int result=actor->DoDropVob(item);worldNativeDropPass=false;
            oCItem* dropped=item->homeWorld==gameWorld?item:nullptr;
            if(!dropped&&result&&gameWorld->voblist_items){int found=0;for(auto* it=gameWorld->voblist_items->GetNextInList();it;it=it->GetNextInList()){auto* candidate=it->GetData();if(candidate&&!before.count(candidate)&&candidate->homeWorld==gameWorld&&candidate->GetInstance()==static_cast<int>(tx.item)&&candidate->amount==static_cast<int>(tx.amount)){dropped=candidate;++found;}}if(found!=1)dropped=nullptr;}
            if(dropped!=item)item->SetObjectName(zSTRING(originalName.c_str()));
            if(!result){item->SetObjectName(zSTRING(originalName.c_str()));tx.decision=rc::Decision::Abort;if(!recoveryStore.Put(tx,role))throw std::runtime_error("Cannot record failed native drop");return true;}
            if(!dropped)throw std::runtime_error("Native drop result ambiguous; refusing guessed inventory accounting");
            dropped->SetObjectName(zSTRING(tx.source.name.c_str()));auto* ground=RegisterGround(dropped);if(!ground)throw std::runtime_error("Native drop registry unavailable");ground->state.name=tx.source.name;ground->state.present=true;ground->state.amount=tx.amount;
            recoveryEffects.insert(tx.id);
            if(HostCombat()){recoveryEffects.insert(tx.id+"c");tx.decision=rc::Decision::Commit;if(!recoveryStore.Put(tx,role))throw std::runtime_error("Cannot commit native host drop");}
            else dropped->RemoveVobFromWorld();
        }else if(!RecoveryReconcile(*recoveryStore.Find(tx.id)))throw std::runtime_error("Cannot reserve ground pickup");
        if(!HostCombat()){rc::Message m;m.op=rc::Op::Request;m.request=NextLootRequest();m.tx=tx;QueueRecovery(m);worldGroundPending=tx.id;LootLog("ground_request_queued","tx="+tx.id);}
    }catch(const std::exception& e){RecoveryFail(e.what());}
    return true;
}
bool WorldTradeTransfer(oCViewDialogTrade* dialog,bool selling,int count){
    if(!CombatConnected())return false;
    if(HostCombat()){if(dialog&&guestLootSource.corpse==dialog->NpcLeft){notice=Ui("ui_3c32a71e053e");return true;}return false;}
    if(!worldTradeReady||!dialog||!clientLootShown||clientPendingTake||!RecoveryCanLoot())return true;
    auto* item=selling?dialog->DlgInventoryPlayer->GetSelectedItem():dialog->DlgInventoryNpc->GetSelectedItem();if(!LootEligible(item)||count<=0)return true;
    if(item->GetInstance()==oCItemContainer::GetCurrencyInstance())return true;
    const unsigned amount=(std::min)(static_cast<unsigned>(count),static_cast<unsigned>(item->amount));
    unsigned token=1;if(!selling){bool found=false;for(const auto& e:clientLootItems)if(e.item==item&&e.entry.amount){token=e.entry.token;found=true;break;}if(!found)return true;}
    unsigned price=0;const auto multiplier=dialog->DlgInventoryNpc->ValueMultiplier;
    if(!ws::Price((std::max)(0,item->GetValue()),amount,selling,multiplier,price)){notice=Ui("ui_85f2a20082b3");return true;}
    if(!selling&&price){
        player->inventory2.UnpackAllItems();LootSource owner;owner.vob=owner.corpse=player;std::uint64_t gold=0;
        for(auto* coin:LootNativeItems(owner))if(coin->GetInstance()==oCItemContainer::GetCurrencyInstance())gold+=static_cast<unsigned>(coin->amount);
        if(gold<price){notice=Ui("ui_d490bc4105ac");LootLog("trade_insufficient_gold","price="+std::to_string(price)+" gold="+std::to_string(gold));return true;}
    }
    try{
        auto tx=RecoveryNew(selling?rc::Action::Sell:rc::Action::Buy,clientLootSource,item->GetInstance(),amount);tx.currency=oCItemContainer::GetCurrencyInstance();tx.price=price;
        if(!recoveryStore.Put(tx,rc::Role::Owner)||!RecoveryReconcile(*recoveryStore.Find(tx.id)))throw std::runtime_error("Cannot reserve trade payment/items");
        rc::Message m;m.op=rc::Op::Request;m.request=clientPendingTake=NextLootRequest();m.lease=clientLease;m.revision=clientLootRevision;m.token=token;m.tx=tx;recoveryUiTx=tx.id;QueueRecovery(m);lootClientWait=Now();
        LootLog("trade_request","tx="+tx.id+" amount="+std::to_string(amount)+" price="+std::to_string(price));
    }catch(const std::exception& e){RecoveryFail(e.what());}return true;
}
bool WorldValidatePrice(const rc::Message& m){
    if(m.tx.action!=rc::Action::Buy&&m.tx.action!=rc::Action::Sell)return true;
    if(m.tx.source.kind!=lo::Source::Merchant||m.tx.item==m.tx.currency||m.tx.currency!=static_cast<unsigned>(oCItemContainer::GetCurrencyInstance()))return false;
    lo::Entry e{1,m.tx.item,1};auto* sample=CreateLootItem(e);if(!sample)return false;unsigned price=0;
    const bool ok=ws::Price((std::max)(0,sample->GetValue()),m.tx.amount,m.tx.action==rc::Action::Sell,oCItemContainer::GetValueMultiplier(),price)&&price==m.tx.price;sample->Release();return ok;
}
void WorldReceive(const std::vector<std::uint8_t>& bytes){
    if(bytes.empty())return;
    if(bytes[0]==50){
        if(!HostCombat())return;auto copy=bytes;copy[0]=48;ws::Ground probe;if(!ws::Decode(copy,probe))return;
        rc::Tx query;query.item=probe.instance;query.source.name=probe.name;query.source.position=probe.position;
        auto* match=WorldGroundFor(query);
        if(match){auto* e=RegisterGround(match);if(e){e->state.revision=++combatRevision;QueueWorld(ws::Encode(e->state));}}
        else {int candidates=0;auto* world=ogame->GetGameWorld();if(world->voblist_items)for(auto* it=world->voblist_items->GetNextInList();it;it=it->GetNextInList()){auto* n=it->GetData();if(n&&n->homeWorld==world&&n->GetInstance()==static_cast<int>(probe.instance)&&Distance2(n->GetPositionWorld(),zVEC3(probe.position[0],probe.position[1],probe.position[2]))<2500)++candidates;}
            if(candidates==0){probe.id=nextGroundId++;probe.revision=++combatRevision;probe.present=false;QueueWorld(ws::Encode(probe));}
            else LootLog("ground_mapping_unresolved","ambiguous guest ground probe");
        }return;
    }
    if(bytes[0]==49||bytes[0]==51){
        unsigned cookie=0;auto copy=bytes;if(bytes[0]==51){if(bytes.size()<5)return;for(int i=0;i<4;++i)cookie|=static_cast<unsigned>(bytes[1+i])<<(8*i);copy.erase(copy.begin()+1,copy.begin()+5);}copy[0]=2;co::Npc state;if(!co::Decode(copy,state)){LootLog("world_invalid","NPC lifecycle codec rejected");return;}
        if(HostCombat()){
            const unsigned responseCookie=state.id;
            auto reply=[&](const co::Npc& answer){auto b=co::Encode(answer);b[0]=51;for(int i=3;i>=0;--i)b.insert(b.begin()+1,static_cast<unsigned char>(responseCookie>>(8*i)));QueueWorld(std::move(b));};
            NpcBinding* match=nullptr;float closest=4000000,second=4000000;
            for(auto& e:npcBindings){if(e->npc->GetInstance()!=state.instance||e->life==3)continue;const float d=Distance2(e->anchor,zVEC3(state.anchor[0],state.anchor[1],state.anchor[2]));if(d<closest){second=closest;closest=d;match=e.get();}else if(d<second)second=d;}
            if(match&&std::sqrt(second)-std::sqrt(closest)>=100){auto answer=WorldNpcState(*match);reply(answer);}
            else if(!match){state.id=nextNpcId++;state.life=3;state.generation=1;state.revision=++combatRevision;reply(state);}
            return;
        }
        NpcBinding* entry=nullptr;
        if(cookie){auto probe=npcProbeTargets.find(cookie);if(probe==npcProbeTargets.end())return;auto* original=probe->second;npcProbeTargets.erase(probe);original->probeCookie=0;
            if(original->npc->GetInstance()!=state.instance)return;
            auto canonical=npcById.find(state.id);
            if(canonical!=npcById.end()&&canonical->second!=original){original->mirror=true;original->life=3;original->hidden=true;original->npc->Disable();entry=canonical->second;LootLog("npc_duplicate_suppressed","instance="+std::to_string(state.instance));}
            else if(!original->mirror){original->id=state.id;original->mirror=true;npcById[state.id]=original;entry=original;}
            else entry=ResolveCombatNpc(state);
        }else entry=ResolveCombatNpc(state);
        if(!entry)return;
        if(!ws::AcceptNpc(entry->last?entry->generation:0,entry->revision,entry->life,state.generation,state.revision,state.life))return;
        entry->mirror=true;entry->generation=state.generation;entry->life=state.life;entry->revision=state.revision;entry->desired=state;entry->last=Now();WorldApplyNpc(*entry,state);WriteCombatStats(entry->npc,state.stats);return;
    }
    if(HostCombat())return;ws::Ground state;if(!ws::Decode(bytes,state)){LootLog("world_invalid","ground codec rejected");return;}
    GroundBinding* entry=nullptr;auto known=groundById.find(state.id);if(known!=groundById.end()){entry=known->second;if(!co::Newer(state.revision,entry->state.revision))return;}
    if(!entry){
        float closest=2500,second=2500;
        for(auto& e:groundBindings){if(e->bound||e->state.instance!=state.instance)continue;const bool tagMatch=state.name.rfind("ZK_DROP_",0)==0&&state.name==e->state.name;const auto d=tagMatch?0.0f:(std::min)(Distance2(zVEC3(e->state.anchor[0],e->state.anchor[1],e->state.anchor[2]),zVEC3(state.anchor[0],state.anchor[1],state.anchor[2])),Distance2(e->item->GetPositionWorld(),zVEC3(state.position[0],state.position[1],state.position[2])));if(d<closest){second=closest;closest=d;entry=e.get();}else if(d<second)second=d;}
        if(entry&&std::sqrt(second)-std::sqrt(closest)<=10){LootLog("ground_mapping_unresolved","ambiguous id="+std::to_string(state.id));return;}
        if(!entry&&state.present){lo::Entry e{1,state.instance,state.amount};auto* item=CreateLootItem(e);if(!item){combatFailure="Cannot create authoritative ground item";return;}item->SetObjectName(zSTRING(state.name.c_str()));item->SetPositionWorld(zVEC3(state.position[0],state.position[1],state.position[2]));ogame->GetWorld()->AddVob(item);entry=RegisterGround(item);if(entry)entry->owned=true;item->Release();}
        if(!entry)return;entry->bound=true;groundById[state.id]=entry;
    }
    entry->state=state;auto* item=entry->item;
    if(!state.present){if(item->homeWorld)item->RemoveVobFromWorld();return;}
    item->SetObjectName(zSTRING(state.name.c_str()));item->amount=state.amount;if(!item->homeWorld)ogame->GetWorld()->AddVob(item);
    item->SetPhysicsEnabled(FALSE);PlaceNetworkActor(*item,zVEC3(state.position[0],state.position[1],state.position[2]),0);
}
void WorldFrame(unsigned long long now){
    if(!CombatConnected())return;if(!worldConnectedAt)worldConnectedAt=now;
    if(now-lastWorldScan>=500){lastWorldScan=now;auto* world=ogame->GetGameWorld();if(world->voblist_items)for(auto* it=world->voblist_items->GetNextInList();it;it=it->GetNextInList())RegisterGround(it->GetData());}
    if(HostCombat()){
        for(auto& npc:npcBindings){const bool pause=WorldTradePaused(npc->npc);if(pause||npc->pausedByPeer){auto* model=zDYNAMIC_CAST<zCModel>(npc->npc->GetVisual());if(model){if(!npc->modelDefaultsCaptured){npc->rootScale=model->aniTransScale;npc->originalRotation=model->doVobRot;npc->modelDefaultsCaptured=true;}model->aniTransScale=pause?zVEC3(0,0,0):npc->rootScale;}npc->pausedByPeer=pause;}}
        if(now-lastWorldSend>=200){lastWorldSend=now;unsigned sent=0;std::size_t visited=0;
            while(!groundBindings.empty()&&visited++<groundBindings.size()&&sent<16&&lootOutbox.size()<80){auto& e=*groundBindings[(groundCursor++)%groundBindings.size()];auto& g=e.state;
                const bool present=e.item->homeWorld==ogame->GetWorld()&&e.item->amount>0;
                const bool changed=present!=g.present||(present&&e.item->amount!=static_cast<int>(g.amount));
                if(present){const auto p=e.item->GetPositionWorld();g.position={p[0],p[1],p[2]};g.amount=e.item->amount;}g.present=present;
                const bool nearby=haveTarget&&ws::Within(g.position,{target.x,target.y,target.z},8000);
                const bool moved=!ws::Within(g.position,e.sentPosition,1);
                if(!changed&&(!nearby||(e.lastSent&&now-e.lastSent<10000&&!moved)))continue;
                g.name=e.item->GetObjectName().ToChar();if(g.name.size()>63)g.name.clear();g.revision=++combatRevision;e.lastSent=now;e.sentPosition=g.position;QueueWorld(ws::Encode(g));++sent;
            }
        }
    }else if(now-worldConnectedAt>2000&&now-lastWorldSend>=250){
        lastWorldSend=now;int groundSent=0;for(auto& e:groundBindings){if(e->bound||e->probed||!e->item->homeWorld||!ws::Within(e->state.position,{player->GetPositionWorld()[0],player->GetPositionWorld()[1],player->GetPositionWorld()[2]},8000))continue;e->probed=true;auto probe=e->state;probe.id=static_cast<unsigned>(groundSent+1);probe.revision=1;auto b=ws::Encode(probe);b[0]=50;QueueWorld(std::move(b));if(++groundSent>=4)break;}
        int sent=0;for(auto& e:npcBindings){if(e->mirror||e->owned||CharacterLocal(e->npc)||Distance2(e->anchor,player->GetPositionWorld())>64000000.0f)continue;
            const auto key=std::to_string(e->npc->GetInstance())+":"+std::to_string(e->anchor[0])+":"+std::to_string(e->anchor[2]);if(npcProbes.count(key)&&now-npcProbes[key]<5000)continue;
            npcProbes[key]=now;if(e->probeCookie)npcProbeTargets.erase(e->probeCookie);if(npcProbeSerial>=999999){combatFailure="NPC probe sequence exhausted";break;}const auto serial=++npcProbeSerial;e->probeCookie=serial;npcProbeTargets[serial]=e.get();co::Npc probe;probe.id=serial;probe.instance=e->npc->GetInstance();probe.anchor={e->anchor[0],e->anchor[1],e->anchor[2]};probe.stats=ReadCombatStats(e->npc);auto b=co::Encode(probe);b[0]=49;QueueWorld(std::move(b));if(++sent>=4)break;
        }
    }
    for(int i=0;i<8&&!worldOutbox.empty()&&lootOutbox.size()<48;++i){lootOutbox.push_back(std::move(worldOutbox.front()));worldOutbox.pop_front();}
    if(!worldGroundPending.empty()){const auto* r=recoveryStore.Find(worldGroundPending);if(r&&r->tx.decision!=rc::Decision::Prepared)worldGroundPending.clear();}
    if(now-lastWorldLog>=2000){lastWorldLog=now;unsigned active=0,owned=0,removed=0;for(auto& e:npcBindings){if(e->npc->homeWorld)++active;if(e->owned)++owned;if(e->life==3)++removed;}
        LootLog("world_sync","npcs="+std::to_string(npcBindings.size())+" active="+std::to_string(active)+" clones="+std::to_string(owned)+" removed="+std::to_string(removed)+" ground="+std::to_string(groundBindings.size())+" dual_passes="+std::to_string(worldDualPasses)+" timers_restored="+std::to_string(worldTimersRestored)+" guest_ai_ticks="+std::to_string(worldGuestTicks)+" spawn_queue="+std::to_string(ogame->spawnman?ogame->spawnman->spawnList.GetNumInList():0)+" world_outbox="+std::to_string(worldOutbox.size())+" outbox="+std::to_string(lootOutbox.size())+" player_separation="+(haveTarget?std::to_string(std::sqrt(Distance2(player->GetPositionWorld(),zVEC3(target.x,target.y,target.z)))):"unknown"));
    }
}
