// Ninja Quickloot's unguarded CreateInvItems path is bypassed in a coop session.
// Native engine access remains on the main thread; all guest credits use recovery.
unsigned long long qolLastLoot=0;
std::deque<std::pair<unsigned long long,std::string>> qolMessages;
qol::SessionLatch qolLatch;bool qolWasGuest=false;
void QolGuard(bool active){
    if(active)qolWasGuest=!HostCombat();const bool block=qol::BlockUpstream(qolLatch,active,recoveryBlocked||!recoveryError.empty());
    QuestSetInt("ZK_COOP_QOL_ACTIVE",block?1:0);QuestSetInt("ZK_COOP_QOL_GUEST",block&&qolWasGuest?1:0);
    if(!active)qolMessages.clear();
}
void QolReset(){qolLatch.Reset();qolWasGuest=false;qolLastLoot=0;QolGuard(false);}
void QolNotify(unsigned instance,unsigned amount){
    if(!player||!parser)return;QuestBindings bindings;std::string label="Gegenstand";
    if(auto* item=player->IsInInv(static_cast<int>(instance),1))label=item->description.Length()?item->description.ToChar():item->name.ToChar();
    if(label.size()>70)label.resize(70);
    qolMessages.push_back({Now()+4000,"Erhalten: "+std::to_string(amount)+" x "+label});
    while(qolMessages.size()>3)qolMessages.pop_front();
    LootLog("quickloot_notice","instance="+std::to_string(instance)+" amount="+std::to_string(amount)+" name="+ConvertText(label,CP_ACP,CP_UTF8));
}
void QolRender(){
    const auto now=Now();while(!qolMessages.empty()&&qolMessages.front().first<now)qolMessages.pop_front();
    int y=5900;for(const auto& line:qolMessages){screen->Print(4200,y,zSTRING(line.second.c_str()));y+=210;}
}
unsigned QolSprint(oCNpc* npc){
    if(!npc||npc->activeOverlays.GetNumInList()>64)return 0;
    for(unsigned id=1;id<=5;++id)for(int i=0;i<npc->activeOverlays.GetNumInList();++i)
        if(_stricmp(npc->activeOverlays[i].ToChar(),qol::SprintOverlay(id))==0)return id;
    return 0;
}
void QolApplySprint(oCNpc* npc,unsigned id){
    if(!npc||npc==player||id>5||QolSprint(npc)==id)return;
    for(int i=npc->activeOverlays.GetNumInList()-1;i>=0;--i){
        zSTRING name=npc->activeOverlays[i];
        for(unsigned old=1;old<=5;++old)if(_stricmp(name.ToChar(),qol::SprintOverlay(old))==0){npc->RemoveOverlay(name);break;}
    }
    if(id){zSTRING name(qol::SprintOverlay(id));npc->ApplyOverlay(name);}
    CombatLog("sprint_overlay","npc="+std::string(npc->GetObjectName().ToChar())+" variant="+std::to_string(id));
}
bool QolPrepareCorpse(oCNpc* npc){
    if(!npc||!parser)return false;
    auto* fn=parser->GetSymbol(zSTRING("ZK_COOP_PREPARE_QUICKLOOT"));
    if(!fn||fn->type!=5||fn->ele!=0){notice="Quickloot-Helfer fehlt; beide Installationen aktualisieren.";return false;}
    QuestBindings bindings;parser->SetInstance(zSTRING("OTHER"),npc);
    parser->CallFunc(parser->GetIndex(zSTRING("ZK_COOP_PREPARE_QUICKLOOT")));
    npc->inventory2.UnpackAllItems();return true;
}
void QolCommitLocal(const rc::Tx& prepared){
    auto tx=prepared;
    if(!recoveryStore.Put(tx,rc::Role::Local))throw std::runtime_error("Cannot prepare host quickloot");
    tx.decision=rc::Decision::Commit;
    if(!recoveryStore.Put(tx,rc::Role::Local))throw std::runtime_error("Cannot commit host quickloot");
    if(!RecoveryReconcile(*recoveryStore.Find(tx.id)))throw std::runtime_error("Host quickloot replay deferred");
    LootLog("quickloot_grant","tx="+tx.id+" instance="+std::to_string(tx.item)+" amount="+std::to_string(tx.amount));
    QolNotify(tx.item,tx.amount);
}
bool QolItem(oCItem* item,bool corpse=false){return item&&qol::QuickItem(LootEligible(item),item->mainflag,item->flags,corpse);}
void QolFrame(unsigned long long now){
    const bool session=network.GetState()!=net::State::Idle&&network.GetState()!=net::State::Error;
    QolGuard(session);
    if(!CombatConnected()||!player||!zinput)return;
    const qol::Context context{true,recoveryReady&&RecoveryCanLoot()&&(HostCombat()||rulesReady)&&haveTarget&&now-lastSnapshotAt<=2000,
        zCMenu::GetActive()!=nullptr,player->talkOther!=nullptr,player->inventory2.IsOpen()!=FALSE,
        player->interactMob!=nullptr,player->attribute[0]>0&&player->IsUnconscious()==FALSE,
        static_cast<unsigned>(player->GetWeaponMode()),static_cast<unsigned>(player->GetBodyState())};
    if(clientQuickLoot){
        if(!qol::CanLoot(context)||!clientLootSource.vob||clientLootSource.vob->homeWorld!=ogame->GetWorld()||Distance2(clientLootSource.vob->GetPositionWorld(),player->GetPositionWorld())>250000){clientLootClosing=true;return;}
        if(clientLootClosing||clientOpenRequest&&!clientLootShown||clientPendingTake||!qol::Due(now,qolLastLoot))return;
        for(const auto& ref:clientLootItems)if(ref.entry.amount&&QolItem(ref.item,clientLootSource.corpse!=nullptr)){
            if(RecoveryClientTransfer(ref.item,ref.entry.amount,ref.entry.token,false)){
                qolLastLoot=now;LootLog("quickloot_request","token="+std::to_string(ref.entry.token)+" amount="+std::to_string(ref.entry.amount));
            }return;
        }
        clientLootClosing=true;player->SetFocusVob(nullptr);notice="Quickloot abgeschlossen.";return;
    }
    if(!qol::CanLoot(context)||!qol::Due(now,qolLastLoot)||clientLootSource.vob||!worldGroundPending.empty())return;
    const int key=QuestInt("_NINJA_QUICKLOOT_LOOTKEY",MOUSE_BUTTONRIGHT);
    if(key<0||key>4096||!zinput->KeyPressed(key))return;
    if(!player->GetFocusVob())player->CollectFocusVob(TRUE);
    auto* focus=player->GetFocusVob();if(!focus||Distance2(focus->GetPositionWorld(),player->GetPositionWorld())>250000)return;
    qolLastLoot=now;
    if(auto* item=zDYNAMIC_CAST<oCItem>(focus)){
        if(!QolItem(item)||item->homeWorld!=ogame->GetWorld())return;
        qol::Hold<oCItem> held(item);
        try{
            if(HostCombat()){
                LootSource source;source.vob=item;source.identity.kind=lo::Source::Ground;source.identity.name=item->GetObjectName().ToChar();const auto p=item->GetPositionWorld();source.identity.position={p[0],p[1],p[2]};
                auto tx=RecoveryNew(rc::Action::GroundTake,source,item->GetInstance(),item->amount);
                if(WorldGroundFor(tx)!=item){notice="Bodenstapel mehrdeutig; bitte regulaer aufnehmen.";}
                else {player->SetFocusVob(nullptr);QolCommitLocal(tx);}
            }else {WorldGroundAction(player,item,false);player->SetFocusVob(nullptr);}
        }catch(const std::exception& e){RecoveryFail(e.what());}
        return;
    }
    LootSource source;
    if(auto* npc=zDYNAMIC_CAST<oCNpc>(focus)){
        if(npc==player||npc==PartnerNpc()||(!npc->IsDead()&&!npc->IsUnconscious()))return;
        source.vob=source.corpse=npc;source.identity=LootIdentity(npc);
    }else if(auto* chest=zDYNAMIC_CAST<oCMobContainer>(focus)){
        if(chest->locked){notice="Kiste verschlossen: erst regulaer aufschliessen.";return;}
        if(chest->IsOccupied()){notice="Kiste wird gerade benutzt.";return;}
        // Scripted chests keep their native conditions/onState/required-item path.
        if(chest->conditionFunc.Length()||chest->onStateFuncName.Length()||chest->useWithItem.Length()){
            notice="Skriptgesteuerte Kiste bitte regulaer oeffnen.";return;
        }
        source.vob=source.chest=chest;source.identity=LootIdentity(chest);
    }else return;
    if(LootUIOpen(source)||source.vob==guestLootSource.vob){notice="Beutequelle wird gerade benutzt.";return;}
    if(!HostCombat()){
        clientQuickLoot=true;
        if(!RequestClientLoot(source)||!clientLootSource.vob)clientQuickLoot=false;
        return;
    }
    try{
        if(source.corpse&&!QolPrepareCorpse(source.corpse))return;
        const auto items=LootNativeItems(source);
        for(auto* item:items)if(QolItem(item,source.corpse!=nullptr)){
            qol::Hold<oCItem> held(item);auto tx=RecoveryNew(rc::Action::Take,source,item->GetInstance(),item->amount);
            if(RecoverySource(tx).vob!=source.vob){notice="Beutequelle fuer Recovery mehrdeutig; bitte regulaer oeffnen.";return;}
            QolCommitLocal(tx);
            return; // bounded: one stack per frame, next 120ms tick
        }
        player->SetFocusVob(nullptr);notice="Keine weitere freigegebene Beute.";
    }catch(const std::exception& e){RecoveryFail(e.what());}
}
