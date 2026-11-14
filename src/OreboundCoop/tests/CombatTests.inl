void TestNetworkMovement(){
    struct Actor {
        int isInMovementMode=0;bool collDetectionStatic=true,collDetectionDynamic=true;
        float position=0,yaw=0;int commits=0;
        void SetCollDetStat(int value){collDetectionStatic=value!=0;}
        void SetCollDetDyn(int value){collDetectionDynamic=value!=0;}
        void BeginMovement(){Check(isInMovementMode==0,"movement transaction begins once");isInMovementMode=1;}
        void SetPositionWorld(float value){Check(isInMovementMode==1,"pose written within movement transaction");if(!collDetectionStatic&&!collDetectionDynamic)position=value;}
        void ResetRotationsWorld(){yaw=0;}
        void RotateWorldY(float value){yaw=value;}
        void EndMovement(int){Check(!collDetectionStatic&&!collDetectionDynamic,"collision bypass lasts through native commit");isInMovementMode=0;++commits;}
    } actor;
    Check(orebound::PlaceNetworkActor(actor,2000.0f,90.0f)&&actor.position==2000&&actor.yaw==90,"network pose accepted despite simulated local collision");
    Check(actor.collDetectionStatic&&actor.collDetectionDynamic&&actor.commits==1,"combat collision flags restored after committed pose");
    actor.collDetectionStatic=false;
    Check(orebound::PlaceNetworkActor(actor,2100.0f,180.0f)&&!actor.collDetectionStatic&&actor.collDetectionDynamic,"asymmetric collision configuration preserved");
    actor.isInMovementMode=1;
    Check(!orebound::PlaceNetworkActor(actor,9999.0f,0.0f)&&actor.position==2100&&actor.commits==2&&actor.isInMovementMode==1,"existing native movement block is never nested or committed by network");
    actor.isInMovementMode=0;actor.collDetectionDynamic=false;
    Check(orebound::PlaceNetworkActor(actor,2200.0f,90.0f)&&!actor.collDetectionStatic&&!actor.collDetectionDynamic,"nonphysical partner stays nonphysical after network pose commit");
}
void TestActiveAnimationPrefix(){
    struct Animation {std::string name;};
    Animation idle{"S_RUN"},attack{"S_ATTACK"},dead{"S_DEAD"},invalid{std::string(64,'X')};
    auto* poison=reinterpret_cast<Animation*>(static_cast<std::uintptr_t>(0x1541));
    Animation* channels[6]={&idle,poison,poison,poison,poison,poison};
    int reads=0;
    auto name=[&](Animation* a){Check(a!=poison,"never dereference an inactive poisoned animation slot");++reads;return a->name;};
    Check(orebound::CombatAnimationName(channels,1,name)=="S_RUN"&&reads==1,"one active channel ignores all five uninitialized slots");
    reads=0;channels[0]=poison;
    Check(orebound::CombatAnimationName(channels,0,name).empty()&&reads==0,"zero active animations never inspect channel storage");
    Check(orebound::CombatAnimationName(channels,-1,name).empty()&&reads==0,"negative animation count rejected");
    Check(orebound::CombatAnimationName(channels,7,name).empty()&&reads==0,"count exceeding native channel capacity rejected");
    channels[0]=&idle;channels[1]=&attack;
    Check(orebound::CombatAnimationName(channels,2,name)=="S_ATTACK","active combat animation preferred over locomotion");
    channels[1]=nullptr;channels[2]=&invalid;channels[3]=&idle;channels[4]=nullptr;channels[5]=&dead;
    Check(orebound::CombatAnimationName(channels,6,name)=="S_DEAD","full active prefix handles null and overlong names");
}
void TestCombatCodec(){
    namespace co=orebound::combat;
    co::Peer peer;peer.revision=12;peer.stats.attributes={40,40,10,10,10,10,0,0};peer.stats.protection[2]=-1;peer.animation="S_RUN";
    co::Peer copy;auto wire=co::Encode(peer);Check(co::Decode(wire,copy)&&copy.stats.attributes[0]==40&&copy.stats.protection[2]==-1&&copy.animation=="S_RUN","player stats codec preserves individual HP and immunity");
    for(std::size_t size=0;size<wire.size();++size){Bytes truncated(wire.begin(),wire.begin()+size);Check(!co::Decode(truncated,copy),"truncated peer state rejected");}
    wire.push_back(0);Check(!co::Decode(wire,copy),"trailing bytes rejected");
    co::Npc npc;npc.id=1;npc.revision=3;npc.instance=400;npc.anchor={1,2,3};npc.pose={10,20,30,90};npc.stats=peer.stats;npc.animation="S_RUNL";
    co::Npc decoded;Check(co::Decode(co::Encode(npc),decoded)&&decoded.id==1&&decoded.pose.x==10&&decoded.stats.attributes[0]==40,"NPC state roundtrip");
    npc.id=0;Check(!co::Decode(co::Encode(npc),decoded),"NPC ID zero cannot alias a player");npc.id=1;
    npc.pose.x=std::numeric_limits<float>::quiet_NaN();Check(!co::Decode(co::Encode(npc),decoded),"nonfinite NPC pose rejected");npc.pose.x=10;
    npc.stats.attributes[0]=41;Check(!co::Decode(co::Encode(npc),decoded),"HP beyond max rejected");npc.stats=peer.stats;
    co::Damage hit;hit.target=1;hit.attacker=co::GuestPlayer;hit.values[1]=20;hit.total=20;co::Damage read;
    Check(co::Decode(co::Encode(hit),read)&&read.target==1&&read.attacker==co::GuestPlayer&&read.values[1]==20,"damage event roundtrip without pointers");
    hit.multiplier=std::numeric_limits<float>::infinity();Check(!co::Decode(co::Encode(hit),read),"invalid damage multiplier rejected");hit.multiplier=1;
    hit.values[1]=1000001;Check(!co::Decode(co::Encode(hit),read),"unbounded damage rejected");
    bool ff=true;std::uint32_t revision=0;Check(co::Rules(co::Rules(42,false),revision,ff)&&!ff&&revision==42,"host friendly-fire rule roundtrip default off");
    Check(co::Newer(1,UINT32_MAX)&&!co::Newer(10,10)&&!co::Newer(9,10),"state ordering including wrap");
}
void TestCombatTransport(){
    Network host,guest;ConnectedPair(host,guest);Bytes event;
    Check(host.SendWorldState({1,2,3}),"world state sent");Pump(host,guest,1001);Check(guest.ConsumeWorldState(event)&&event==Bytes({1,2,3}),"world state delivered");
    for(int i=0;i<64;++i)Check(guest.SendEvent(Bytes{static_cast<std::uint8_t>(i)},1010),"bounded reliable event accepted");
    Check(!guest.SendEvent({255},1010),"full reliable window backpressure");
    Pump(host,guest,1011);
    for(int i=0;i<64;++i)Check(host.ConsumeEvent(event)&&event==Bytes{static_cast<std::uint8_t>(i)},"reliable event delivered once in order");
    Check(!host.ConsumeEvent(event)&&guest.PendingEvents()==0,"ACK clears sender and delivery queue has no duplicate");
    Check(!host.SendEvent({},1020)&&!host.SendEvent(Bytes(MaximumDatagramBytes,1),1020),"empty and oversized events rejected");
    guest.Stop();host.Poll(1030);Check(host.PendingEvents()==0&&!host.ConsumeWorldState(event),"disconnect clears combat state");
    Check(guest.Connect("127.0.0.1",host.BoundPort(),Compatible(),1040),"combat reconnect");Pump(host,guest,1040);
    Check(guest.SendEvent({77},1041),"fresh session accepts first event");Pump(host,guest,1042);Check(host.ConsumeEvent(event)&&event==Bytes{77},"new session resets event ordering");
}
void TestReliableReorderingAndLoss(){
    Network host;Check(host.StartHost(0,Compatible(),1000),"raw reliable host start");RawPeer raw(host.BoundPort());const std::uint64_t nonce=0x34567890123;
    raw.Send(Hello(nonce));host.Poll(1000);Bytes welcome;
    for(int i=0;i<10;++i){auto p=raw.Receive();if(p.empty())break;if(p[6]==static_cast<std::uint8_t>(Message::Welcome))welcome=p;}
    Check(!welcome.empty(),"raw welcome captured");const auto token=Read64(welcome.data()+16);
    raw.Send(Packet(Message::ReliableEvent,{2},token,nonce,2));host.Poll(1001);Bytes event;
    Check(!host.ConsumeEvent(event),"out-of-order event held until missing predecessor");
    raw.Send(Packet(Message::ReliableEvent,{1},token,nonce,1));host.Poll(1002);
    Check(host.ConsumeEvent(event)&&event==Bytes{1},"predecessor first");Check(host.ConsumeEvent(event)&&event==Bytes{2},"buffered successor second");
    raw.Send(Packet(Message::ReliableEvent,{1},token,nonce,1));host.Poll(1003);Check(!host.ConsumeEvent(event),"replayed damage event not delivered twice");
    raw.Send(Packet(Message::ReliableEvent,{9},token^1,nonce,3));host.Poll(1004);Check(!host.ConsumeEvent(event),"wrong session token cannot apply damage");
    raw.Send(Packet(Message::ReliableEvent,{9},token,nonce,1000));host.Poll(1005);Check(!host.ConsumeEvent(event),"reorder-window overflow discarded");
    while(!raw.Receive().empty()){}
    Check(host.SendEvent({8},1100),"host reliable event queued");auto dropped=raw.Receive();Check(!dropped.empty()&&dropped[6]==10,"first reliable transmission deliberately dropped");
    host.Poll(1249);Check(raw.Receive().empty(),"no premature reliable retry");host.Poll(1250);auto retry=raw.Receive();Check(retry==dropped,"lost event retransmitted with same ID and payload");
    raw.Send(Packet(Message::EventAck,{},token,nonce,1));host.Poll(1251);Check(host.PendingEvents()==0,"retry ACK stops retransmission");host.Poll(1401);Check(raw.Receive().empty(),"ACKed event not retried");
    Check(host.SendEvent({9},1500),"event before timeout");host.SetTimeoutMs(100);host.Poll(1601);Check(host.GetState()==State::Error&&host.PendingEvents()==0,"unconfirmed damage fails closed on acknowledgement timeout");
}
