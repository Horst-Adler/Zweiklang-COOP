#include "coop/MenuArtwork.h"
void TestMenuArtwork(){
 using orebound::menuart::Eligible;
 Check(Eligible("MENU_MAIN",false,1,true,true),"verified out-of-game main menu accepts artwork");
 for(const auto* name:{"MENU_ZWEIKLANG_COOP","MENU_OPTIONS","MENU_SAVEGAME","MENU_LOADGAME","","MENU_MAIN_OTHER"})
  Check(!Eligible(name,false,1,true,true),"submenus and unknown names retain native backgrounds");
 Check(!Eligible("MENU_MAIN",true,1,true,true),"pause menu keeps the game view");
 Check(!Eligible("MENU_MAIN",false,0,true,true),"missing menu contract leaves native menu intact");
 Check(!Eligible("MENU_MAIN",false,2,true,true),"unknown menu ABI leaves native menu intact");
 Check(!Eligible("MENU_MAIN",false,1,false,true),"unknown artwork never clears the menu window");
 Check(!Eligible("MENU_MAIN",false,1,true,false),"incomplete native windows retain original state");
}
