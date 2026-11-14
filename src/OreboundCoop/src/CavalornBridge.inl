namespace cv=cavalorn;
const char* cavalornInfos[]={
    "DIA_ADDON_CAVALORN_HALLO","DIA_ADDON_CAVALORN_WASMACHSTDU","DIA_ADDON_CAVALORN_BANDITEN",
    "DIA_ADDON_CAVALORN_ARTEFAKT","DIA_ADDON_CAVALORN_HELFEN","DIA_ADDON_CAVALORN_AUSRUESTUNG",
    "DIA_ADDON_CAVALORN_LETSKILLBANDITS","DIA_ADDON_CAVALORN_BRAGOKILLED","DIA_ADDON_CAVALORN_PCKILLEDBRAGO",
    "DIA_ADDON_CAVALORN_MEETINGISRUNNING","DIA_ADDON_CAVALORN_JUNGS"};
cv::Rewards cavalornRewards;
std::string questError,questStateKey;
bool questSessionBound=false,questReplaying=false;
zCParser* questCheckedParser=nullptr;
oCInfoManager* questCheckedInfoManager=nullptr;
unsigned questRevision=0,questReceivedRevision=0;
unsigned long long questLastState=0,questLastSample=0;
std::map<std::string,unsigned long long> questLastReward;
std::set<std::string> questReceipts;
std::map<oCInfo*,int> questOriginalConditions;

