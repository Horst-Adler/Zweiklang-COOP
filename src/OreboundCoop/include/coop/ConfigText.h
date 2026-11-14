#pragma once
#include <windows.h>
#include <string>
#include <vector>
namespace orebound::config_text {
inline bool Utf16(const std::string& path){HANDLE f=CreateFileA(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);if(f==INVALID_HANDLE_VALUE)return false;unsigned char bytes[2]{};DWORD read=0;const bool ok=ReadFile(f,bytes,2,&read,nullptr)!=FALSE;CloseHandle(f);return ok&&read==2&&bytes[0]==255&&bytes[1]==254;}
inline std::wstring Wide(const std::string& text,UINT page){const int n=MultiByteToWideChar(page,0,text.c_str(),-1,nullptr,0);if(n<=0)return {};std::vector<wchar_t> b(n);MultiByteToWideChar(page,0,text.c_str(),-1,b.data(),n);return b.data();}
inline std::string Utf8(const wchar_t* text){const int n=WideCharToMultiByte(CP_UTF8,0,text,-1,nullptr,0,nullptr,nullptr);if(n<=0)return {};std::string out(n,'\0');WideCharToMultiByte(CP_UTF8,0,text,-1,&out[0],n,nullptr,nullptr);out.pop_back();return out;}
inline std::string ReadName(const std::string& path,const char* fallback){if(Utf16(path)){wchar_t value[1024]{};const auto file=Wide(path,CP_ACP),initial=Wide(fallback,CP_UTF8);GetPrivateProfileStringW(L"Player",L"Name",initial.c_str(),value,1024,file.c_str());return Utf8(value);}char value[1024]{};GetPrivateProfileStringA("Player","Name",fallback,value,sizeof(value),path.c_str());if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value,-1,nullptr,0))return Utf8(Wide(value,CP_ACP).c_str());return value;}
inline bool WriteName(const std::string& path,const std::string& utf8){if(Utf16(path)){const auto file=Wide(path,CP_ACP),value=Wide(utf8,CP_UTF8);return WritePrivateProfileStringW(L"Player",L"Name",value.c_str(),file.c_str())!=FALSE;}return WritePrivateProfileStringA("Player","Name",utf8.c_str(),path.c_str())!=FALSE;}
}
