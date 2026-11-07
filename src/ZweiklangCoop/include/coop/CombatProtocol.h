#pragma once
#include "coop/Protocol.h"
#include <array>
#include <vector>
#include <cstring>
#include <cmath>
#include <algorithm>
namespace zweiklang::combat {
using Bytes=std::vector<std::uint8_t>;
// Social focus is available with weapons sheathed; damage/enemy rules stay separate.
inline bool BlockPartnerFocus(bool friendlyFire,int weaponMode){return !friendlyFire&&weaponMode!=0;}
enum class Kind:std::uint8_t { Peer=1,Npc=2,Rules=3,Attack=4,Injury=5 };
constexpr std::uint32_t HostPlayer=0xfffffffeu,GuestPlayer=0xffffffffu;
inline bool Newer(std::uint32_t a,std::uint32_t b){return a!=b&&static_cast<std::int32_t>(a-b)>0;}
struct Writer {
    Bytes data;
    void u(std::uint32_t v){for(int i=0;i<4;++i)data.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
    void i(int v){u(static_cast<std::uint32_t>(v));}
    void f(float v){std::uint32_t n;memcpy(&n,&v,4);u(n);}
    void text(const std::string& s){data.push_back(static_cast<std::uint8_t>(s.size()));data.insert(data.end(),s.begin(),s.end());}
};
struct Reader {
    const Bytes& data;std::size_t at=1;bool ok=true;
    std::uint32_t u(){if(at+4>data.size()){ok=false;return 0;}std::uint32_t n=0;for(int j=0;j<4;++j)n|=static_cast<std::uint32_t>(data[at++])<<(8*j);return n;}
    int i(){return static_cast<std::int32_t>(u());}
    float f(){auto n=u();float v;memcpy(&v,&n,4);if(!std::isfinite(v))ok=false;return v;}
    std::string text(){if(at>=data.size()){ok=false;return{};}const auto n=data[at++];if(n>63||at+n>data.size()){ok=false;return{};}std::string s(data.begin()+at,data.begin()+at+n);at+=n;for(unsigned char c:s)if(c<32||c>126)ok=false;return s;}
    bool done()const{return ok&&at==data.size();}
};
struct Stats {
    std::array<int,8> attributes{},protection{},damage{};
    std::array<int,5> hit{};
    int damageType=0,weapon=0,guild=0;
    std::uint32_t bodyState=0;
};
inline bool Valid(const Stats& s){
    if(s.attributes[1]<1||s.attributes[0]<0||s.attributes[0]>s.attributes[1]||s.weapon<0||s.weapon>7||s.guild<0||s.guild>255||s.damageType<0||s.damageType>255||s.bodyState>0x7ffff)return false;
    for(int v:s.attributes)if(v<0||v>1000000)return false;
    for(int v:s.protection)if(v< -1||v>1000000)return false;
    for(int v:s.damage)if(v<0||v>1000000)return false;
    for(int v:s.hit)if(v<0||v>1000000)return false;
    return true;
}
inline void Write(Writer& w,const Stats& s){for(auto v:s.attributes)w.i(v);for(auto v:s.protection)w.i(v);for(auto v:s.damage)w.i(v);for(auto v:s.hit)w.i(v);w.i(s.damageType);w.i(s.weapon);w.i(s.guild);w.u(s.bodyState);}
inline Stats ReadStats(Reader& r){Stats s;for(auto& v:s.attributes)v=r.i();for(auto& v:s.protection)v=r.i();for(auto& v:s.damage)v=r.i();for(auto& v:s.hit)v=r.i();s.damageType=r.i();s.weapon=r.i();s.guild=r.i();s.bodyState=r.u();if(!Valid(s))r.ok=false;return s;}
struct Peer {std::uint32_t revision=0,interaction=0,sprint=0;Stats stats;std::string animation;};
struct Npc {std::uint32_t id=0,revision=0,generation=1,life=1,sprint=0;int instance=0;std::array<float,3> anchor{};net::Snapshot pose;Stats stats;std::uint32_t enemy=0;std::string animation,model,body,head;int bodyTex=0,headTex=0,bodyColor=0,headColor=0,armor=-1;};
inline std::string AppearanceKey(const Npc& n){
    // Length-prefix strings so model names and numeric fields cannot collide.
    const auto text=[](const std::string& s){return std::to_string(s.size())+":"+s;};
    return text(n.model)+text(n.body)+text(n.head)+"|"+std::to_string(n.bodyTex)+","+std::to_string(n.headTex)+","+std::to_string(n.bodyColor)+","+std::to_string(n.headColor)+","+std::to_string(n.armor);
}
struct Damage {
    Kind kind=Kind::Attack;std::uint32_t target=0,attacker=0,mode=0,weapon=0,spell=0,spellCategory=0,spellLevel=0;
    std::array<std::uint32_t,8> values{};float total=0,multiplier=1;bool dontKill=false;
};
inline bool Valid(const Damage& d){if(!std::isfinite(d.total)||d.total<0||d.total>1000000||!std::isfinite(d.multiplier)||d.multiplier<0||d.multiplier>100||d.mode>255||d.weapon>65535||d.spell>100000||d.spellCategory>65535||d.spellLevel>100)return false;for(auto v:d.values)if(v>1000000)return false;return d.kind==Kind::Attack||d.kind==Kind::Injury;}
inline Writer Begin(Kind kind){Writer w;w.data.push_back(static_cast<std::uint8_t>(kind));return w;}
inline Bytes Encode(const Peer& p){auto w=Begin(Kind::Peer);w.u(p.revision);w.u(p.interaction);Write(w,p.stats);w.text(p.animation);w.u(p.sprint);return w.data;}
inline bool Decode(const Bytes& b,Peer& p){if(b.empty()||b[0]!=1)return false;Reader r{b};p.revision=r.u();p.interaction=r.u();p.stats=ReadStats(r);p.animation=r.text();p.sprint=r.u();return p.sprint<=5&&p.interaction<1000000&&r.done();}
inline Bytes Encode(const Npc& p){auto w=Begin(Kind::Npc);w.u(p.id);w.u(p.revision);w.u(p.generation);w.u(p.life);w.i(p.instance);for(auto v:p.anchor)w.f(v);w.f(p.pose.x);w.f(p.pose.y);w.f(p.pose.z);w.f(p.pose.yaw);Write(w,p.stats);w.u(p.enemy);w.text(p.animation);w.text(p.model);w.text(p.body);w.text(p.head);w.i(p.bodyTex);w.i(p.headTex);w.i(p.bodyColor);w.i(p.headColor);w.i(p.armor);w.u(p.sprint);return w.data;}
inline bool Decode(const Bytes& b,Npc& p){if(b.empty()||b[0]!=2)return false;Reader r{b};p.id=r.u();p.revision=r.u();p.generation=r.u();p.life=r.u();p.instance=r.i();for(auto& v:p.anchor){v=r.f();if(std::abs(v)>1000000)r.ok=false;}p.pose={r.f(),r.f(),r.f(),r.f()};if(std::abs(p.pose.x)>1000000||std::abs(p.pose.y)>1000000||std::abs(p.pose.z)>1000000||std::abs(p.pose.yaw)>36000)r.ok=false;p.stats=ReadStats(r);p.enemy=r.u();p.animation=r.text();p.model=r.text();p.body=r.text();p.head=r.text();p.bodyTex=r.i();p.headTex=r.i();p.bodyColor=r.i();p.headColor=r.i();p.armor=r.i();p.sprint=r.u();return p.sprint<=5&&r.done()&&p.bodyTex>=0&&p.bodyTex<=65535&&p.headTex>=0&&p.headTex<=65535&&p.bodyColor>=0&&p.bodyColor<=65535&&p.headColor>=0&&p.headColor<=65535&&p.armor>=-1&&p.armor<1000000&&p.generation>0&&p.life>=1&&p.life<=3&&p.id>0&&p.id<1000000&&p.instance>=0&&p.instance<1000000;}
inline Bytes Encode(const Damage& d){auto w=Begin(d.kind);w.u(d.target);w.u(d.attacker);w.u(d.mode);w.u(d.weapon);w.u(d.spell);w.u(d.spellCategory);w.u(d.spellLevel);for(auto v:d.values)w.u(v);w.f(d.total);w.f(d.multiplier);w.u(d.dontKill?1:0);return w.data;}
inline bool Decode(const Bytes& b,Damage& d){if(b.empty()||(b[0]!=4&&b[0]!=5))return false;Reader r{b};d.kind=static_cast<Kind>(b[0]);d.target=r.u();d.attacker=r.u();d.mode=r.u();d.weapon=r.u();d.spell=r.u();d.spellCategory=r.u();d.spellLevel=r.u();for(auto& v:d.values)v=r.u();d.total=r.f();d.multiplier=r.f();const auto flag=r.u();d.dontKill=flag!=0;return r.done()&&flag<2&&Valid(d);}
inline Bytes Rules(std::uint32_t revision,bool ff){auto w=Begin(Kind::Rules);w.u(revision);w.u(ff?1:0);return w.data;}
inline bool Rules(const Bytes& b,std::uint32_t& revision,bool& ff){if(b.empty()||b[0]!=3)return false;Reader r{b};revision=r.u();auto value=r.u();ff=value!=0;return value<2&&r.done();}
}
