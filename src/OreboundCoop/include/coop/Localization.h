#pragma once
#include <windows.h>
#include <atomic>
#include <string>
#include <vector>
#include <array>
#include <functional>
#include <algorithm>
#include "coop/LocalizationCatalogue.h"
namespace orebound::i18n {
enum class Language : unsigned {English=0,German=1};
inline std::atomic<Language> current{Language::English};
inline std::function<void(const std::string&)> missingSink;
inline Language Parse(std::string value){std::transform(value.begin(),value.end(),value.begin(),[](unsigned char c){return static_cast<char>(c>='A'&&c<='Z'?c+32:c);});return value=="de"||value=="deutsch"||value=="german"?Language::German:Language::English;}
inline const char* Code(Language language){return language==Language::German?"de":"en";}
inline void Set(Language language){current.store(language);}
inline Language Get(){return current.load();}
inline const Entry* Find(const std::string& id){for(const auto& entry:Catalogue)if(id==entry.id)return &entry;return nullptr;}
inline const char* Text(const std::string& id,Language language){const auto* e=Find(id);if(!e){if(missingSink)missingSink(id);return "Translation unavailable";}return language==Language::German&&*e->de?e->de:e->en;}
inline std::string ToNative(const char* utf8){
 const int size=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8,-1,nullptr,0);if(size<=0)return "Translation unavailable";
 std::vector<wchar_t> wide(size);MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,utf8,-1,wide.data(),size);
 const int bytes=WideCharToMultiByte(CP_ACP,0,wide.data(),-1,nullptr,0,nullptr,nullptr);if(bytes<=0)return "Translation unavailable";
 std::string out(bytes,'\0');WideCharToMultiByte(CP_ACP,0,wide.data(),-1,&out[0],bytes,nullptr,nullptr);out.pop_back();return out;
}
inline const char* Native(const std::string& id){
 static const auto texts=[](){std::vector<std::array<std::string,2>> out;for(const auto& e:Catalogue)out.push_back({ToNative(e.en),ToNative(*e.de?e.de:e.en)});return out;}();
 for(std::size_t i=0;i<sizeof(Catalogue)/sizeof(Catalogue[0]);++i)if(id==Catalogue[i].id)return texts[i][Get()==Language::German?1:0].c_str();
 if(missingSink)missingSink(id);return "Translation unavailable";
}
// Error details are technical English. Only known whole status messages are translated;
// player names, item descriptions and remote data never undergo substring replacement.
inline std::string Status(const std::string& english){for(const auto& e:Catalogue)if(english==e.en)return Native(e.id);return english;}
}
namespace orebound {inline const char* Ui(const char* id){return i18n::Native(id);}}
