session::Ready localReadiness;
unsigned readinessRevision=0;
unsigned long long lastReadiness=0,lastReadinessLog=0;
std::string lastReadinessKey,lastReadinessText,lastTestMarker;
std::set<std::string> testMarkerIds;
bool markerPressed=false,overlayPressed=false,previousMarkerPressed=false,nextMarkerPressed=false;
std::string MarkerContext(){if(!player)return {};const auto p=player->GetPositionWorld();return " local_hp="+std::to_string(player->attribute[0])+" local_xp="+std::to_string(player->experience_points)+" local_level="+std::to_string(player->level)+" local_pos="+std::to_string(p[0])+","+std::to_string(p[1])+","+std::to_string(p[2])+" quickloot_batch="+std::to_string(clientQuickLoot)+" sprint_local="+std::to_string(QolSprint(player))+" sprint_peer="+std::to_string(peerCombat.sprint)+" pending="+std::to_string(network.PendingEvents())+" quest="+std::to_string(QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO"));}

void SessionReset(){
    QuestSessionReset();localReadiness=session::Ready{};readinessRevision=0;lastReadiness=lastReadinessLog=0;
    lastReadinessKey.clear();lastReadinessText.clear();testMarkerIds.clear();lastTestMarker.clear();
}
session::Ready SessionLocalReady(){
    session::Ready r;r.profile=recoveryStore.Profile();r.timeline=recoveryTimeline;
    if(recoveryReady&&!r.timeline.empty())r.flags|=session::Checkpoint;
    if(recoveryBlocked||!recoveryError.empty())r.flags|=session::Blocked;
    if(clientQuickLoot||clientPendingTake||!worldGroundPending.empty()||lootLedger.HasUnapplied()||QuestPendingAwards())r.flags|=session::Pending;
    if(player&&!cv::EligibleForCredit(player->attribute[0],player->IsUnconscious()!=FALSE))r.flags|=session::Incapacitated;
    if(rulesReady)r.flags|=session::Rules;if(remote&&havePeerCombat)r.flags|=session::Actor;
    if(QuestSupported()&&questError.empty())r.flags|=session::Quest;return r;
}
void SessionReceive(const std::vector<std::uint8_t>& bytes){
    try{
    if(bytes.empty())return;
    if(bytes[0]==60){
        session::Ready r;if(!session::Decode(bytes,r)){LootLog("readiness_reject","Malformed peer readiness");return;}
        if(havePeerReadiness&&!combat::Newer(r.revision,peerReadiness.revision))return;
        if(havePeerReadiness&&(r.profile!=peerReadiness.profile||(!peerReadiness.timeline.empty()&&r.timeline!=peerReadiness.timeline))){combatFailure="Peer profile/save lineage changed during session";return;}
        peerReadiness=r;havePeerReadiness=true;return;
    }
    if(bytes[0]==61){session::Cavalorn s;if(session::Decode(bytes,s))QuestReceiveState(s);else LootLog("quest_state_reject","Malformed Cavalorn state");return;}
    if(bytes[0]==62){
        session::Marker marker;if(!session::Decode(bytes,marker)||marker.world!=net::HashString(CurrentWorld())||!havePeerReadiness||marker.profile!=peerReadiness.profile){LootLog("test_marker_reject","Marker/profile/world mismatch");return;}
        if(testMarkerIds.size()>=1024||!testMarkerIds.insert(marker.id).second)return;
        lastTestMarker=session::Step(marker.step);markerStep=session::NextStep(marker.step);
        WritePrivateProfileStringA("Diagnostics","NextTestStep",std::to_string(markerStep).c_str(),iniPath.c_str());
        LootLog("test_marker","id="+marker.id+" step="+lastTestMarker+" origin=peer profile="+marker.profile+" host_timeline="+(HostCombat()?recoveryTimeline:recoveryPeerTimeline)+MarkerContext());diagnostics.Flush();
    }
    }catch(const std::exception& e){QuestFail(e.what());}
}
void SessionMark(){
    if(!recoveryStoreReady||options.logLevel==0){notice="Testmarke benoetigt aktiviertes Logging.";return;}
    if(testMarkerIds.size()>=1024){notice="Testmarkenlimit erreicht; neuen Testdurchgang beginnen.";return;}
    if(CombatConnected()&&(!havePeerReadiness||lootOutbox.size()>=96)){notice="Testmarke wartet auf Partner/Sendekanal.";return;}
    session::Marker marker;marker.id=rc::NewGuid();marker.profile=recoveryStore.Profile();marker.step=markerStep;marker.world=net::HashString(CurrentWorld());
    testMarkerIds.insert(marker.id);lastTestMarker=session::Step(marker.step);markerStep=session::NextStep(marker.step);
    WritePrivateProfileStringA("Diagnostics","NextTestStep",std::to_string(markerStep).c_str(),iniPath.c_str());
    if(CombatConnected())lootOutbox.push_back(session::Encode(marker));
    LootLog("test_marker","id="+marker.id+" step="+lastTestMarker+" origin=local shared="+std::to_string(CombatConnected())+" profile="+marker.profile+" host_timeline="+(HostCombat()?recoveryTimeline:recoveryPeerTimeline)+MarkerContext());
    diagnostics.Flush();notice="Testmarke "+lastTestMarker+" geschrieben; naechste "+session::Step(markerStep)+".";
}
bool SessionInput(int key){
    if(!zinput||loading||saving||coopTextEditing||zCMenu::GetActive())return false;
    const bool control=zinput->KeyPressed(KEY_LCONTROL)||zinput->KeyPressed(KEY_RCONTROL);
    const bool shift=zinput->KeyPressed(KEY_LSHIFT)||zinput->KeyPressed(KEY_RSHIFT);
    static_assert(KEY_M==0x32&&KEY_N==0x31&&KEY_B==0x30&&KEY_D==0x20,"Diagnostic key map must match native scan codes");
    const auto command=session::Hotkey(key,control,shift);
    if(command==session::Command::Marker){if(!markerPressed){markerPressed=true;try{SessionMark();}catch(const std::exception& e){notice="Testmarke konnte nicht geschrieben werden.";LootLog("test_marker_error",e.what());}}return true;}
    if(command==session::Command::Overlay){if(!overlayPressed){overlayPressed=true;diagnosticOverlay=!diagnosticOverlay;WritePrivateProfileStringA("Diagnostics","Overlay",diagnosticOverlay?"1":"0",iniPath.c_str());}return true;}
    if(command==session::Command::Next){if(!nextMarkerPressed){nextMarkerPressed=true;markerStep=session::NextStep(markerStep);WritePrivateProfileStringA("Diagnostics","NextTestStep",std::to_string(markerStep).c_str(),iniPath.c_str());notice="Naechste Testmarke: "+session::Step(markerStep);}return true;}
    if(command==session::Command::Previous){if(!previousMarkerPressed){previousMarkerPressed=true;markerStep=markerStep<=1?65:markerStep-1;WritePrivateProfileStringA("Diagnostics","NextTestStep",std::to_string(markerStep).c_str(),iniPath.c_str());notice="Naechste Testmarke: "+session::Step(markerStep);}return true;}
    return false;
}
void SessionFrame(unsigned long long now){
    if(zinput){if(!zinput->KeyPressed(KEY_M))markerPressed=false;if(!zinput->KeyPressed(KEY_D))overlayPressed=false;if(!zinput->KeyPressed(KEY_B))previousMarkerPressed=false;if(!zinput->KeyPressed(KEY_N))nextMarkerPressed=false;}
    localReadiness=SessionLocalReady();
    if(!CombatConnected())return;
    const auto key=localReadiness.timeline+":"+std::to_string(localReadiness.flags);
    if((key!=lastReadinessKey||now-lastReadiness>=3000)&&lootOutbox.size()<96){localReadiness.revision=++readinessRevision;lootOutbox.push_back(session::Encode(localReadiness));lastReadinessKey=key;lastReadiness=now;}
    const auto reason=session::Readiness(localReadiness,havePeerReadiness?&peerReadiness:nullptr,haveTarget&&now-lastSnapshotAt<=2000);
    const auto text=std::string(session::Text(reason));
    if(text!=lastReadinessText||now-lastReadinessLog>=5000){
        LootLog("coop_readiness","status="+text+" local_flags="+std::to_string(localReadiness.flags)+" peer_flags="+std::to_string(havePeerReadiness?peerReadiness.flags:0)+" quickloot_batch="+std::to_string(clientQuickLoot)+" sprint_local="+std::to_string(QolSprint(player))+" sprint_peer="+std::to_string(peerCombat.sprint)+" pending="+std::to_string(network.PendingEvents())+" source_queue="+std::to_string(lootOutbox.size())+" recovery="+recoveryError+" quest="+questError);
        lastReadinessText=text;lastReadinessLog=now;
    }
}
std::vector<std::string> SessionStatusLines(){
    std::vector<std::string> lines;
    if(!CombatConnected()){lines.push_back("Nicht verbunden. Rolle/IP im Union-Menue; F6 Host / F7 Beitritt.");return lines;}
    const auto now=Now();const auto reason=session::Readiness(localReadiness,havePeerReadiness?&peerReadiness:nullptr,haveTarget&&now-lastSnapshotAt<=2000);
    lines.push_back(session::Text(reason));
    if(!diagnosticOverlay)return lines;
    lines.push_back("Protokoll 8 | Welt "+CurrentWorld()+" | Zustand "+(haveTarget?std::to_string(now-lastSnapshotAt)+" ms":"fehlt")+" | offen "+std::to_string(network.PendingEvents()+lootOutbox.size()+worldOutbox.size()+QuestPendingAwards()));
    lines.push_back("Abstand "+(haveTarget?std::to_string(static_cast<int>(std::sqrt(Distance2(player->GetPositionWorld(),zVEC3(target.x,target.y,target.z)))/100))+" m":"?")+" | Cavalorn "+std::to_string(QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO"))+" | naechste Marke "+session::Step(markerStep));
    if(!recoveryError.empty())lines.push_back("Sperre: "+recoveryError.substr(0,90));else if(recoveryBlocked)lines.push_back("Sperre: gespeicherte Inventarvorgaenge warten auf Abgleich.");
    if(!questError.empty())lines.push_back("Quest: "+questError.substr(0,90));
    lines.push_back("Strg+Umschalt: M Marke | N weiter | B zurueck | D Diagnose");return lines;
}
