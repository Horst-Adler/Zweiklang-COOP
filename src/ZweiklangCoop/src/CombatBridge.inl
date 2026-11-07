// Included inside the adapter's private engine namespace. All engine access is
// on the engine thread. No borrowed game pointers are serialized.
namespace co=combat;
struct NpcBinding {
    oCNpc* npc=nullptr;std::uint32_t id=0,revision=0;zVEC3 anchor,rootScale;
    bool mirror=false,originalRotation=false,modelDefaultsCaptured=false,owned=false,hidden=false,pausedByPeer=false;std::uint32_t generation=1,life=1,probeCookie=0;std::string visualKey;co::Npc desired;unsigned long long last=0;int loggedHp=-1;
};
std::vector<std::unique_ptr<NpcBinding>> npcBindings;
std::map<oCNpc*,NpcBinding*> npcByPointer;
std::map<std::uint32_t,NpcBinding*> npcById;
std::map<std::uint32_t,unsigned long long> mappingWarnings;
co::Peer peerCombat;
bool combatSession=false,havePeerCombat=false,rulesReady=false,sessionFriendlyFire=false,replayingCombat=false;
std::uint32_t combatRevision=0,ruleRevision=0,nextNpcId=1;
unsigned long long lastCombatSend=0,lastNpcScan=0,lastAggro=0;
unsigned long long lastAggroLog=0;
std::size_t npcCursor=0;
std::string combatFailure;
struct DeferredHit {co::Damage damage;unsigned long long at;};
std::deque<DeferredHit> deferredHits;

