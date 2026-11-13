#pragma once
#include "coop/CombatProtocol.h"
#include <map>
#include <stdexcept>
namespace zweiklang::character {
constexpr unsigned CounterCount=29;
constexpr std::array<unsigned,10> PersonalAiv{0,1,2,5,6,24,46,47,66,67};
constexpr std::array<unsigned,8> CrimeAiv{0,1,2,24,46,47,66,67};
using Counters=std::array<int,CounterCount>;
inline bool ValidCounters(const Counters& v){for(int n:v)if(n<0||n>1000000)return false;return true;}
inline Counters Difference(const Counters& after,const Counters& before){Counters d{};for(unsigned i=0;i<CounterCount;++i)d[i]=after[i]-before[i];return d;}
inline Counters Add(const Counters& base,const Counters& delta){Counters next{};for(unsigned i=0;i<CounterCount;++i){const auto n=static_cast<long long>(base[i])+delta[i];if(n>1000000||n< -1000000)throw std::runtime_error("Personal crime counter overflow");next[i]=static_cast<int>((std::max)(0ll,n));}return next;}
inline bool Changed(const Counters& d){for(int n:d)if(n)return true;return false;}
struct Relation {int permanent=2,temporary=2;std::array<int,10> aiv{};int knows=0;};
inline bool Valid(const Relation& r){for(int n:r.aiv)if(n< -1000000||n>1000000)return false;return r.permanent>=0&&r.permanent<=4&&r.temporary>=0&&r.temporary<=4&&r.knows>=0&&r.knows<=1;}
struct RelationUpdate {unsigned npc=0;int value=2;bool permanent=true;};
inline combat::Bytes Encode(const RelationUpdate& m){combat::Writer w;w.data.push_back(73);w.u(m.npc);w.i(m.value);w.u(m.permanent?1:0);return w.data;}
inline bool Decode(const combat::Bytes& b,RelationUpdate& m){if(b.empty()||b[0]!=73)return false;combat::Reader r{b};m.npc=r.u();m.value=r.i();const auto p=r.u();m.permanent=p!=0;return r.done()&&p<=1&&m.npc>0&&m.npc<1000000&&m.value>=0&&m.value<=4;}
struct Authority {
    Counters values{},lastDelta{};std::uint32_t revision=0,ownerAck=0;
    bool Apply(std::uint32_t sequence,const Counters& delta){
        if(sequence<=ownerAck){if(sequence==ownerAck&&sequence&&delta!=lastDelta)throw std::runtime_error("Personal counter duplicate payload conflict");return false;}
        if(!sequence||ownerAck==UINT32_MAX||sequence!=ownerAck+1||revision==UINT32_MAX)throw std::runtime_error("Personal counter sequence mismatch");
        for(int n:delta)if(n< -1000000||n>1000000)throw std::runtime_error("Personal counter delta out of range");
        values=Add(values,delta);lastDelta=delta;ownerAck=sequence;++revision;return true;
    }
    void Observe(const Counters& next){if(!ValidCounters(next))throw std::runtime_error("Invalid native personal crime counters");if(values!=next){if(revision==UINT32_MAX)throw std::runtime_error("Personal counter revision exhausted");values=next;++revision;}}
};
struct CounterBank {
    Counters host{};
    Counters Switch(bool fromGuest,bool toGuest,const Counters& native,Authority& guest){
        if(fromGuest)guest.Observe(native);else host=native;
        return toGuest?guest.values:host;
    }
};
enum class CounterOp:std::uint32_t {Hello=0,Delta=1,Snapshot=2,Receipt=3};
struct CounterMessage {CounterOp op=CounterOp::Hello;std::uint32_t revision=0,ack=0;std::string timeline;Counters values{};};
inline combat::Bytes Encode(const CounterMessage& m){combat::Writer w;w.data.push_back(70);w.u(static_cast<unsigned>(m.op));w.u(m.revision);w.u(m.ack);w.text(m.timeline);for(int n:m.values)w.i(n);return w.data;}
inline bool Decode(const combat::Bytes& bytes,CounterMessage& m){if(bytes.empty()||bytes[0]!=70)return false;combat::Reader r{bytes};const auto op=r.u();m.op=static_cast<CounterOp>(op);m.revision=r.u();m.ack=r.u();m.timeline=r.text();for(int& n:m.values)n=r.i();if(op>3||m.timeline.size()!=32||!r.done())return false;for(unsigned char c:m.timeline)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;if(m.op==CounterOp::Delta){if(!m.ack)return false;for(int n:m.values)if(n< -1000000||n>1000000)return false;return true;}return ValidCounters(m.values);}
inline bool IsSummonSpell(unsigned spell){switch(spell){case 22:case 25:case 31:case 35:case 40:case 45:case 84:case 88:return true;default:return false;}}
struct Summon {unsigned spell=0;std::string token;};
inline combat::Bytes Encode(const Summon& s){combat::Writer w;w.data.push_back(71);w.u(s.spell);w.text(s.token);return w.data;}
inline bool Decode(const combat::Bytes& b,Summon& s){if(b.empty()||b[0]!=71)return false;combat::Reader r{b};s.spell=r.u();s.token=r.text();if(!r.done()||!IsSummonSpell(s.spell)||s.token.size()!=32)return false;for(char c:s.token)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;return true;}
struct Passive {unsigned perception=0,target=0;};
inline combat::Bytes Encode(const Passive& p){combat::Writer w;w.data.push_back(72);w.u(p.perception);w.u(p.target);return w.data;}
inline bool Decode(const combat::Bytes& b,Passive& p){if(b.empty()||b[0]!=72)return false;combat::Reader r{b};p.perception=r.u();p.target=r.u();return r.done()&&(p.perception==17||p.perception==6||p.perception==7||p.perception==8||p.perception==9||p.perception==10)&&p.target<1000000;}
inline bool LocalVisualAllowed(bool humanoid,unsigned bodyState){return humanoid&&(bodyState&4096u)==0;}
inline bool SpecialMovement(unsigned state){const auto body=state&127u;return body>=5&&body<=10;}
inline const char* MovementName(unsigned state){switch(state&127u){case 5:return "swim";case 6:return "crawl";case 7:return "dive";case 8:return "jump";case 9:return "climb";case 10:return "fall";default:return "ground";}}
inline bool ValidOwner(unsigned owner){return owner==0||owner==combat::HostPlayer||owner==combat::GuestPlayer;}
struct Archive {
    std::string profile,timeline,fingerprint,guestLineage;
    Authority guest;
    std::uint32_t ownerSequence=0;
    std::map<std::uint32_t,Counters> pending;
    std::map<std::string,Relation> relations;
};
inline combat::Bytes Encode(const Archive& a){
    combat::Writer w;w.data.push_back(0xa3);w.u(1);w.text(a.profile);w.text(a.timeline);w.text(a.fingerprint);w.text(a.guestLineage);
    w.u(a.guest.revision);w.u(a.guest.ownerAck);for(int n:a.guest.values)w.i(n);for(int n:a.guest.lastDelta)w.i(n);w.u(a.ownerSequence);
    w.u(static_cast<unsigned>(a.pending.size()));for(const auto& e:a.pending){w.u(e.first);for(int n:e.second)w.i(n);}
    w.u(static_cast<unsigned>(a.relations.size()));for(const auto& e:a.relations){w.text(e.first);w.i(e.second.permanent);w.i(e.second.temporary);for(int n:e.second.aiv)w.i(n);w.i(e.second.knows);}
    const auto checksum=net::HashString(std::string(w.data.begin(),w.data.end()));w.u(static_cast<unsigned>(checksum));w.u(static_cast<unsigned>(checksum>>32));return w.data;
}
inline Archive DecodeArchive(const combat::Bytes& b){
    if(b.size()>2*1024*1024||b.size()<9||b[0]!=0xa3)throw std::runtime_error("Invalid character archive size/header");
    const auto end=b.size()-8;std::uint64_t stored=0;for(unsigned i=0;i<8;++i)stored|=static_cast<std::uint64_t>(b[end+i])<<(i*8);
    if(stored!=net::HashString(std::string(b.begin(),b.begin()+end)))throw std::runtime_error("Character archive checksum mismatch");
    combat::Reader r{b};if(r.u()!=1)throw std::runtime_error("Unsupported character archive format");Archive a;
    a.profile=r.text();a.timeline=r.text();a.fingerprint=r.text();a.guestLineage=r.text();a.guest.revision=r.u();a.guest.ownerAck=r.u();for(int& n:a.guest.values)n=r.i();for(int& n:a.guest.lastDelta){n=r.i();if(n< -1000000||n>1000000)r.ok=false;}a.ownerSequence=r.u();
    auto count=r.u();if(count>64)throw std::runtime_error("Character pending operation limit");
    for(unsigned i=0;i<count;++i){const auto seq=r.u();Counters d{};for(int& n:d)n=r.i();for(int n:d)if(n< -1000000||n>1000000)r.ok=false;if(!seq||seq>a.ownerSequence||!a.pending.emplace(seq,d).second)r.ok=false;}
    count=r.u();if(count>8192)throw std::runtime_error("Character relation limit");
    for(unsigned i=0;i<count;++i){auto key=r.text();Relation value;value.permanent=r.i();value.temporary=r.i();for(int& n:value.aiv)n=r.i();value.knows=r.i();if(key.empty()||!Valid(value)||!a.relations.emplace(key,value).second)r.ok=false;}
    r.u();r.u();
    if(!r.done()||a.profile.size()!=32||a.timeline.size()!=32||a.fingerprint.empty()||(!a.guestLineage.empty()&&a.guestLineage.size()!=32)||!ValidCounters(a.guest.values))throw std::runtime_error("Invalid character archive payload");return a;
}
}
