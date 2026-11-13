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
    int damageType=0,weapon=0,guild=0,trueGuild=0;
    std::uint32_t bodyState=0;
};
inline bool Valid(const Stats& s){
    if(s.attributes[1]<1||s.attributes[0]<0||s.attributes[0]>s.attributes[1]||s.weapon<0||s.weapon>7||s.guild<0||s.guild>255||s.trueGuild<0||s.trueGuild>255||s.damageType<0||s.damageType>255||s.bodyState>0x7ffff)return false;
    for(int v:s.attributes)if(v<0||v>1000000)return false;
    for(int v:s.protection)if(v< -1||v>1000000)return false;
    for(int v:s.damage)if(v<0||v>1000000)return false;
    for(int v:s.hit)if(v<0||v>1000000)return false;
    return true;
}
inline void Write(Writer& w,const Stats& s){for(auto v:s.attributes)w.i(v);for(auto v:s.protection)w.i(v);for(auto v:s.damage)w.i(v);for(auto v:s.hit)w.i(v);w.i(s.damageType);w.i(s.weapon);w.i(s.guild);w.i(s.trueGuild);w.u(s.bodyState);}
inline Stats ReadStats(Reader& r){Stats s;for(auto& v:s.attributes)v=r.i();for(auto& v:s.protection)v=r.i();for(auto& v:s.damage)v=r.i();for(auto& v:s.hit)v=r.i();s.damageType=r.i();s.weapon=r.i();s.guild=r.i();s.trueGuild=r.i();s.bodyState=r.u();if(!Valid(s))r.ok=false;return s;}
struct Visual {std::string model,body,head;int bodyTex=0,headTex=0,bodyColor=0,headColor=0,armor=-1;std::array<float,3> scale{1,1,1};bool humanoid=true,transformed=false;std::vector<std::string> overlays;};
inline bool VisualName(const std::string& s){if(s.size()>63||s.find("..")!=std::string::npos)return false;for(unsigned char c:s)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='.'||c=='-'))return false;return true;}
inline bool SpecialOverlay(const std::string& name){if(name.empty()||!VisualName(name))return false;auto s=name;for(char& c:s)if(c>='a'&&c<='z')c=static_cast<char>(c-'a'+'A');return s.find("LADDER")!=std::string::npos||s.find("CLIMB")!=std::string::npos||s.find("SWIM")!=std::string::npos||s.find("DIVE")!=std::string::npos;}
inline bool Valid(const Visual& v){if(!VisualName(v.model)||!VisualName(v.body)||!VisualName(v.head)||v.overlays.size()>2)return false;for(const auto& name:v.overlays)if(!SpecialOverlay(name))return false;for(int n:{v.bodyTex,v.headTex,v.bodyColor,v.headColor})if(n<0||n>65535)return false;for(float n:v.scale)if(!std::isfinite(n)||n<0.01f||n>10)return false;return v.armor>=-1&&v.armor<1000000;}
inline void Write(Writer& w,const Visual& v){w.text(v.model);w.text(v.body);w.text(v.head);w.i(v.bodyTex);w.i(v.headTex);w.i(v.bodyColor);w.i(v.headColor);w.i(v.armor);for(float n:v.scale)w.f(n);w.u((v.humanoid?1u:0u)|(v.transformed?2u:0u));w.u(static_cast<unsigned>(v.overlays.size()));for(const auto& name:v.overlays)w.text(name);}
inline Visual ReadVisual(Reader& r){Visual v;v.model=r.text();v.body=r.text();v.head=r.text();v.bodyTex=r.i();v.headTex=r.i();v.bodyColor=r.i();v.headColor=r.i();v.armor=r.i();for(float& n:v.scale)n=r.f();const auto flags=r.u();v.humanoid=(flags&1)!=0;v.transformed=(flags&2)!=0;const auto count=r.u();if(count>2){r.ok=false;return v;}for(unsigned i=0;i<count;++i)v.overlays.push_back(r.text());if(flags>3||!Valid(v))r.ok=false;return v;}
inline std::string VisualKey(const Visual& v){Writer w;Write(w,v);return std::string(w.data.begin(),w.data.end());}
struct Peer {std::uint32_t revision=0,interaction=0,sprint=0;int absolution=0,level=0;unsigned learnPoints=0,experience=0,nextExperience=0;std::array<int,24> skills{},talents{};Stats stats;std::string animation;Visual visual;};
struct Npc {std::uint32_t id=0,revision=0,generation=1,life=1,sprint=0;int instance=0;std::array<float,3> anchor{};net::Snapshot pose;Stats stats;std::uint32_t enemy=0;std::string animation,model,body,head;int bodyTex=0,headTex=0,bodyColor=0,headColor=0,armor=-1;std::uint32_t owner=0;std::string spawnToken;int hostAttitude=2,guestAttitude=2,hostTemporary=2,guestTemporary=2;std::array<int,8> guestCrime{};};
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
inline Bytes Encode(const Peer& p){auto w=Begin(Kind::Peer);w.u(p.revision);w.u(p.interaction);Write(w,p.stats);w.text(p.animation);w.u(p.sprint);Write(w,p.visual);w.i(p.absolution);w.i(p.level);w.u(p.learnPoints);w.u(p.experience);w.u(p.nextExperience);for(int n:p.skills)w.i(n);for(int n:p.talents)w.i(n);return w.data;}
inline bool Decode(const Bytes& b,Peer& p){if(b.empty()||b[0]!=1)return false;Reader r{b};p.revision=r.u();p.interaction=r.u();p.stats=ReadStats(r);p.animation=r.text();p.sprint=r.u();p.visual=ReadVisual(r);p.absolution=r.i();p.level=r.i();p.learnPoints=r.u();p.experience=r.u();p.nextExperience=r.u();for(int& n:p.skills){n=r.i();if(n<0||n>1000000)r.ok=false;}for(int& n:p.talents){n=r.i();if(n<0||n>1000000)r.ok=false;}return p.level>=0&&p.level<=1000000&&p.learnPoints<=1000000&&p.absolution>=0&&p.absolution<=1000000&&p.sprint<=5&&p.interaction<1000000&&r.done();}
inline Bytes Encode(const Npc& p){auto w=Begin(Kind::Npc);w.u(p.id);w.u(p.revision);w.u(p.generation);w.u(p.life);w.i(p.instance);for(auto v:p.anchor)w.f(v);w.f(p.pose.x);w.f(p.pose.y);w.f(p.pose.z);w.f(p.pose.yaw);Write(w,p.stats);w.u(p.enemy);w.text(p.animation);w.text(p.model);w.text(p.body);w.text(p.head);w.i(p.bodyTex);w.i(p.headTex);w.i(p.bodyColor);w.i(p.headColor);w.i(p.armor);w.u(p.sprint);w.u(p.owner);w.text(p.spawnToken);w.i(p.hostAttitude);w.i(p.guestAttitude);w.i(p.hostTemporary);w.i(p.guestTemporary);for(int n:p.guestCrime)w.i(n);return w.data;}
inline bool Decode(const Bytes& b,Npc& p){if(b.empty()||b[0]!=2)return false;Reader r{b};p.id=r.u();p.revision=r.u();p.generation=r.u();p.life=r.u();p.instance=r.i();for(auto& v:p.anchor){v=r.f();if(std::abs(v)>1000000)r.ok=false;}p.pose={r.f(),r.f(),r.f(),r.f()};if(std::abs(p.pose.x)>1000000||std::abs(p.pose.y)>1000000||std::abs(p.pose.z)>1000000||std::abs(p.pose.yaw)>36000)r.ok=false;p.stats=ReadStats(r);p.enemy=r.u();p.animation=r.text();p.model=r.text();p.body=r.text();p.head=r.text();p.bodyTex=r.i();p.headTex=r.i();p.bodyColor=r.i();p.headColor=r.i();p.armor=r.i();p.sprint=r.u();p.owner=r.u();p.spawnToken=r.text();p.hostAttitude=r.i();p.guestAttitude=r.i();p.hostTemporary=r.i();p.guestTemporary=r.i();for(int& n:p.guestCrime){n=r.i();if(n< -1000000||n>1000000)r.ok=false;}return (p.owner==0||p.owner==HostPlayer||p.owner==GuestPlayer)&&(p.owner!=0||p.spawnToken.empty())&&p.hostAttitude>=0&&p.hostAttitude<=4&&p.guestAttitude>=0&&p.guestAttitude<=4&&p.hostTemporary>=0&&p.hostTemporary<=4&&p.guestTemporary>=0&&p.guestTemporary<=4&&p.sprint<=5&&r.done()&&p.bodyTex>=0&&p.bodyTex<=65535&&p.headTex>=0&&p.headTex<=65535&&p.bodyColor>=0&&p.bodyColor<=65535&&p.headColor>=0&&p.headColor<=65535&&p.armor>=-1&&p.armor<1000000&&p.generation>0&&p.life>=1&&p.life<=3&&p.id>0&&p.id<1000000&&p.instance>=0&&p.instance<1000000;}
inline Bytes Encode(const Damage& d){auto w=Begin(d.kind);w.u(d.target);w.u(d.attacker);w.u(d.mode);w.u(d.weapon);w.u(d.spell);w.u(d.spellCategory);w.u(d.spellLevel);for(auto v:d.values)w.u(v);w.f(d.total);w.f(d.multiplier);w.u(d.dontKill?1:0);return w.data;}
inline bool Decode(const Bytes& b,Damage& d){if(b.empty()||(b[0]!=4&&b[0]!=5))return false;Reader r{b};d.kind=static_cast<Kind>(b[0]);d.target=r.u();d.attacker=r.u();d.mode=r.u();d.weapon=r.u();d.spell=r.u();d.spellCategory=r.u();d.spellLevel=r.u();for(auto& v:d.values)v=r.u();d.total=r.f();d.multiplier=r.f();const auto flag=r.u();d.dontKill=flag!=0;return r.done()&&flag<2&&Valid(d);}
inline Bytes Rules(std::uint32_t revision,bool ff){auto w=Begin(Kind::Rules);w.u(revision);w.u(ff?1:0);return w.data;}
inline bool Rules(const Bytes& b,std::uint32_t& revision,bool& ff){if(b.empty()||b[0]!=3)return false;Reader r{b};revision=r.u();auto value=r.u();ff=value!=0;return value<2&&r.done();}
}
