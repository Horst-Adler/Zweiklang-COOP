namespace ext=extensions;
ext::Profile extensionProfile;
bool extensionChecked=false,extensionCoreValid=false,extensionCalling=false;
std::string extensionError;
bool ExtensionFunction(const char* name){auto* s=parser?parser->GetSymbol(zSTRING(name)):nullptr;return s&&s->type==5&&s->ele==0;}
bool ExtensionInteger(const char* name){auto* s=parser?parser->GetSymbol(zSTRING(name)):nullptr;return s&&s->type==2&&s->ele==1;}
void ExtensionReset(){extensionProfile={};extensionChecked=extensionCoreValid=false;extensionError.clear();}
bool ExtensionEnabled(ext::Module module){return extensionChecked&&extensionCoreValid&&extensionProfile.AllowsSession()&&extensionProfile.IsActive(module);}
void ExtensionCall(ext::Command command,int value){
 if(!extensionChecked&&!ExtensionValidate())throw std::runtime_error(extensionError);
 if(!parser||!extensionCoreValid||extensionCalling||!ext::ValidCommand(static_cast<int>(command),value))throw std::runtime_error("Ninja bridge unavailable/reentrant or invalid command");
 const char* names[]={"Patch_ZweiklangCore_Command","Patch_ZweiklangCore_Value","Patch_ZweiklangCore_Result"};
 const int old[]={QuestInt(names[0]),QuestInt(names[1]),QuestInt(names[2])};
 struct Restore {const char** names;const int* old;Restore(const char** n,const int* v):names(n),old(v){extensionCalling=true;}~Restore(){for(int i=0;i<3;++i)QuestSetInt(names[i],old[i]);extensionCalling=false;}} restore(names,old);
 if(!QuestSetInt(names[0],static_cast<int>(command))||!QuestSetInt(names[1],value)||!QuestSetInt(names[2],0))throw std::runtime_error("Ninja bridge variables missing");
 QuestBindings bindings;parser->CallFunc(parser->GetIndex(zSTRING("Patch_ZweiklangCore_Dispatch")));
 if(QuestInt(names[2])!=1)throw std::runtime_error("Ninja bridge rejected script command");
 diagnostics.Write(2,"ninja_command","abi=2 command="+std::to_string(static_cast<int>(command))+" value="+std::to_string(value));
}
bool ExtensionValidate(){
 ExtensionReset();extensionChecked=true;
 try{
  const auto base=systemDirectory+"\\..\\Data\\";
  extensionCoreValid=ext::CoreReady(QuestInt("NINJA_VERSION"),QuestInt("Patch_ZweiklangCore_ABI"),
   ExtensionFunction("Patch_ZweiklangCore_Dispatch"),ExtensionInteger("Patch_ZweiklangCore_Command"),ExtensionInteger("Patch_ZweiklangCore_Value"),ExtensionInteger("Patch_ZweiklangCore_Result"),
   ext::FileSha256(base+"OreboundCoop_Core.vdf")==ext::CoreArchiveHash&&ext::FileSha256(systemDirectory+"\\Ninja.dll")=="38CE2836A74227BEAF08C6972CD263389BC3D6F591BFF35AE812C83C6241CA8B");
  if(!extensionCoreValid)throw std::runtime_error("Ninja 4.1.16 / Orebound Core ABI 2 is missing or incompatible");
  ExtensionCall(ext::Command::Probe);
  const char* ids[]={"NINJA_ID_QUICKLOOT","NINJA_ID_SPRINT","NINJA_ID_TOOLKIT","NINJA_ID_PATCHMENU"};
  for(unsigned i=0;i<4;++i){const auto& d=ext::Definitions()[i];const std::string path=base+d.file;ext::Evidence evidence;
   const DWORD attributes=GetFileAttributesA(path.c_str());evidence.file=attributes!=INVALID_FILE_ATTRIBUTES;
   if(!evidence.file&&GetLastError()!=ERROR_FILE_NOT_FOUND&&GetLastError()!=ERROR_PATH_NOT_FOUND)throw std::runtime_error(std::string("Cannot read optional archive: ")+d.file);
   evidence.loaded=QuestInt(ids[i],-1)>=0;evidence.marker=*d.marker?QuestInt(d.marker,-1):-1;if(evidence.file)evidence.hash=ext::FileSha256(path);
   extensionProfile.modules[i]=ext::Detect(static_cast<ext::Module>(i),evidence);
   diagnostics.Write(1,"extension_profile",std::string("module=")+d.name+" state="+(extensionProfile.modules[i]==ext::State::Absent?"absent":extensionProfile.modules[i]==ext::State::Supported?"supported":"unsafe")+" parsed="+std::to_string(evidence.loaded)+" bridge="+std::to_string(evidence.marker));
   if(extensionProfile.modules[i]==ext::State::Unsafe)throw std::runtime_error(std::string("Unsupported or unloaded optional archive: ")+d.file+"; remove it or install the documented profile");
  }
  if(!extensionProfile.AllowsSession())throw std::runtime_error("Active comfort modules require Toolkit 16.0; disable all four comfort archives for the minimal profile");
  diagnostics.Write(1,"ninja_bridge","abi=2 ninja=4.1.16 validated; Union owns networking/recovery; optional modules detected");return true;
 }catch(const std::exception& e){extensionCoreValid=false;extensionError=e.what();notice=Ui("ui_e8da68b3aa14")+extensionError;Log(notice);return false;}
}

void ExtensionApplyLanguage(){
 ExtensionCall(ext::Command::ApplyLanguage,i18n::Get()==i18n::Language::German?1:0);
 const char* source[]={"Patch_ZweiklangCore_CavalornActive","Patch_ZweiklangCore_CavalornDone","Patch_ZweiklangCore_CavalornLetter"};
 const char* target[]={"ZK_COOP_CAV_TEXT_ACTIVE","ZK_COOP_CAV_TEXT_DONE","ZK_COOP_CAV_TEXT_LETTER"};
 for(int i=0;i<3;++i){auto* from=parser->GetSymbol(zSTRING(source[i]));auto* to=parser->GetSymbol(zSTRING(target[i]));if(!from||!to||from->type!=3||to->type!=3||from->ele!=1||to->ele!=1)throw std::runtime_error("Localized Ninja journal helper missing or invalid");zSTRING value;from->GetValue(value,0);to->SetValue(value,0);}
 diagnostics.Write(2,"script_language",std::string("abi=2 locale=")+i18n::Code(i18n::Get()));
}