oCNpc* PartnerNpc(){return remote?zDYNAMIC_CAST<oCNpc>(remote):nullptr;}
bool CombatConnected(){return combatSession&&network.GetState()==net::State::Connected&&!loading&&!saving;}
bool HostCombat(){return network.GetRole()==net::Role::Host;}
float Distance2(const zVEC3& a,const zVEC3& b){const auto d=a-b;return d[0]*d[0]+d[1]*d[1]+d[2]*d[2];}
void CombatLog(const char* event,const std::string& detail){diagnostics.Write(1,event,detail);}
void CombatReset(){
    LootReset();WorldReset();SessionReset();
    auto* partner=PartnerNpc();
    if(partner&&player){if(player->enemy==partner)player->SetEnemy(nullptr);if(player->GetFocusVob()==partner)player->SetFocusVob(nullptr);}
    for(auto& entry:npcBindings){
        auto* n=entry->npc;
        if(partner&&(n->enemy==partner||n->state.parOther==partner||n->state.parVictim==partner)){n->SetEnemy(nullptr);n->state.ClearAIState();}
        // Guest conversations also pause host NPC root motion. Restore it on
        // disconnect even though those NPCs are authoritative, not mirrors.
        if(entry->modelDefaultsCaptured&&(entry->mirror||entry->pausedByPeer)){auto* model=zDYNAMIC_CAST<zCModel>(n->GetVisual());if(model){model->aniTransScale=entry->rootScale;if(entry->mirror)model->doVobRot=entry->originalRotation;}}
        entry->pausedByPeer=false;
        if(entry->owned&&n->homeWorld)n->RemoveVobFromWorld();
        n->Release();
    }
    npcBindings.clear();npcByPointer.clear();npcById.clear();mappingWarnings.clear();deferredHits.clear();
    combatSession=false;havePeerCombat=false;rulesReady=false;ruleRevision=0;combatRevision=0;nextNpcId=1;npcCursor=0;
    lastCombatSend=lastNpcScan=lastAggro=lastAggroLog=0;combatFailure.clear();peerCombat=co::Peer{};
}
NpcBinding* RegisterCombatNpc(oCNpc* npc){
    if(!npc||npc==player||npc==PartnerNpc()||npc->GetInstance()<0)return nullptr;
    const auto found=npcByPointer.find(npc);if(found!=npcByPointer.end())return found->second;
    if(npcBindings.size()>=8192){combatFailure="NPC registry limit reached";return nullptr;}
    auto entry=std::make_unique<NpcBinding>();entry->npc=npc;npc->AddRef();entry->anchor=npc->GetPositionWorld();
    auto* model=zDYNAMIC_CAST<zCModel>(npc->GetVisual());
    entry->rootScale=model?model->aniTransScale:zVEC3(1,1,1);
    entry->originalRotation=model?model->doVobRot:false;entry->life=npc->homeWorld?1:2;
    entry->modelDefaultsCaptured=model!=nullptr;
    if(HostCombat()){entry->id=nextNpcId++;npcById[entry->id]=entry.get();}
    auto* value=entry.get();npcByPointer[npc]=value;npcBindings.push_back(std::move(entry));return value;
}
void ScanCombatNpcs(){
    auto* world=ogame?ogame->GetGameWorld():nullptr;if(!world||!world->voblist_npcs)return;
    for(auto* it=world->voblist_npcs->GetNextInList();it;it=it->GetNextInList())RegisterCombatNpc(it->GetData());
    if(ogame->spawnman)for(int i=0;i<ogame->spawnman->spawnList.GetNumInList();++i){auto* node=ogame->spawnman->spawnList[i];if(node){auto* e=RegisterCombatNpc(node->npc);if(e&&!node->npc->homeWorld)e->anchor=node->spawnPos;}}
}
co::Stats ReadCombatStats(oCNpc* npc){
    co::Stats s;
    for(int i=0;i<8;++i){s.attributes[i]=(std::max)(0,(std::min)(1000000,npc->attribute[i]));s.protection[i]=(std::max)(-1,(std::min)(1000000,npc->protection[i]));s.damage[i]=(std::max)(0,(std::min)(1000000,npc->damage[i]));}
    s.attributes[1]=(std::max)(1,s.attributes[1]);s.attributes[0]=(std::min)(s.attributes[0],s.attributes[1]);
    for(int i=0;i<5;++i)s.hit[i]=(std::max)(0,(std::min)(1000000,npc->hitChance[i]));
    s.weapon=npc->GetWeaponMode();s.guild=npc->guild;s.damageType=npc->damagetype;s.bodyState=npc->GetBodyState()&0x7ffff;
    return s;
}
void WriteCombatStats(oCNpc* npc,const co::Stats& s){
    // Only called for the proxy or bound world NPCs, NEVER for the local player.
    if(!npc||npc==player)return;
    for(int i=0;i<8;++i){npc->attribute[i]=s.attributes[i];npc->protection[i]=s.protection[i];npc->damage[i]=s.damage[i];}
    for(int i=0;i<5;++i)npc->hitChance[i]=s.hit[i];
    npc->guild=s.guild;npc->guildTrue=s.guild;npc->damagetype=s.damageType;npc->fmode=s.weapon;
    if(!WorldLocalTalksTo(npc)&&static_cast<std::uint32_t>(npc->GetBodyState())!=s.bodyState)npc->SetBodyState(s.bodyState);
}
std::string CombatAnimation(zCModel* model){
    if(!model)return{};
    return CombatAnimationName(model->aniChannels,model->numActiveAnis,[](zCModelAniActive* active)->std::string {
        return active->protoAni?active->protoAni->GetAniName().ToChar():"";
    });
}
bool SetCombatAnimation(zCModel* model,const std::string& name){
    if(!model||name.empty())return false;
    const int id=model->GetAniIDFromAniName(zSTRING(name.c_str()));if(id<0)return false;
    if(!model->GetActiveAni(id)){model->StopAnisLayerRange(0,2);model->StartAni(id,FALSE);}
    return true;
}
bool CombatMirror(oCNpc* npc){
    if(!npc||npc==player||!CombatConnected())return false;
    if(npc==PartnerNpc())return true;
    if(HostCombat())return WorldTradePaused(npc);
    return rulesReady&&!WorldLocalTalksTo(npc);
}
oCNpc* CreateCombatActor(){
    if(!havePeerCombat||!parser||!ogame)return nullptr;
    const int instance=parser->GetIndex(zSTRING("ZK_COOP_PARTNER"));
    if(instance<0){combatFailure="ZweiklangCoop_Npc.d missing/not parsed; partner instance unavailable";return nullptr;}
    auto* npc=zDYNAMIC_CAST<oCNpc>(ogame->GetGameWorld()->CreateVob(zVOB_TYPE_NSC,instance));
    if(npc){WriteCombatStats(npc,peerCombat.stats);npc->noFocus=0;npc->ClearPerception();}
    return npc;
}
void UpdateCombatActor(){
    auto* npc=PartnerNpc();if(!npc||!havePeerCombat)return;
    WriteCombatStats(npc,peerCombat.stats);QolApplySprint(npc,peerCombat.sprint);npc->name[0]=zSTRING(ConvertText(network.PeerName(),CP_UTF8,CP_ACP).c_str());
    // noFocus is global and also hides the partner from enemy AI. Filter only
    // the local player's focus through the hooks, never enemy perception.
    npc->noFocus=0;
    if(!sessionFriendlyFire&&player){if(co::BlockPartnerFocus(sessionFriendlyFire,player->GetWeaponMode())&&player->GetFocusVob()==npc)player->SetFocusVob(nullptr);if(player->enemy==npc)player->SetEnemy(nullptr);}
}
NpcBinding* ResolveCombatNpc(const co::Npc& message){
    auto existing=npcById.find(message.id);if(existing!=npcById.end())return existing->second;
    const zVEC3 anchor(message.anchor[0],message.anchor[1],message.anchor[2]);
    NpcBinding* best=nullptr;float closest=4000000.0f,second=4000000.0f;
    NpcBinding* unique=nullptr;int matches=0;
    for(auto& entry:npcBindings){if(entry->mirror||entry->npc->GetInstance()!=message.instance)continue;++matches;unique=entry.get();const float d=Distance2(entry->anchor,anchor);if(d<closest){second=closest;closest=d;best=entry.get();}else if(d<second)second=d;}
    if(matches==1&&unique->npc->IsHuman()){best=unique;second=4000000.0f;closest=0;}
    if(best&&std::sqrt(second)-std::sqrt(closest)<100.0f){
        if(Now()-mappingWarnings[message.id]>5000){mappingWarnings[message.id]=Now();CombatLog("npc_mapping_unresolved","ambiguous id="+std::to_string(message.id));}return nullptr;
    }
    if(!best){
        if(message.life!=1)return nullptr;
        best=WorldCreateNpc(message);if(!best)return nullptr;
    }
    best->id=message.id;best->mirror=true;npcById[message.id]=best;
    CombatLog("npc_bind","id="+std::to_string(message.id)+" instance="+std::to_string(message.instance)+" distance="+std::to_string(std::sqrt(closest)));return best;
}
co::Damage CaptureCombatDamage(co::Kind kind,std::uint32_t targetId,std::uint32_t attackerId,const oCNpc::oSDamageDescriptor& source){
    co::Damage d;d.kind=kind;d.target=targetId;d.attacker=attackerId;d.mode=source.enuModeDamage;d.weapon=source.enuModeWeapon;
    d.spell=(source.dwFieldsValid&oCNpc::oEDamageDescFlag_SpellID)&&source.nSpellID!=UINT32_MAX?source.nSpellID:0;
    d.spellCategory=d.spell&&source.nSpellCat!=UINT32_MAX?source.nSpellCat:0;
    d.spellLevel=(source.dwFieldsValid&oCNpc::oEDamageDescFlag_SpellLevel)&&source.nSpellLevel!=UINT32_MAX?source.nSpellLevel:0;
    if(!(source.dwFieldsValid&oCNpc::oEDamageDescFlag_WeaponType)||d.weapon==UINT32_MAX)d.weapon=0;
    d.total=source.fDamageTotal;d.multiplier=source.fDamageMultiplier;d.dontKill=source.bDamageDontKill!=0;
    for(int i=0;i<8;++i)d.values[i]=source.aryDamage[i];return d;
}
bool QueueCombatHit(const co::Damage& hit){
    if(!co::Valid(hit)){CombatLog("combat_reject","invalid damage descriptor mode="+std::to_string(hit.mode)+" weapon="+std::to_string(hit.weapon)+" total="+std::to_string(hit.total)+" multiplier="+std::to_string(hit.multiplier));return false;}
    if(!network.SendEvent(co::Encode(hit),Now())){combatFailure="Combat event queue unavailable/full; session stopped to avoid unsynchronized damage";return false;}
    CombatLog("combat_send","kind="+std::to_string(static_cast<int>(hit.kind))+" target="+std::to_string(hit.target)+" attacker="+std::to_string(hit.attacker)+" raw="+std::to_string(hit.total));return true;
}
bool InterceptCombatDamage(oCNpc* victim,oCNpc::oSDamageDescriptor& damage){
    if(!CombatConnected()||replayingCombat||!victim)return false;
    auto* partner=PartnerNpc();
    auto* attacker=(damage.dwFieldsValid&oCNpc::oEDamageDescFlag_Npc)?damage.pNpcAttacker:nullptr;
    if(!attacker&&(damage.dwFieldsValid&oCNpc::oEDamageDescFlag_Attacker)&&damage.pVobAttacker)attacker=zDYNAMIC_CAST<oCNpc>(damage.pVobAttacker);
    if(!sessionFriendlyFire&&((victim==partner&&attacker==player)||(victim==player&&attacker==partner))){CombatLog("friendly_fire_block","partner damage suppressed");return true;}
    if(victim==partner){
        if(attacker==player){
            if(sessionFriendlyFire)QueueCombatHit(CaptureCombatDamage(HostCombat()?co::Kind::Injury:co::Kind::Attack,HostCombat()?co::GuestPlayer:0,HostCombat()?co::HostPlayer:co::GuestPlayer,damage));
            return true;
        }
        if(HostCombat()&&attacker){
            if(attacker==player){if(sessionFriendlyFire)QueueCombatHit(CaptureCombatDamage(co::Kind::Injury,co::GuestPlayer,co::HostPlayer,damage));}
            else if(auto* entry=RegisterCombatNpc(attacker))QueueCombatHit(CaptureCombatDamage(co::Kind::Injury,co::GuestPlayer,entry->id,damage));
        }
        return true;
    }
    if(victim==player){
        // Own environment damage remains local. Host-generated NPC hits arrive
        // through the reliable injury channel and execute with replayingCombat.
        if(!HostCombat()&&attacker&&attacker!=player&&attacker!=partner){CombatLog("combat_client_ai_block","unconfirmed local NPC hit on player");return true;}
        return false;
    }
    if(!HostCombat()){
        if(attacker==player){
            auto found=npcByPointer.find(victim);
            if(found!=npcByPointer.end()&&found->second->mirror)QueueCombatHit(CaptureCombatDamage(co::Kind::Attack,found->second->id,co::GuestPlayer,damage));
            else CombatLog("combat_unmapped_hit","NPC not yet uniquely bound; attack not applied");
        }
        return true;
    }
    return false;
}
void ApplyCombatDamage(oCNpc* victim,oCNpc* attacker,const co::Damage& source){
    if(!victim||!attacker)return;
    struct ActorRefs {oCNpc* a; oCNpc* v;ActorRefs(oCNpc* attacker,oCNpc* victim):a(attacker),v(victim){a->AddRef();v->AddRef();}~ActorRefs(){a->Release();v->Release();}} refs(attacker,victim);
    oCNpc::oSDamageDescriptor d;
    d.dwFieldsValid=oCNpc::oEDamageDescFlag_Attacker|oCNpc::oEDamageDescFlag_Npc|oCNpc::oEDamageDescFlag_DamageType|oCNpc::oEDamageDescFlag_WeaponType|oCNpc::oEDamageDescFlag_Damage|oCNpc::oEDamageDescFlag_HitLocation|oCNpc::oEDamageDescFlag_SpellID|oCNpc::oEDamageDescFlag_SpellLevel;
    d.pVobAttacker=attacker;d.pNpcAttacker=attacker;d.pVobHit=victim;d.pFXHit=nullptr;d.pItemWeapon=nullptr;
    d.nSpellID=source.spell;d.nSpellCat=source.spellCategory;d.nSpellLevel=source.spellLevel;d.enuModeDamage=source.mode;d.enuModeWeapon=source.weapon;
    for(int i=0;i<8;++i){d.aryDamage[i]=source.values[i];d.aryDamageEffective[i]=0;}
    d.fDamageTotal=source.total;d.fDamageMultiplier=source.multiplier;d.vecLocationHit=victim->GetPositionWorld();d.vecDirectionFly=zVEC3(0,0,0);d.strVisualFX=zSTRING("");
    d.fTimeDuration=d.fTimeInterval=d.fDamagePerInterval=0;d.bDamageDontKill=source.dontKill;d.bOnce=TRUE;d.bFinished=FALSE;d.bIsDead=FALSE;d.bIsUnconscious=FALSE;d.lReserved=0;
    d.fAzimuth=d.fElevation=d.fTimeCurrent=d.fDamageReal=d.fDamageEffective=0;d.pVobParticleFX=nullptr;d.pParticleFX=nullptr;d.pVisualFX=nullptr;
    const int before=victim->GetAttribute(NPC_ATR_HITPOINTS);
    struct ReplayGuard {bool before;ReplayGuard():before(replayingCombat){replayingCombat=true;}~ReplayGuard(){replayingCombat=before;}} guard;
    victim->OnDamage(d);
    CombatLog("combat_apply","victim="+std::string(victim->GetObjectName().ToChar())+" hp_before="+std::to_string(before)+" hp_after="+std::to_string(victim->GetAttribute(NPC_ATR_HITPOINTS))+" raw="+std::to_string(source.total));
}
bool ExecuteCombatHit(const co::Damage& hit){
    if(!PartnerNpc()||!havePeerCombat||!rulesReady)return false;
    if(HostCombat()){
        if(hit.kind!=co::Kind::Attack||hit.attacker!=co::GuestPlayer){CombatLog("combat_reject","wrong event direction/owner");return true;}
        if(peerCombat.stats.attributes[0]<=0)return true;
        oCNpc* victim=nullptr;
        if(hit.target==0){if(sessionFriendlyFire)victim=player;}
        else {auto entry=npcById.find(hit.target);if(entry!=npcById.end())victim=entry->second->npc;}
        if(!victim){CombatLog("combat_reject","unknown target or friendly fire disabled");return true;}
        const float range=peerCombat.stats.weapon>=5?5000.0f:700.0f;
        if(!haveTarget||Now()-lastSnapshotAt>1000)return false;
        const zVEC3 ownerPosition(target.x,target.y,target.z);
        const float distance=std::sqrt(Distance2(victim->GetPositionWorld(),ownerPosition));
        if(victim->homeWorld!=ogame->GetWorld()||distance>range){CombatLog("combat_reject","target outside validated attack range distance="+std::to_string(distance)+" range="+std::to_string(range)+" snapshot_age_ms="+std::to_string(Now()-lastSnapshotAt));return true;}
        if(victim->GetAttribute(NPC_ATR_HITPOINTS)<=0)return true;
        ApplyCombatDamage(victim,PartnerNpc(),hit);return true;
    }
    if(hit.kind!=co::Kind::Injury||hit.target!=co::GuestPlayer){CombatLog("combat_reject","wrong injury direction/owner");return true;}
    if(hit.attacker==co::HostPlayer){if(sessionFriendlyFire)ApplyCombatDamage(player,PartnerNpc(),hit);return true;}
    const auto entry=npcById.find(hit.attacker);if(entry==npcById.end())return false;
    ApplyCombatDamage(player,entry->second->npc,hit);return true;
}
void CombatFrame(unsigned long long now,float elapsed){
    if(network.GetState()!=net::State::Connected){if(combatSession)CombatReset();return;}
    if(!combatSession){combatSession=true;rulesReady=HostCombat();sessionFriendlyFire=options.friendlyFire;ScanCombatNpcs();CombatLog("combat_start","world="+CurrentWorld()+" authority="+(HostCombat()?"HOST":"CLIENT")+" friendly_fire="+std::to_string(sessionFriendlyFire));}
    if(now-lastNpcScan>=1000){lastNpcScan=now;ScanCombatNpcs();}
    co::Bytes bytes;
    for(int i=0;i<256&&network.ConsumeWorldState(bytes);++i){
        if(bytes.empty())continue;
        if(bytes[0]==static_cast<unsigned>(co::Kind::Peer)){
            co::Peer p;if(co::Decode(bytes,p)&&(!havePeerCombat||co::Newer(p.revision,peerCombat.revision))){peerCombat=p;havePeerCombat=true;}
        }else if(bytes[0]==static_cast<unsigned>(co::Kind::Rules)&&!HostCombat()){
            bool value;std::uint32_t revision;if(co::Rules(bytes,revision,value)&&(!rulesReady||co::Newer(revision,ruleRevision))){rulesReady=true;ruleRevision=revision;sessionFriendlyFire=value;}
        }else if(bytes[0]==static_cast<unsigned>(co::Kind::Npc)&&!HostCombat()){
            co::Npc p;if(!co::Decode(bytes,p)){CombatLog("combat_reject","invalid NPC state");continue;}
            auto* entry=ResolveCombatNpc(p);if(entry&&(worldsync::AcceptNpc(entry->last?entry->generation:0,entry->revision,entry->life,p.generation,p.revision,p.life))){entry->revision=p.revision;entry->generation=p.generation;entry->life=p.life;entry->desired=p;entry->last=now;WorldApplyNpc(*entry,p);WriteCombatStats(entry->npc,p.stats);if(entry->loggedHp!=p.stats.attributes[0]){entry->loggedHp=p.stats.attributes[0];CombatLog("npc_health","source=host id="+std::to_string(p.id)+" hp="+std::to_string(p.stats.attributes[0])+" max="+std::to_string(p.stats.attributes[1]));}}
        }
    }
    UpdateCombatActor();
    for(int i=0;i<64&&network.ConsumeEvent(bytes);++i){if(!bytes.empty()&&bytes[0]>=60&&bytes[0]<=62){SessionReceive(bytes);continue;}if(!bytes.empty()&&(bytes[0]>=48&&bytes[0]<=51)){WorldReceive(bytes);continue;}if(recovery::IsMessage(bytes)){RecoveryReceive(bytes);continue;}if(loot::IsMessage(bytes)){LootReceive(bytes);continue;}co::Damage hit;if(co::Decode(bytes,hit)){if(deferredHits.size()>=64){combatFailure="Combat application queue full";break;}deferredHits.push_back({hit,now});}else CombatLog("combat_reject","malformed reliable payload");}
    while(!deferredHits.empty()){
        auto& hit=deferredHits.front();if(!ExecuteCombatHit(hit.damage)){if(now-hit.at>3000)combatFailure="Combat target not mapped before event timeout";break;}deferredHits.pop_front();
    }
    if(!HostCombat())for(auto& entry:npcBindings){
        auto* npc=entry->npc;if(entry->life!=1||!entry->mirror||!entry->last||npc->homeWorld!=ogame->GetWorld()||!npc->GetModel()||WorldLocalTalksTo(npc))continue;
        npc->GetModel()->aniTransScale=zVEC3(0,0,0);
        const auto& p=entry->desired.pose;const auto old=npc->GetPositionWorld();
        const float blend=Distance2(old,zVEC3(p.x,p.y,p.z))>4000000?1.0f:1.0f-std::exp(-elapsed*15.0f);
        PlaceNetworkActor(*npc,zVEC3(old[0]+(p.x-old[0])*blend,old[1]+(p.y-old[1])*blend,old[2]+(p.z-old[2])*blend),p.yaw);
        npc->GetModel()->doVobRot=FALSE;npc->SetSleepingMode(zVOB_AWAKE);SetCombatAnimation(npc->GetModel(),entry->desired.animation);
    }
    if(now-lastCombatSend>=100){
        lastCombatSend=now;co::Peer p;p.revision=++combatRevision;p.interaction=WorldLocalInteraction();p.stats=ReadCombatStats(player);p.sprint=QolSprint(player);p.animation=CombatAnimation(player->GetModel());network.SendWorldState(co::Encode(p));
        if(HostCombat()){
            network.SendWorldState(co::Rules(++combatRevision,options.friendlyFire));sessionFriendlyFire=options.friendlyFire;
            int sent=0;std::size_t visited=0;
            while(!npcBindings.empty()&&visited++<npcBindings.size()&&sent<32){
                auto& e=*npcBindings[(npcCursor++)%npcBindings.size()];auto* n=e.npc;
                if(e.life==3)continue;if(n->homeWorld!=ogame->GetWorld()||!n->GetModel())continue;const auto pos=n->GetPositionWorld();
                const bool nearbyPlayers=haveTarget&&Distance2(pos,zVEC3(target.x,target.y,target.z))<64000000.0f;if(!nearbyPlayers)continue;
                co::Npc s;s.id=e.id;s.revision=++combatRevision;s.generation=e.generation;s.life=1;WorldNpcVisual(s,n);s.instance=n->GetInstance();s.anchor={e.anchor[0],e.anchor[1],e.anchor[2]};const auto direction=n->GetAtVectorWorld();s.pose={pos[0],pos[1],pos[2],std::atan2(direction[0],direction[2])*57.295779513f};s.stats=ReadCombatStats(n);s.animation=CombatAnimation(n->GetModel());
                if(!co::Valid(s.stats)){CombatLog("combat_reject","invalid native NPC stats id="+std::to_string(e.id));continue;}
                if(e.loggedHp!=s.stats.attributes[0]){e.loggedHp=s.stats.attributes[0];CombatLog("npc_health","authority=host id="+std::to_string(e.id)+" hp="+std::to_string(s.stats.attributes[0])+" max="+std::to_string(s.stats.attributes[1]));}
                network.SendWorldState(co::Encode(s));++sent;
            }
        }
    }
    // Retaliation is decided by host AI; give it a real, separate NPC target.
    if(HostCombat()&&PartnerNpc()&&havePeerCombat&&peerCombat.stats.attributes[0]>0&&now-lastAggro>=1000){
        int nearby=0,hostile=0,occluded=0,selected=0;
        lastAggro=now;for(auto& e:npcBindings){auto* n=e->npc;if(e->life==3||n->homeWorld!=ogame->GetWorld()||n->IsDead()||n->IsUnconscious())continue;
            const float toGuest=Distance2(n->GetPositionWorld(),remote->GetPositionWorld());
            if(toGuest>=2250000.0f)continue;++nearby;
            if(n->enemy==PartnerNpc()){++selected;continue;}
            if(n->GetAttitude(PartnerNpc())>NPC_ATT_ANGRY)continue;++hostile;
            if(!n->CanSee(PartnerNpc(),FALSE)){++occluded;continue;}
            if(toGuest<Distance2(n->GetPositionWorld(),player->GetPositionWorld())){
                n->SetEnemy(PartnerNpc());const int assessed=n->AssessEnemy_S(PartnerNpc());
                CombatLog("combat_aggro","id="+std::to_string(e->id)+" distance="+std::to_string(std::sqrt(toGuest))+" assessed="+std::to_string(assessed)+" selected="+std::to_string(n->enemy==PartnerNpc())+" state="+std::to_string(n->state.GetState()));
            }
        }
        if(now-lastAggroLog>=5000){lastAggroLog=now;CombatLog("combat_aggro_scan","nearby="+std::to_string(nearby)+" hostile="+std::to_string(hostile)+" occluded="+std::to_string(occluded)+" already_targeting_guest="+std::to_string(selected)+" partner_noFocus="+std::to_string(PartnerNpc()->noFocus));}
    }
}
