#pragma once
#include "coop/LootProtocol.h"
#include <windows.h>
#include <bcrypt.h>
#include <stdexcept>
#include <functional>
namespace orebound::recovery {
constexpr unsigned MaxRecords=8192;
struct NpcCandidate {std::string name;bool human=false;std::array<float,3> position{},anchor{};};
inline int ResolveNpcSource(const std::string& name,const std::array<float,3>& position,const std::vector<NpcCandidate>& candidates){
    int named=-1,names=0,best=-1;float closest=4000000,second=4000000;
    for(std::size_t i=0;i<candidates.size();++i){
        const auto& n=candidates[i];
        if(n.human&&!name.empty()&&n.name==name){named=static_cast<int>(i);++names;}
        float current=0,anchor=0;for(int axis=0;axis<3;++axis){const auto a=n.position[axis]-position[axis],b=n.anchor[axis]-position[axis];current+=a*a;anchor+=b*b;}
        const float d=(std::min)(current,anchor);
        if(d<closest){second=closest;closest=d;best=static_cast<int>(i);}else if(d<second)second=d;
    }
    // A named human instance can walk away between a durable decision and replay.
    // Repeated creature instances still require a nearby, unambiguous anchor.
    if(names==1)return named;
    return best>=0&&std::sqrt(second)-std::sqrt(closest)>=100?best:-1;
}
enum class Action:std::uint32_t {Take=1,Put=2,Unlock=3,GroundTake=4,Drop=5,Buy=6,Sell=7,QuestXp=8,QuestItem=9};
enum class Decision:std::uint32_t {Prepared,Commit,Abort};
enum class Role:std::uint32_t {Host=1,Owner=2,Local=3};
enum class Op:std::uint8_t {Hello=32,Request,Result,Query,LockState,Receipt};
struct Tx {
    std::string id,hostTimeline,ownerTimeline,world;
    Action action=Action::Take;Decision decision=Decision::Prepared;
    loot::Identity source;int instance=0;std::uint32_t item=0,amount=0,currency=0,price=0;
};
struct Message {Op op=Op::Hello;Tx tx;std::uint32_t request=1,lease=0,revision=0,token=1;bool locked=false;};
inline bool Guid(const std::string& s){if(s.size()!=32)return false;for(char c:s)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))return false;return true;}
inline std::string NewGuid(){unsigned char b[16];if(BCryptGenRandom(nullptr,b,sizeof(b),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0)throw std::runtime_error("Recovery RNG failed");const char* hex="0123456789abcdef";std::string s;for(auto c:b){s+=hex[c>>4];s+=hex[c&15];}return s;}
inline bool Valid(const Tx& t){
    if(!Guid(t.id)||!Guid(t.hostTimeline)||!Guid(t.ownerTimeline)||t.world.empty()||t.world.size()>63||t.source.name.size()>63||t.decision>Decision::Abort)return false;
    if(t.source.kind!=loot::Source::Chest&&t.source.kind!=loot::Source::Corpse&&t.source.kind!=loot::Source::Ground&&t.source.kind!=loot::Source::Merchant&&t.source.kind!=loot::Source::Quest)return false;
    for(float p:t.source.position)if(!std::isfinite(p)||std::abs(p)>1000000)return false;
    if(t.source.kind==loot::Source::Quest)return t.source.name=="CAVALORN_KILLBRAGO"&&t.instance>=1&&t.instance<=4&&t.currency==0&&t.price==0&&t.amount>0&&t.amount<=1000000&&((t.action==Action::QuestXp&&t.item==0&&t.instance==1)||(t.action==Action::QuestItem&&t.item>0&&t.item<1000000&&t.instance>=2));
    if((t.source.kind==loot::Source::Corpse||t.source.kind==loot::Source::Merchant)&&(t.instance<=0||t.instance>=1000000))return false;
    if(t.action==Action::Buy||t.action==Action::Sell)return t.source.kind==loot::Source::Merchant&&t.item>0&&t.item<1000000&&t.amount>0&&t.amount<=1000000&&t.currency>0&&t.currency<1000000&&t.item!=t.currency&&t.price<=1000000;
    if(t.currency||t.price)return false;
    return t.action==Action::Unlock?t.source.kind==loot::Source::Chest&&t.item==0&&t.amount==0:
        (((t.action==Action::Take||t.action==Action::Put)&&(t.source.kind==loot::Source::Chest||t.source.kind==loot::Source::Corpse))||((t.action==Action::GroundTake||t.action==Action::Drop)&&t.source.kind==loot::Source::Ground))&&t.item>0&&t.item<1000000&&t.amount>0&&t.amount<=1000000;
}
inline bool Same(const Tx& a,const Tx& b){return a.id==b.id&&a.hostTimeline==b.hostTimeline&&a.ownerTimeline==b.ownerTimeline&&a.world==b.world&&a.action==b.action&&a.source.kind==b.source.kind&&a.source.name==b.source.name&&a.source.position==b.source.position&&a.instance==b.instance&&a.item==b.item&&a.amount==b.amount&&a.currency==b.currency&&a.price==b.price;}
inline void Write(combat::Writer& w,const Tx& t){w.text(t.id);w.text(t.hostTimeline);w.text(t.ownerTimeline);w.text(t.world);w.u(static_cast<unsigned>(t.action));w.u(static_cast<unsigned>(t.decision));w.u(static_cast<unsigned>(t.source.kind));w.u(t.source.npc);w.text(t.source.name);for(float p:t.source.position)w.f(p);w.i(t.instance);w.u(t.item);w.u(t.amount);w.u(t.currency);w.u(t.price);}
inline Tx Read(combat::Reader& r,bool legacy=false){Tx t;t.id=r.text();t.hostTimeline=r.text();t.ownerTimeline=r.text();t.world=r.text();t.action=static_cast<Action>(r.u());t.decision=static_cast<Decision>(r.u());t.source.kind=static_cast<loot::Source>(r.u());t.source.npc=r.u();t.source.name=r.text();for(float& p:t.source.position)p=r.f();t.instance=r.i();t.item=r.u();t.amount=r.u();if(!legacy){t.currency=r.u();t.price=r.u();}if(!Valid(t))r.ok=false;return t;}
inline bool IsMessage(const combat::Bytes& b){return !b.empty()&&b[0]>=32&&b[0]<=37;}
inline combat::Bytes Encode(const Message& m){combat::Writer w;w.data.push_back(static_cast<unsigned char>(m.op));w.u(m.request);w.u(m.lease);w.u(m.revision);w.u(m.token);if(m.op==Op::Hello)w.text(m.tx.hostTimeline);else if(m.op==Op::LockState){w.text(m.tx.hostTimeline);w.text(m.tx.world);w.text(m.tx.source.name);for(float p:m.tx.source.position)w.f(p);w.u(m.locked?1:0);}else Write(w,m.tx);return w.data;}
inline bool Decode(const combat::Bytes& b,Message& m){
    if(!IsMessage(b)||b.size()>net::MaximumDatagramBytes-net::ProtocolHeaderBytes)return false;
    combat::Reader r{b};m=Message{};m.op=static_cast<Op>(b[0]);m.request=r.u();m.lease=r.u();m.revision=r.u();m.token=r.u();
    if(m.op==Op::Hello){m.tx.hostTimeline=r.text();if(!Guid(m.tx.hostTimeline))return false;}
    else if(m.op==Op::LockState){m.tx.hostTimeline=r.text();m.tx.world=r.text();m.tx.source.kind=loot::Source::Chest;m.tx.source.name=r.text();for(float& p:m.tx.source.position){p=r.f();if(std::abs(p)>1000000)r.ok=false;}auto value=r.u();if(!Guid(m.tx.hostTimeline)||m.tx.world.empty()||value>1)return false;m.locked=value!=0;}
    else m.tx=Read(r);
    return m.request>0&&r.done();
}
inline std::uint32_t Crc(const combat::Bytes& bytes){std::uint32_t c=~0u;for(auto b:bytes){c^=b;for(int k=0;k<8;++k)c=(c>>1)^(0xedb88320u&(~(c&1)+1));}return ~c;}
inline combat::Bytes Seal(combat::Writer w){const auto crc=Crc(w.data);w.u(crc);return w.data;}
inline combat::Bytes Unseal(combat::Bytes b){if(b.size()<9)throw std::runtime_error("Recovery file truncated");const auto n=b.size();std::uint32_t crc=0;for(int i=0;i<4;++i)crc|=static_cast<std::uint32_t>(b[n-4+i])<<(8*i);b.resize(n-4);if(Crc(b)!=crc)throw std::runtime_error("Recovery checksum mismatch");return b;}
inline combat::Bytes ReadFile(const std::string& path){
    HANDLE h=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)throw std::runtime_error("Recovery file unavailable: "+path);
    LARGE_INTEGER size;bool ok=GetFileSizeEx(h,&size)!=FALSE;combat::Bytes b;
    if(ok&&size.QuadPart>=0&&size.QuadPart<=16*1024*1024){b.resize(static_cast<std::size_t>(size.QuadPart));DWORD read=0;ok=::ReadFile(h,b.data(),static_cast<DWORD>(b.size()),&read,nullptr)&&read==b.size();}else ok=false;
    CloseHandle(h);if(!ok)throw std::runtime_error("Recovery file read failed");return b;
}
using TraceSink=std::function<void(const std::string&,const std::string&)>;
inline TraceSink& TraceTarget(){static TraceSink sink;return sink;}
inline void SetTraceSink(TraceSink sink){TraceTarget()=std::move(sink);}
inline void Trace(const std::string& phase,const std::string& detail)noexcept{try{if(TraceTarget())TraceTarget()(phase,detail);}catch(...){}}
inline bool Exists(const std::string& path){
    if(GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES)return true;
    const auto error=GetLastError();if(error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND)return false;
    throw std::runtime_error("Recovery path inaccessible win32="+std::to_string(error));
}
inline void IoFail(const char* stage,const std::string& path,DWORD error){
    Trace("io_error","stage="+std::string(stage)+" path="+path+" win32="+std::to_string(error));
    throw std::runtime_error("Recovery "+std::string(stage)+" failed win32="+std::to_string(error)+" path="+path);
}
inline void PublishPending(const std::string& path){
    const auto started=GetTickCount64();Trace("replace_begin","path="+path);
    if(!MoveFileExA((path+".pending").c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))IoFail("replace",path,GetLastError());
    Trace("replace_end","path="+path+" elapsed_ms="+std::to_string(GetTickCount64()-started));
}
inline void AtomicWrite(const std::string& path,const combat::Bytes& b){
    const auto started=GetTickCount64();const auto tmp=path+".pending";
    Trace("write_begin","path="+path+" bytes="+std::to_string(b.size()));
    HANDLE h=CreateFileA(tmp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(h==INVALID_HANDLE_VALUE)IoFail("create",path,GetLastError());
    DWORD written=0;const bool ok=WriteFile(h,b.data(),static_cast<DWORD>(b.size()),&written,nullptr)!=FALSE;
    if(!ok||written!=b.size()){const auto error=ok?ERROR_WRITE_FAULT:GetLastError();CloseHandle(h);IoFail("write",path,error);}
    Trace("write_end","path="+path+" bytes="+std::to_string(written));
    Trace("flush_begin","path="+path);
    if(!FlushFileBuffers(h)){const auto error=GetLastError();CloseHandle(h);IoFail("flush",path,error);}
    if(!CloseHandle(h))IoFail("close",path,GetLastError());
    Trace("flush_end","path="+path+" elapsed_ms="+std::to_string(GetTickCount64()-started));
    PublishPending(path);
}
struct Record {Tx tx;Role role=Role::Host;std::uint32_t ordinal=0;};
struct StoreSnapshot {std::string profile;std::map<std::string,Record> records;};
inline StoreSnapshot DecodeStore(combat::Bytes b){
    b=Unseal(std::move(b));if(b[0]!=0xa7)throw std::runtime_error("Recovery store format mismatch");
    combat::Reader r{b};const auto version=r.u();if(version<1||version>4)throw std::runtime_error("Recovery store version mismatch");
    StoreSnapshot s;s.profile=r.text();const auto count=r.u();if(!Guid(s.profile)||count>MaxRecords)throw std::runtime_error("Recovery store bounds");
    std::set<unsigned> ordinals;
    for(unsigned i=0;i<count;++i){
        const auto role=static_cast<Role>(r.u());const auto ordinal=r.u();auto tx=Read(r,version==1);
        if((role!=Role::Host&&role!=Role::Owner&&role!=Role::Local)||!ordinal||ordinal>count||!ordinals.insert(ordinal).second||!s.records.emplace(tx.id,Record{tx,role,ordinal}).second)throw std::runtime_error("Recovery store records invalid");
    }
    if(!r.done())throw std::runtime_error("Recovery store invalid tail");return s;
}
inline StoreSnapshot ReadStore(const std::string& path){
    try{return DecodeStore(ReadFile(path));}
    catch(const std::exception& e){Trace("journal_invalid","path="+path+" reason="+e.what());throw std::runtime_error("Recovery journal invalid path="+path+" reason="+e.what());}
}
// One Store::Put changes one decision OR appends one immutable transaction.
// CRC alone is insufficient: profile, payload, ordering and terminal decisions
// must also agree before any interrupted publication can be completed.
inline bool StoreSuccessor(const StoreSnapshot& newer,const StoreSnapshot& older){
    if(newer.profile!=older.profile||newer.records.size()<older.records.size()||newer.records.size()>older.records.size()+1)return false;
    unsigned changes=static_cast<unsigned>(newer.records.size()-older.records.size());
    for(const auto& entry:older.records){
        const auto found=newer.records.find(entry.first);if(found==newer.records.end())return false;
        const auto& a=found->second;const auto& b=entry.second;
        if(a.role!=b.role||a.ordinal!=b.ordinal||!Same(a.tx,b.tx))return false;
        if(a.tx.decision!=b.tx.decision){if(b.tx.decision!=Decision::Prepared||a.tx.decision==Decision::Prepared)return false;++changes;}
    }
    for(const auto& entry:newer.records)if(!older.records.count(entry.first)&&entry.second.ordinal!=newer.records.size())return false;
    return changes<=1;
}
enum class StoreRecovery {Clean,Identical,Promoted,Stale};
inline bool UnknownCommitQuery(const Message& m){return m.op==Op::Query&&m.tx.decision==Decision::Commit;}
class Store {
    std::string path_,profile_;std::map<std::string,Record> records_;bool open_=false;StoreRecovery recovery_=StoreRecovery::Clean;
    void Save(const std::map<std::string,Record>& candidate){combat::Writer w;w.data.push_back(0xa7);w.u(4);w.text(profile_);w.u(static_cast<unsigned>(candidate.size()));for(const auto& pair:candidate){w.u(static_cast<unsigned>(pair.second.role));w.u(pair.second.ordinal);Write(w,pair.second.tx);}AtomicWrite(path_,Seal(w));}
public:
    void Open(const std::string& path){path_=path;records_.clear();profile_.clear();open_=false;recovery_=StoreRecovery::Clean;
        const auto pending=path+".pending";const bool mainExists=Exists(path),hasPending=Exists(pending);
        if(!mainExists&&!hasPending){profile_=NewGuid();Save(records_);open_=true;return;}
        StoreSnapshot main;if(mainExists)main=ReadStore(path);
        if(hasPending){
            const auto candidate=ReadStore(pending);
            if(!mainExists&&!candidate.records.empty())throw std::runtime_error("Recovery main journal missing with nonempty pending; originals retained");
            const bool same=mainExists&&StoreSuccessor(candidate,main)&&StoreSuccessor(main,candidate);
            const bool forward=!mainExists||StoreSuccessor(candidate,main),backward=mainExists&&StoreSuccessor(main,candidate);
            if(!forward&&!backward){Trace("journal_conflict","path="+path);throw std::runtime_error("Recovery main/pending conflict; originals retained");}
            const auto evidence=path+".interrupted-"+NewGuid();
            Trace("journal_pending","path="+path+" main_records="+std::to_string(main.records.size())+" pending_records="+std::to_string(candidate.records.size()));
            if(forward&&!same){
                if(mainExists&&!CopyFileA(path.c_str(),(evidence+".main").c_str(),TRUE))IoFail("archive_main",path,GetLastError());
                if(!CopyFileA(pending.c_str(),(evidence+".pending").c_str(),TRUE))IoFail("archive_pending",path,GetLastError());
                HANDLE h=CreateFileA(pending.c_str(),GENERIC_WRITE,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
                if(h==INVALID_HANDLE_VALUE)IoFail("open_pending",path,GetLastError());
                if(!FlushFileBuffers(h)){const auto error=GetLastError();CloseHandle(h);IoFail("flush_pending",path,error);}if(!CloseHandle(h))IoFail("close_pending",path,GetLastError());
                PublishPending(path);main=candidate;recovery_=StoreRecovery::Promoted;
            }else{
                if(!MoveFileExA(pending.c_str(),(evidence+".pending").c_str(),MOVEFILE_WRITE_THROUGH))IoFail("archive_pending",path,GetLastError());
                recovery_=same?StoreRecovery::Identical:StoreRecovery::Stale;
            }
            Trace("journal_recovered","path="+path+" mode="+std::to_string(static_cast<unsigned>(recovery_))+" evidence="+evidence);
        }
        profile_=main.profile;records_=std::move(main.records);open_=true;
    }
    StoreRecovery RecoveryStatus()const{return recovery_;}
    const std::string& Profile()const{return profile_;}
    const std::map<std::string,Record>& Records()const{return records_;}
    std::vector<Record> Ordered()const{std::vector<Record> out;for(const auto& p:records_)out.push_back(p.second);std::sort(out.begin(),out.end(),[](const Record& a,const Record& b){return a.ordinal<b.ordinal;});return out;}
    const Record* Find(const std::string& id)const{auto it=records_.find(id);return it==records_.end()?nullptr:&it->second;}
    bool Put(const Tx& t,Role role){
        if(!open_||!Valid(t))return false;auto copy=records_;auto it=copy.find(t.id);
        if(it!=copy.end()){
            if(!Same(it->second.tx,t)||it->second.role!=role)return false;
            if(it->second.tx.decision!=Decision::Prepared&&it->second.tx.decision!=t.decision)return false;
            if(it->second.tx.decision==t.decision){Trace("store_noop","tx="+t.id);return true;}
            it->second.tx=t;
        }else {if(copy.size()>=MaxRecords)return false;const auto ordinal=static_cast<unsigned>(copy.size()+1);copy.emplace(t.id,Record{t,role,ordinal});}
        Trace("store_begin","tx="+t.id+" role="+std::to_string(static_cast<unsigned>(role))+" decision="+std::to_string(static_cast<unsigned>(t.decision)));
        try{Save(copy);}catch(...){open_=false;throw;}records_=std::move(copy);Trace("store_end","tx="+t.id);return true;
    }
};
struct Checkpoint {std::string profile,timeline,fingerprint;std::set<std::string> effects;};
inline combat::Bytes Encode(const Checkpoint& c){combat::Writer w;w.data.push_back(0xc7);w.u(2);w.text(c.profile);w.text(c.timeline);w.text(c.fingerprint);w.u(static_cast<unsigned>(c.effects.size()));for(const auto& id:c.effects)w.text(id);return Seal(w);}
inline Checkpoint DecodeCheckpoint(combat::Bytes b){b=Unseal(std::move(b));combat::Reader r{b};const auto version=r.u();if(b[0]!=0xc7||(version!=1&&version!=2))throw std::runtime_error("Recovery checkpoint format mismatch");Checkpoint c;c.profile=r.text();c.timeline=r.text();c.fingerprint=r.text();auto n=r.u();if(!Guid(c.profile)||!Guid(c.timeline)||n>2*MaxRecords)throw std::runtime_error("Recovery checkpoint bounds");for(unsigned i=0;i<n;++i){auto id=r.text();if(!(Guid(id)||(version>=2&&id.size()==33&&id.back()=='c'&&Guid(id.substr(0,32))))||!c.effects.insert(id).second)throw std::runtime_error("Recovery checkpoint IDs invalid");}if(!r.done())throw std::runtime_error("Recovery checkpoint truncated");return c;}
inline bool Desired(const Record& r){if(r.tx.decision==Decision::Abort)return false;if(r.role==Role::Host)return r.tx.decision==Decision::Commit;return r.tx.action==Action::Put||r.tx.action==Action::Drop||r.tx.action==Action::Buy||r.tx.action==Action::Sell||r.tx.action==Action::Unlock||r.tx.decision==Decision::Commit;}
// Caller maintains effects in memory and stores them WITH each completed native
// save. After load it supplies the saved set, not the last process's set.
template<class Apply> bool Reconcile(const Record& r,std::set<std::string>& effects,Apply apply){
    auto leg=[&](const Record& effect,const std::string& key,bool desired,bool zeroIsNoop=false){const bool before=effects.count(key)!=0;if(before==desired)return true;if(!(zeroIsNoop&&effect.tx.amount==0)&&!apply(effect,desired))return false;if(desired)effects.insert(key);else effects.erase(key);return true;};
    Record translated=r;
    if(r.role==Role::Local&&(r.tx.action==Action::Take||r.tx.action==Action::GroundTake)){
        Record source=r,owner=r;source.role=Role::Host;owner.role=Role::Owner;owner.tx.action=Action::Take;
        if(!leg(source,r.tx.id,r.tx.decision==Decision::Commit))return false;
        return leg(owner,r.tx.id+"c",r.tx.decision==Decision::Commit);
    }
    if(r.role==Role::Local&&r.tx.action==Action::Drop){
        Record reserve=r,spawn=r;reserve.role=Role::Owner;reserve.tx.action=Action::Put;spawn.role=Role::Host;
        if(!leg(reserve,r.tx.id,r.tx.decision!=Decision::Abort))return false;
        return leg(spawn,r.tx.id+"c",r.tx.decision==Decision::Commit);
    }
    if(r.tx.action==Action::Buy||r.tx.action==Action::Sell){
        if(r.role==Role::Host){translated.tx.action=r.tx.action==Action::Buy?Action::Take:Action::Put;return leg(translated,r.tx.id,Desired(r));}
        Record reserve=r,credit=r;reserve.tx.action=Action::Put;credit.tx.action=Action::Take;
        if(r.tx.action==Action::Buy){reserve.tx.item=r.tx.currency;reserve.tx.amount=r.tx.price;}else{credit.tx.item=r.tx.currency;credit.tx.amount=r.tx.price;}
        if(!leg(reserve,r.tx.id,r.tx.decision!=Decision::Abort,true))return false;
        return leg(credit,r.tx.id+"c",r.tx.decision==Decision::Commit,true);
    }
    if(r.role==Role::Owner){if(r.tx.action==Action::Drop)translated.tx.action=Action::Put;if(r.tx.action==Action::GroundTake)translated.tx.action=Action::Take;}
    return leg(translated,r.tx.id,Desired(r));
}
}