int QuestInt(const char* name,int fallback=0){auto* s=parser?parser->GetSymbol(zSTRING(name)):nullptr;int value=fallback;if(s&&s->type==2)s->GetValue(value,0);return value;}
bool QuestSetInt(const char* name,int value){auto* s=parser?parser->GetSymbol(zSTRING(name)):nullptr;if(!s||s->type!=2)return false;s->SetValue(value,0);return true;}
struct QuestBindings {
    zCParser* owner=parser;void* values[5]{};bool present[5]{};
    const char* names[5]={"SELF","OTHER","VICTIM","ITEM","HERO"};
    QuestBindings(){if(owner)for(int i=0;i<5;++i){auto* s=owner->GetSymbol(zSTRING(names[i]));if(s){present[i]=true;values[i]=s->GetInstanceAdr();}}}
    ~QuestBindings(){if(owner&&owner==parser)for(int i=0;i<5;++i)if(present[i])owner->SetInstance(zSTRING(names[i]),values[i]);}
};
struct QuestReplayScope {bool previous=questReplaying;QuestReplayScope(){questReplaying=true;}~QuestReplayScope(){questReplaying=previous;}};
struct QuestXpScope {int previous=QuestInt("ZK_COOP_QUEST_XP");explicit QuestXpScope(int amount){QuestSetInt("ZK_COOP_QUEST_XP",amount);}~QuestXpScope(){QuestSetInt("ZK_COOP_QUEST_XP",previous);}};
oCInfo* QuestInfo(const char* name){if(!parser||!ogame||!ogame->infoman)return nullptr;const int id=parser->GetIndex(zSTRING(name));return id<0?nullptr:ogame->infoman->GetInformation(id);}
int QuestInfoIndex(oCInfo* info){if(!info)return -1;for(int i=0;i<11;++i)if(_stricmp(info->name.ToChar(),cavalornInfos[i])==0)return i;return -1;}
bool QuestSupported(){
    if(!parser||!player||!ogame||CurrentWorld()!="NEWWORLD.ZEN")return false;
    static bool checked=false,profileMatches=false;
    if(!checked){checked=true;try{const auto bytes=rc::ReadFile(systemDirectory+"\\..\\_work\\Data\\Scripts\\_compiled\\GOTHIC.DAT");std::uint64_t hash=14695981039346656037ull;for(auto b:bytes){hash^=b;hash*=1099511628211ull;}profileMatches=hash==cv::SupportedScriptFingerprint;}catch(const std::exception&){profileMatches=false;}LootLog("quest_profile",profileMatches?"Verified installed Gothic2 script profile":"Unsupported script profile; Cavalorn adapter disabled");}
    if(!profileMatches||parser->GetIndex(zSTRING("DIA_ADDON_CAVALORN_HELFEN"))!=13386)return false;
    if(questCheckedParser==parser&&questCheckedInfoManager==ogame->infoman)return true;
    for(const auto* name:{"MIS_ADDON_CAVALORN_KILLBRAGO","MIS_ADDON_CAVALORN_LETTER2VATRAS","BDT_1013_AWAY","ZK_COOP_QUEST_XP"}){auto* s=parser->GetSymbol(zSTRING(name));if(!s||s->type!=2)return false;}
    for(const auto* name:{"ZK_COOP_APPLY_QUEST_XP","ZK_COOP_CAVALORN_LOG","ZK_COOP_CAVALORN_WAIT"}){auto* s=parser->GetSymbol(zSTRING(name));if(!s||s->type!=5||s->ele!=0)return false;}
    for(const auto* name:cavalornInfos)if(!QuestInfo(name))return false;
    if(QuestInt("XP_ADDON_CAVALORN_KILLBRAGO")!=100||QuestInt("LOG_RUNNING")!=1||QuestInt("LOG_SUCCESS")!=2)return false;
    const int gold=parser->GetIndex(zSTRING("ITMI_GOLD")),knife=parser->GetIndex(zSTRING("ITMW_ADDON_KNIFE01")),potion=parser->GetIndex(zSTRING("ITPO_HEALTH_01"));
    if(gold<=0||knife<=0||potion<=0)return false;cavalornRewards={100,static_cast<unsigned>(gold),static_cast<unsigned>(knife),static_cast<unsigned>(potion)};questCheckedParser=parser;questCheckedInfoManager=ogame->infoman;return true;
}
void QuestCall(const char* name){
    if(std::string(name)=="ZK_COOP_CAVALORN_LOG")ExtensionApplyLanguage();
    const int index=parser->GetIndex(zSTRING(name));if(index<0)throw std::runtime_error("Own quest helper missing");
    QuestBindings bindings;
    parser->SetInstance(zSTRING("HERO"),player);if(std::string(name)=="ZK_COOP_APPLY_QUEST_XP")ExtensionCall(extensions::Command::QuestXp,QuestInt("ZK_COOP_QUEST_XP"));else parser->CallFunc(index);
}
unsigned QuestInventory(unsigned instance){
    if(!player)return 0;QuestBindings bindings;player->inventory2.UnpackAllItems();LootSource source;source.vob=source.corpse=player;std::uint64_t amount=0;
    for(auto* item:LootNativeItems(source))if(static_cast<unsigned>(item->GetInstance())==instance)amount+=item->amount;
    return static_cast<unsigned>((std::min)(amount,std::uint64_t(UINT32_MAX)));
}
void QuestFail(const std::string& reason){if(questError!=reason)LootLog("quest_blocked",reason);questError=reason;notice=Ui("ui_f02982da1dfb")+reason;}
void QuestRestoreConditions(){cv::RestoreConditions(questOriginalConditions);}
void QuestSessionReset(){QuestRestoreConditions();havePeerReadiness=false;peerReadiness=session::Ready{};questReceivedRevision=questRevision=0;questLastState=questLastSample=0;questStateKey.clear();questLastReward.clear();questReceipts.clear();}
const rc::Record* QuestEvent(unsigned event){
    for(const auto& pair:recoveryStore.Records()){const auto& r=pair.second;if(r.role==rc::Role::Host&&r.tx.source.kind==lo::Source::Quest&&r.tx.hostTimeline==recoveryTimeline&&r.tx.instance==static_cast<int>(event))return &r;}
    return nullptr;
}
unsigned QuestPendingAwards(){unsigned count=0;if(!HostCombat()||!havePeerReadiness)return 0;for(const auto& pair:recoveryStore.Records()){const auto& r=pair.second;if(r.role==rc::Role::Host&&r.tx.source.kind==lo::Source::Quest&&r.tx.hostTimeline==recoveryTimeline&&r.tx.ownerTimeline==peerReadiness.timeline&&!questReceipts.count(r.tx.id))++count;}return count;}
bool QuestRecoveryEffect(const rc::Record& record,bool desired,bool& handled){
    handled=record.tx.source.kind==lo::Source::Quest;if(!handled)return false;
    if(!desired||!QuestSupported()||!cv::Allowed(record.tx,cavalornRewards))return false;
    if(!cv::EligibleForCredit(player->attribute[0],player->IsUnconscious()!=FALSE))return false;
    if(record.role==rc::Role::Host){
        auto* supply=QuestInfo(cavalornInfos[5]);
        if(!cv::NativeSaveRepresents(record.tx.instance,QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO"),supply&&supply->WasTold())){
            QuestFail("Host-Save liegt vor bereits vergebener Questbelohnung; passenden neueren Testsave laden.");return false;
        }
    }
    if(record.tx.action==rc::Action::QuestXp){
        const auto before=player->experience_points;QuestXpScope xp(record.tx.amount);QuestReplayScope replay;QuestCall("ZK_COOP_APPLY_QUEST_XP");
        if(player->experience_points-before!=record.tx.amount)return false;
    }else {QuestBindings bindings;LootSource owner;owner.vob=owner.corpse=player;if(!RecoveryAdd(owner,record.tx.item,record.tx.amount))return false;}
    LootLog("quest_reward_applied","tx="+record.tx.id+" event="+std::to_string(record.tx.instance)+" amount="+std::to_string(record.tx.amount));return true;
}
bool QuestPartyReady(){
    const unsigned required=session::Checkpoint|session::Rules|session::Actor|session::Quest;
    return CombatConnected()&&HostCombat()&&QuestSupported()&&questError.empty()&&recoveryReady&&recoveryError.empty()&&!recoveryBlocked&&!lootLedger.HasUnapplied()&&cv::EligibleForCredit(player->attribute[0],player->IsUnconscious()!=FALSE)&&rulesReady&&remote&&haveTarget&&Now()-lastSnapshotAt<=2000&&havePeerReadiness&&(peerReadiness.flags&required)==required&&!(peerReadiness.flags&(session::Blocked|session::Pending|session::Incapacitated))&&!peerReadiness.timeline.empty();
}
void QuestUpdateConditions(){
    if(!QuestSupported()){QuestRestoreConditions();return;}
    const bool block=(CombatConnected()||questSessionBound)&&!QuestPartyReady();
    if(!block){QuestRestoreConditions();return;}
    const int wait=parser->GetIndex(zSTRING("ZK_COOP_CAVALORN_WAIT"));
    if(questOriginalConditions.size()==11){for(const auto& entry:questOriginalConditions)entry.first->pd.conditions=wait;return;}
    for(const auto* name:cavalornInfos)cv::WaitCondition(questOriginalConditions,QuestInfo(name),wait);
}
bool QuestAllowInfo(oCInfo* info){
    const int which=QuestInfoIndex(info);if(which<0||questReplaying||(!CombatConnected()&&!questSessionBound))return true;
    if(!CombatConnected()||!HostCombat()){notice=Ui("ui_37398218e189");return false;}
    if(!QuestPartyReady()){
        notice=Ui("ui_06d05082ebea");return false;
    }
    if((which==7||which==8)&&QuestEvent(1))return false;
    if(which==5&&(QuestEvent(3)||QuestEvent(4)))return false;
    return true;
}
struct QuestBefore {int which=-1;unsigned xp=0,gold=0,knife=0,potion=0;std::string host,owner;};
QuestBefore QuestBeforeInfo(oCInfo* info){
    QuestBefore before;before.which=QuestInfoIndex(info);
    if(before.which<0||!CombatConnected()||!HostCombat()||!QuestSupported()){before.which=-1;return before;}
    before.host=recoveryTimeline;before.owner=peerReadiness.timeline;before.xp=player->experience_points;
    before.gold=QuestInventory(cavalornRewards.goldItem);before.knife=QuestInventory(cavalornRewards.knifeItem);before.potion=QuestInventory(cavalornRewards.potionItem);return before;
}
void QuestRecordNative(const QuestBefore& before,unsigned event,unsigned item,unsigned amount){
    if(!amount)return;if(QuestEvent(event))throw std::runtime_error("Repeated native quest reward event");
    rc::Tx tx;tx.id=rc::NewGuid();tx.hostTimeline=before.host;tx.ownerTimeline=before.owner;tx.world=CurrentWorld();tx.source.kind=lo::Source::Quest;tx.source.name="CAVALORN_KILLBRAGO";tx.instance=event;
    tx.action=event==1?rc::Action::QuestXp:rc::Action::QuestItem;tx.item=item;tx.amount=amount;tx.decision=rc::Decision::Commit;
    if(!cv::Allowed(tx,cavalornRewards)||!recoveryStore.Put(tx,rc::Role::Host))throw std::runtime_error("Native Cavalorn reward did not match verified adapter");
    // Native host script just applied this effect, on the same game thread and
    // before another native save can run. Do not grant it to the host again.
    recoveryEffects.insert(tx.id);questSessionBound=true;
    LootLog("quest_reward_committed","tx="+tx.id+" event="+std::to_string(event)+" amount="+std::to_string(amount));
}
void QuestAfterInfo(const QuestBefore& before){
    if(before.which<0)return;
    try{
        if(before.host!=recoveryTimeline)throw std::runtime_error("Quest lineage changed during native dialogue");
        if(before.which==5){
            const auto knife=QuestInventory(cavalornRewards.knifeItem),potion=QuestInventory(cavalornRewards.potionItem);
            if(knife<before.knife||potion<before.potion)throw std::runtime_error("Native support gift inventory unexpectedly decreased");
            QuestRecordNative(before,3,cavalornRewards.knifeItem,knife-before.knife);QuestRecordNative(before,4,cavalornRewards.potionItem,potion-before.potion);
        }
        if(before.which==7||before.which==8){
            const auto xp=player->experience_points;const auto gold=QuestInventory(cavalornRewards.goldItem);
            if(xp<before.xp||xp-before.xp!=cavalornRewards.xp||gold<before.gold||QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO")!=2)throw std::runtime_error("Native quest completion incomplete");
            QuestRecordNative(before,1,0,xp-before.xp);QuestRecordNative(before,2,cavalornRewards.goldItem,gold-before.gold);
        }
        questSessionBound=true;questStateKey.clear();LootLog("quest_dialogue","host_info="+std::to_string(before.which)+" mission="+std::to_string(QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO")));
    }catch(const std::exception& e){QuestFail(e.what());RecoveryFail("Quest reward journal incomplete; do not continue with this save");}
}
void QuestReceiveReward(const rc::Message& m){
    if(!QuestSupported()||!cv::Allowed(m.tx,cavalornRewards)||m.tx.world!=CurrentWorld())throw std::runtime_error("Invalid quest reward payload");
    if(HostCombat()){
        const auto* existing=recoveryStore.Find(m.tx.id);
        if(m.op!=rc::Op::Receipt||!existing||existing->role!=rc::Role::Host||!rc::Same(existing->tx,m.tx)||!havePeerReadiness||m.tx.ownerTimeline!=peerReadiness.timeline)throw std::runtime_error("Invalid quest receipt");
        questReceipts.insert(m.tx.id);LootLog("quest_reward_receipt",m.tx.id);return;
    }
    if(m.op!=rc::Op::Result||m.tx.hostTimeline!=recoveryPeerTimeline||m.tx.ownerTimeline!=recoveryTimeline||!havePeerReadiness||m.tx.hostTimeline!=peerReadiness.timeline)throw std::runtime_error("Quest reward belongs to different save/host");
    if(!questReceivedRevision||!questError.empty()){LootLog("quest_reward_deferred","Waiting for matching Cavalorn state before award: "+m.tx.id);return;}
    if(!cv::EligibleForCredit(player->attribute[0],player->IsUnconscious()!=FALSE)){LootLog("quest_reward_deferred","Recipient dead/unconscious; no credit or receipt yet: "+m.tx.id);return;}
    for(const auto& pair:recoveryStore.Records())if(pair.second.role==rc::Role::Owner&&cv::SameEvent(pair.second.tx,m.tx)&&pair.second.tx.id!=m.tx.id)throw std::runtime_error("Conflicting duplicate quest award event");
    const auto* old=recoveryStore.Find(m.tx.id);
    if(old&&(old->role!=rc::Role::Owner||!rc::Same(old->tx,m.tx)))throw std::runtime_error("Quest reward ID conflict");
    if(!recoveryStore.Put(m.tx,rc::Role::Owner)||!RecoveryReconcile(*recoveryStore.Find(m.tx.id)))throw std::runtime_error("Quest reward application deferred");
    auto ack=m;ack.op=rc::Op::Receipt;QueueRecovery(ack);
}
session::Cavalorn QuestState(){
    session::Cavalorn s;s.hostTimeline=recoveryTimeline;s.ownerTimeline=peerReadiness.timeline;s.mission=QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO");s.letter=QuestInt("MIS_ADDON_CAVALORN_LETTER2VATRAS");s.away=QuestInt("BDT_1013_AWAY")?1:0;
    for(unsigned i=0;i<11;++i){auto* info=QuestInfo(cavalornInfos[i]);if(info&&info->WasTold())s.told|=1u<<i;}
    const char* enemies[]={"AMBUSHER_1013","AMBUSHER_1014","AMBUSHER_1015"};
    for(unsigned i=0;i<3;++i){const int instance=parser->GetIndex(zSTRING(enemies[i]));int matches=0;bool dead=false;for(const auto& binding:npcBindings)if(binding->npc->GetInstance()==instance){++matches;dead=binding->npc->IsDead()!=FALSE;}if(matches==1&&dead)s.dead|=1u<<i;}
    return s;
}
void QuestReceiveState(const session::Cavalorn& s){
    if(HostCombat()||!QuestSupported()||s.hostTimeline!=recoveryPeerTimeline||s.ownerTimeline!=recoveryTimeline||!havePeerReadiness||s.hostTimeline!=peerReadiness.timeline)return;
    if(!combat::Newer(s.revision,questReceivedRevision))return;
    const auto local=QuestInt("MIS_ADDON_CAVALORN_KILLBRAGO");
    if(!session::ProgressCompatible(s.mission,local)||!session::ProgressCompatible(s.letter,QuestInt("MIS_ADDON_CAVALORN_LETTER2VATRAS"))){QuestFail("Queststand passt nicht zum Host; passenden gemeinsamen Teststand laden.");return;}
    questReceivedRevision=s.revision;questSessionBound=true;
    QuestSetInt("MIS_ADDON_CAVALORN_KILLBRAGO",s.mission);QuestSetInt("MIS_ADDON_CAVALORN_LETTER2VATRAS",s.letter);QuestSetInt("BDT_1013_AWAY",s.away);
    for(unsigned i=0;i<11;++i){auto* info=QuestInfo(cavalornInfos[i]);if(info)info->SetTold((s.told&(1u<<i))?TRUE:FALSE);}
    QuestCall("ZK_COOP_CAVALORN_LOG");
    LootLog("quest_state_applied","mission="+std::to_string(s.mission)+" told="+std::to_string(s.told)+" dead_mask="+std::to_string(s.dead));
}
void QuestFrame(unsigned long long now){
    QuestUpdateConditions();
    if(!CombatConnected()||!QuestSupported()||!recoveryReady||recoveryBlocked||!recoveryError.empty()||!questError.empty()||!havePeerReadiness||!(peerReadiness.flags&session::Checkpoint)||peerReadiness.timeline.empty())return;
    if(!HostCombat())return;
    if(questStateKey.empty()||now-questLastSample>=250){
        questLastSample=now;auto state=QuestState();auto key=std::to_string(state.mission)+","+std::to_string(state.letter)+","+std::to_string(state.told)+","+std::to_string(state.dead)+","+std::to_string(state.away);
        if(key!=questStateKey||now-questLastState>=3000){state.revision=++questRevision;auto bytes=session::Encode(state);if(lootOutbox.size()<96){lootOutbox.push_back(std::move(bytes));questStateKey=key;questLastState=now;}}
    }
    if(!(peerReadiness.flags&session::Quest)||(peerReadiness.flags&session::Blocked))return;
    unsigned sent=0;for(const auto& pair:recoveryStore.Records()){
        const auto& record=pair.second;const auto& tx=record.tx;
        if(record.role!=rc::Role::Host||tx.source.kind!=lo::Source::Quest||tx.hostTimeline!=recoveryTimeline||tx.ownerTimeline!=peerReadiness.timeline||questReceipts.count(tx.id)||now-questLastReward[tx.id]<2000)continue;
        if(!recoveryEffects.count(tx.id))continue;
        rc::Message m;m.op=rc::Op::Result;m.tx=tx;m.request=NextLootRequest();QueueRecovery(m);questLastReward[tx.id]=now;if(++sent>=4)break;
    }
}
