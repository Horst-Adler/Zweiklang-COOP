#pragma once
#include "coop/CombatProtocol.h"
#include <map>
#include <type_traits>
namespace zweiklang::worldsync {
enum class Life:std::uint32_t {Visible=1,Dormant=2,Removed=3};
inline unsigned RemovalLife(bool culling){return static_cast<unsigned>(culling?Life::Dormant:Life::Removed);}
struct Ground {std::uint32_t id=0,revision=0,instance=0,amount=0;bool present=false;std::array<float,3> anchor{},position{};std::string name;};
inline combat::Bytes Encode(const Ground& g){combat::Writer w;w.data.push_back(48);w.u(g.id);w.u(g.revision);w.u(g.instance);w.u(g.amount);w.u(g.present?1:0);for(float p:g.anchor)w.f(p);for(float p:g.position)w.f(p);w.text(g.name);return w.data;}
inline bool Decode(const combat::Bytes& b,Ground& g){if(b.empty()||b[0]!=48)return false;combat::Reader r{b};g.id=r.u();g.revision=r.u();g.instance=r.u();g.amount=r.u();auto state=r.u();g.present=state!=0;for(float& p:g.anchor){p=r.f();if(std::abs(p)>1000000)r.ok=false;}for(float& p:g.position){p=r.f();if(std::abs(p)>1000000)r.ok=false;}g.name=r.text();return r.done()&&g.id>0&&g.id<1000000&&g.revision>0&&g.instance>0&&g.instance<1000000&&g.amount<=1000000&&state<=1&&(!g.present||g.amount>0);}
inline bool Within(const std::array<float,3>& p,const std::array<float,3>& a,float range){float d=0;for(int i=0;i<3;++i){float v=p[i]-a[i];d+=v*v;}return std::isfinite(d)&&range>=0&&d<=range*range;}
inline bool InEitherBubble(const std::array<float,3>& p,const std::array<float,3>& a,const std::array<float,3>& b,float range){return Within(p,a,range)||Within(p,b,range);}
inline bool NewGeneration(std::uint32_t incoming,std::uint32_t current){return incoming>0&&(current==0||combat::Newer(incoming,current));}
inline bool AcceptNpc(unsigned generation,unsigned revision,unsigned life,unsigned nextGeneration,unsigned nextRevision,unsigned nextLife){if(!nextGeneration||nextLife<1||nextLife>3)return false;if(!generation||nextGeneration>generation)return true;if(nextGeneration!=generation||life==3&&nextLife!=3)return false;return combat::Newer(nextRevision,revision);}
template<class Manager,class Position,class Pass> unsigned GuestSpawnPass(Manager& manager,const Position& guest,Pass pass){
    using NodePtr=std::remove_reference_t<decltype(manager.spawnList[0])>;
    using NpcPtr=decltype(manager.spawnList[0]->npc);
    if(manager.spawnList.GetNumInList()>8192)return 0;
    std::map<NodePtr,std::pair<NpcPtr,float>> timers;
    for(int i=0;i<manager.spawnList.GetNumInList();++i){auto* n=manager.spawnList[i];if(n)timers[n]={n->npc,n->timer};}
    const auto old=manager.camPos;const auto oldInsert=manager.insertTime;manager.camPos=guest;pass();manager.camPos=old;manager.insertTime=oldInsert;unsigned restored=0;
    for(int i=0;i<manager.spawnList.GetNumInList();++i){auto* n=manager.spawnList[i];auto t=timers.find(n);if(n&&t!=timers.end()&&n->npc==t->second.first){n->timer=t->second.second;++restored;}}
    return restored;
}
inline bool Price(unsigned value,unsigned count,bool sale,float multiplier,unsigned& total){
    if(!count||count>1000000||value>1000000||!std::isfinite(multiplier)||multiplier<0||multiplier>100)return false;
    const double scaled=sale?std::nearbyint(static_cast<float>(value)*multiplier):value;
    const auto unit=static_cast<std::uint64_t>(value?((std::max)(1.0,scaled)):0);
    const auto result=unit*count;if(result>1000000)return false;total=static_cast<unsigned>(result);return true;
}
}
