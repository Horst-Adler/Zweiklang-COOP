int __fastcall CharacterGetAttitude(oCNpc*,void*,oCNpc*);
CInvoke<int(__thiscall*)(oCNpc*,oCNpc*)> originalCharacterAttitude(0x0072FAB0,&CharacterGetAttitude);
int __fastcall CharacterGetAttitude(oCNpc* npc,void*,oCNpc* other){using namespace zweiklang::engine;if(CharacterAllied(npc,other))return NPC_ATT_FRIENDLY;int value=0;if(CharacterAttitude(npc,other,false,value))return value;return originalCharacterAttitude(npc,other);}
int __fastcall CharacterGetPermanent(oCNpc*,void*,oCNpc*);
CInvoke<int(__thiscall*)(oCNpc*,oCNpc*)> originalCharacterPermanent(0x0072FB30,&CharacterGetPermanent);
int __fastcall CharacterGetPermanent(oCNpc* npc,void*,oCNpc* other){using namespace zweiklang::engine;if(CharacterAllied(npc,other))return NPC_ATT_FRIENDLY;int value=0;if(CharacterAttitude(npc,other,true,value))return value;return originalCharacterPermanent(npc,other);}
void __fastcall CharacterSetPermanent(oCNpc*,void*,int);
CInvoke<void(__thiscall*)(oCNpc*,int)> originalCharacterSetPermanent(0x0072FB80,&CharacterSetPermanent);
void __fastcall CharacterSetPermanent(oCNpc* npc,void*,int value){if(!zweiklang::engine::CharacterSetAttitude(npc,value,true))originalCharacterSetPermanent(npc,value);}
void __fastcall CharacterSetTemporary(oCNpc*,void*,int);
CInvoke<void(__thiscall*)(oCNpc*,int)> originalCharacterSetTemporary(0x0072FBD0,&CharacterSetTemporary);
void __fastcall CharacterSetTemporary(oCNpc* npc,void*,int value){if(!zweiklang::engine::CharacterSetAttitude(npc,value,false))originalCharacterSetTemporary(npc,value);}
int __fastcall CharacterIsPlayer(oCNpc*,void*);
CInvoke<int(__thiscall*)(oCNpc*)> originalCharacterIsPlayer(0x007425A0,&CharacterIsPlayer);
int __fastcall CharacterIsPlayer(oCNpc* npc,void*){using namespace zweiklang::engine;if(characterGuestDepth&&npc==PartnerNpc())return TRUE;return originalCharacterIsPlayer(npc);}
int __fastcall CharacterIsSelfPlayer(oCNpc*,void*);
CInvoke<int(__thiscall*)(oCNpc*)> originalCharacterIsSelfPlayer(0x007425B0,&CharacterIsSelfPlayer);
int __fastcall CharacterIsSelfPlayer(oCNpc* npc,void*){using namespace zweiklang::engine;if(characterGuestDepth&&npc==PartnerNpc())return TRUE;return originalCharacterIsSelfPlayer(npc);}
void __fastcall CharacterPassive(oCNpc*,void*,int,zCVob*,zCVob*);
CInvoke<void(__thiscall*)(oCNpc*,int,zCVob*,zCVob*)> originalCharacterPassive(0x0075B270,&CharacterPassive);
void __fastcall CharacterPassive(oCNpc* emitter,void*,int perception,zCVob* other,zCVob* victim){
    using namespace zweiklang::engine;
    if(!CombatConnected()){originalCharacterPassive(emitter,perception,other,victim);return;}
    const bool personal=perception==NPC_PERC_ASSESSTHEFT||perception==NPC_PERC_ASSESSMURDER||perception==NPC_PERC_ASSESSDEFEAT||perception==NPC_PERC_ASSESSDAMAGE||perception==NPC_PERC_ASSESSOTHERSDAMAGE||perception==NPC_PERC_ASSESSTHREAT;
    if(!HostCombat()&&personal&&!characterReplaying){
        if(other==player||(other==nullptr&&emitter==player&&perception==NPC_PERC_ASSESSTHEFT)){
            if(!characterReady)return;
            zweiklang::character::Passive event;event.perception=perception;
            if(victim){auto* target=zDYNAMIC_CAST<oCNpc>(victim);auto found=npcByPointer.find(target);if(found==npcByPointer.end())return;event.target=found->second->id;}
            try{CharacterQueue(zweiklang::character::Encode(event));}catch(const std::exception& e){combatFailure=e.what();}
        }
        return;
    }
    const bool guest=other==PartnerNpc()||(other==nullptr&&emitter==PartnerNpc());
    CharacterGuestScope context(guest,nullptr,personal);
    if(context.failed)return;
    originalCharacterPassive(emitter,perception,other,victim);
    if(personal)CombatLog("crime_perception","owner="+std::string(guest?"guest":"host/world")+" type="+std::to_string(perception));
}
int __fastcall CharacterCast(oCSpell*,void*);
CInvoke<int(__thiscall*)(oCSpell*)> originalCharacterCast(0x00485360,&CharacterCast);
int __fastcall CharacterCast(oCSpell* spell,void*){
    using namespace zweiklang::engine;
    if(CombatConnected()&&spell&&spell->spellCasterNpc==player&&zweiklang::character::IsSummonSpell(spell->spellID)&&!CharacterReady()){notice="Beschwoerung wartet auf den persoenlichen Abgleich.";return FALSE;}
    CharacterSpellScope context(spell);const auto result=originalCharacterCast(spell);context.Finish(result);return result;
}
int __fastcall CharacterTransform(oCSpell*,void*);
CInvoke<int(__thiscall*)(oCSpell*)> originalCharacterTransform(0x00486960,&CharacterTransform);
int __fastcall CharacterTransform(oCSpell* spell,void*){
    using namespace zweiklang::engine;
    struct SwitchScope {bool previous=characterChangingForm;bool active=false;explicit SwitchScope(oCSpell* s){active=s&&s->spellCasterNpc==player&&s->spellID>=47&&s->spellID<=58;if(active)characterChangingForm=true;}~SwitchScope(){characterChangingForm=previous;if(active&&player&&ogame&&!loading)CharacterObserve(Now());}} scope(spell);
    return originalCharacterTransform(spell);
}
void __fastcall CharacterEndTransform(oCSpell*,void*);
CInvoke<void(__thiscall*)(oCSpell*)> originalCharacterEndTransform(0x00486E10,&CharacterEndTransform);
void __fastcall CharacterEndTransform(oCSpell* spell,void*){
    using namespace zweiklang::engine;
    struct EndScope {bool previous=characterChangingForm;bool active=false;explicit EndScope(oCSpell* s){active=s&&s->spellID>=47&&s->spellID<=58&&(s->spellCasterNpc==player||CharacterLocal(s->saveNpc));if(active){characterChangingForm=true;CharacterHoldForm(s->saveNpc);}}~EndScope(){characterChangingForm=previous;if(active&&player&&ogame&&!loading)CharacterObserve(Now());}} scope(spell);
    originalCharacterEndTransform(spell);
}
