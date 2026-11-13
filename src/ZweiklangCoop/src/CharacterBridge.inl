// Main-thread character identity, private social state and summon ownership.
namespace ch=character;
ch::Archive characterArchive;
std::vector<oCNpc*> characterForms;
oCNpc* characterHumanRoot=nullptr;
std::string characterPeerVisualKey,characterProgressKey,characterMovementKey;
unsigned long long characterLastSend=0;
bool characterReady=false,characterHello=false,characterReplaying=false;
bool characterChangingForm=false;
unsigned characterGuestDepth=0,characterReceipt=0,characterLastSnapshot=0;
unsigned characterEpoch=0;
std::uint32_t characterLastDeltaSent=0;
ch::Counters characterObserved{};
struct CharacterWitnessBank {unsigned references=0;bool guest=false;std::array<int,10> hostAiv{};int hostKnown=0;std::string key;};
std::map<oCNpc*,CharacterWitnessBank> characterWitnessBanks;
ch::CounterBank characterCounterBank;
std::map<std::string,unsigned> characterSummons;
std::map<unsigned,ch::RelationUpdate> characterPendingRelations;
std::deque<co::Bytes> characterOutbox;
const char* characterCounterNames[ch::CounterCount]={
    "PETZCOUNTER_OLDCAMP_THEFT","PETZCOUNTER_CITY_THEFT","PETZCOUNTER_MONASTERY_THEFT","PETZCOUNTER_FARM_THEFT","PETZCOUNTER_BL_THEFT",
    "PETZCOUNTER_OLDCAMP_ATTACK","PETZCOUNTER_OLDCAMP_MURDER","PETZCOUNTER_OLDCAMP_SHEEPKILLER",
    "PETZCOUNTER_CITY_ATTACK","PETZCOUNTER_CITY_MURDER","PETZCOUNTER_CITY_SHEEPKILLER",
    "PETZCOUNTER_MONASTERY_ATTACK","PETZCOUNTER_MONASTERY_MURDER","PETZCOUNTER_MONASTERY_SHEEPKILLER",
    "PETZCOUNTER_FARM_ATTACK","PETZCOUNTER_FARM_MURDER","PETZCOUNTER_FARM_SHEEPKILLER",
    "PETZCOUNTER_BL_ATTACK","PETZCOUNTER_BL_MURDER",
    "ABSOLUTIONLEVEL_OLDCAMP","ABSOLUTIONLEVEL_CITY","ABSOLUTIONLEVEL_MONASTERY","ABSOLUTIONLEVEL_FARM","ABSOLUTIONLEVEL_BL",
    "PARLAN_LASTPETZCRIME","ANDRE_LASTPETZCRIME","HAGEN_LASTPETZCRIME","GAROND_LASTPETZCRIME","LEE_LASTPETZCRIME"};
