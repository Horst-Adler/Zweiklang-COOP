namespace world_test {
namespace ws=zweiklang::worldsync;
namespace rc=zweiklang::recovery;
rc::Tx Trade(rc::Action action){auto t=recovery_test::Transaction(9,action);t.source.kind=zweiklang::loot::Source::Merchant;t.instance=123;t.currency=200;t.price=12;return t;}
struct Node {void* npc=nullptr;float timer=0;};
struct List {std::vector<Node*> nodes;int GetNumInList()const{return static_cast<int>(nodes.size());}Node*& operator[](int i){return nodes[i];}};
struct Manager {List spawnList;std::array<float,3> camPos{};float insertTime=0;};
}
void TestWorldCodecAndLifecycle(){
    namespace ws=zweiklang::worldsync;
    Check(ws::MatchAnchor({0,2500,4900})==0,"unique exact creature spawn wins even with close same-instance neighbours");
    Check(ws::MatchAnchor({0,0,40000})==-1,"coincident spawn anchors do not bind a guessed creature");
    Check(ws::MatchAnchor({2500,3600})==-1,"nearby inexact same-instance creatures remain ambiguous");
    Check(ws::MatchAnchor({400,90000})==0,"well separated nearest spawn remains accepted");
    ws::TalkLease lease;lease.Begin(238,1000);
    Check(lease.Allows(238,1000)&&!lease.Allows(242,1000),"guest dialogue transition releases only requested NPC");
    Check(lease.Allows(238,3999)&&!lease.Allows(238,4000),"failed dialogue attempt cannot release mirror AI indefinitely");
    lease.Clear();Check(!lease.Allows(238,1001),"disconnect clears guest dialogue lease");
    namespace co=zweiklang::combat;
    co::Npc visual;visual.head="HEAD1";visual.bodyTex=23;const auto oldKey=co::AppearanceKey(visual);
    visual.head="HEAD12";visual.bodyTex=3;Check(co::AppearanceKey(visual)!=oldKey,"head-name and texture boundary cannot collide");
    const auto colors=co::AppearanceKey(visual);++visual.bodyColor;Check(co::AppearanceKey(visual)!=colors,"body color triggers native visual refresh");
    const auto bodyColor=co::AppearanceKey(visual);++visual.headColor;Check(co::AppearanceKey(visual)!=bodyColor,"head color triggers native visual refresh");
    const auto unchanged=co::AppearanceKey(visual);++visual.revision;++visual.stats.attributes[0];Check(co::AppearanceKey(visual)==unchanged,"health and revisions do not rebuild unchanged visuals");
    namespace rc=zweiklang::recovery;
    rc::NpcCandidate moved{"MERCHANT",true,{9000,0,0},{9000,0,0}};
    Check(rc::ResolveNpcSource("MERCHANT",{0,0,0},{moved})==0,"unique named merchant resolves after walking and reload");
    Check(rc::ResolveNpcSource("OTHER",{0,0,0},{moved})==-1,"distant differently named human not guessed");
    moved.human=false;Check(rc::ResolveNpcSource("MERCHANT",{0,0,0},{moved})==-1,"distant repeated creature does not use human identity shortcut");
    moved.anchor={0,0,0};Check(rc::ResolveNpcSource("",{0,0,0},{moved})==0,"moving creature resolves by registered original anchor");
    auto duplicate=moved;Check(rc::ResolveNpcSource("",{0,0,0},{moved,duplicate})==-1,"ambiguous identical NPC anchors fail closed");
    moved.human=duplicate.human=true;moved.anchor=duplicate.anchor={9000,0,0};Check(rc::ResolveNpcSource("MERCHANT",{0,0,0},{moved,duplicate})==-1,"duplicate human names cannot bypass ambiguity guard");
    Check(rc::ResolveNpcSource("",{0,0,0},{})==-1,"missing NPC source remains unresolved");
    ws::Ground g;g.id=1;g.revision=1;g.instance=123;g.amount=5;g.present=true;g.anchor={10,20,30};g.position={40,50,60};g.name=std::string(63,'A');ws::Ground out;
    auto bytes=ws::Encode(g);Check(bytes.size()<=MaximumDatagramBytes-ProtocolHeaderBytes&&ws::Decode(bytes,out)&&out.position==g.position&&out.name==g.name,"ground state bounded roundtrip");
    for(std::size_t i=0;i<bytes.size();++i)Check(!ws::Decode(Bytes(bytes.begin(),bytes.begin()+i),out),"ground state rejects every truncated prefix");
    bytes.push_back(0);Check(!ws::Decode(bytes,out),"ground state rejects trailing data");
    g.present=false;g.amount=0;Check(ws::Decode(ws::Encode(g),out)&&!out.present,"ground tombstone may have zero stock");
    g.present=true;Check(!ws::Decode(ws::Encode(g),out),"visible zero-quantity ground item rejected");g.amount=1;
    g.position[0]=std::numeric_limits<float>::quiet_NaN();Check(!ws::Decode(ws::Encode(g),out),"nonfinite ground position rejected");
    Check(ws::AcceptNpc(0,0,0,1,1,1),"first NPC generation accepted");
    Check(!ws::AcceptNpc(2,10,1,1,999,1),"old generation cannot replace newer NPC");
    Check(!ws::AcceptNpc(2,10,1,2,10,2),"duplicate revision ignored");
    Check(ws::AcceptNpc(2,10,1,2,11,2)&&ws::AcceptNpc(2,11,2,2,12,1),"dormant NPC can reactivate within generation");
    Check(!ws::AcceptNpc(2,10,3,2,11,1)&&!ws::AcceptNpc(2,10,3,2,11,2),"removed generation cannot be resurrected by late packets");
    Check(ws::AcceptNpc(2,100,3,3,1,1),"explicit new generation may spawn after removal");
    Check(!ws::AcceptNpc(2,10,1,0,11,1)&&!ws::AcceptNpc(2,10,1,3,1,4),"invalid lifecycle values rejected");
    Check(ws::RemovalLife(true)==2&&ws::RemovalLife(false)==3,"distance culling is dormant rather than a permanent NPC tombstone");
    co::Npc npc;npc.id=npc.revision=1;npc.instance=123;npc.stats.attributes[1]=100;
    npc.animation=npc.model=npc.body=npc.head=std::string(63,'A');npc.bodyTex=65535;npc.headTex=65535;npc.armor=999999;
    auto n=co::Encode(npc);co::Npc decoded;
    Check(n.size()+4<=MaximumDatagramBytes-ProtocolHeaderBytes&&co::Decode(n,decoded),"maximum NPC appearance with probe cookie fits reliable datagram");
    Check(decoded.head==npc.head&&decoded.bodyTex==65535&&decoded.armor==999999,"NPC appearance survives wire roundtrip");
    co::Peer peer;peer.stats.attributes[1]=1;peer.interaction=999999;co::Peer peerOut;
    Check(co::Decode(co::Encode(peer),peerOut)&&peerOut.interaction==peer.interaction,"guest dialogue target survives peer codec");
    peer.interaction=1000000;Check(!co::Decode(co::Encode(peer),peerOut),"invalid dialogue target rejected");
}
void TestDualActivation(){
    namespace wt=world_test;namespace ws=zweiklang::worldsync;
    const std::array<float,3> host{0,0,0},guest{50000,0,0};
    Check(ws::InEitherBubble({500,0,0},host,guest,6000),"host area remains active with distant guest");
    Check(ws::InEitherBubble({50500,0,0},host,guest,6000),"distant guest area also remains active");
    Check(!ws::InEitherBubble({25000,0,0},host,guest,6000),"unobserved area need not remain active");
    Check(ws::InEitherBubble({56000,0,0},host,guest,6000),"activation boundary is inclusive");
    int a=1,b=2,c=3,d=4;wt::Node survivor{&a,90},removed{&b,80},reused{&c,70},fresh{&d,55};wt::Manager m;
    m.camPos=host;m.insertTime=15;m.spawnList.nodes={&survivor,&removed,&reused};bool called=false;
    const auto restored=ws::GuestSpawnPass(m,guest,[&](){called=true;Check(m.camPos==guest,"guest spawn pass uses guest camera position");m.insertTime=0;survivor.timer=60;removed.timer=50;reused.npc=&d;reused.timer=40;m.spawnList.nodes={&survivor,&reused,&fresh};});
    Check(called&&restored==1,"only surviving original spawn nodes are restored");
    Check(m.camPos==host&&m.insertTime==15,"host camera and insertion clock restored after guest pass");
    Check(survivor.timer==90,"surviving spawn timer not advanced twice");
    Check(reused.timer==40&&fresh.timer==55&&m.spawnList.GetNumInList()==3,"reused nodes and new spawn timers retained without resurrecting removed node");
    m.spawnList.nodes.assign(8193,&survivor);called=false;
    Check(ws::GuestSpawnPass(m,guest,[&](){called=true;})==0&&!called&&m.camPos==host,"oversize guest spawn pass fails without modifying native manager");
}
void TestWorldTransport(){
    namespace co=zweiklang::combat;namespace ws=zweiklang::worldsync;namespace rc=zweiklang::recovery;
    Network host,guest;ConnectedPair(host,guest);co::Npc npc;npc.id=npc.revision=1;npc.instance=123;npc.stats.attributes[1]=100;
    npc.animation=npc.model=npc.body=npc.head=std::string(63,'A');auto state=co::Encode(npc);state[0]=51;state.insert(state.begin()+1,{1,0,0,0});
    Check(host.SendEvent(state,1200),"maximum appearance probe response accepted by actual reliable UDP channel");Pump(host,guest,1201);Bytes wire;
    Check(guest.ConsumeEvent(wire)&&wire==state,"full lifecycle appearance and probe cookie delivered intact");
    ws::Ground ground;ground.id=ground.revision=ground.instance=ground.amount=1;ground.present=true;auto g=ws::Encode(ground);
    Check(host.SendEvent(g,1210),"ground state sent over reliable UDP");Pump(host,guest,1211);ws::Ground decoded;
    Check(guest.ConsumeEvent(wire)&&ws::Decode(wire,decoded)&&decoded.present,"ground state received and decoded intact");
    rc::Message trade;trade.op=rc::Op::Request;trade.tx=world_test::Trade(rc::Action::Buy);
    Check(guest.SendEvent(rc::Encode(trade),1220),"trade reservation request sent over reliable UDP");Pump(host,guest,1221);rc::Message received;
    Check(host.ConsumeEvent(wire)&&rc::Decode(wire,received)&&rc::Same(trade.tx,received.tx),"priced trade request and timelines arrive unchanged");
    received.op=rc::Op::Result;received.tx.decision=rc::Decision::Commit;
    Check(host.SendEvent(rc::Encode(received),1230),"trade terminal decision sent over reliable UDP");Pump(host,guest,1231);
    Check(guest.ConsumeEvent(wire)&&rc::Decode(wire,trade)&&trade.tx.decision==rc::Decision::Commit&&host.PendingEvents()==0,"trade commit acknowledged through real UDP loopback");
    Check(!guest.ConsumeEvent(wire),"trade commit delivered once to application");
}
void TestTradeRecoveryMatrix(){
    namespace rc=zweiklang::recovery;namespace ws=zweiklang::worldsync;
    for(auto action:{rc::Action::Buy,rc::Action::Sell})for(auto decision:{rc::Decision::Prepared,rc::Decision::Commit,rc::Decision::Abort})for(int mask=0;mask<8;++mask){
        auto tx=world_test::Trade(action);tx.decision=decision;
        const bool buy=action==rc::Action::Buy;const int stockDelta=buy?-3:3;
        int stock=20+((mask&1)?stockDelta:0),items=20,gold=100,calls=0;
        std::set<std::string> hostEffects,ownerEffects;
        if(mask&1)hostEffects.insert(tx.id);
        if(mask&2){ownerEffects.insert(tx.id);if(buy)gold-=12;else items-=3;}
        if(mask&4){ownerEffects.insert(tx.id+"c");if(buy)items+=3;else gold+=12;}
        auto hostApply=[&](const rc::Record& e,bool desired){stock+=(desired?1:-1)*(e.tx.action==rc::Action::Take?-static_cast<int>(e.tx.amount):static_cast<int>(e.tx.amount));++calls;return true;};
        auto ownerApply=[&](const rc::Record& e,bool desired){int& amount=e.tx.item==tx.currency?gold:items;const int sign=e.tx.action==rc::Action::Take?1:-1;amount+=(desired?1:-1)*sign*e.tx.amount;++calls;return true;};
        rc::Record host{tx,rc::Role::Host,1},owner{tx,rc::Role::Owner,1};
        Check(rc::Reconcile(host,hostEffects,hostApply)&&rc::Reconcile(owner,ownerEffects,ownerApply),"trade replay reconciles independently saved stock, reservation and credit");
        const bool committed=decision==rc::Decision::Commit,reserved=decision!=rc::Decision::Abort;
        Check(stock==20+(committed?stockDelta:0)&&items==20+(buy?(committed?3:0):(reserved?-3:0))&&gold==100+(buy?(reserved?-12:0):(committed?12:0)),"buy and sell crash matrix has exact stock, item and gold balances");
        const int before=calls;for(int retry=0;retry<4;++retry){rc::Reconcile(host,hostEffects,hostApply);rc::Reconcile(owner,ownerEffects,ownerApply);}
        Check(calls==before,"trade repeated recovery cannot duplicate payment, goods or credit");
    }
    auto tx=world_test::Trade(rc::Action::Buy);tx.decision=rc::Decision::Commit;rc::Record record{tx,rc::Role::Owner,1};std::set<std::string> effects;int gold=100,items=0;
    auto failCredit=[&](const rc::Record& e,bool desired){if(e.tx.action==rc::Action::Take)return false;gold+=desired?-static_cast<int>(e.tx.amount):static_cast<int>(e.tx.amount);return true;};
    Check(!rc::Reconcile(record,effects,failCredit)&&gold==88&&effects.count(tx.id)&&!effects.count(tx.id+"c"),"failed credit preserves reservation without pretending credit succeeded");
    Check(rc::Reconcile(record,effects,[&](const rc::Record& e,bool desired){if(e.tx.action==rc::Action::Take)items+=desired?e.tx.amount:-static_cast<int>(e.tx.amount);else gold+=desired?-static_cast<int>(e.tx.amount):static_cast<int>(e.tx.amount);return true;})&&gold==88&&items==3,"retry credits purchased goods without charging twice");
    unsigned price=0;Check(ws::Price(100,3,false,.15f,price)&&price==300,"purchase uses full item value");
    Check(ws::Price(100,3,true,.15f,price)&&price==45,"sale quote matches native multiplier");
    Check(ws::Price(1,3,true,.15f,price)&&price==3,"nonzero sale value has native one-gold floor");
    Check(ws::Price(0,3,true,.15f,price)&&price==0,"worthless item does not invent sale proceeds");
    Check(!ws::Price(1000000,2,false,1,price)&&!ws::Price(1,0,true,.15f,price)&&!ws::Price(1,1,true,std::numeric_limits<float>::infinity(),price),"invalid or excessive trade quotes rejected");
    auto bad=tx;bad.currency=0;Check(!rc::Valid(bad),"trade without currency identity rejected");bad=tx;bad.price=1000001;Check(!rc::Valid(bad),"excessive persisted trade quote rejected");
    bad=tx;bad.action=rc::Action::Take;bad.currency=bad.price=0;Check(!rc::Valid(bad),"merchant stock cannot be withdrawn through free container transfer");
    bad=tx;bad.item=bad.currency;Check(!rc::Valid(bad),"currency cannot be traded as merchandise");
}
void TestLocalDropRecovery(){
    namespace rc=zweiklang::recovery;
    for(auto decision:{rc::Decision::Prepared,rc::Decision::Commit,rc::Decision::Abort})for(int mask=0;mask<4;++mask){
        auto tx=recovery_test::Transaction(10,rc::Action::Drop);tx.source.kind=zweiklang::loot::Source::Ground;tx.decision=decision;
        rc::Record local{tx,rc::Role::Local,1};std::set<std::string> effects;int inventory=10-((mask&1)?3:0),world=(mask&2)?3:0,calls=0;
        if(mask&1)effects.insert(tx.id);if(mask&2)effects.insert(tx.id+"c");
        auto apply=[&](const rc::Record& e,bool desired){if(e.role==rc::Role::Owner)inventory+=desired?-3:3;else world+=desired?3:-3;++calls;return true;};
        Check(rc::Reconcile(local,effects,apply)&&inventory==10-(decision!=rc::Decision::Abort?3:0)&&world==(decision==rc::Decision::Commit?3:0),"host drop reconciles reservation and world spawn separately");
        const auto before=calls;rc::Reconcile(local,effects,apply);Check(calls==before,"host drop cannot spawn twice on replay");
        rc::Checkpoint c;c.profile=std::string(32,'c');c.timeline=std::string(32,'d');c.effects=effects;
        Check(rc::DecodeCheckpoint(rc::Encode(c)).effects==effects,"checkpoint persists both host-drop effect legs");
    }
}
void TestLegacyRecoveryMigration(){
    namespace rc=zweiklang::recovery;namespace fs=std::filesystem;
    char executable[MAX_PATH];GetModuleFileNameA(nullptr,executable,MAX_PATH);auto root=fs::path(executable).parent_path().parent_path().parent_path().parent_path();
    const auto fixture=root/"src/ZweiklangCoop/tests/fixtures";const auto oldStore=rc::ReadFile((fixture/"Recovery17Store.bin").string());
    Check(oldStore[1]==1,"golden store was produced by actual Alpha17 version-one serializer");
    const auto c=rc::DecodeCheckpoint(rc::ReadFile((fixture/"Recovery17Checkpoint.bin").string()));
    Check(c.fingerprint=="alpha17-real-serializer"&&c.effects.count(std::string(32,'1')),"real Alpha17 checkpoint loads without losing represented effects");
    const auto dir=recovery_test::Temp(),path=dir+"\\transactions.bin";rc::AtomicWrite(path,oldStore);rc::Store migrated;migrated.Open(path);
    Check(migrated.Profile()==c.profile&&migrated.Records().size()==1&&migrated.Find(std::string(32,'1'))->tx.currency==0&&migrated.Find(std::string(32,'1'))->tx.decision==rc::Decision::Commit,"real Alpha17 journal preserves identity, order and terminal decision");
    auto trade=world_test::Trade(rc::Action::Sell);Check(migrated.Put(trade,rc::Role::Owner),"legacy journal accepts new trade after migration");
    rc::Store restart;restart.Open(path);Check(rc::ReadFile(path)[1]==4&&restart.Profile()==c.profile&&restart.Records().size()==2&&restart.Find(trade.id)->tx.price==12,"version-three rewrite reopens with old and new transactions intact");
    auto bytes=rc::Encode(c);bytes[bytes.size()/2]^=1;bool rejected=false;try{rc::DecodeCheckpoint(bytes);}catch(const std::exception&){rejected=true;}Check(rejected,"corrupt migrated checkpoint fails closed");
    DeleteFileA(path.c_str());RemoveDirectoryA(dir.c_str());
}
