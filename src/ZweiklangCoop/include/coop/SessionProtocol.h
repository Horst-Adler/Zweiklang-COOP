#pragma once
#include "coop/Recovery.h"
namespace zweiklang::session {
enum class Command {None,Marker,Next,Previous,Overlay};
inline Command Hotkey(unsigned scan,bool control,bool shift,bool blocked=false){if(blocked||!control||!shift)return Command::None;switch(scan){case 0x32:return Command::Marker;case 0x31:return Command::Next;case 0x30:return Command::Previous;case 0x20:return Command::Overlay;default:return Command::None;}}
enum Flag:unsigned {Checkpoint=1,Blocked=2,Rules=4,Actor=8,Quest=16,Pending=32,Incapacitated=64};
enum class Reason:unsigned {Ready,Checkpoint,Recovery,Deferred,Rules,Actor,Quest,Snapshot,Peer,Transfer,Player,PeerPlayer};
struct Ready {std::string profile,timeline;unsigned revision=0,flags=0;};
enum class SaveBlock {Allowed,Recovery,LocalTransfer,PeerTransfer,PeerSaving,PeerUnknown};
inline SaveBlock SaveReadiness(const Ready& local,const Ready* peer,bool connected,bool peerSaving,bool localOpen){
    if(local.flags&Blocked)return SaveBlock::Recovery;
    if((local.flags&Pending)||localOpen)return SaveBlock::LocalTransfer;
    if(peerSaving)return SaveBlock::PeerSaving;
    if(connected&&(!peer||!(peer->flags&Checkpoint)))return SaveBlock::PeerUnknown;
    if(connected&&peer&&(peer->flags&(Blocked|Pending)))return SaveBlock::PeerTransfer;
    return SaveBlock::Allowed;
}
inline const char* SaveText(SaveBlock block){switch(block){
    case SaveBlock::Recovery:return "Speichern gesperrt: lokaler Inventar-/Questabgleich.";
    case SaveBlock::LocalTransfer:return "Speichern wartet: eigenen Inventarvorgang zuerst beenden.";
    case SaveBlock::PeerTransfer:return "Speichern wartet: Partner muss offenen Inventarvorgang beenden.";
    case SaveBlock::PeerSaving:return "Speichern wartet: Partner speichert gerade.";
    case SaveBlock::PeerUnknown:return "Speichern wartet: Partnerbereitschaft noch nicht bekannt.";
    default:return "Speichern bereit.";}}
struct Marker {std::string id,profile;unsigned step=1;std::uint64_t world=0;};
struct Cavalorn {std::string hostTimeline,ownerTimeline;unsigned revision=0,mission=0,letter=0,told=0,dead=0,away=0;};
inline combat::Bytes Encode(const Ready& s){combat::Writer w;w.data.push_back(60);w.text(s.profile);w.text(s.timeline);w.u(s.revision);w.u(s.flags);return w.data;}
inline bool Decode(const combat::Bytes& b,Ready& s){if(b.empty()||b[0]!=60)return false;combat::Reader r{b};s.profile=r.text();s.timeline=r.text();s.revision=r.u();s.flags=r.u();return r.done()&&recovery::Guid(s.profile)&&(s.timeline.empty()||recovery::Guid(s.timeline))&&s.revision>0&&s.flags<128&&(!(s.flags&Checkpoint)||recovery::Guid(s.timeline));}
inline combat::Bytes Encode(const Marker& s){combat::Writer w;w.data.push_back(62);w.text(s.id);w.text(s.profile);w.u(s.step);w.u(static_cast<unsigned>(s.world));w.u(static_cast<unsigned>(s.world>>32));return w.data;}
inline bool Decode(const combat::Bytes& b,Marker& s){if(b.empty()||b[0]!=62)return false;combat::Reader r{b};s.id=r.text();s.profile=r.text();s.step=r.u();auto lo=r.u(),hi=r.u();s.world=lo|(static_cast<std::uint64_t>(hi)<<32);return r.done()&&recovery::Guid(s.id)&&recovery::Guid(s.profile)&&s.step>0&&s.step<=65&&s.world!=0;}
inline combat::Bytes Encode(const Cavalorn& s){combat::Writer w;w.data.push_back(61);w.text(s.hostTimeline);w.text(s.ownerTimeline);w.u(s.revision);w.u(s.mission);w.u(s.letter);w.u(s.told);w.u(s.dead);w.u(s.away);return w.data;}
inline bool Decode(const combat::Bytes& b,Cavalorn& s){if(b.empty()||b[0]!=61)return false;combat::Reader r{b};s.hostTimeline=r.text();s.ownerTimeline=r.text();s.revision=r.u();s.mission=r.u();s.letter=r.u();s.told=r.u();s.dead=r.u();s.away=r.u();return r.done()&&recovery::Guid(s.hostTimeline)&&recovery::Guid(s.ownerTimeline)&&s.revision>0&&s.mission<=4&&s.letter<=4&&s.told<2048&&s.dead<8&&s.away<2;}
inline Reason Readiness(const Ready& local,const Ready* peer,bool fresh){
    if(local.flags&Blocked)return Reason::Recovery;
    if(!(local.flags&Checkpoint))return Reason::Checkpoint;
    if(local.flags&Incapacitated)return Reason::Player;
    if(!(local.flags&Rules))return Reason::Rules;
    if(!(local.flags&Actor))return Reason::Actor;
    if(peer&&(peer->flags&Blocked))return Reason::Deferred;
    if(!peer||!(peer->flags&Checkpoint))return Reason::Peer;
    if(peer->flags&Incapacitated)return Reason::PeerPlayer;
    if((local.flags|peer->flags)&Pending)return Reason::Transfer;
    if(!(peer->flags&Rules)||!(peer->flags&Actor))return Reason::Peer;
    if(!fresh)return Reason::Snapshot;
    if(!(local.flags&Quest)||!(peer->flags&Quest))return Reason::Quest;
    return Reason::Ready;
}
inline const char* Text(Reason reason){switch(reason){
    case Reason::Ready:return "Bereit: Kampf, Loot und Cavalorn-Quest";
    case Reason::Checkpoint:return "Warten: Testspielstand laden oder speichern";
    case Reason::Recovery:return "Gesperrt: lokaler Inventar-/Questabgleich";
    case Reason::Deferred:return "Warten: Partner klaert offene Vorgaenge";
    case Reason::Rules:return "Warten: Hostregeln werden uebertragen";
    case Reason::Actor:return "Warten: Partnerfigur wird aufgebaut";
    case Reason::Quest:return "Basis bereit; Cavalorn-Adapter noch nicht bereit";
    case Reason::Snapshot:return "Warten: aktuelle Partnerposition fehlt";
    case Reason::Transfer:return "Inventarabgleich laeuft; bitte kurz warten";
    case Reason::Player:return "Spieler tot/bewusstlos: Questgaben warten";
    case Reason::PeerPlayer:return "Partner tot/bewusstlos: Questgaben warten";
    default:return "Warten: Bereitschaft des Partners";}}
inline std::string Step(unsigned index){const unsigned counts[]={7,7,12,5,7,12,5,6,4};for(unsigned i=0;i<9;++i){if(index>0&&index<=counts[i])return std::string(1,static_cast<char>('A'+i))+(index<10?"0":"")+std::to_string(index);index-=counts[i];}return "???";}
inline unsigned NextStep(unsigned current){return current>=65?1:current+1;}
inline bool ProgressCompatible(unsigned host,unsigned guest){return host<=4&&guest<=4&&(host==guest||guest==0||(guest==1&&host==2));}
}