bool CharacterLocal(oCNpc* npc){return npc&&(npc==player||std::find(characterForms.begin(),characterForms.end(),npc)!=characterForms.end());}
bool CharacterReady(){return !CombatConnected()||characterReady;}
bool CharacterFormSwitch(){return characterChangingForm;}
bool CharacterBusy(){return CombatConnected()&&(!characterReady||!characterOutbox.empty()||!characterArchive.pending.empty()||!characterPendingRelations.empty()||(HostCombat()?characterReceipt!=characterArchive.guest.revision:CharacterCounters()!=characterObserved));}
void CharacterPruneForms(){for(auto it=characterForms.begin();it!=characterForms.end();){auto* n=*it;if(n!=player&&n!=characterHumanRoot&&!n->homeWorld){n->Release();it=characterForms.erase(it);}else ++it;}}
void CharacterHoldForm(oCNpc* npc){
    if(!npc||npc==PartnerNpc())return;
    if(character::LocalVisualAllowed(npc->IsHuman()!=FALSE,static_cast<unsigned>(npc->GetBodyState())))characterHumanRoot=npc;
    if(std::find(characterForms.begin(),characterForms.end(),npc)!=characterForms.end())return;
    CharacterPruneForms();if(characterForms.size()>=32){combatFailure="Character form safety limit reached; reload a trusted save";return;}
    npc->AddRef();characterForms.push_back(npc);CombatLog("character_form","forms="+std::to_string(characterForms.size())+" instance="+std::to_string(npc->GetInstance()));
}
void CharacterClearForms(){for(auto* npc:characterForms)npc->Release();characterForms.clear();characterHumanRoot=nullptr;appearancePlayer=nullptr;appliedAppearance=-1;characterProgressKey.clear();characterMovementKey.clear();}
void CharacterSessionReset(){
    if(characterGuestDepth){CharacterCounters(characterCounterBank.host);if(parser&&player)parser->SetInstance(zSTRING("HERO"),player);}
    for(auto& e:characterWitnessBanks)if(e.second.guest){for(unsigned i=0;i<ch::PersonalAiv.size();++i)e.first->aiscriptvars[ch::PersonalAiv[i]]=e.second.hostAiv[i];e.first->knowsPlayer=e.second.hostKnown;}
    ++characterEpoch;characterReady=characterHello=false;characterGuestDepth=0;characterReceipt=characterLastSnapshot=characterLastDeltaSent=0;characterLastSend=0;characterPeerVisualKey.clear();characterWitnessBanks.clear();characterSummons.clear();characterPendingRelations.clear();characterOutbox.clear();
}
void CharacterNewGame(){CharacterClearForms();characterArchive=ch::Archive{};CharacterSessionReset();}
ch::Counters CharacterCounters(){ch::Counters values{};for(unsigned i=0;i<ch::CounterCount;++i)values[i]=QuestInt(characterCounterNames[i]);return values;}
void CharacterCounters(const ch::Counters& values){for(unsigned i=0;i<ch::CounterCount;++i)QuestSetInt(characterCounterNames[i],values[i]);}
std::string CharacterNpcKey(oCNpc* npc){std::ostringstream out;out<<std::hex<<net::HashString(CurrentWorld()+"|"+std::to_string(npc->GetInstance())+"|"+npc->GetObjectName().ToChar());return out.str();}
ch::Relation& CharacterRelation(oCNpc* npc){
    const auto key=CharacterNpcKey(npc);const auto found=characterArchive.relations.find(key);if(found!=characterArchive.relations.end())return found->second;
    if(characterArchive.relations.size()>=8192)throw std::runtime_error("Personal NPC relation limit reached");
    const int attitude=npc->attitude>=NPC_ATT_NEUTRAL?npc->attitude:(PartnerNpc()?npc->GetGuildAttitude(PartnerNpc()->guild):NPC_ATT_NEUTRAL);
    return characterArchive.relations.emplace(key,ch::Relation{attitude,attitude,{}}).first->second;
}
struct CharacterWitnessScope {
    oCNpc* npc=nullptr;bool previousGuest=false;unsigned epoch=characterEpoch;
    static void Store(oCNpc* n,CharacterWitnessBank& bank){
        if(bank.guest){auto& r=characterArchive.relations.at(bank.key);for(unsigned i=0;i<ch::PersonalAiv.size();++i)r.aiv[i]=n->aiscriptvars[ch::PersonalAiv[i]];r.knows=n->knowsPlayer?1:0;}
        else {for(unsigned i=0;i<ch::PersonalAiv.size();++i)bank.hostAiv[i]=n->aiscriptvars[ch::PersonalAiv[i]];bank.hostKnown=n->knowsPlayer;}
    }
    static void Load(oCNpc* n,const CharacterWitnessBank& bank){
        if(bank.guest){const auto& r=characterArchive.relations.at(bank.key);for(unsigned i=0;i<ch::PersonalAiv.size();++i)n->aiscriptvars[ch::PersonalAiv[i]]=r.aiv[i];n->knowsPlayer=r.knows;}
        else {for(unsigned i=0;i<ch::PersonalAiv.size();++i)n->aiscriptvars[ch::PersonalAiv[i]]=bank.hostAiv[i];n->knowsPlayer=bank.hostKnown;}
    }
    CharacterWitnessScope(oCNpc* value,bool guest){
        if(!value||CharacterLocal(value)||value==PartnerNpc())return;
        auto it=characterWitnessBanks.find(value);
        if(it==characterWitnessBanks.end()){
            if(!guest)return;
            CharacterRelation(value);CharacterWitnessBank bank;for(unsigned i=0;i<ch::PersonalAiv.size();++i)bank.hostAiv[i]=value->aiscriptvars[ch::PersonalAiv[i]];bank.hostKnown=value->knowsPlayer;bank.key=CharacterNpcKey(value);
            it=characterWitnessBanks.emplace(value,std::move(bank)).first;
        }
        npc=value;npc->AddRef();auto& bank=it->second;previousGuest=bank.guest;++bank.references;Store(npc,bank);bank.guest=guest;Load(npc,bank);
    }
    ~CharacterWitnessScope(){
        if(!npc)return;auto it=characterWitnessBanks.find(npc);
        if(epoch==characterEpoch&&it!=characterWitnessBanks.end()){
            auto& bank=it->second;Store(npc,bank);bank.guest=previousGuest;Load(npc,bank);if(!--bank.references)characterWitnessBanks.erase(it);
        }
        npc->Release();
    }
};
struct CharacterGuestScope {
    bool active=false,failed=false,guest=false;unsigned epoch=characterEpoch,previousDepth=characterGuestDepth;std::unique_ptr<QuestBindings> bindings;
    ch::CounterBank previousBank;ch::Authority previousAuthority;ch::Counters previousNative{};
    std::vector<std::unique_ptr<CharacterWitnessScope>> witnesses;
    CharacterGuestScope(bool wantGuest,oCNpc* witness=nullptr,bool allWitnesses=false):guest(wantGuest){
        if(!HostCombat()||!CombatConnected()||!characterReady||!PartnerNpc()||!parser||(!guest&&!characterGuestDepth))return;
        active=true;
        previousBank=characterCounterBank;previousAuthority=characterArchive.guest;previousNative=CharacterCounters();
        try{
            bindings=std::make_unique<QuestBindings>();
            CharacterCounters(characterCounterBank.Switch(previousDepth!=0,guest,CharacterCounters(),characterArchive.guest));
            characterGuestDepth=guest?(previousDepth?previousDepth+1:1):0;
            parser->SetInstance(zSTRING("HERO"),guest?PartnerNpc():player);
            if(allWitnesses)for(auto& e:npcBindings)witnesses.push_back(std::make_unique<CharacterWitnessScope>(e->npc,guest));
            else if(witness)witnesses.push_back(std::make_unique<CharacterWitnessScope>(witness,guest));
        }catch(const std::exception& e){
            witnesses.clear();characterCounterBank=previousBank;characterArchive.guest=previousAuthority;CharacterCounters(previousNative);
            characterGuestDepth=previousDepth;bindings.reset();active=false;failed=true;combatFailure=e.what();
        }
    }
    ~CharacterGuestScope(){
        witnesses.clear();if(!active)return;
        if(epoch!=characterEpoch){if(bindings)bindings->owner=nullptr;bindings.reset();return;}
        try{const auto before=characterArchive.guest.revision;CharacterCounters(characterCounterBank.Switch(guest,previousDepth!=0,CharacterCounters(),characterArchive.guest));if(before!=characterArchive.guest.revision)CombatLog("crime_private_counters","owner=guest revision="+std::to_string(characterArchive.guest.revision));}
        catch(const std::exception& e){combatFailure=e.what();}
        characterGuestDepth=previousDepth;bindings.reset();
    }
};
bool CharacterGuestTarget(oCNpc* npc){
    if(!npc||CharacterLocal(npc)||npc==PartnerNpc()||!PartnerNpc()||!HostCombat()||!CombatConnected()||WorldLocalTalksTo(npc)||npc->state.parOther==player)return false;
    const auto binding=npcByPointer.find(npc);
    return (binding!=npcByPointer.end()&&binding->second->desired.owner==co::GuestPlayer)||npc->state.parOther==PartnerNpc()||npc->enemy==PartnerNpc();
}
bool CharacterAttitude(oCNpc* npc,oCNpc* other,bool permanent,int& result){
    if(!CombatConnected()||!npc||other!=PartnerNpc()||npc==other||CharacterLocal(npc))return false;
    if(HostCombat()){const auto& r=CharacterRelation(npc);result=permanent?r.permanent:r.temporary;return true;}
    auto entry=npcByPointer.find(npc);if(entry==npcByPointer.end()||!entry->second->mirror)return false;
    result=permanent?entry->second->desired.hostAttitude:entry->second->desired.hostTemporary;return true;
}
bool CharacterSetAttitude(oCNpc* npc,int value,bool permanent){
    value=(std::max)(0,(std::min)(4,value));
    if(CombatConnected()&&!HostCombat()&&characterReady&&npc&&WorldLocalTalksTo(npc)){
        auto found=npcByPointer.find(npc);if(found!=npcByPointer.end()&&found->second->mirror){ch::RelationUpdate update;update.npc=found->second->id;update.value=value;update.permanent=permanent;try{CharacterQueue(ch::Encode(update));characterPendingRelations[update.npc]=update;}catch(const std::exception& e){combatFailure=e.what();}}
        return false;
    }
    if(!characterGuestDepth||!CombatConnected()||!HostCombat()||!npc||CharacterLocal(npc)||npc==PartnerNpc())return false;
    auto& r=CharacterRelation(npc);if(permanent)r.permanent=value;else r.temporary=value;
    CombatLog("crime_relation","owner=guest npc="+std::string(npc->GetObjectName().ToChar())+" permanent="+std::to_string(r.permanent)+" temporary="+std::to_string(r.temporary));return true;
}
void CharacterNpcCapture(co::Npc& s,oCNpc* npc){
    const auto found=npcByPointer.find(npc);if(found!=npcByPointer.end()){s.owner=found->second->desired.owner;s.spawnToken=found->second->desired.spawnToken;}
    s.hostAttitude=npc->attitude;s.hostTemporary=npc->tmp_attitude;
    if(HostCombat()&&PartnerNpc()){const auto& r=CharacterRelation(npc);s.guestAttitude=r.permanent;s.guestTemporary=r.temporary;for(unsigned i=0;i<ch::CrimeAiv.size();++i)for(unsigned j=0;j<ch::PersonalAiv.size();++j)if(ch::CrimeAiv[i]==ch::PersonalAiv[j])s.guestCrime[i]=r.aiv[j];}
}
void CharacterNpcApply(NpcBinding& binding,const co::Npc& s){
    if(CharacterLocal(binding.npc))return;
    if(!HostCombat()){
        auto pending=characterPendingRelations.find(s.id);
        if(pending!=characterPendingRelations.end()){
            const int actual=pending->second.permanent?s.guestAttitude:s.guestTemporary;
            if(actual==pending->second.value)characterPendingRelations.erase(pending);else return;
        }
        binding.npc->attitude=s.guestAttitude;binding.npc->tmp_attitude=s.guestTemporary;
        if(!WorldLocalTalksTo(binding.npc))for(unsigned i=0;i<ch::CrimeAiv.size();++i)binding.npc->aiscriptvars[ch::CrimeAiv[i]]=s.guestCrime[i];
    }
    if(s.owner){binding.npc->isSummoned=TRUE;binding.desired.owner=s.owner;binding.desired.spawnToken=s.spawnToken;}
}
co::Visual CharacterVisual(oCNpc* npc){
    co::Visual v;if(!npc)return v;v.model=npc->mds_name.ToChar();v.body=npc->body_visualName.ToChar();v.head=npc->head_visualName.ToChar();v.bodyTex=npc->body_TexVarNr;v.headTex=npc->head_TexVarNr;v.bodyColor=npc->body_TexColorNr;
    auto* armor=npc->GetEquippedArmor();v.armor=armor?armor->GetInstance():-1;v.scale={npc->model_scale[0],npc->model_scale[1],npc->model_scale[2]};v.transformed=(npc->GetBodyState()&BS_MOD_TRANSFORMED)!=0||!npc->IsHuman();v.humanoid=npc->IsHuman()!=FALSE&&!v.transformed;
    if(v.humanoid&&npc->activeOverlays.GetNumInList()<=64)for(int i=0;i<npc->activeOverlays.GetNumInList();++i){const std::string name=npc->activeOverlays[i].ToChar();if(co::SpecialOverlay(name))v.overlays.push_back(name);}
    if(v.humanoid)if(auto* model=npc->GetModel())if(model->modelProtoList.GetNumInList()<=64)for(int i=0;i<model->modelProtoList.GetNumInList();++i){
        auto* prototype=model->modelProtoList[i];if(!prototype)continue;std::string name=prototype->modelProtoFileName.ToChar();const auto slash=name.find_last_of("/\\");if(slash!=std::string::npos)name.erase(0,slash+1);const auto dot=name.find_last_of('.');if(dot!=std::string::npos)name.resize(dot);name+=".MDS";
        if(co::SpecialOverlay(name)&&std::none_of(v.overlays.begin(),v.overlays.end(),[&](const std::string& existing){return _stricmp(existing.c_str(),name.c_str())==0;}))v.overlays.push_back(name);
    }
    if(!co::Valid(v)){combatFailure="Invalid local character visual";return {};}
    return v;
}
void CharacterApplyPeerVisual(){
    auto* npc=PartnerNpc();if(!npc||!havePeerCombat)return;const auto& v=peerCombat.visual;const auto key=co::VisualKey(v);if(key==characterPeerVisualKey)return;
    if(v.model.empty())return;
    if(_stricmp(v.model.c_str(),npc->mds_name.ToChar())!=0)static_cast<zCVob*>(npc)->SetVisual(zSTRING(v.model.c_str()));
    if(!v.body.empty()){zSTRING body(v.body.c_str()),head(v.head.c_str());npc->SetAdditionalVisuals(body,v.bodyTex,v.bodyColor,head,v.headTex,v.headColor,v.armor);}
    for(int i=npc->activeOverlays.GetNumInList()-1;i>=0;--i){zSTRING name=npc->activeOverlays[i];if(co::SpecialOverlay(name.ToChar()))npc->RemoveOverlay(name);}
    for(const auto& name:v.overlays){zSTRING overlay(name.c_str());npc->ApplyOverlay(overlay);}
    npc->SetModelScale(zVEC3(v.scale[0],v.scale[1],v.scale[2]));
    remoteModel=npc->GetModel();if(!remoteModel){combatFailure="Partner transformation visual unavailable";return;}
    // Preserve the proven morph-head texture path after model or armor changes.
    if(v.humanoid&&!v.head.empty()){
        auto* node=remoteModel->SearchNode(zSTRING("BIP01 HEAD"));auto headName=v.head;const auto dot=headName.find_last_of('.');if(dot!=std::string::npos)headName.resize(dot);
        auto* head=node?zCMorphMesh::Load(zSTRING(headName.c_str())):nullptr;
        if(head){zSTRING filter("HUM_HEAD");head->SetTextureVar(0,0,nullptr);head->SetTextureVar(1,0,nullptr);head->SetTextureVar(0,v.headTex,&filter);remoteModel->SetNodeVisual(node,head,TRUE);head->Release();remoteModel->SetNodeMeshTexture(zSTRING("BIP01 HEAD"),0,v.headTex,&filter);}
    }
    remoteModel->SetRandAnisEnabled(FALSE);remoteModel->aniTransScale=zVEC3(0,0,0);remoteModel->doVobRot=FALSE;remoteAnimation=-1;characterPeerVisualKey=key;
    CombatLog("character_visual","peer model="+v.model+" humanoid="+std::to_string(v.humanoid)+" transformed="+std::to_string(v.transformed));
}
void CharacterObserve(unsigned long long){
    if(!player)return;
    CharacterHoldForm(player);CharacterPruneForms();
    std::ostringstream progress;progress<<"instance="<<player->GetInstance()<<" lp="<<player->learn_points<<" xp="<<player->experience_points<<" level="<<player->level;
    for(int i:{1,3,4,5,6,7})progress<<" atr"<<i<<"="<<player->attribute[i];
    for(int i=0;i<5;++i)progress<<" hit"<<i<<"="<<player->hitChance[i];
    if(player->talents.GetNumInList()<=64)for(int i=0;i<player->talents.GetNumInList();++i){auto* t=player->talents[i];if(t)progress<<" talent"<<int(t->m_talent)<<"="<<t->m_value<<","<<t->m_skill;}
    const auto key=progress.str();if(key!=characterProgressKey){CombatLog("character_progress_local",key+" remote_applied=0");characterProgressKey=key;}
    const unsigned state=static_cast<unsigned>(player->GetBodyState());const std::string movement=std::string(ch::MovementName(state))+"|"+std::to_string(state&BS_MOD_TRANSFORMED);
    if(movement!=characterMovementKey){CombatLog("character_movement","mode="+std::string(ch::MovementName(state))+" body="+std::to_string(state)+" animation="+CombatAnimation(player->GetModel()));characterMovementKey=movement;}
}
void CharacterSave(const std::string& dir,const std::string& profile,const std::string& timeline,const std::string& fingerprint){
    characterArchive.profile=profile;characterArchive.timeline=timeline;characterArchive.fingerprint=fingerprint;
    rc::AtomicWrite(dir+"ZWEIKLANG_ACTORS.BIN",ch::Encode(characterArchive));
}
void CharacterLoad(const std::string& dir,const std::string& profile,const std::string& timeline,const std::string& fingerprint){
    ++characterEpoch;
    characterArchive=ch::Archive{};const auto file=dir+"ZWEIKLANG_ACTORS.BIN";
    if(GetFileAttributesA(file.c_str())==INVALID_FILE_ATTRIBUTES)return;
    auto restored=ch::DecodeArchive(rc::ReadFile(file));if(restored.profile!=profile||restored.timeline!=timeline||restored.fingerprint!=fingerprint)throw std::runtime_error("Character state/save profile mismatch");characterArchive=std::move(restored);
}
void CharacterQueue(const co::Bytes& bytes){if(bytes.empty()||bytes.size()>net::MaximumDatagramBytes-net::ProtocolHeaderBytes||characterOutbox.size()>=128)throw std::runtime_error("Character application queue limit reached");characterOutbox.push_back(bytes);}
void CharacterReceive(const co::Bytes& bytes){
    try{
        ch::CounterMessage m;if(ch::Decode(bytes,m)){
            if(HostCombat()){
                if(m.op==ch::CounterOp::Hello){
                    if(characterArchive.guestLineage!=m.timeline){CharacterRemoveOldSummons(m.timeline);characterArchive.guestLineage=m.timeline;characterArchive.guest=ch::Authority{};characterArchive.guest.values=m.values;characterArchive.guest.revision=1;characterArchive.guest.ownerAck=m.ack;characterArchive.relations.clear();}
                    characterReady=true;characterLastSend=0;return;
                }
                if(!characterReady||m.timeline!=characterArchive.guestLineage)throw std::runtime_error("Personal counter owner mismatch");
                if(m.op==ch::CounterOp::Delta){characterArchive.guest.Apply(m.ack,m.values);characterLastSend=0;return;}
                if(m.op==ch::CounterOp::Receipt&&m.revision<=characterArchive.guest.revision){characterReceipt=m.revision;return;}
                throw std::runtime_error("Wrong personal counter event direction");
            }
            if(m.op!=ch::CounterOp::Snapshot||m.timeline!=recoveryTimeline||m.revision<characterLastSnapshot)throw std::runtime_error("Invalid personal counter snapshot");
            if(m.ack>characterArchive.ownerSequence)throw std::runtime_error("Personal counter acknowledgement exceeds owner history");
            for(auto it=characterArchive.pending.begin();it!=characterArchive.pending.end()&&it->first<=m.ack;)it=characterArchive.pending.erase(it);
            auto visible=m.values;for(const auto& pending:characterArchive.pending)visible=ch::Add(visible,pending.second);
            CharacterCounters(visible);characterObserved=visible;characterLastSnapshot=m.revision;characterReady=true;
            ch::CounterMessage receipt;receipt.op=ch::CounterOp::Receipt;receipt.revision=m.revision;receipt.timeline=recoveryTimeline;CharacterQueue(ch::Encode(receipt));return;
        }
        if(bytes.size()&&bytes[0]==71){CharacterReceiveSummon(bytes);return;}
        ch::RelationUpdate relation;if(ch::Decode(bytes,relation)&&HostCombat()&&characterReady&&PartnerNpc()){
            auto entry=npcById.find(relation.npc);if(entry==npcById.end()||!haveTarget||Now()-lastSnapshotAt>2000||Distance2(entry->second->npc->GetPositionWorld(),PartnerNpc()->GetPositionWorld())>2250000)throw std::runtime_error("Personal relation does not belong to guest dialogue range");
            auto& value=CharacterRelation(entry->second->npc);if(relation.permanent)value.permanent=relation.value;else value.temporary=relation.value;characterLastSend=0;
            CombatLog("crime_relation_owner","npc="+std::to_string(relation.npc)+" value="+std::to_string(relation.value));return;
        }
        ch::Passive p;if(ch::Decode(bytes,p)&&HostCombat()&&characterReady&&PartnerNpc()){
            zCVob* victim=nullptr;if(p.target){const auto found=npcById.find(p.target);if(found==npcById.end())return;victim=found->second->npc;}
            if(victim&&Distance2(victim->GetPositionWorld(),PartnerNpc()->GetPositionWorld())>2250000)return;
            characterReplaying=true;PartnerNpc()->CreatePassivePerception(p.perception,PartnerNpc(),victim);characterReplaying=false;return;
        }
        throw std::runtime_error("Malformed character event");
    }catch(const std::exception& e){characterReplaying=false;combatFailure=e.what();CombatLog("character_error",e.what());}
}
void CharacterFrame(unsigned long long now){
    if(!CombatConnected()||!recoveryReady)return;
    try{
        for(unsigned i=0;i<16&&!characterOutbox.empty();++i){if(!network.SendEvent(characterOutbox.front(),now))break;characterOutbox.pop_front();}
        if(!HostCombat()){
            if(!characterHello){ch::CounterMessage hello;hello.timeline=recoveryTimeline;hello.values=CharacterCounters();characterObserved=hello.values;hello.ack=characterArchive.pending.empty()?characterArchive.ownerSequence:characterArchive.pending.begin()->first-1;for(const auto& pending:characterArchive.pending){auto reverse=pending.second;for(int& n:reverse)n=-n;hello.values=ch::Add(hello.values,reverse);}CharacterQueue(ch::Encode(hello));characterHello=true;}
            if(characterReady){
                const auto values=CharacterCounters();const auto delta=ch::Difference(values,characterObserved);
                if(ch::Changed(delta)){if(characterArchive.pending.size()>=64||characterArchive.ownerSequence==UINT32_MAX)throw std::runtime_error("Personal counter pending limit");characterArchive.pending.emplace(++characterArchive.ownerSequence,delta);characterObserved=values;}
                for(const auto& pending:characterArchive.pending)if(pending.first>characterLastDeltaSent){ch::CounterMessage update;update.op=ch::CounterOp::Delta;update.timeline=recoveryTimeline;update.ack=pending.first;update.values=pending.second;CharacterQueue(ch::Encode(update));characterLastDeltaSent=pending.first;}
            }
        }else if(characterReady&&(!characterLastSend||now-characterLastSend>=1000)){
            ch::CounterMessage snapshot;snapshot.op=ch::CounterOp::Snapshot;snapshot.timeline=characterArchive.guestLineage;snapshot.revision=characterArchive.guest.revision;snapshot.ack=characterArchive.guest.ownerAck;snapshot.values=characterArchive.guest.values;CharacterQueue(ch::Encode(snapshot));characterLastSend=now;
        }
    }catch(const std::exception& e){combatFailure=e.what();}
}
