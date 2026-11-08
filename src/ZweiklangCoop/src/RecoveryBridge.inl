namespace rc=recovery;
rc::Store recoveryStore;
std::string recoveryDir,recoveryTimeline,recoveryPeerTimeline,recoveryError,recoveryUiTx;
std::set<std::string> recoveryEffects;
std::map<std::string,unsigned long long> recoveryQueries;
std::map<std::string,bool> recoveryLockSent;
std::map<std::string,rc::Message> recoveryResponses;
bool recoveryReady=false,recoveryLoading=false,recoveryHello=false,recoveryBlocked=false,recoveryStoreReady=false;
unsigned long long recoveryLastLocks=0,recoveryLastReplay=0,recoveryLastQueryScan=0;
int recoverySaveSlot=-1;
bool recoverySaveStarted=false;
bool recoveryNativeSaveCompleted=false;
native_save::Stamp recoverySavedatBefore,recoveryWorldBefore;
std::string recoverySavingWorld;
rc::Checkpoint recoverySavingCheckpoint;
std::string recoveryLastWorld;
std::map<std::string,unsigned long long> recoveryLockTimes;

void RecoveryFail(const std::string& why){recoveryError=why;recoveryBlocked=true;LootLog("recovery_error",why);notice="Inventarabgleich angehalten; Details im Recovery-Log.";}
void QueueRecovery(const rc::Message& m){if(lootOutbox.size()>=128){RecoveryFail("Recovery queue full");return;}lootOutbox.push_back(rc::Encode(m));}
void RecoverySessionReset(){recoveryResponses.clear();recoveryPeerTimeline.clear();recoveryHello=false;recoveryUiTx.clear();recoveryQueries.clear();recoveryLockSent.clear();recoveryLastLocks=0;recoveryLockTimes.clear();recoveryLastQueryScan=0;}
void RecoveryInit(){
    try{recoveryDir=systemDirectory+"\\ZweiklangCoopState";if(!CreateDirectoryA(recoveryDir.c_str(),nullptr)&&GetLastError()!=ERROR_ALREADY_EXISTS)throw std::runtime_error("Cannot create recovery directory");recoveryStore.Open(recoveryDir+"\\transactions.bin");recoveryStoreReady=true;}
    catch(const std::exception& e){RecoveryFail(e.what());}
}
std::string RecoverySlot(int slot){
    if(!zoptions||!gameMan||!gameMan->savegameManager||slot<0||slot>20)throw std::runtime_error("Recovery save slot unavailable");
    // GetDirString is a configured path; GetFullPath resolves the engine's
    // native directory without relying on Gothic's changing working directory.
    auto* directory=zoptions->GetDir(DIR_SAVEGAMES);
    if(!directory)throw std::runtime_error("Native save directory unavailable");
    const std::string base=directory->GetFullPath().ToChar();
    const std::string relative=gameMan->savegameManager->GetSlotDirName(slot).ToChar();
    const auto path=native_save::Slot(base,relative);
    LootLog("recovery_save_path","slot="+std::to_string(slot)+" configured="+zoptions->GetDirString(DIR_SAVEGAMES).ToChar()+" native="+base+" resolved="+path);
    return path;
}
std::string RecoveryFingerprint(const std::string& slot){
    const auto path=slot+"SAVEINFO.SAV";auto b=rc::ReadFile(path);std::uint64_t hash=14695981039346656037ull;for(auto c:b){hash^=c;hash*=1099511628211ull;}
    WIN32_FILE_ATTRIBUTE_DATA attrs;if(!GetFileAttributesExA(path.c_str(),GetFileExInfoStandard,&attrs))throw std::runtime_error("Recovery save header unavailable");
    std::ostringstream s;s<<std::hex<<hash<<"-"<<attrs.ftLastWriteTime.dwHighDateTime<<"-"<<attrs.ftLastWriteTime.dwLowDateTime;return s.str();
}
void RecoveryBeginLoad(int slot){
    recoveryLastReplay=0;recoveryLoading=true;recoveryReady=false;recoveryTimeline.clear();recoveryEffects.clear();recoveryBlocked=false;
    if(!recoveryStoreReady)return;recoveryError.clear();
    try{
        const auto dir=RecoverySlot(slot),fp=RecoveryFingerprint(dir);const auto checkpoint=dir+"ZWEIKLANG_RECOVERY.BIN";
        if(GetFileAttributesA((recoveryDir+"\\saving-slot"+std::to_string(slot)+".bin").c_str())!=INVALID_FILE_ATTRIBUTES)throw std::runtime_error("Native save was interrupted; checkpoint cannot be trusted");
        rc::Checkpoint c;
        if(GetFileAttributesA(checkpoint.c_str())!=INVALID_FILE_ATTRIBUTES){c=rc::DecodeCheckpoint(rc::ReadFile(checkpoint));if(c.profile!=recoveryStore.Profile()||c.fingerprint!=fp)throw std::runtime_error("Save checkpoint/profile mismatch; do not replay against another save");}
        else {
            const auto baseline=recoveryDir+"\\baseline-"+fp+".bin";
            if(GetFileAttributesA(baseline.c_str())!=INVALID_FILE_ATTRIBUTES)c=rc::DecodeCheckpoint(rc::ReadFile(baseline));
            else {c.profile=recoveryStore.Profile();c.timeline=rc::NewGuid();c.fingerprint=fp;rc::AtomicWrite(baseline,rc::Encode(c));}
            if(c.profile!=recoveryStore.Profile()||c.fingerprint!=fp)throw std::runtime_error("Recovery baseline mismatch");
        }
        for(const auto& id:c.effects)if(!recoveryStore.Find(id.substr(0,32)))throw std::runtime_error("Checkpoint references missing transaction; state files must be restored together");
        recoveryTimeline=c.timeline;recoveryEffects=c.effects;
        LootLog("recovery_checkpoint_load","slot="+std::to_string(slot)+" effects="+std::to_string(c.effects.size()));
    }catch(const std::exception& e){RecoveryFail(e.what());}
}
void RecoveryEndLoad(){recoveryLoading=false;recoveryReady=!recoveryTimeline.empty()&&recoveryError.empty();}
rc::Checkpoint RecoveryBeforeSave(int slot){
    rc::Checkpoint c;
    try{
        if(!recoveryError.empty())throw std::runtime_error(recoveryError);
        if(recoveryTimeline.empty())recoveryTimeline=rc::NewGuid();
        c.profile=recoveryStore.Profile();c.timeline=recoveryTimeline;c.effects=recoveryEffects;
        const auto dir=RecoverySlot(slot);
        native_save::PrepareDirectory(dir);
        recoverySavedatBefore=native_save::Capture(dir+"SAVEDAT.SAV");
        recoverySavingWorld=CurrentWorld();const auto dot=recoverySavingWorld.find_last_of('.');if(dot!=std::string::npos)recoverySavingWorld.resize(dot);recoverySavingWorld+=".SAV";
        recoveryWorldBefore=native_save::Capture(dir+recoverySavingWorld);
        if(GetFileAttributesA((dir+"SAVEINFO.SAV").c_str())!=INVALID_FILE_ATTRIBUTES)c.fingerprint=RecoveryFingerprint(dir);
        rc::AtomicWrite(recoveryDir+"\\saving-slot"+std::to_string(slot)+".bin",rc::Encode(c));
    }catch(const std::exception& e){RecoveryFail(e.what());}
    return c;
}
void RecoveryAfterSave(int slot,rc::Checkpoint c){
    if(!recoveryError.empty())return;
    try{const auto dir=RecoverySlot(slot);const auto fp=RecoveryFingerprint(dir);if(!recoveryNativeSaveCompleted||!native_save::Advanced(recoverySavedatBefore,native_save::Capture(dir+"SAVEDAT.SAV"))||!native_save::Advanced(recoveryWorldBefore,native_save::Capture(dir+recoverySavingWorld)))throw std::runtime_error("Native save incomplete: world/character files not advanced");if(!c.fingerprint.empty()&&fp==c.fingerprint)throw std::runtime_error("Native save header unchanged; checkpoint not advanced");c.fingerprint=fp;rc::AtomicWrite(dir+"ZWEIKLANG_RECOVERY.BIN",rc::Encode(c));if(!DeleteFileA((recoveryDir+"\\saving-slot"+std::to_string(slot)+".bin").c_str()))throw std::runtime_error("Cannot finish save checkpoint guard");recoveryReady=true;LootLog("recovery_checkpoint_save","slot="+std::to_string(slot)+" effects="+std::to_string(c.effects.size()));}
    catch(const std::exception& e){RecoveryFail(e.what());}
}
LootSource RecoverySource(const rc::Tx& tx){
    LootSource s;s.identity=tx.source;
    if(tx.world!=CurrentWorld())return s;
    auto* world=ogame->GetGameWorld();
    if(tx.source.kind==lo::Source::Chest){
        int matches=0;if(world->voblist)for(auto* it=world->voblist->GetNextInList();it;it=it->GetNextInList()){
            auto* c=zDYNAMIC_CAST<oCMobContainer>(it->GetData());if(c&&tx.source.name==c->GetObjectName().ToChar()&&Distance2(c->GetPositionWorld(),zVEC3(tx.source.position[0],tx.source.position[1],tx.source.position[2]))<2500){s.vob=s.chest=c;++matches;}}
        if(matches!=1)s=LootSource{};
    }else {
        std::vector<oCNpc*> npcs;std::vector<rc::NpcCandidate> candidates;
        if(world->voblist_npcs)for(auto* it=world->voblist_npcs->GetNextInList();it;it=it->GetNextInList()){
            auto* n=it->GetData();if(!n||n==player||n==PartnerNpc()||n->GetInstance()!=tx.instance)continue;
            const auto current=n->GetPositionWorld();auto anchor=current;const auto binding=npcByPointer.find(n);
            if(binding!=npcByPointer.end()){anchor=binding->second->anchor;if(binding->second->mirror){const auto& p=binding->second->desired.anchor;anchor=zVEC3(p[0],p[1],p[2]);}}
            candidates.push_back({n->GetObjectName().ToChar(),n->IsHuman()!=FALSE,{current[0],current[1],current[2]},{anchor[0],anchor[1],anchor[2]}});npcs.push_back(n);
        }
        const int resolved=rc::ResolveNpcSource(tx.source.name,tx.source.position,candidates);
        if(resolved>=0)s.vob=s.corpse=npcs[resolved];
    }
    return s;
}
bool RecoveryAdd(const LootSource& source,unsigned instance,unsigned amount){
    lo::Entry e{1,instance,amount};auto* item=CreateLootItem(e);if(!item)return false;
    bool ok=true;if(source.corpse)ok=source.corpse->PutInInv(item)!=nullptr;else if(source.chest)source.chest->Insert(item);else ok=false;item->Release();return ok;
}
bool RecoveryRemove(const LootSource& source,unsigned instance,unsigned amount){
    if(source.corpse)source.corpse->inventory2.UnpackAllItems();
    const auto items=LootNativeItems(source);unsigned total=0;for(auto* i:items)if(static_cast<unsigned>(i->GetInstance())==instance)total+=static_cast<unsigned>(i->amount);
    if(total<amount)return false;
    for(auto* i:items)if(static_cast<unsigned>(i->GetInstance())==instance&&amount){const auto n=(std::min)(amount,static_cast<unsigned>(i->amount));i->AddRef();const bool ok=LootRemove(source,i,n);i->Release();if(!ok)return false;amount-=n;}
    return amount==0;
}
lo::Identity RecoveryIdentity(const LootSource& source);
bool RecoveryApply(const rc::Record& record,bool desired){
    const auto& tx=record.tx;
    bool questHandled=false;const auto questResult=QuestRecoveryEffect(record,desired,questHandled);if(questHandled)return questResult;
    bool worldHandled=false;const auto worldResult=WorldRecoveryEffect(record,desired,worldHandled);if(worldHandled)return worldResult;
    LootSource source;
    if(record.role==rc::Role::Owner&&tx.action!=rc::Action::Unlock)source.vob=source.corpse=player;
    else {
        if(record.role==rc::Role::Host&&guestLootSource.vob){const auto id=RecoveryIdentity(guestLootSource);if(id.kind==tx.source.kind&&id.name==tx.source.name&&id.position==tx.source.position)source=guestLootSource;}
        if(!source.vob)source=RecoverySource(tx);
    }
    if(!source.vob)return false;
    bool ok;
    if(tx.action==rc::Action::Unlock){if(!source.chest)return false;source.chest->SetLocked(desired?FALSE:TRUE);ok=true;}
    else {const bool increase=record.role==rc::Role::Host?(tx.action==rc::Action::Put):(tx.action==rc::Action::Take);ok=(increase==desired)?RecoveryAdd(source,tx.item,tx.amount):RecoveryRemove(source,tx.item,tx.amount);}
    if(ok)LootLog("recovery_effect","tx="+tx.id+" role="+std::to_string(static_cast<unsigned>(record.role))+" desired="+std::to_string(desired)+" instance="+std::to_string(tx.item)+" amount="+std::to_string(tx.amount));
    return ok;
}
bool RecoveryReconcile(const rc::Record& r){return rc::Reconcile(r,recoveryEffects,RecoveryApply);}
bool RecoveryCanLoot(){
    if(saving||network.PeerSaving()){notice="Inventar wartet: Speichervorgang laeuft.";return false;}
    if(!recoveryError.empty()||recoveryBlocked){notice="Inventarabgleich gesperrt; konkrete Ursache in der Diagnose (Strg+Umschalt+D).";return false;}
    if(CombatConnected()&&!recoveryReady){notice="Bitte Testspielstand laden oder einmal speichern, damit Inventar-Recovery gebunden ist.";return false;}
    return true;
}
lo::Identity RecoveryIdentity(const LootSource& source){
    auto id=source.identity;
    if(source.corpse){id.name=source.corpse->GetObjectName().ToChar();auto it=npcByPointer.find(source.corpse);auto p=it==npcByPointer.end()?source.corpse->GetPositionWorld():it->second->anchor;if(it!=npcByPointer.end()&&it->second->mirror)p=zVEC3(it->second->desired.anchor[0],it->second->desired.anchor[1],it->second->desired.anchor[2]);id.position={p[0],p[1],p[2]};}
    return id;
}
rc::Tx RecoveryNew(rc::Action action,const LootSource& source,unsigned instance,unsigned amount){
    rc::Tx tx;tx.id=rc::NewGuid();tx.hostTimeline=HostCombat()?recoveryTimeline:recoveryPeerTimeline;tx.ownerTimeline=recoveryTimeline;tx.world=CurrentWorld();tx.action=action;tx.source=RecoveryIdentity(source);tx.instance=source.corpse?source.corpse->GetInstance():0;tx.item=instance;tx.amount=amount;return tx;
}
bool RecoveryClientTransfer(oCItem* item,unsigned count,unsigned token,bool put){
    if(!item||!count||!clientLease||clientPendingTake||!clientLootShown||!RecoveryCanLoot())return false;
    if(!recoveryReady||recoveryPeerTimeline.empty())return false;
    try{
        auto tx=RecoveryNew(put?rc::Action::Put:rc::Action::Take,clientLootSource,item->GetInstance(),count);
        if(!recoveryStore.Put(tx,rc::Role::Owner))throw std::runtime_error("Cannot prepare owner transfer");
        if(put&&!RecoveryReconcile(*recoveryStore.Find(tx.id)))throw std::runtime_error("Cannot reserve deposit inventory");
        rc::Message m;m.op=rc::Op::Request;m.tx=tx;m.request=clientPendingTake=NextLootRequest();m.lease=clientLease;m.revision=clientLootRevision;m.token=token;recoveryUiTx=tx.id;QueueRecovery(m);lootClientWait=Now();
        return true;
    }catch(const std::exception& e){RecoveryFail(e.what());return false;}
}
bool RecoveryTransfer(oCItemContainer* container,int amount){
    if(!RecoveryCanLoot())return true;
    if(!CombatConnected()||HostCombat()||!clientLootSource.vob)return false;
    if(container!=LootUI(clientLootSource)&&container!=&player->inventory2)return false;
    if(clientLootSource.identity.kind==lo::Source::Merchant)return true;
    if(clientPendingTake||!clientLease||!clientLootShown)return true;
    if(!recoveryReady||recoveryPeerTimeline.empty()){notice="Recovery wartet auf Spielstand und Host; neuen Spielstand einmal speichern.";return true;}
    auto* item=container->GetSelectedItem();if(!LootEligible(item)||amount<=0)return true;
    const bool put=container==&player->inventory2;
    unsigned token=1,count=(std::min)(static_cast<unsigned>(item->amount),static_cast<unsigned>(container->CanTransferMoreThanOneItem()?amount:1));
    if(!put){bool found=false;for(const auto& e:clientLootItems)if(e.item==item&&e.entry.amount){token=e.entry.token;count=(std::min)(count,e.entry.amount);found=true;break;}if(!found)return true;}
    RecoveryClientTransfer(item,count,token,put);
    return true;
}
void RecoveryResult(const rc::Message& m){
    const auto* old=recoveryStore.Find(m.tx.id);
    if(!old||old->role!=rc::Role::Owner||!rc::Same(old->tx,m.tx)||m.tx.decision==rc::Decision::Prepared)throw std::runtime_error("Unsolicited/conflicting persistent decision");
    const bool first=!recoveryEffects.count(m.tx.id+(m.tx.action==rc::Action::Buy?"c":""));
    if(!recoveryStore.Put(m.tx,rc::Role::Owner))throw std::runtime_error("Conflicting terminal decision");
    if(!RecoveryReconcile(*recoveryStore.Find(m.tx.id)))throw std::runtime_error("Owner decision requires unavailable inventory/source; replay deferred");
    if(first&&m.tx.decision==rc::Decision::Commit&&(clientQuickLoot||m.tx.action==rc::Action::GroundTake))QolNotify(m.tx.item,m.tx.amount);
    if(m.tx.id==recoveryUiTx){
        if(m.tx.decision==rc::Decision::Commit&&(m.tx.action==rc::Action::Take||m.tx.action==rc::Action::Buy)&&first&&clientLootShown){
            if(m.token==0||m.token>clientLootItems.size())throw std::runtime_error("Grant mirror token invalid");auto& item=clientLootItems[m.token-1];
            if(item.entry.instance!=m.tx.item||item.entry.amount<m.tx.amount||!LootRemove(clientLootSource,item.item,m.tx.amount))throw std::runtime_error("Owner granted, source mirror refresh required");item.entry.amount-=m.tx.amount;
        }
        if(m.tx.action==rc::Action::Put||m.tx.action==rc::Action::Sell||m.tx.decision==rc::Decision::Abort||!m.lease)clientLootClosing=true;
        else clientLootRevision=m.revision;
        if(!clientQuickLoot)if(auto* ui=LootUI(clientLootSource))ui->CheckSelectedItem();
        clientPendingTake=0;lootClientWait=0;recoveryUiTx.clear();
    }
    auto ack=m;ack.op=rc::Op::Receipt;QueueRecovery(ack);
}
bool RecoveryValidateHostRequest(const rc::Message& m){
    if(!recoveryReady||m.tx.hostTimeline!=recoveryTimeline||m.tx.world!=CurrentWorld()||m.tx.decision!=rc::Decision::Prepared)return false;
    LootSource s;if(m.tx.action!=rc::Action::Unlock&&guestLootSource.vob){const auto id=RecoveryIdentity(guestLootSource);if(id.kind==m.tx.source.kind&&id.name==m.tx.source.name&&id.position==m.tx.source.position)s=guestLootSource;}else s=RecoverySource(m.tx);
    if(!s.vob||s.vob->homeWorld!=ogame->GetWorld()||!haveTarget||Now()-lastSnapshotAt>1000||Distance2(s.vob->GetPositionWorld(),zVEC3(target.x,target.y,target.z))>250000)return false;
    if(m.tx.action==rc::Action::Unlock)return s.chest&&(!guestLootSource.vob||guestLootSource.vob==s.vob)&&!LootUIOpen(s);
    return s.vob==guestLootSource.vob&&m.lease==lootLedger.Lease()&&m.revision==lootLedger.Revision()&&(!s.chest||!s.chest->locked)&&(!s.corpse||(m.tx.source.kind==lo::Source::Merchant?!s.corpse->IsDead():(s.corpse->IsDead()||s.corpse->IsUnconscious())))&&WorldValidatePrice(m);
}
void RecoveryReceive(const std::vector<std::uint8_t>& bytes){
    try{
        rc::Message m;if(!rc::Decode(bytes,m))throw std::runtime_error("Malformed recovery message");
        if(m.op==rc::Op::Hello){if(!HostCombat()){if(!recoveryPeerTimeline.empty()&&recoveryPeerTimeline!=m.tx.hostTimeline)throw std::runtime_error("Host save lineage changed during session");recoveryPeerTimeline=m.tx.hostTimeline;}return;}
        if(m.op==rc::Op::LockState){
            if(!HostCombat()&&m.tx.hostTimeline==recoveryPeerTimeline&&m.tx.world==CurrentWorld()){
                bool pending=false;for(const auto& r:recoveryStore.Records()){const auto& tx=r.second.tx;if(r.second.role==rc::Role::Owner&&tx.decision==rc::Decision::Prepared&&tx.action==rc::Action::Unlock&&tx.source.name==m.tx.source.name&&tx.source.position==m.tx.source.position){pending=true;break;}}
                if(!pending){auto s=RecoverySource(m.tx);if(s.chest)s.chest->SetLocked(m.locked?TRUE:FALSE);}
            }return;
        }
        if(!recoveryError.empty()){LootLog("recovery_blocked",recoveryError);return;}
        if(!recoveryReady)throw std::runtime_error("No trusted save checkpoint for recovery");
        if(m.tx.source.kind==lo::Source::Quest){QuestReceiveReward(m);return;}
        if(!HostCombat()){if(m.op!=rc::Op::Result||m.tx.hostTimeline!=recoveryPeerTimeline)throw std::runtime_error("Wrong recovery direction/lineage");RecoveryResult(m);return;}
        if(m.op==rc::Op::Receipt){lootLedger.Applied(m.request,m.lease);lootLastTraffic=Now();LootLog("recovery_receipt",m.tx.id);return;}
        if(m.op!=rc::Op::Request&&m.op!=rc::Op::Query)throw std::runtime_error("Wrong owner recovery message");
        if(m.tx.hostTimeline!=recoveryTimeline)throw std::runtime_error("Recovery belongs to another host save lineage");
        const auto* previous=recoveryStore.Find(m.tx.id);
        if(previous){
            if(previous->role!=rc::Role::Host||!rc::Same(previous->tx,m.tx))throw std::runtime_error("Recovery ID payload conflict");
            if(!RecoveryReconcile(*previous))return;
            const auto cached=recoveryResponses.find(m.tx.id);if(cached!=recoveryResponses.end()){QueueRecovery(cached->second);return;}
            m.tx=previous->tx;m.op=rc::Op::Result;QueueRecovery(m);return;
        }
        if(WorldHostRequest(m))return;
        auto decided=m.tx;bool accepted=false;
        if(m.op==rc::Op::Request&&RecoveryValidateHostRequest(m)){
            auto apply=[&](const lo::Entry& entry,std::uint32_t){
                if((m.tx.action==rc::Action::Take||m.tx.action==rc::Action::Buy)&&(m.token>guestLootItems.size()||!m.token||guestLootItems[m.token-1].item->amount!=static_cast<int>(entry.amount)))return false;
                decided.decision=rc::Decision::Commit;if(!recoveryStore.Put(decided,rc::Role::Host))throw std::runtime_error("Cannot persist host decision");
                if(m.tx.action==rc::Action::Take||m.tx.action==rc::Action::Buy)return rc::Reconcile(*recoveryStore.Find(decided.id),recoveryEffects,[&](const rc::Record&,bool desired){return desired&&LootRemove(guestLootSource,guestLootItems[m.token-1].item,m.tx.amount);});
                return RecoveryReconcile(*recoveryStore.Find(decided.id));
            };
            if(m.tx.action==rc::Action::Unlock)accepted=apply({},0);
            else {lo::Message legacy;legacy.op=lo::Op::Take;legacy.request=m.request;legacy.lease=m.lease;legacy.revision=m.revision;legacy.item={m.token,m.tx.item,m.tx.amount};
                const auto result=(m.tx.action==rc::Action::Put||m.tx.action==rc::Action::Sell)?lootLedger.Put(legacy,apply):lootLedger.Take(legacy,apply);accepted=result.status==lo::Status::Ok;m.revision=result.revision;m.token=result.item.token;
            }
        }
        previous=recoveryStore.Find(m.tx.id);
        if(!previous){
            if(m.op==rc::Op::Query&&m.tx.decision==rc::Decision::Commit)throw std::runtime_error("Owner has commit but host record is missing");
            decided.decision=rc::Decision::Abort;if(!recoveryStore.Put(decided,rc::Role::Host))throw std::runtime_error("Cannot persist abort tombstone");
        }else if(!accepted&&!recoveryEffects.count(m.tx.id))return;
        m.tx=recoveryStore.Find(m.tx.id)->tx;m.op=rc::Op::Result;recoveryResponses[m.tx.id]=m;QueueRecovery(m);lootLastTraffic=Now();
        LootLog("recovery_decision","tx="+m.tx.id+" action="+std::to_string(static_cast<unsigned>(m.tx.action))+" decision="+std::to_string(static_cast<unsigned>(m.tx.decision)));
    }catch(const std::exception& e){RecoveryFail(e.what());}
}
// The original Gothic Unlock performs key/talent/lockpick tests. Record BEFORE
// entering it; only a real locked->unlocked transition is sent to the host.
rc::Tx RecoveryUnlockBegin(oCMobLockable* lock,oCNpc* who){
    rc::Tx empty;if(lootNativeBypass||who!=player||!CombatConnected()||!lock->locked||!recoveryReady||(!HostCombat()&&recoveryPeerTimeline.empty()))return empty;
    auto* chest=zDYNAMIC_CAST<oCMobContainer>(lock);if(!chest)return empty;
    try{LootSource s;s.vob=s.chest=chest;s.identity=LootIdentity(chest);auto tx=RecoveryNew(rc::Action::Unlock,s,0,0);if(!recoveryStore.Put(tx,HostCombat()?rc::Role::Host:rc::Role::Owner))throw std::runtime_error("Cannot prepare native unlock");return tx;}
    catch(const std::exception& e){RecoveryFail(e.what());return empty;}
}
void RecoveryUnlockEnd(oCMobLockable* lock,rc::Tx tx){
    if(tx.id.empty())return;
    try{
        const auto* prepared=recoveryStore.Find(tx.id);if(!prepared)throw std::runtime_error("Unlock preparation missing");const bool wasHost=prepared->role==rc::Role::Host;
        if(lock->locked){tx.decision=rc::Decision::Abort;if(!recoveryStore.Put(tx,prepared->role))throw std::runtime_error("Cannot persist failed unlock");return;}
        recoveryEffects.insert(tx.id);
        if(wasHost){tx.decision=rc::Decision::Commit;if(!recoveryStore.Put(tx,rc::Role::Host))throw std::runtime_error("Cannot persist host unlock");}
        else {rc::Message m;m.op=rc::Op::Request;m.request=NextLootRequest();m.tx=tx;QueueRecovery(m);}
    }catch(const std::exception& e){RecoveryFail(e.what());}
}
void RecoveryFrame(){
    if(recoveryLoading||recoverySaveSlot>=0||!player||!ogame||!ogame->GetWorld()||!recoveryError.empty())return;
    try{
        const auto now=Now();
        if(recoveryReady&&(recoveryLastReplay==0||now-recoveryLastReplay>=1000||recoveryLastWorld!=CurrentWorld())){
        recoveryLastReplay=now;recoveryLastWorld=CurrentWorld();recoveryBlocked=false;
        for(auto r:recoveryStore.Ordered()){
            if(r.role==rc::Role::Local&&r.tx.decision==rc::Decision::Prepared){r.tx.decision=rc::Decision::Abort;if(!recoveryStore.Put(r.tx,r.role))throw std::runtime_error("Cannot abort interrupted local operation");}
            if((r.role==rc::Role::Host?r.tx.hostTimeline:r.tx.ownerTimeline)!=recoveryTimeline)continue;
            if((r.role==rc::Role::Host||r.role==rc::Role::Local||r.tx.action==rc::Action::Unlock)&&r.tx.world!=CurrentWorld())continue;
            if(!RecoveryReconcile(r))recoveryBlocked=true;
        }}
        if(!CombatConnected()){recoveryHello=false;recoveryLockSent.clear();return;}
        if(HostCombat()){
            if(recoveryReady&&!recoveryHello){rc::Message hello;hello.tx.hostTimeline=recoveryTimeline;QueueRecovery(hello);recoveryHello=true;}
            if(recoveryReady&&now-recoveryLastLocks>=1000){
                recoveryLastLocks=now;
                struct Candidate {oCMobContainer* chest;lo::Identity identity;std::string key;unsigned long long last;};
                std::vector<Candidate> candidates;auto* world=ogame->GetGameWorld();
                if(world->voblist)for(auto* it=world->voblist->GetNextInList();it;it=it->GetNextInList()){
                    auto* c=zDYNAMIC_CAST<oCMobContainer>(it->GetData());if(!c)continue;
                    if(Distance2(c->GetPositionWorld(),player->GetPositionWorld())>36000000.0f&&(!haveTarget||Distance2(c->GetPositionWorld(),zVEC3(target.x,target.y,target.z))>36000000.0f))continue;
                    auto id=LootIdentity(c);const auto key=id.name+std::to_string(id.position[0])+","+std::to_string(id.position[1])+","+std::to_string(id.position[2]);
                    const auto old=recoveryLockSent.find(key);const bool changed=old==recoveryLockSent.end()||old->second!=(c->locked!=0);
                    const auto last=changed?0:recoveryLockTimes[key];
                    if(changed||now-last>=5000)candidates.push_back({c,id,key,last});
                }
                std::sort(candidates.begin(),candidates.end(),[](const Candidate& a,const Candidate& b){return a.last<b.last;});
                for(std::size_t i=0;i<(std::min)(std::size_t(8),candidates.size());++i){
                    const auto& c=candidates[i];rc::Message m;m.op=rc::Op::LockState;m.tx.hostTimeline=recoveryTimeline;m.tx.world=CurrentWorld();m.tx.source=c.identity;m.locked=c.chest->locked!=0;
                    QueueRecovery(m);recoveryLockSent[c.key]=m.locked;recoveryLockTimes[c.key]=now;
                }
            }
        }else if(!recoveryPeerTimeline.empty()&&recoveryReady&&now-recoveryLastQueryScan>=500){
            recoveryLastQueryScan=now;int sent=0;for(const auto& r:recoveryStore.Ordered()){
                if(r.role!=rc::Role::Owner||r.tx.ownerTimeline!=recoveryTimeline||r.tx.hostTimeline!=recoveryPeerTimeline||r.tx.decision!=rc::Decision::Prepared||now-recoveryQueries[r.tx.id]<2000)continue;
                recoveryQueries[r.tx.id]=now;rc::Message query;query.op=rc::Op::Query;query.request=NextLootRequest();query.tx=r.tx;QueueRecovery(query);if(++sent>=4)break;
            }
        }
    }catch(const std::exception& e){RecoveryFail(e.what());}
}
