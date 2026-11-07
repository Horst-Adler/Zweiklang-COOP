namespace recovery_test {
namespace rc=zweiklang::recovery;
rc::Tx Transaction(unsigned id=1,rc::Action action=rc::Action::Take){
    rc::Tx t;t.id=std::string(32,'0');const char* hex="0123456789abcdef";t.id[30]=hex[(id>>4)&15];t.id[31]=hex[id&15];t.hostTimeline=std::string(32,'a');t.ownerTimeline=std::string(32,'b');t.world="NEWWORLD.ZEN";t.source.kind=zweiklang::loot::Source::Chest;t.source.name="XARDAS_CHEST";t.source.position={100,200,300};t.action=action;t.item=100;t.amount=3;if(action==rc::Action::Unlock)t.item=t.amount=0;return t;
}
std::string Temp(){char path[MAX_PATH];GetTempPathA(MAX_PATH,path);const auto dir=std::string(path)+"ZweiklangRecoveryTests-"+rc::NewGuid();Check(CreateDirectoryA(dir.c_str(),nullptr)!=FALSE,"create isolated recovery test directory");return dir;}
}
void TestRecoveryWire(){
    namespace rc=zweiklang::recovery;rc::Message m;m.op=rc::Op::Request;m.tx=recovery_test::Transaction();m.lease=12;m.revision=4;m.token=2;rc::Message decoded;
    auto bytes=rc::Encode(m);Check(rc::Decode(bytes,decoded)&&rc::Same(m.tx,decoded.tx)&&decoded.lease==12,"persistent transaction wire roundtrip");
    for(std::size_t n=0;n<bytes.size();++n)Check(!rc::Decode(Bytes(bytes.begin(),bytes.begin()+n),decoded),"truncated recovery request rejected");
    bytes.push_back(0);Check(!rc::Decode(bytes,decoded),"recovery trailing bytes rejected");
    m.tx.source.name=std::string(63,'C');m.tx.world=std::string(63,'W');bytes=rc::Encode(m);
    Check(bytes.size()<=MaximumDatagramBytes-ProtocolHeaderBytes&&rc::Decode(bytes,decoded),"largest supported recovery identity fits datagram");
    m.tx.amount=UINT32_MAX;Check(!rc::Decode(rc::Encode(m),decoded),"recovery quantity overflow rejected");
    m.tx=recovery_test::Transaction();m.tx.id="../bad";Check(!rc::Decode(rc::Encode(m),decoded),"transaction ID must be fixed hexadecimal GUID");
    m.op=rc::Op::Hello;m.tx.hostTimeline=std::string(32,'a');Check(rc::Decode(rc::Encode(m),decoded),"host timeline hello codec");
    m.op=rc::Op::LockState;m.tx.world="NEWWORLD.ZEN";m.tx.source.name="CHEST";m.locked=true;Check(rc::Decode(rc::Encode(m),decoded)&&decoded.locked,"host lock state codec");
}
void TestRecoveryStore(){
    namespace rc=zweiklang::recovery;const auto dir=recovery_test::Temp(),path=dir+"\\transactions.bin";
    rc::Store store;store.Open(path);const auto profile=store.Profile();Check(rc::Guid(profile),"durable profile identity generated");
    auto tx=recovery_test::Transaction();Check(store.Put(tx,rc::Role::Owner),"owner prepare persisted before effect");
    rc::Store restart;restart.Open(path);Check(restart.Profile()==profile&&restart.Find(tx.id)&&restart.Find(tx.id)->tx.decision==rc::Decision::Prepared,"prepare survives process restart");
    auto conflicting=tx;conflicting.amount=4;Check(!restart.Put(conflicting,rc::Role::Owner),"persistent ID cannot change quantity");
    tx.decision=rc::Decision::Commit;Check(restart.Put(tx,rc::Role::Owner),"prepared transaction can commit durably");
    tx.decision=rc::Decision::Abort;Check(!restart.Put(tx,rc::Role::Owner),"committed decision cannot be reversed after lost acknowledgement");
    rc::Store again;again.Open(path);Check(again.Find(tx.id)->tx.decision==rc::Decision::Commit,"terminal decision survives another restart");
    auto second=recovery_test::Transaction(2,rc::Action::Put);Check(again.Put(second,rc::Role::Owner),"another transaction appended");
    auto ordered=again.Ordered();Check(ordered.size()==2&&ordered[0].tx.id==tx.id&&ordered[1].tx.id==second.id,"replay preserves insertion order rather than random GUID order");
    auto raw=rc::ReadFile(path);raw[raw.size()/2]^=1;rc::AtomicWrite(path,raw);bool rejected=false;try{rc::Store corrupt;corrupt.Open(path);}catch(const std::exception&){rejected=true;}Check(rejected,"corrupt durable store fails closed instead of creating an empty authority");
    DeleteFileA(path.c_str());RemoveDirectoryA(dir.c_str());
}
void TestRecoveryCheckpoints(){
    namespace rc=zweiklang::recovery;rc::Checkpoint c;c.profile=std::string(32,'c');c.timeline=std::string(32,'d');c.fingerprint="save-123";c.effects.insert(recovery_test::Transaction().id);
    auto decoded=rc::DecodeCheckpoint(rc::Encode(c));Check(decoded.effects==c.effects&&decoded.timeline==c.timeline&&decoded.fingerprint==c.fingerprint,"save checkpoint records exactly represented transaction effects");
    auto b=rc::Encode(c);b.resize(b.size()-1);bool rejected=false;try{rc::DecodeCheckpoint(b);}catch(const std::exception&){rejected=true;}Check(rejected,"truncated save checkpoint is not trusted");
}
void TestRecoveryCrashMatrix(){
    namespace rc=zweiklang::recovery;
    for(auto action:{rc::Action::Take,rc::Action::Put})for(auto decision:{rc::Decision::Commit,rc::Decision::Abort})for(int hostSaved=0;hostSaved<2;++hostSaved)for(int ownerSaved=0;ownerSaved<2;++ownerSaved){
        auto tx=recovery_test::Transaction(1,action);tx.decision=decision;
        rc::Record host{tx,rc::Role::Host,1},owner{tx,rc::Role::Owner,1};
        // Owner saved while a deposit was only prepared, or while a withdrawal
        // was already granted. Host may have saved the commit independently.
        const bool hostEffect=hostSaved&&decision==rc::Decision::Commit;
        const bool ownerEffect=ownerSaved&&(action==rc::Action::Put||decision==rc::Decision::Commit);
        const int sign=action==rc::Action::Take?-1:1;
        int source=10+(hostEffect?sign*3:0),inventory=5-(ownerEffect?sign*3:0),calls=0;
        std::set<std::string> hostCheckpoint,ownerCheckpoint;
        if(hostEffect)hostCheckpoint.insert(tx.id);if(ownerEffect)ownerCheckpoint.insert(tx.id);
        auto sourceApply=[&](const rc::Record&,bool desired){source+=(desired?1:-1)*sign*3;++calls;return true;};
        auto ownerApply=[&](const rc::Record&,bool desired){inventory-=(desired?1:-1)*sign*3;++calls;return true;};
        Check(rc::Reconcile(host,hostCheckpoint,sourceApply)&&rc::Reconcile(owner,ownerCheckpoint,ownerApply),"crash recovery reconciles independent host and owner save checkpoints");
        const int delta=decision==rc::Decision::Commit?sign*3:0;
        Check(source==10+delta&&inventory==5-delta&&source+inventory==15,"commit/abort crash matrix conserves quantities");
        const auto previous=calls;
        for(int retry=0;retry<4;++retry){rc::Reconcile(host,hostCheckpoint,sourceApply);rc::Reconcile(owner,ownerCheckpoint,ownerApply);}
        Check(calls==previous&&source+inventory==15,"lost responses and application acknowledgements cannot reapply native changes");
    }
    auto deposit=recovery_test::Transaction(3,rc::Action::Put);rc::Record reservation{deposit,rc::Role::Owner,1};std::set<std::string> effects;int inventory=5;
    auto apply=[&](const rc::Record&,bool desired){inventory+=desired?-3:3;return true;};
    Check(rc::Reconcile(reservation,effects,apply)&&inventory==2,"prepared deposit reserves items before host confirmation");
    reservation.tx.decision=rc::Decision::Abort;Check(rc::Reconcile(reservation,effects,apply)&&inventory==5,"host abort returns escrow exactly once");
    rc::Reconcile(reservation,effects,apply);Check(inventory==5,"repeated abort does not duplicate returned escrow");
    auto unlock=recovery_test::Transaction(4,rc::Action::Unlock);rc::Record lock{unlock,rc::Role::Owner,1};effects.clear();bool locked=true;
    Check(rc::Reconcile(lock,effects,[&](const rc::Record&,bool desired){locked=!desired;return true;})&&!locked,"native unlock can be provisionally represented");
    lock.tx.decision=rc::Decision::Abort;Check(rc::Reconcile(lock,effects,[&](const rc::Record&,bool desired){locked=!desired;return true;})&&locked,"denied unlock restores shared lock state");
    effects.clear();deposit.decision=rc::Decision::Commit;rc::Record unavailable{deposit,rc::Role::Owner,1};
    Check(!rc::Reconcile(unavailable,effects,[](const rc::Record&,bool){return false;})&&effects.empty(),"unavailable native inventory is deferred, never marked as applied");
}
void TestDepositLedger(){
    namespace lo=zweiklang::loot;lo::Ledger ledger;const auto lease=ledger.Open({{1,100,5}});int native=5;
    lo::Message put;put.request=1;put.lease=lease;put.revision=1;put.item={1,100,3};
    auto result=ledger.Put(put,[&](const lo::Entry&,unsigned amount){native+=amount;return true;});
    Check(result.status==lo::Status::Ok&&ledger.Stock().at(1).amount==8&&native==8,"deposit advances source stock and revision");
    Check(!ledger.Close(lease)&&ledger.Applied(1,lease),"deposit source remains exclusive until application receipt");
    put.request=2;put.revision=2;put.item={1,200,1};
    result=ledger.Put(put,[](const lo::Entry&,unsigned){return true;});Check(result.status==lo::Status::Ok&&ledger.Stock().size()==2&&result.item.token==2,"new item instance receives a source token");
}
