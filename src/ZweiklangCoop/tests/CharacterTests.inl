void TestCharacterRules(){
    namespace ch=zweiklang::character;namespace co=zweiklang::combat;
    Check(!ch::LocalVisualAllowed(false,0)&&!ch::LocalVisualAllowed(true,4096)&&ch::LocalVisualAllowed(true,0),"do not paint human appearance onto animal or transformed character");
    for(unsigned state=0;state<32;++state){Check(ch::SpecialMovement(state)==(state>=5&&state<=10),"native special locomotion classification");Check(ch::SpecialMovement(state|4096)==ch::SpecialMovement(state),"transform modifier preserves movement category");}
    for(const auto& model:std::vector<std::string>{"WOLF.MDS","SCAVENGER.MDS","SHEEP.MDS","WARAN.MDS","HUMANS.MDS"}){
        co::Peer source;source.revision=31;source.stats.attributes[1]=100;source.stats.guild=28;source.stats.trueGuild=1;source.stats.bodyState=4096|8;
        source.animation="T_JUMP";source.visual.model=model;source.visual.humanoid=model=="HUMANS.MDS";source.visual.transformed=!source.visual.humanoid;
        source.visual.scale={1.1f,0.9f,1.0f};if(source.visual.humanoid){source.visual.body="HUM_BODY_BABE0";source.visual.head="HUM_HEAD_BABE";source.visual.headTex=137;}
        const auto bytes=co::Encode(source);co::Peer copy;
        Check(bytes.size()+zweiklang::net::ProtocolHeaderBytes<=zweiklang::net::MaximumDatagramBytes,"transformed peer fits supported UDP envelope");
        Check(co::Decode(bytes,copy)&&co::VisualKey(source.visual)==co::VisualKey(copy.visual),"human and animal appearance roundtrip with empty animal head");
        Check(copy.stats.trueGuild==1&&copy.stats.guild==28,"transformation/disguise does not overwrite true faction");
        auto truncated=bytes;truncated.pop_back();Check(!co::Decode(truncated,copy),"truncated visual is rejected");
    }
    co::Peer peer;peer.stats.attributes[1]=1;peer.visual.model="../outside.MDS";co::Peer decoded;
    Check(!co::Decode(co::Encode(peer),decoded),"visual traversal refused");peer.visual.model="WOLF.MDS";peer.visual.scale[0]=std::numeric_limits<float>::quiet_NaN();Check(!co::Decode(co::Encode(peer),decoded),"nonfinite transform scale refused");
    struct Ani {std::string name;};Ani run{"S_RUNL"},climb{"T_CLIMB_2_STAND"},swim{"S_SWIM"},fall{"S_FALL"},dead{"S_DEAD"};Ani* active[6]={&run,&climb,reinterpret_cast<Ani*>(1),nullptr,nullptr,nullptr};
    const auto read=[](Ani* p){return p->name;};
    Check(zweiklang::CombatAnimationName(active,2,read)==climb.name,"climb transition wins over active running loop without touching invalid inactive slot");
    active[1]=&swim;Check(zweiklang::CombatAnimationName(active,2,read)==swim.name,"swim wins over running");active[1]=&fall;Check(zweiklang::CombatAnimationName(active,2,read)==fall.name,"fall wins over running");active[2]=&dead;Check(zweiklang::CombatAnimationName(active,3,read)==dead.name,"death wins over special movement");
    ch::Authority authority;authority.revision=1;authority.values[0]=5;ch::Counters fine{};fine[0]=-5;
    {
        ch::Authority guest;guest.values[0]=2;guest.revision=1;ch::CounterBank bank;ch::Counters native{};native[0]=7;
        native=bank.Switch(false,true,native,guest);Check(native[0]==2&&bank.host[0]==7,"guest script sees its own counter bank");
        ++native[0];native=bank.Switch(true,false,native,guest);Check(native[0]==7&&guest.values[0]==3,"nested host perception restores innocent host bank");
        native[0]+=4;native=bank.Switch(false,true,native,guest);Check(native[0]==3&&bank.host[0]==11,"nested host offence is not lost when returning to guest context");
        native[0]+=2;native=bank.Switch(true,false,native,guest);Check(native[0]==11&&guest.values[0]==5,"both offenders retain separate nested updates");
    }
    auto crime=authority.values;crime[0]+=3;authority.Observe(crime);Check(authority.Apply(1,fine)&&authority.values[0]==3,"paying old fine preserves newer independent offence");
    Check(!authority.Apply(1,fine)&&authority.values[0]==3,"duplicate fine receipt never subtracts twice");
    bool conflict=false;try{auto wrong=fine;wrong[0]=-2;authority.Apply(1,wrong);}catch(...){conflict=true;}Check(conflict,"duplicate sequence with conflicting fine payload rejected");
    conflict=false;try{authority.Apply(3,fine);}catch(...){conflict=true;}Check(conflict,"missing personal counter predecessor rejected");
    for(unsigned spell=0;spell<90;++spell){ch::Summon source{spell,std::string(32,'a')},copy;Check(ch::Decode(ch::Encode(source),copy)==ch::IsSummonSpell(spell),"only explicitly supported summons can be replayed");}
    ch::Summon bad{25,"not-a-token"},summon;Check(!ch::Decode(ch::Encode(bad),summon),"unbound summon identity rejected");
    ch::Archive stored;stored.profile=std::string(32,'1');stored.timeline=std::string(32,'2');stored.fingerprint="matching-native-save";stored.guestLineage=std::string(32,'3');stored.guest=authority;stored.ownerSequence=2;stored.pending[2]=fine;stored.relations.emplace("npc-key",ch::Relation{0,1,{0,0,0,0,0,0,4}});
    const auto bytes=ch::Encode(stored);auto restored=ch::DecodeArchive(bytes);
    Check(restored.guest.values==stored.guest.values&&restored.guest.ownerAck==stored.guest.ownerAck&&restored.guest.lastDelta==stored.guest.lastDelta,"private crime counters and duplicate guard persist");
    Check(restored.pending==stored.pending&&restored.relations.at("npc-key").aiv[6]==4,"pending owner fine and per-NPC absolution persist");
    for(std::size_t length=0;length<bytes.size();++length){bool rejected=false;try{ch::DecodeArchive(co::Bytes(bytes.begin(),bytes.begin()+length));}catch(...){rejected=true;}Check(rejected,"every truncated private character archive rejected");}
    auto large=stored;large.relations.at("npc-key").temporary=5;bool invalid=false;try{ch::DecodeArchive(ch::Encode(large));}catch(...){invalid=true;}Check(invalid,"invalid native attitude rejected on restore");
    auto damaged=bytes;damaged[damaged.size()/2]^=1;invalid=false;try{ch::DecodeArchive(damaged);}catch(...){invalid=true;}Check(invalid,"damaged private character archive fails checksum before application");
    co::Npc npc;npc.id=1;npc.instance=100;npc.stats.attributes[1]=1;npc.owner=co::GuestPlayer;npc.spawnToken=std::string(32,'b')+"_1";npc.hostAttitude=3;npc.guestAttitude=0;co::Npc npcCopy;
    Check(co::Decode(co::Encode(npc),npcCopy)&&npcCopy.owner==co::GuestPlayer&&npcCopy.spawnToken==npc.spawnToken&&npcCopy.hostAttitude==3&&npcCopy.guestAttitude==0,"summoner identity and distinct NPC attitudes roundtrip");npc.owner=123;Check(!co::Decode(co::Encode(npc),npcCopy),"arbitrary summon owner rejected");
    ch::CounterMessage message;message.timeline=std::string(32,'c');message.values[0]=1;ch::CounterMessage messageCopy;Check(ch::Decode(ch::Encode(message),messageCopy),"counter handshake roundtrip");message.op=ch::CounterOp::Delta;message.ack=1;message.values=fine;Check(ch::Decode(ch::Encode(message),messageCopy)&&messageCopy.values[0]==-5,"signed owner fine delta roundtrip");
}
