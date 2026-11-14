const char* characterSummonInstances[]={"SUMMONED_GOBBO_SKELETON","SUMMONED_WOLF","SUMMONED_SKELETON","SUMMONED_GOLEM","SUMMONED_DEMON","SUMMONED_GUARDIAN","SUMMONED_ZOMBIE"};
bool CharacterSummonNpc(oCNpc* npc){if(!npc||!parser)return false;for(const auto* name:characterSummonInstances)if(npc->GetInstance()==parser->GetIndex(zSTRING(name)))return true;return false;}
std::set<oCNpc*> CharacterNpcSet(){std::set<oCNpc*> result;auto* world=ogame?ogame->GetGameWorld():nullptr;if(world&&world->voblist_npcs)for(auto* i=world->voblist_npcs->GetNextInList();i;i=i->GetNextInList())if(i->GetData())result.insert(i->GetData());return result;}
unsigned CharacterOwner(oCNpc* npc){
    if(!npc)return 0;if(CharacterLocal(npc))return HostCombat()?co::HostPlayer:co::GuestPlayer;if(npc==PartnerNpc())return HostCombat()?co::GuestPlayer:co::HostPlayer;
    auto i=npcByPointer.find(npc);return i==npcByPointer.end()?0:i->second->desired.owner;
}
bool CharacterAllied(oCNpc* a,oCNpc* b){return CombatConnected()&&!sessionFriendlyFire&&a!=b&&CharacterOwner(a)&&CharacterOwner(b);}
std::string CharacterSummonName(unsigned owner,const std::string& token){return std::string(owner==co::GuestPlayer?"ZKSG_":"ZKSH_")+token;}
std::string CharacterLineageTag(const std::string& lineage){std::ostringstream text;text<<std::hex<<net::HashString(lineage);return text.str()+"_";}
bool CharacterWaitingForOwner(oCNpc* npc){
    if(!npc||!npc->isSummoned)return false;const std::string name=npc->GetObjectName().ToChar();
    return name.rfind("ZKSG_",0)==0&&(!CombatConnected()||!characterReady||!PartnerNpc());
}
void CharacterRestoreSummon(NpcBinding& binding){
    const std::string name=binding.npc->GetObjectName().ToChar();unsigned owner=0;
    if(name.rfind("ZKSG_",0)==0)owner=co::GuestPlayer;else if(name.rfind("ZKSH_",0)==0)owner=co::HostPlayer;
    if(owner&&CharacterSummonNpc(binding.npc)&&name.size()>37&&name.size()<63){binding.desired.owner=owner;binding.desired.spawnToken=name.substr(5);binding.npc->isSummoned=TRUE;}
}
unsigned CharacterCaptureSummons(const std::set<oCNpc*>& before,unsigned owner,const std::string& token){
    unsigned count=0;for(auto* npc:CharacterNpcSet()){
        if(before.count(npc)||CharacterLocal(npc)||npc==PartnerNpc()||!CharacterSummonNpc(npc))continue;
        if(++count>16)throw std::runtime_error("Summon group limit exceeded");
        auto* binding=RegisterCombatNpc(npc);if(!binding)throw std::runtime_error("Cannot register summoned NPC");
        const auto lineage=(HostCombat()?owner==co::HostPlayer:owner==co::GuestPlayer)?recoveryTimeline:(owner==co::GuestPlayer?characterArchive.guestLineage:recoveryPeerTimeline);
        if(lineage.empty())throw std::runtime_error("Summon has no character lineage");
        binding->desired.owner=owner;binding->desired.spawnToken=CharacterLineageTag(lineage)+token+"_"+std::to_string(count);npc->isSummoned=TRUE;
        npc->SetObjectName(zSTRING(CharacterSummonName(owner,binding->desired.spawnToken).c_str()));
        if(!HostCombat()){binding->owned=binding->mirror=true;npc->dontWriteIntoArchive=TRUE;}
        if(HostCombat()&&owner==co::GuestPlayer)binding->owned=true;
        CombatLog("summon_created","owner="+std::to_string(owner)+" token="+binding->desired.spawnToken+" instance="+std::to_string(npc->GetInstance()));
    }
    return count;
}
void CharacterRemoveOldSummons(const std::string& lineage){
    if(!HostCombat()||!ogame||!ogame->spawnman)return;const auto prefix=CharacterLineageTag(lineage);
    for(auto& entry:npcBindings)if(entry->desired.owner==co::GuestPlayer&&entry->desired.spawnToken.rfind(prefix,0)!=0&&entry->life!=3){
        entry->owned=true;CombatLog("summon_owner_changed","remove="+entry->desired.spawnToken);ogame->spawnman->DeleteNpc(entry->npc);entry->life=3;
    }
}
struct CharacterSpellScope {
    bool active=false;unsigned spell=0;std::string token;std::set<oCNpc*> before;
    explicit CharacterSpellScope(oCSpell* cast){if(!cast||!CombatConnected()||cast->spellCasterNpc!=player||!ch::IsSummonSpell(cast->spellID))return;active=true;spell=cast->spellID;token=rc::NewGuid();before=CharacterNpcSet();}
    void Finish(int result){if(!active)return;try{const auto count=CharacterCaptureSummons(before,HostCombat()?co::HostPlayer:co::GuestPlayer,token);if(!count){CombatLog("summon_empty","spell="+std::to_string(spell)+" native_result="+std::to_string(result)+" no native creature created");return;}if(!HostCombat()){ch::Summon request;request.spell=spell;request.token=token;CharacterQueue(ch::Encode(request));}CombatLog("summon_cast","spell="+std::to_string(spell)+" count="+std::to_string(count)+" token="+token);}catch(const std::exception& e){combatFailure=e.what();}}
};
void CharacterReceiveSummon(const co::Bytes& bytes){
    ch::Summon request;if(!ch::Decode(bytes,request)||!HostCombat()||!characterReady||!PartnerNpc()||!havePeerCombat||peerCombat.stats.attributes[0]<=0||!haveTarget||Now()-lastSnapshotAt>1000)throw std::runtime_error("Invalid guest summon request");
    auto previous=characterSummons.find(request.token);if(previous!=characterSummons.end()){if(previous->second!=request.spell)throw std::runtime_error("Summon token payload conflict");return;}
    if(characterSummons.size()>=1024)throw std::runtime_error("Summon session safety limit");
    if(!parser||parser->GetIndex(zSTRING("ZK_COOP_REPLAY_SUMMON"))<0)throw std::runtime_error("Summon script helper missing");
    const auto before=CharacterNpcSet();characterSummons[request.token]=request.spell;
    {CharacterGuestScope guest(true);QuestBindings bindings;const auto old=QuestInt("ZK_COOP_SUMMON_SPELL");QuestSetInt("ZK_COOP_SUMMON_SPELL",static_cast<int>(request.spell));parser->SetInstance(zSTRING("SELF"),PartnerNpc());parser->SetInstance(zSTRING("HERO"),PartnerNpc());parser->CallFunc(parser->GetIndex(zSTRING("ZK_COOP_REPLAY_SUMMON")));QuestSetInt("ZK_COOP_SUMMON_SPELL",old);}
    const auto count=CharacterCaptureSummons(before,co::GuestPlayer,request.token);if(!count)throw std::runtime_error("Guest summon produced no authoritative NPC");
    CombatLog("summon_authority","owner=guest spell="+std::to_string(request.spell)+" count="+std::to_string(count)+" token="+request.token);
}
void CharacterGroundTheft(oCItem* item,const std::string& ownerTimeline){
    if(!item||!CombatConnected()||!HostCombat()||!characterReady)return;
    auto* actor=ownerTimeline==recoveryTimeline?player:PartnerNpc();if(!actor)return;
    actor->CheckForOwner(item);
    CombatLog("crime_ground_check","owner="+std::string(actor==player?"host":"guest")+" item="+std::to_string(item->GetInstance()));
}
