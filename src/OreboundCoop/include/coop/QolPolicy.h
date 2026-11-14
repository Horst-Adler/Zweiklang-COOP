#pragma once
#include <string>
#include <cstdint>
namespace orebound::qol {
struct SessionLatch {bool used=false;bool Update(bool active){used=used||active;return used;}void Reset(){used=false;}};
inline bool BlockUpstream(SessionLatch& latch,bool active,bool recoveryFault){return latch.Update(active)||recoveryFault;}
template<class T> class Hold {
    T* value;
public:
    explicit Hold(T* item):value(item){value->AddRef();}
    ~Hold(){value->Release();}
    Hold(const Hold&)=delete;Hold& operator=(const Hold&)=delete;
};
inline const char* SprintOverlay(unsigned value){
    switch(value){case 1:return "NINJA_SPRINT_DEFAULT.MDS";case 2:return "NINJA_SPRINT_ALTERNATIVE.MDS";case 3:return "NINJA_SPRINT_ALTERNATIVE_FAST.MDS";case 4:return "NINJA_SPRINT_SHEEP.MDS";case 5:return "HUMANS_SPRINT.MDS";default:return "";}
}
inline bool BodyAllowsLoot(unsigned body){body&=31;return body<=5||body==7;}
struct Context {bool connected=false,ready=false,menu=false,dialog=false,inventory=false,interaction=false,alive=false;unsigned weapon=0,body=0;};
inline bool CanLoot(const Context& c){return c.connected&&c.ready&&!c.menu&&!c.dialog&&!c.inventory&&!c.interaction&&c.alive&&!c.weapon&&BodyAllowsLoot(c.body);}
inline bool Due(std::uint64_t now,std::uint64_t last){return !last||now-last>=120;}
inline bool QuickItem(bool eligible,unsigned mainFlag,unsigned flags,bool corpse=false){return eligible&&(!corpse||!(mainFlag&16))&&!(flags&((1u<<12)|(1u<<30)));}
}
