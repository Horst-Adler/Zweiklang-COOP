#pragma once
#include <string>
namespace orebound::menuart {
// The native root window already spans the entire screen. Keep its child menu
// geometry, alpha, items and logos unchanged; only replace the two backgrounds.
inline bool Eligible(const std::string& name,bool inGame,int menuAbi,bool archiveVerified,bool windowsReady){
 return name=="MENU_MAIN"&&!inGame&&menuAbi==1&&archiveVerified&&windowsReady;
}
}
