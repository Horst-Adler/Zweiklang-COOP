struct MenuArtworkState {
 zCMenu* menu=nullptr;zCView* outer=nullptr;zCView* inner=nullptr;
 zCTexture* originalOuter=nullptr;zCTexture* originalInner=nullptr;
};
MenuArtworkState menuArtwork;
void ReleaseMenuArtworkCopies(){
 if(menuArtwork.originalOuter)menuArtwork.originalOuter->Release();
 if(menuArtwork.originalInner)menuArtwork.originalInner->Release();
 menuArtwork=MenuArtworkState{};
}
void RestoreMenuArtwork(zCMenu* menu){
 if(!menu||menuArtwork.menu!=menu)return;
 // Called before native ScreenDone: both views are still owned by this menu.
 menuArtwork.outer->InsertBack(menuArtwork.originalOuter);
 menuArtwork.inner->InsertBack(menuArtwork.originalInner);
 ReleaseMenuArtworkCopies();
 diagnostics.Write(1,"menu_artwork_restore","menu=MENU_MAIN native_backgrounds_restored=1");
}
void ApplyMenuArtwork(zCMenu* menu){
 if(!menu||menuArtwork.menu||systemDirectory.empty()||zCMenu::inGameMenu||_stricmp(menu->GetName().ToChar(),"MENU_MAIN")!=0)return;
 if(ReadIni("Interface","MenuArtwork","1")=="0")return;
 try{
  auto* menuParser=zCMenu::GetParser();if(!menuParser)return;
  auto* abi=menuParser->GetSymbol(zSTRING("Patch_ZweiklangCore_MenuABI"));
  auto* image=menuParser->GetSymbol(zSTRING("Patch_ZweiklangCore_MenuArtwork"));
  if(!abi||!image||abi->type!=2||abi->ele!=1||image->type!=3||image->ele!=1)throw std::runtime_error("Ninja menu contract unavailable");
  int version=0;abi->GetValue(version,0);zSTRING texture;image->GetValue(texture,0);
  if(version!=menuart::MenuABI||std::string(texture.ToChar())!=menuart::TextureName)throw std::runtime_error("Ninja menu contract mismatch");
  const auto path=systemDirectory+"\\..\\Data\\OreboundCoop_Menu.vdf";
  if(GetFileAttributesA(path.c_str())==INVALID_FILE_ATTRIBUTES)return;
  const bool verified=extensions::FileSha256(path)==menuart::ArchiveHash;
  if(!menuart::Eligible(menu->GetName().ToChar(),zCMenu::inGameMenu!=0,version,verified,menu->m_pWindow&&menu->m_pInnerWindow))throw std::runtime_error("Artwork hash or native windows invalid");
  auto* outer=menu->m_pWindow;auto* inner=menu->m_pInnerWindow;
  MenuArtworkState state;state.menu=menu;state.outer=outer;state.inner=inner;
  state.originalOuter=outer->backTex;state.originalInner=inner->backTex;
  if(state.originalOuter)state.originalOuter->AddRef();
  if(state.originalInner)state.originalInner->AddRef();
  menuArtwork=state;
  outer->InsertBack(texture);
  if(!outer->backTex){RestoreMenuArtwork(menu);throw std::runtime_error("Artwork texture could not be loaded");}
  inner->InsertBack(static_cast<zCTexture*>(nullptr));
  diagnostics.Write(1,"menu_artwork_apply",std::string("menu=MENU_MAIN mode=outgame menu_abi=1 texture=")+menuart::TextureName+" logos=native render=native_root_window");
 }catch(const std::exception& error){RestoreMenuArtwork(menu);diagnostics.Write(1,"menu_artwork_fallback",error.what());}
}
