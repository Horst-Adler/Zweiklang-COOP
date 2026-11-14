#pragma once
#include "coop/Recovery.h"
namespace orebound::cavalorn {
constexpr std::uint64_t SupportedScriptFingerprint=0x75fb8f2f6ce47a3cull;
struct Rewards {unsigned xp=100,goldItem=0,knifeItem=0,potionItem=0;};
inline bool EligibleForCredit(int hp,bool unconscious){return hp>0&&!unconscious;}
inline bool Allowed(const recovery::Tx& t,const Rewards& r){
    if(!recovery::Valid(t)||t.source.kind!=loot::Source::Quest||t.world!="NEWWORLD.ZEN"||t.decision!=recovery::Decision::Commit)return false;
    switch(t.instance){
    case 1:return t.action==recovery::Action::QuestXp&&t.item==0&&t.amount==r.xp;
    case 2:return t.action==recovery::Action::QuestItem&&t.item==r.goldItem&&t.amount==50;
    case 3:return t.action==recovery::Action::QuestItem&&t.item==r.knifeItem&&t.amount==1;
    case 4:return t.action==recovery::Action::QuestItem&&t.item==r.potionItem&&t.amount==2;
    default:return false;}
}
inline bool SameEvent(const recovery::Tx& a,const recovery::Tx& b){return a.source.kind==loot::Source::Quest&&b.source.kind==loot::Source::Quest&&a.source.name==b.source.name&&a.hostTimeline==b.hostTimeline&&a.ownerTimeline==b.ownerTimeline&&a.instance==b.instance;}
inline bool NativeSaveRepresents(unsigned event,unsigned mission,bool suppliesTold){return event<=2?mission==2:suppliesTold;}
template<class Info> void WaitCondition(std::map<Info*,int>& originals,Info* info,int wait){if(!info)return;originals.emplace(info,info->pd.conditions);info->pd.conditions=wait;}
template<class Info> void RestoreConditions(std::map<Info*,int>& originals){for(const auto& pair:originals)pair.first->pd.conditions=pair.second;originals.clear();}
}
