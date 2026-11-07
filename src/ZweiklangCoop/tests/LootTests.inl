void TestLootProtocol(){
    namespace l=zweiklang::loot;
    l::Message m;m.op=l::Op::Open;m.request=10;m.source.kind=l::Source::Chest;m.source.name="CHEST_XARDAS";m.source.position={100,200,-300};
    l::Message decoded;auto wire=l::Encode(m);
    Check(l::Decode(wire,decoded)&&decoded.source.name==m.source.name&&decoded.source.position[2]==-300,"loot source identity codec");
    for(std::size_t i=0;i<wire.size();++i)Check(!l::Decode(Bytes(wire.begin(),wire.begin()+i),decoded),"truncated loot open rejected");
    wire.push_back(0);Check(!l::Decode(wire,decoded),"loot trailing data rejected");
    m.source.position[0]=std::numeric_limits<float>::quiet_NaN();Check(!l::Decode(l::Encode(m),decoded),"nonfinite loot identity rejected");
    m=l::Message{};m.op=l::Op::Page;m.request=1;m.lease=7;m.revision=1;
    for(unsigned i=0;i<l::PageEntries;++i)m.entries.push_back({i+1,100+i,200});
    Check(l::Encode(m).size()<=MaximumDatagramBytes-ProtocolHeaderBytes&&l::Decode(l::Encode(m),decoded)&&decoded.entries.size()==24,"full loot page fits bounded datagram");
    m.entries.push_back({25,125,1});Check(!l::Decode(l::Encode(m),decoded),"oversized loot page rejected");
    m.entries.resize(1);m.entries[0].amount=0;Check(!l::Decode(l::Encode(m),decoded),"zero-sized snapshot stock rejected");
    m=l::Message{};m.op=l::Op::Take;m.request=2;m.lease=1;m.revision=1;m.item={1,200,1};
    Check(l::Decode(l::Encode(m),decoded),"loot take codec");m.item.amount=UINT32_MAX;Check(!l::Decode(l::Encode(m),decoded),"loot amount overflow rejected");
    m.item.amount=1;m.item.token=0;Check(!l::Decode(l::Encode(m),decoded),"zero item token rejected");
    m.item.token=1;m.status=static_cast<l::Status>(100);Check(!l::Decode(l::Encode(m),decoded),"unknown result status rejected");
}
void TestLootLedger(){
    namespace l=zweiklang::loot;l::Ledger ledger;int nativeCount=10,calls=0;
    const auto lease=ledger.Open({{1,100,10},{2,101,1}});Check(lease>0&&ledger.Revision()==1,"host loot lease created");
    Check(!ledger.Open({{1,200,99}}),"second opener cannot replace active source");
    l::Message take;take.op=l::Op::Take;take.request=10;take.lease=lease;take.revision=1;take.item={1,100,4};
    auto remove=[&](const l::Entry& e,std::uint32_t n){++calls;Check(e.amount==static_cast<unsigned>(nativeCount),"host stock agrees before native removal");nativeCount-=n;return true;};
    auto result=ledger.Take(take,remove);Check(result.status==l::Status::Ok&&nativeCount==6&&ledger.Stock().at(1).amount==6&&result.revision==2,"partial take commits host count once");
    result=ledger.Take(take,remove);Check(result.status==l::Status::Ok&&nativeCount==6&&calls==1,"duplicate transaction replays result without a second item removal");
    auto conflict=take;conflict.item.amount=5;Check(ledger.Take(conflict,remove).status==l::Status::BadRequest&&calls==1,"same transaction ID cannot change quantity");
    Check(!ledger.Close(lease)&&ledger.HasUnapplied(),"unacknowledged owner grant prevents source release");
    Check(!ledger.Applied(10,lease+1)&&ledger.Applied(10,lease)&&!ledger.HasUnapplied(),"only correct application receipt confirms grant");
    take.request=11;Check(ledger.Take(take,remove).status==l::Status::Stale&&calls==1,"stale revision cannot consume current stock");
    take.request=12;take.revision=2;take.item.amount=7;Check(ledger.Take(take,remove).status==l::Status::Empty&&calls==1,"insufficient stock never invokes native removal");
    take.request=13;take.item.amount=1;take.item.instance=999;Check(ledger.Take(take,remove).status==l::Status::Empty&&calls==1,"item instance cannot be substituted for token");
    take.request=14;take.item.instance=100;
    Check(ledger.Take(take,[](const l::Entry&,std::uint32_t){return false;}).status==l::Status::NativeFailure&&ledger.Revision()==2&&ledger.Stock().at(1).amount==6,"native failure leaves authority unchanged");
    take.request=15;take.item.amount=6;result=ledger.Take(take,remove);Check(result.status==l::Status::Ok&&nativeCount==0&&ledger.Stock().at(1).amount==0,"remaining stack can be taken exactly once");
    Check(ledger.Applied(15,lease)&&ledger.Close(lease),"receipt and close release source");
    const auto next=ledger.Open({{1,200,2}});Check(next!=lease&&next>0,"new source gets a fresh lease token");
    result=ledger.Take(take,remove);Check(result.status==l::Status::Ok&&calls==2&&ledger.Stock().at(1).amount==2,"late duplicate cannot consume stock in a new lease");
    take.request=16;Check(ledger.Take(take,remove).status==l::Status::Stale,"old lease with new request is rejected");
    l::Ledger malformed;Check(!malformed.Open({{1,100,1},{1,101,1}}),"duplicate snapshot tokens rejected atomically");
}
void TestLootSnapshotAndUdp(){
    namespace l=zweiklang::loot;l::Snapshot snapshot;l::Message begin;begin.op=l::Op::Begin;begin.request=8;begin.lease=3;begin.revision=1;begin.total=2;
    Check(snapshot.Begin(begin)&&!snapshot.Complete(),"snapshot waits for all entries before engine mutation");
    auto page=begin;page.op=l::Op::Page;page.entries={{1,100,5}};
    Check(snapshot.Page(page)&&!snapshot.Complete(),"partial snapshot buffered");
    Check(!snapshot.Page(page)&&snapshot.Entries().size()==1,"duplicate page cannot duplicate inventory");
    page.index=1;Check(!snapshot.Page(page)&&snapshot.Entries().size()==1,"repeated token in later page rejected atomically");
    page.entries={{2,101,1}};Check(snapshot.Page(page)&&snapshot.Complete(),"complete consistent snapshot ready");
    begin.total=0;Check(snapshot.Begin(begin)&&snapshot.Complete(),"empty source snapshot supported");
    Network host,guest;ConnectedPair(host,guest);
    l::Message open;open.request=1;open.source.kind=l::Source::Corpse;open.source.npc=42;
    Check(guest.SendEvent(l::Encode(open),1100),"loot request uses reliable event channel");Pump(host,guest,1101);Bytes wire;l::Message read;
    Check(host.ConsumeEvent(wire)&&l::Decode(wire,read)&&read.source.npc==42,"host receives loot request with intact identity");
    Check(host.SendEvent(l::Encode(begin),1102),"snapshot response sent reliably");Pump(host,guest,1103);
    Check(guest.ConsumeEvent(wire)&&l::Decode(wire,read)&&read.op==l::Op::Begin,"guest receives snapshot header");
    Check(!guest.ConsumeEvent(wire)&&host.PendingEvents()==0,"snapshot response acknowledged and delivered once");
}
void TestLootConservation(){
    namespace l=zweiklang::loot;l::Ledger ledger;int source=1000,granted=0,calls=0;const auto lease=ledger.Open({{1,100,1000}});
    std::set<unsigned> ownerApplied;
    for(unsigned id=1;id<=80;++id){
        l::Message m;m.op=l::Op::Take;m.request=id;m.lease=lease;m.revision=ledger.Revision();m.item={1,100,1+id%7};
        auto remove=[&](const l::Entry&,unsigned count){++calls;source-=count;return true;};
        auto result=ledger.Take(m,remove);Check(result.status==l::Status::Ok,"sequential loot commit accepted");
        for(int retry=0;retry<3;++retry){auto replay=ledger.Take(m,remove);if(ownerApplied.insert(replay.request).second)granted+=replay.item.amount;}
        Check(source+granted==1000&&calls==static_cast<int>(id),"host stock plus deduplicated owner grants conserved under repeated delivery");
        Check(ledger.Applied(id,lease),"application receipt accepted during repeated transfers");
    }
    Check(ledger.Close(lease),"conserved source releases without outstanding grants");
}
