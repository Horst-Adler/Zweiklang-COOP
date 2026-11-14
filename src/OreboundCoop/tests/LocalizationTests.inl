void TestLocalization(){
 namespace l=orebound::i18n;
 Check(l::Get()==l::Language::English,"fresh public process uses English");
 for(const auto& e:l::Catalogue){
  Check(l::Find(e.id)==&e,"every catalogue key resolves exactly");
  Check(*l::Text(e.id,l::Language::English)&&*l::Text(e.id,l::Language::German),"English and German entries are complete");
  Check(std::string(e.en).find("Zweiklang")==std::string::npos&&std::string(e.de).find("Zweiklang")==std::string::npos,"visible catalogue uses the current brand");
  for(auto lang:{l::Language::English,l::Language::German}){l::Set(lang);Check(std::string(l::Native(e.id))==l::ToNative(l::Text(e.id,lang)),"UTF-8 catalogue converts to native font encoding in either language");}
 }
 Check(l::Parse("de")==l::Language::German&&l::Parse("Deutsch")==l::Language::German&&l::Parse("EN")==l::Language::English&&l::Parse("unknown")==l::Language::English,"locale parsing has explicit English fallback");
 int missing=0;l::missingSink=[&](const std::string& key){if(key=="missing.test")++missing;};Check(std::string(l::Text("missing.test",l::Language::German))=="Translation unavailable"&&missing==1,"missing key logs and returns a safe English fallback");l::missingSink={};
 for(const auto& menu:l::MenuCatalogue)Check(l::Find(menu.id)!=nullptr&&menu.index>=0&&menu.index<=1,"all menu labels/tooltips refer to valid text keys");
 const std::string player="Connected: guest joined Horst";l::Set(l::Language::German);Check(l::Status(player)==player,"status localization never replaces substrings inside player data");Check(l::Status("Connected: guest joined")=="Verbunden: Client beigetreten","known whole network status translates locally");
 for(auto host:{l::Language::English,l::Language::German})for(auto guest:{l::Language::English,l::Language::German}){
  Network a,b;ConnectedPair(a,b);l::Set(host);const auto hostText=l::Status(a.LastStatus());l::Set(guest);const auto guestText=l::Status(b.LastStatus());Check(!hostText.empty()&&!guestText.empty()&&a.GetState()==State::Connected&&b.GetState()==State::Connected,"all four host/client UI locale combinations preserve the actual connection");
 }
 {
  namespace c=orebound::config_text;namespace rc=orebound::recovery;
  const auto directory=recovery_test::Temp();const auto file=directory+"\\profile.ini";
  const auto name=std::string(u8"M\u00fcller");const auto wide=c::Wide("[Player]\r\nName="+name+"\r\n",CP_UTF8);orebound::combat::Bytes bytes{255,254};const auto* raw=reinterpret_cast<const unsigned char*>(wide.data());bytes.insert(bytes.end(),raw,raw+wide.size()*sizeof(wchar_t));rc::AtomicWrite(file,bytes);
  Check(c::ReadName(file,"Player")==name,"UTF-16 legacy name reads as UTF-8 without mojibake");Check(c::WriteName(file,u8"G\u00fcnther")&&c::ReadName(file,"Player")==u8"G\u00fcnther","UTF-16 name edit keeps native encoding and exact characters");
  const std::string utf8="[Player]\r\nName="+name+"\r\n";const auto plain=directory+"\\utf8.ini";rc::AtomicWrite(plain,orebound::combat::Bytes(utf8.begin(),utf8.end()));Check(c::ReadName(plain,"Player")==name,"existing UTF-8 names remain unchanged");Check(c::WriteName(plain,u8"G\u00fcnther")&&c::ReadName(plain,"Player")==u8"G\u00fcnther","UTF-8 name edit round trips");
  const auto ansi=directory+"\\ansi.ini";const auto native=orebound::i18n::ToNative(utf8.c_str());rc::AtomicWrite(ansi,orebound::combat::Bytes(native.begin(),native.end()));Check(c::ReadName(ansi,"Player")==name,"legacy native-codepage name upgrades to valid UTF-8");std::filesystem::remove_all(directory);
 }
 l::Set(l::Language::English);
}
