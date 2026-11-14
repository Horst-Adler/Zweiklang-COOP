void TestQolCompatibility(){
    namespace q=orebound::qol;namespace co=orebound::combat;namespace rc=orebound::recovery;namespace lo=orebound::loot;
    q::SessionLatch latch;Check(!latch.Update(false),"fresh single-player load keeps upstream quickloot available");
    Check(latch.Update(true),"session start blocks unguarded upstream inventory mutations");
    Check(latch.Update(false),"disconnect with held loot key cannot fall back to copying a local mirror");
    Check(latch.Update(true),"reconnect retains the safe quickloot path");latch.Reset();
    Check(!latch.Update(false),"explicit load/new game releases the session-bound guard");
    q::SessionLatch faultLatch;
    Check(!q::BlockUpstream(faultLatch,false,false),"healthy fresh load permits original quickloot");
    Check(q::BlockUpstream(faultLatch,false,true),"loaded recovery fault blocks original quickloot before any connection");
    Check(!q::BlockUpstream(faultLatch,false,false),"cleared fault alone does not latch a never-started session");
    Check(q::BlockUpstream(faultLatch,true,false),"session start latches upstream guard");
    Check(q::BlockUpstream(faultLatch,false,false),"disconnect cannot bypass upstream guard");
    faultLatch.Reset();Check(q::BlockUpstream(faultLatch,false,true),"reset cannot override a persistent recovery fault");
    struct Ref {int refs=1,releases=0;void AddRef(){++refs;}void Release(){--refs;++releases;}};
    Ref normal;{q::Hold<Ref> held(&normal);Check(normal.refs==2,"scoped inventory item stays alive during mutation");}
    Check(normal.refs==1&&normal.releases==1,"normal transfer releases only its temporary reference");
    Ref failed;try{q::Hold<Ref> held(&failed);throw std::runtime_error("transaction creation failed");}catch(const std::exception&){}
    Check(failed.refs==1&&failed.releases==1,"transaction creation exception releases temporary inventory reference");
    Ref early;auto abort=[&](){q::Hold<Ref> held(&early);return;};abort();
    Check(early.refs==1&&early.releases==1,"ambiguous source early return releases temporary inventory reference");
    q::Context c;c.connected=c.ready=c.alive=true;
    Check(q::CanLoot(c),"ready unarmed standing player may quickloot");
    c.menu=true;Check(!q::CanLoot(c),"menus suppress quickloot without consuming normal input");c.menu=false;
    c.dialog=true;Check(!q::CanLoot(c),"dialogue suppresses quickloot");c.dialog=false;
    c.inventory=true;Check(!q::CanLoot(c),"native inventory suppresses second transfer path");c.inventory=false;
    c.interaction=true;Check(!q::CanLoot(c),"mob interaction cannot be bypassed by fast loot");c.interaction=false;
    c.alive=false;Check(!q::CanLoot(c),"dead and unconscious players receive no fast loot");c.alive=true;
    c.weapon=1;Check(!q::CanLoot(c),"combat RMB is not turned into a loot action");c.weapon=0;
    c.ready=false;Check(!q::CanLoot(c),"untrusted checkpoint blocks quickloot");c.ready=true;
    for(unsigned body=0;body<32;++body){c.body=body;Check(q::CanLoot(c)==(body<=5||body==7),"only normal movement and swimming/dive body states may loot");}
    Check(!q::Due(1199,1100)&&q::Due(1220,1100),"fast-loot timer bounds per-frame work");
    Check(q::QuickItem(true,16,0,false)&&!q::QuickItem(true,16,0,true),"loose/chest armor may be looted but corpse armor stays");
    Check(!q::QuickItem(true,1,1u<<12)&&!q::QuickItem(true,1,1u<<30)&&!q::QuickItem(false,1,0),"mission/equipped and native-ineligible items stay on safe path");
    for(unsigned sprint=0;sprint<=5;++sprint){
        co::Peer p;p.stats.attributes[1]=1;p.sprint=sprint;co::Peer out;
        Check(co::Decode(co::Encode(p),out)&&out.sprint==sprint,"all player sprint variants survive codec");
        co::Npc npc;npc.id=npc.revision=1;npc.stats.attributes[1]=1;npc.sprint=sprint;co::Npc decoded;
        Check(co::Decode(co::Encode(npc),decoded)&&decoded.sprint==sprint,"companion sprint variants survive NPC codec");
        Check((sprint==0)==std::string(q::SprintOverlay(sprint)).empty(),"zero overlay removes sprint; known variants have fixed names");
    }
    co::Peer invalid;invalid.stats.attributes[1]=1;invalid.sprint=6;co::Peer out;
    Check(!co::Decode(co::Encode(invalid),out),"unknown sprint identifier cannot request arbitrary model files");
    co::Npc npc;npc.id=npc.revision=1;npc.stats.attributes[1]=1;npc.sprint=6;co::Npc decoded;
    Check(!co::Decode(co::Encode(npc),decoded),"unknown NPC sprint identifier rejected");
    lo::Message open;open.op=lo::Op::QuickOpen;open.request=1;open.source.kind=lo::Source::Chest;open.source.name="XARDAS";lo::Message got;
    auto wire=lo::Encode(open);Check(lo::Decode(wire,got)&&got.op==lo::Op::QuickOpen,"host can distinguish headless from native UI open");
    {Network host,guest;ConnectedPair(host,guest);Check(guest.SendEvent(wire,3000),"headless open enters actual reliable UDP channel");Pump(host,guest,3001);Bytes received;Check(host.ConsumeEvent(received)&&lo::Decode(received,got)&&got.op==lo::Op::QuickOpen,"headless source request arrives intact over loopback UDP");}
    for(std::size_t i=0;i<wire.size();++i)Check(!lo::Decode(Bytes(wire.begin(),wire.begin()+i),got),"truncated headless source request rejected");
    open.source.kind=lo::Source::Merchant;open.source.npc=1;Check(!lo::Decode(lo::Encode(open),got),"quickloot cannot acquire merchant stock for free");
    for(auto action:{rc::Action::Take,rc::Action::GroundTake})for(auto decision:{rc::Decision::Prepared,rc::Decision::Commit,rc::Decision::Abort})for(int mask=0;mask<4;++mask){
        auto tx=recovery_test::Transaction(12,action);if(action==rc::Action::GroundTake)tx.source.kind=lo::Source::Ground;tx.decision=decision;
        rc::Record record{tx,rc::Role::Local,1};std::set<std::string> effects;int source=10-((mask&1)?3:0),inventory=(mask&2)?3:0,calls=0;
        if(mask&1)effects.insert(tx.id);if(mask&2)effects.insert(tx.id+"c");
        auto apply=[&](const rc::Record& e,bool desired){if(e.role==rc::Role::Host)source+=desired?-3:3;else inventory+=desired?3:-3;++calls;return true;};
        Check(rc::Reconcile(record,effects,apply)&&source==(decision==rc::Decision::Commit?7:10)&&inventory==(decision==rc::Decision::Commit?3:0),"local quickloot replays both source removal and owner credit from checkpoint subsets");
        const auto previous=calls;Check(rc::Reconcile(record,effects,apply)&&calls==previous,"repeated local quickloot replay cannot duplicate credit or debit");
        rc::Checkpoint checkpoint;checkpoint.profile=std::string(32,'a');checkpoint.timeline=std::string(32,'b');checkpoint.effects=effects;
        Check(rc::DecodeCheckpoint(rc::Encode(checkpoint)).effects==effects,"local quickloot checkpoint retains both effect legs");
    }
    auto tx=recovery_test::Transaction(13,rc::Action::Take);tx.decision=rc::Decision::Commit;rc::Record record{tx,rc::Role::Local,1};std::set<std::string> effects;int source=10,inventory=0;
    auto interrupted=[&](const rc::Record& e,bool){if(e.role==rc::Role::Host){source-=3;return true;}return false;};
    Check(!rc::Reconcile(record,effects,interrupted)&&source==7&&effects.count(tx.id)&&!effects.count(tx.id+"c"),"failed credit preserves exactly the already removed source leg");
    auto resumed=[&](const rc::Record& e,bool){Check(e.role==rc::Role::Owner,"resumed quickloot never removes source a second time");inventory+=3;return true;};
    Check(rc::Reconcile(record,effects,resumed)&&source==7&&inventory==3,"deferred host quickloot credit completes without duplication");
}
