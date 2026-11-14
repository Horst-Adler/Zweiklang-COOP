#pragma once
#include <array>
#include <string>
namespace zweiklang::extensions {
constexpr int ScriptAbi=1, NinjaVersion=4116;
enum class Module : unsigned {Quickloot,Sprint,Toolkit,PatchMenu};
enum class State {Absent,Supported,Unsafe};
struct Definition {const char* name;const char* file;const char* hash;const char* marker;};
inline const std::array<Definition,4>& Definitions(){static const std::array<Definition,4> d{{
 {"Quickloot 1.9.5 + bridge 1","Quickloot.vdf","0352BC03B6E9C53CB2D3D4DC89426073C7E45AEE9272A994C7C9E178108B772F","NINJA_QUICKLOOT_ZK_BRIDGE_REV"},
 {"Sprint 2.4.0 + bridge 1","Sprint.vdf","9D06C29735810D9D07F551F9409ADE23494E983FF6F560C157855B82B8C77967","NINJA_SPRINT_ZK_BRIDGE_REV"},
 {"Toolkit 16.0","Toolkit.vdf","08A8873043AE3E95C808E0AD519E00F302239BF58A561F3CA22EA6D00F5DC260",""},
 {"PatchMenu 1.1.1","PatchMenu.vdf","8DA288D5FCFF42EDDC2CBA99548A9C03A464E146DD9C5CFE39F7DAD2FFD08F7A",""}
 }};return d;}
struct Evidence {bool file=false,loaded=false;std::string hash;int marker=-1;};
inline State Detect(Module module,const Evidence& e){
 const auto i=static_cast<unsigned>(module);if(i>=Definitions().size())return State::Unsafe;
 if(!e.file)return e.loaded?State::Unsafe:State::Absent;
 const auto& d=Definitions()[i];if(e.hash!=d.hash)return State::Unsafe;
 if(module!=Module::Toolkit&&!e.loaded)return State::Unsafe;
 return !*d.marker||e.marker==1?State::Supported:State::Unsafe;
}
struct Profile {
 std::array<State,4> modules{};
 bool AllowsSession()const{for(auto s:modules)if(s==State::Unsafe)return false;return !(IsActive(Module::Quickloot)||IsActive(Module::Sprint)||IsActive(Module::PatchMenu))||IsActive(Module::Toolkit);}
 bool IsActive(Module m)const{return modules[static_cast<unsigned>(m)]==State::Supported;}
};
enum class Command : int {Probe=0,PrepareTrade=1,PrepareCorpse=2,QuestXp=3};
inline bool ValidCommand(int cmd,int value){return cmd>=0&&cmd<=3&&(cmd==3?(value>0&&value<=1000000):value==0);}
inline bool CoreReady(int ninja,int abi,bool dispatch,bool command,bool argument,bool result,bool coreHash){return ninja==NinjaVersion&&abi==ScriptAbi&&dispatch&&command&&argument&&result&&coreHash;}
}
