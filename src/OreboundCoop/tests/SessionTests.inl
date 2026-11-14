void TestSaveTransport(){
    Network host,guest;ConnectedPair(host,guest);
    Check(guest.SetSaving(true,1400),"guest announces native save without disconnect");
    Check(!guest.SavingReady(),"native save must wait for pause acknowledgement");
    host.Poll(1401);Check(host.PeerSaving(),"save pause processed in transport before engine events");
    guest.Poll(1401);Check(guest.SavingReady(),"native save allowed only after pause control and previous reliable events are acknowledged");
    Check(host.SendEvent(Bytes{60,1},1402),"outstanding event retained during native save");
    host.Poll(21401);Check(host.GetState()==State::Connected,"20 second save survives normal heartbeat and event timeout");
    Check(guest.SetSaving(false,21402),"save resume sent after synchronous native callback");
    Pump(host,guest,21403);Check(!host.PeerSaving()&&host.GetState()==State::Connected&&guest.GetState()==State::Connected,"same session resumes on both peers without reconnect");
    Bytes b;Check(!host.ConsumeEvent(b),"save control never leaks into engine combat decoder");
    Check(guest.ConsumeEvent(b)&&b==Bytes({60,1}),"queued event survives save pause exactly once");
    Check(!guest.ConsumeEvent(b),"save pause does not duplicate application event");
    Check(host.SetSaving(true,22000),"host can save as well");guest.Poll(22001);Check(guest.PeerSaving(),"host save observed by guest");
    host.Stop();guest.Poll(22002);Check(!guest.PeerSaving(),"disconnect clears save pause");
    Check(!guest.SavingReady(),"disconnected session cannot authorize native save");
    Network a,c;ConnectedPair(a,c);Check(c.SetSaving(true,1400),"bounded pause starts");a.Poll(1401);a.Poll(121402);Check(a.GetState()!=State::Connected&&!a.PeerSaving(),"abandoned save pause expires after 120 seconds");
    Network x,y;ConnectedPair(x,y);Check(x.SetSaving(true,1400)&&y.SetSaving(true,1400),"simultaneous pause race starts on both peers");Pump(x,y,1401);
    Check(!x.SavingReady()&&!y.SavingReady(),"simultaneous save announcements cannot both authorize file writes");
    x.SetSaving(false,1402);y.SetSaving(false,1402);Pump(x,y,1403);
    Check(!x.PeerSaving()&&!y.PeerSaving(),"cancelled simultaneous attempts leave neither peer stuck saving");
}
void TestSaveAnnouncementLoss(){
    Network host;Check(host.StartHost(0,Compatible(),1000),"save-loss host start");RawPeer raw(host.BoundPort());const std::uint64_t nonce=0x56789123456;
    raw.Send(Hello(nonce));host.Poll(1000);Bytes welcome;
    for(int i=0;i<10;++i){auto p=raw.Receive();if(p.empty())break;if(p[6]==static_cast<unsigned>(Message::Welcome))welcome=p;}
    Check(!welcome.empty(),"save-loss handshake captured");const auto token=Read64(welcome.data()+16);
    Check(host.SetSaving(true,1100),"pause queued before native writer");auto dropped=raw.Receive();
    Check(dropped.size()==ProtocolHeaderBytes+2&&dropped[6]==10&&dropped[ProtocolHeaderBytes]==63,"initial pause packet deliberately discarded");
    Check(!host.SavingReady(),"lost announcement cannot authorize file writes");host.Poll(1250);auto retry=raw.Receive();Check(retry==dropped,"pause retry survives initial datagram loss");
    raw.Send(Packet(Message::EventAck,{},token,nonce,1));host.Poll(1251);Check(host.SavingReady(),"acknowledged retransmission releases save barrier");
    host.SetSaving(false,1300);raw.Receive();raw.Send(Packet(Message::EventAck,{},token,nonce,2));host.Poll(1301);
    Check(host.SendEvent({7},1400)&&host.SetSaving(true,1400),"older reliable event precedes next save announcement");while(!raw.Receive().empty()){}
    raw.Send(Packet(Message::EventAck,{},token,nonce,4));host.Poll(1401);Check(!host.SavingReady(),"pause ACK alone cannot bypass an unacknowledged predecessor");
    raw.Send(Packet(Message::EventAck,{},token,nonce,3));host.Poll(1402);Check(host.SavingReady(),"ordered save barrier opens after all earlier ACKs arrive");
}
void TestSessionReadiness(){
    namespace ss=orebound::session;
    {
        ss::Ready local,peer;peer.flags=ss::Checkpoint;
        Check(ss::SaveReadiness(local,nullptr,false,false,false)==ss::SaveBlock::Allowed,"first offline save requires no preexisting checkpoint or peer");
        Check(ss::SaveReadiness(local,&peer,true,false,false)==ss::SaveBlock::Allowed,"idle connected peers may save sequentially");
        Check(ss::SaveReadiness(local,nullptr,true,false,false)==ss::SaveBlock::PeerUnknown,"connected save waits until peer checkpoint readiness is known");
        Check(ss::SaveReadiness(local,&peer,true,true,false)==ss::SaveBlock::PeerSaving,"simultaneous save rejected while peer pause is known");
        Check(ss::SaveReadiness(local,&peer,true,false,true)==ss::SaveBlock::LocalTransfer,"native or mirrored open inventory blocks save");
        peer.flags|=ss::Pending;Check(ss::SaveReadiness(local,&peer,true,false,false)==ss::SaveBlock::PeerTransfer,"remote transfer/container state blocks local save");
        peer.flags=ss::Checkpoint|ss::Blocked;Check(ss::SaveReadiness(local,&peer,true,false,false)==ss::SaveBlock::PeerTransfer,"remote recovery fault blocks connected save");
        local.flags=ss::Pending;Check(ss::SaveReadiness(local,&peer,false,false,false)==ss::SaveBlock::LocalTransfer,"local pending work remains blocked when offline");
        local.flags=ss::Blocked;Check(ss::SaveReadiness(local,&peer,true,false,false)==ss::SaveBlock::Recovery,"local recovery error takes precedence over other save blockers");
        Check(std::string(ss::SaveText(ss::SaveBlock::PeerTransfer)).find("partner")!=std::string::npos,"save blocker tells player which side must finish");
    }
    Check(ss::Hotkey(0x32,true,true)==ss::Command::Marker&&ss::Hotkey(0x31,true,true)==ss::Command::Next&&ss::Hotkey(0x30,true,true)==ss::Command::Previous&&ss::Hotkey(0x20,true,true)==ss::Command::Overlay,"four explicit diagnostic chords mapped");
    Check(ss::Hotkey(0x32,false,false)==ss::Command::None&&ss::Hotkey(0x32,true,false)==ss::Command::None&&ss::Hotkey(0x32,false,true)==ss::Command::None,"ordinary map/key input and single modifier never create test markers");
    Check(ss::Hotkey(0x32,true,true,true)==ss::Command::None&&ss::Hotkey(0x21,true,true)==ss::Command::None,"text/menu context and unrelated chords do not trigger diagnostics");
    ss::Ready local,peer;local.profile=std::string(32,'a');local.timeline=std::string(32,'b');local.flags=31&~ss::Blocked;local.revision=1;peer=local;
    Check(ss::Readiness(local,&peer,true)==ss::Reason::Ready,"both ready only after checkpoints, rules, actor, fresh snapshot and quest adapter");
    Check(ss::Readiness(local,nullptr,true)==ss::Reason::Peer,"socket connection alone cannot claim both players ready");
    auto changed=local;changed.flags&=~ss::Checkpoint;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Checkpoint,"missing local save binding reported");
    changed=local;changed.flags|=ss::Blocked;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Recovery,"local recovery problem takes priority over ready");
    changed.flags&=~ss::Checkpoint;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Recovery,"actual recovery failure takes priority over generic save reminder");
    changed=peer;changed.flags|=ss::Blocked;Check(ss::Readiness(local,&changed,true)==ss::Reason::Deferred,"remote recovery blocker is visible locally");
    changed.flags&=~ss::Checkpoint;Check(ss::Readiness(local,&changed,true)==ss::Reason::Deferred,"remote failure remains visible when peer checkpoint is absent");
    changed=local;changed.flags|=ss::Pending;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Transfer,"local pending transaction not mislabeled fully ready");
    changed=peer;changed.flags|=ss::Pending;Check(ss::Readiness(local,&changed,true)==ss::Reason::Transfer,"remote in-flight inventory transaction visible locally");
    changed=local;changed.flags|=ss::Incapacitated;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Player,"local death/unconsciousness not mislabeled ready");
    changed=peer;changed.flags|=ss::Incapacitated;Check(ss::Readiness(local,&changed,true)==ss::Reason::PeerPlayer,"remote incapacitation clearly reported");
    changed=local;changed.flags&=~ss::Rules;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Rules,"host rule handshake must finish");
    changed=local;changed.flags&=~ss::Actor;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Actor,"missing partner visual cannot claim ready");
    Check(ss::Readiness(local,&peer,false)==ss::Reason::Snapshot,"stale position blocks complete readiness");
    changed=local;changed.flags&=~ss::Quest;Check(ss::Readiness(changed,&peer,true)==ss::Reason::Quest,"unsupported or blocked quest adapter distinguished from full readiness");
    std::set<std::string> steps;for(unsigned i=1;i<=65;++i)Check(steps.insert(ss::Step(i)).second&&ss::Step(i).size()==3,"all 65 checklist marker IDs unique");
    Check(ss::Step(1)=="A01"&&ss::Step(7)=="A07"&&ss::Step(8)=="B01"&&ss::Step(65)=="I04"&&ss::NextStep(65)==1,"checklist marker boundaries match route and wrap safely");
    Check(ss::ProgressCompatible(1,0)&&ss::ProgressCompatible(2,1)&&ss::ProgressCompatible(2,0),"guest can catch up to host acceptance and completion");
    Check(!ss::ProgressCompatible(0,2)&&!ss::ProgressCompatible(1,2)&&!ss::ProgressCompatible(2,3),"ahead or contradictory guest quest state rejected");
}
void TestSessionCodecsAndTransport(){
    namespace ss=orebound::session;
    ss::Ready ready;ready.profile=std::string(32,'a');ready.timeline=std::string(32,'b');ready.revision=5;ready.flags=31;ss::Ready r;
    const auto a=ss::Encode(ready);Check(ss::Decode(a,r)&&r.flags==31&&r.timeline==ready.timeline,"readiness profile and save lineage roundtrip");
    for(std::size_t n=0;n<a.size();++n)Check(!ss::Decode(Bytes(a.begin(),a.begin()+n),r),"readiness truncated prefix rejected");
    ready.timeline.clear();Check(!ss::Decode(ss::Encode(ready),r),"ready checkpoint requires explicit valid lineage");
    ss::Marker mark;mark.id=std::string(32,'c');mark.profile=std::string(32,'a');mark.step=65;mark.world=HashString("NEWWORLD.ZEN");ss::Marker m;
    const auto b=ss::Encode(mark);Check(ss::Decode(b,m)&&m.id==mark.id&&m.world==mark.world&&m.step==65,"shared marker UUID, world and step preserved");
    for(std::size_t n=0;n<b.size();++n)Check(!ss::Decode(Bytes(b.begin(),b.begin()+n),m),"marker truncated prefix rejected");
    mark.step=66;Check(!ss::Decode(ss::Encode(mark),m),"marker cannot invent unknown checklist step");
    ss::Cavalorn q;q.hostTimeline=std::string(32,'a');q.ownerTimeline=std::string(32,'b');q.revision=4;q.mission=2;q.letter=1;q.told=2047;q.dead=7;q.away=1;ss::Cavalorn out;
    const auto c=ss::Encode(q);Check(ss::Decode(c,out)&&out.told==2047&&out.dead==7&&out.mission==2,"bounded Cavalorn state roundtrip");
    for(std::size_t n=0;n<c.size();++n)Check(!ss::Decode(Bytes(c.begin(),c.begin()+n),out),"Cavalorn state truncated prefix rejected");
    q.told=2048;Check(!ss::Decode(ss::Encode(q),out),"Cavalorn adapter cannot overwrite arbitrary info bits");q.told=1;q.mission=5;Check(!ss::Decode(ss::Encode(q),out),"invalid quest state rejected");
    Network host,guest;ConnectedPair(host,guest);
    for(const auto& payload:{a,b,c}){Check(host.SendEvent(payload,1400),"session diagnostic and quest event accepted by real UDP");Pump(host,guest,1401);Bytes incoming;Check(guest.ConsumeEvent(incoming)&&incoming==payload,"diagnostic and quest event delivered intact exactly once");Check(!guest.ConsumeEvent(incoming),"session event not delivered twice");}
}
void TestCavalornRewards(){
    namespace rc=orebound::recovery;namespace cv=orebound::cavalorn;
    const cv::Rewards rewards{100,6929,6355,7065};std::vector<rc::Tx> transactions;
    Check(cv::EligibleForCredit(1,false)&&cv::EligibleForCredit(100,false),"living player may receive quest credits");
    Check(!cv::EligibleForCredit(0,false)&&!cv::EligibleForCredit(-1,false)&&!cv::EligibleForCredit(1,true),"quest XP cannot resurrect a dead player or alter an unconscious one");
    for(unsigned event=1;event<=4;++event){
        auto tx=recovery_test::Transaction(20+event);tx.source.kind=orebound::loot::Source::Quest;tx.source.name="CAVALORN_KILLBRAGO";tx.instance=event;
        tx.action=event==1?rc::Action::QuestXp:rc::Action::QuestItem;tx.decision=rc::Decision::Commit;
        tx.item=event==1?0:event==2?6929:event==3?6355:7065;tx.amount=event==1?100:event==2?50:event==3?1:2;
        Check(cv::Allowed(tx,rewards),"native verified Cavalorn XP/gold/knife/potions admitted");
        rc::Message msg;msg.op=rc::Op::Result;msg.tx=tx;rc::Message decoded;
        Check(rc::Decode(rc::Encode(msg),decoded)&&rc::Same(tx,decoded.tx),"committed quest reward wire roundtrip");
        auto forged=tx;++forged.amount;Check(!cv::Allowed(forged,rewards),"quest adapter rejects excessive or different award");
        forged=tx;forged.source.name="ANY_QUEST";Check(!cv::Allowed(forged,rewards),"quest adapter is scoped to one verified quest");
        forged=tx;forged.world="OLDWORLD.ZEN";Check(!cv::Allowed(forged,rewards),"quest reward cannot cross unsupported world");
        forged=tx;forged.id=std::string(32,'e');Check(cv::SameEvent(tx,forged),"different UUID cannot hide a repeated semantic quest award");
        transactions.push_back(tx);
    }
    for(int mask=0;mask<16;++mask){
        std::set<std::string> effects;int xp=(mask&1)?100:0,gold=(mask&2)?50:0,knife=(mask&4)?1:0,potions=(mask&8)?2:0,calls=0;
        for(unsigned i=0;i<4;++i)if(mask&(1u<<i))effects.insert(transactions[i].id);
        auto apply=[&](const rc::Record& r,bool desired){Check(desired,"committed quest credit never debits previous rewards");++calls;switch(r.tx.instance){case 1:xp+=r.tx.amount;break;case 2:gold+=r.tx.amount;break;case 3:knife+=r.tx.amount;break;case 4:potions+=r.tx.amount;}return true;};
        for(const auto& t:transactions)Check(rc::Reconcile(rc::Record{t,rc::Role::Owner,1},effects,apply),"quest credits recover from independently checkpointed effect subsets");
        Check(xp==100&&gold==50&&knife==1&&potions==2,"guest receives full exact quest rewards without split or duplication");
        const int before=calls;for(int retry=0;retry<4;++retry)for(const auto& t:transactions)rc::Reconcile(rc::Record{t,rc::Role::Owner,1},effects,apply);
        Check(calls==before,"reconnect/receipt retries cannot repeat quest credit");
        rc::Checkpoint cp;cp.profile=std::string(32,'c');cp.timeline=std::string(32,'d');cp.effects=effects;
        Check(rc::DecodeCheckpoint(rc::Encode(cp)).effects==effects,"save checkpoint retains all quest reward receipts");
    }
    Check(cv::NativeSaveRepresents(1,2,false)&&cv::NativeSaveRepresents(2,2,false)&&cv::NativeSaveRepresents(3,1,true),"matching completed quest/supply saves admit reconciliation");
    Check(!cv::NativeSaveRepresents(1,1,false)&&!cv::NativeSaveRepresents(4,1,false),"older host saves fail closed rather than repeating native quest rewards");
    std::set<std::string> effects;int grants=0;const rc::Record owner{transactions[0],rc::Role::Owner,1};
    Check(!rc::Reconcile(owner,effects,[](const rc::Record&,bool){return false;})&&effects.empty(),"failed XP application does not create an applied receipt");
    Check(rc::Reconcile(owner,effects,[&](const rc::Record&,bool){++grants;return true;})&&grants==1,"pending XP application retries once successfully");
    const auto dir=recovery_test::Temp(),path=dir+"\\transactions.bin";rc::Store store;store.Open(path);
    for(const auto& t:transactions)Check(store.Put(t,rc::Role::Host),"quest rewards committed durably before sending guest grants");
    rc::Store restart;restart.Open(path);Check(restart.Records().size()==4&&restart.Find(transactions[0].id)->tx.amount==100,"quest decisions and full XP survive process restart");
    DeleteFileA(path.c_str());RemoveDirectoryA(dir.c_str());
    char executable[MAX_PATH];GetModuleFileNameA(nullptr,executable,MAX_PATH);
    const auto root=std::filesystem::path(executable).parent_path().parent_path().parent_path().parent_path();
    const auto fixture=root/"src/OreboundCoop/tests/fixtures";auto legacy=rc::ReadFile((fixture/"Recovery18Store.bin").string());
    Check(legacy[1]==2,"golden Alpha18 trade journal produced by actual old serializer");
    const auto cp=rc::DecodeCheckpoint(rc::ReadFile((fixture/"Recovery18Checkpoint.bin").string()));
    Check(cp.effects.size()==2&&cp.fingerprint=="alpha18-real-serializer","Alpha18 reservation and credit checkpoint loads unchanged");
    const auto migrationDir=recovery_test::Temp(),migrationPath=migrationDir+"\\transactions.bin";rc::AtomicWrite(migrationPath,legacy);rc::Store migrated;migrated.Open(migrationPath);
    Check(migrated.Profile()==cp.profile&&migrated.Find(std::string(32,'2'))->tx.price==12,"actual Alpha18 trade price/profile survive newer reader");
    Check(migrated.Put(transactions[0],rc::Role::Owner),"new quest award appended alongside old trade");rc::Store reopened;reopened.Open(migrationPath);
    Check(reopened.Records().size()==2&&rc::ReadFile(migrationPath)[1]==4&&reopened.Find(transactions[0].id)->tx.amount==100,"migrated trade and quest records reopen together in format four");
    DeleteFileA(migrationPath.c_str());RemoveDirectoryA(migrationDir.c_str());
    struct Info {struct {int conditions;}pd;};Info first{{123}},second{{456}};std::map<Info*,int> originals;
    cv::WaitCondition(originals,&first,999);cv::WaitCondition(originals,&first,999);cv::WaitCondition(originals,&second,999);
    Check(first.pd.conditions==999&&second.pd.conditions==999&&originals.size()==2,"temporary guest dialog filter applies without losing original conditions on repeat");
    cv::RestoreConditions(originals);Check(first.pd.conditions==123&&second.pd.conditions==456&&originals.empty(),"save/disconnect restoration recovers original native condition indices exactly");
    cv::RestoreConditions(originals);Check(first.pd.conditions==123,"repeated condition restoration is harmless");
}
