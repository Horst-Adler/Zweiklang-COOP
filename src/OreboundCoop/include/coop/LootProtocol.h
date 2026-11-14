#pragma once
#include "coop/CombatProtocol.h"
#include <map>
#include <set>
namespace orebound::loot {
using Bytes=combat::Bytes;
constexpr std::size_t MaxEntries=1024,PageEntries=24,MaxTransactions=4096;
enum class Op:std::uint8_t {Open=16,Begin,Page,Take,Result,Close,Denied,KeepAlive,Applied,QuickOpen};
enum class Source:std::uint32_t {Corpse=1,Chest=2,Ground=3,Merchant=4,Quest=5};
enum class Status:std::uint32_t {Ok,Busy,Missing,Range,Locked,BadRequest,Stale,Empty,NativeFailure,Limit};
struct Identity {Source kind=Source::Corpse;std::uint32_t npc=0;std::string name;std::array<float,3> position{};};
struct Entry {std::uint32_t token=0,instance=0,amount=0;};
struct Message {
    Op op=Op::Open;std::uint32_t request=0,lease=0,revision=0,index=0,total=0;
    Entry item;Identity source;Status status=Status::Ok;std::vector<Entry> entries;
};
inline bool IsMessage(const Bytes& bytes){return !bytes.empty()&&bytes[0]>=16&&bytes[0]<=25;}
inline bool Valid(const Entry& e){return e.token>0&&e.token<=MaxEntries&&e.instance>0&&e.instance<1000000&&e.amount<=1000000;}
inline Bytes Encode(const Message& m){
    combat::Writer w;w.data.push_back(static_cast<std::uint8_t>(m.op));w.u(m.request);w.u(m.lease);w.u(m.revision);
    switch(m.op){
    case Op::QuickOpen:case Op::Open:w.u(static_cast<std::uint32_t>(m.source.kind));w.u(m.source.npc);w.text(m.source.name);for(float p:m.source.position)w.f(p);break;
    case Op::Begin:w.u(m.total);break;
    case Op::Page:w.u(m.index);w.u(static_cast<std::uint32_t>(m.entries.size()));for(const auto& e:m.entries){w.u(e.token);w.u(e.instance);w.u(e.amount);}break;
    case Op::Take:case Op::Result:w.u(m.item.token);w.u(m.item.instance);w.u(m.item.amount);w.u(static_cast<std::uint32_t>(m.status));break;
    case Op::Denied:w.u(static_cast<std::uint32_t>(m.status));break;
    default:break;
    }
    return w.data;
}
inline bool Decode(const Bytes& b,Message& m){
    if(!IsMessage(b)||b.size()>net::MaximumDatagramBytes-net::ProtocolHeaderBytes)return false;
    m=Message{};m.op=static_cast<Op>(b[0]);combat::Reader r{b};m.request=r.u();m.lease=r.u();m.revision=r.u();
    bool valid=true;
    switch(m.op){
    case Op::QuickOpen:case Op::Open:
        m.source.kind=static_cast<Source>(r.u());m.source.npc=r.u();m.source.name=r.text();for(float& p:m.source.position){p=r.f();if(std::abs(p)>1000000)valid=false;}
        valid=valid&&((m.source.kind==Source::Corpse||m.source.kind==Source::Merchant)?m.source.npc>0&&m.source.npc<1000000:m.source.kind==Source::Chest&&m.source.npc==0)&&m.lease==0&&(m.op!=Op::QuickOpen||m.source.kind!=Source::Merchant);break;
    case Op::Begin:m.total=r.u();valid=m.lease>0&&m.revision>0&&m.total<=MaxEntries;break;
    case Op::Page:{m.index=r.u();const auto n=r.u();if(n>PageEntries||m.index>MaxEntries||m.index+n>MaxEntries)return false;for(std::uint32_t i=0;i<n;++i){Entry e{r.u(),r.u(),r.u()};if(!Valid(e)||e.amount==0)valid=false;m.entries.push_back(e);}valid=valid&&m.lease>0&&m.revision>0;break;}
    case Op::Take:case Op::Result:m.item={r.u(),r.u(),r.u()};m.status=static_cast<Status>(r.u());valid=m.lease>0&&m.revision>0&&Valid(m.item)&&m.item.amount>0&&m.status<=Status::Limit;break;
    case Op::Denied:m.status=static_cast<Status>(r.u());valid=m.status!=Status::Ok&&m.status<=Status::Limit;break;
    default:valid=m.lease>0;break;
    }
    return m.request>0&&valid&&r.done();
}
// One guest can hold one source. The host's native UI is checked separately.
// No timeout or disconnect restores committed stock: this would duplicate a
// grant whose application acknowledgement was lost.
class Ledger {
    struct Record {Message request,result;bool applied=false;};
    std::map<std::uint32_t,Record> history_;
    std::map<std::uint32_t,Entry> stock_;
    std::uint32_t next_=1,lease_=0,revision_=0;
public:
    std::uint32_t Lease()const{return lease_;}
    std::uint32_t Revision()const{return revision_;}
    const std::map<std::uint32_t,Entry>& Stock()const{return stock_;}
    bool HasUnapplied()const{for(const auto& pair:history_)if(pair.second.result.status==Status::Ok&&!pair.second.applied)return true;return false;}
    std::uint32_t Open(const std::vector<Entry>& entries){
        if(lease_||HasUnapplied()||entries.size()>MaxEntries||next_==0)return 0;
        std::map<std::uint32_t,Entry> candidate;
        for(const auto& e:entries)if(!Valid(e)||e.amount==0||!candidate.emplace(e.token,e).second)return 0;
        stock_=std::move(candidate);lease_=next_++;revision_=1;return lease_;
    }
    template<class Remove> Message Take(const Message& request,Remove remove){
        const auto old=history_.find(request.request);
        if(old!=history_.end()){
            const auto& a=old->second.request;
            if(a.lease==request.lease&&a.revision==request.revision&&a.item.token==request.item.token&&a.item.instance==request.item.instance&&a.item.amount==request.item.amount)return old->second.result;
            Message denied=request;denied.op=Op::Result;denied.status=Status::BadRequest;return denied;
        }
        Message result=request;result.op=Op::Result;result.status=Status::BadRequest;
        if(history_.size()>=MaxTransactions){result.status=Status::Limit;return result;}
        const auto it=stock_.find(request.item.token);
        if(request.op!=Op::Take||!request.request||!Valid(request.item)||!request.item.amount)result.status=Status::BadRequest;
        else if(!lease_||request.lease!=lease_||request.revision!=revision_)result.status=Status::Stale;
        else if(it==stock_.end()||it->second.instance!=request.item.instance||it->second.amount<request.item.amount)result.status=Status::Empty;
        else if(!remove(it->second,request.item.amount))result.status=Status::NativeFailure;
        else {it->second.amount-=request.item.amount;result.status=Status::Ok;result.revision=++revision_;}
        history_.emplace(request.request,Record{request,result,false});return result;
    }
    bool Applied(std::uint32_t request,std::uint32_t lease){const auto it=history_.find(request);if(it==history_.end()||it->second.result.lease!=lease||it->second.result.status!=Status::Ok)return false;it->second.applied=true;return true;}
    template<class Insert> Message Put(const Message& request,Insert insert){
        Message result=request;result.op=Op::Result;result.status=Status::BadRequest;
        if(history_.count(request.request)||history_.size()>=MaxTransactions)return result;
        if(!lease_||request.lease!=lease_||request.revision!=revision_)result.status=Status::Stale;
        else if(!Valid(request.item)||!request.item.amount)result.status=Status::BadRequest;
        else {
            auto found=std::find_if(stock_.begin(),stock_.end(),[&](const auto& p){return p.second.instance==request.item.instance;});
            std::uint32_t token=found==stock_.end()?(stock_.empty()?1:stock_.rbegin()->first+1):found->first;
            if(token>MaxEntries||(found!=stock_.end()&&found->second.amount>1000000-request.item.amount))result.status=Status::Limit;
            else if(!insert(request.item,request.item.amount))result.status=Status::NativeFailure;
            else {if(found==stock_.end())stock_[token]=Entry{token,request.item.instance,request.item.amount};else found->second.amount+=request.item.amount;result.item.token=token;result.status=Status::Ok;result.revision=++revision_;}
        }
        history_.emplace(request.request,Record{request,result,false});return result;
    }
    bool Close(std::uint32_t lease){if(lease_!=lease||HasUnapplied())return false;lease_=0;stock_.clear();return true;}
};
// Snapshot pages are assembled and validated before any engine inventory is
// touched. Duplicate/missing/reordered pages never produce a partial inventory.
class Snapshot {
    std::uint32_t request_=0,lease_=0,revision_=0,total_=0;
    std::vector<Entry> entries_;std::set<std::uint32_t> tokens_;
public:
    bool Begin(const Message& m){if(m.op!=Op::Begin||!m.request||!m.lease||!m.revision||m.total>MaxEntries)return false;request_=m.request;lease_=m.lease;revision_=m.revision;total_=m.total;entries_.clear();tokens_.clear();return true;}
    bool Page(const Message& m){if(m.op!=Op::Page||m.request!=request_||m.lease!=lease_||m.revision!=revision_||m.index!=entries_.size()||m.entries.empty()||m.entries.size()>PageEntries||entries_.size()+m.entries.size()>total_)return false;auto seen=tokens_;for(auto e:m.entries)if(!Valid(e)||!e.amount||!seen.insert(e.token).second)return false;tokens_=std::move(seen);entries_.insert(entries_.end(),m.entries.begin(),m.entries.end());return true;}
    bool Complete()const{return request_&&entries_.size()==total_;}
    const std::vector<Entry>& Entries()const{return entries_;}
};
}
