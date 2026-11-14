#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "coop/Settings.h"
#include <cstdlib>
namespace orebound {
bool ValidName(const std::string& name) {
    if(name.empty()||name.size()>32)return false;
    for(unsigned char c:name)if(c<32||c==127)return false;
    return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.c_str(),static_cast<int>(name.size()),nullptr,0)>0;
}
bool ValidHost(const std::string& host) { IN_ADDR address{};return InetPtonA(AF_INET,host.c_str(),&address)==1 && address.S_un.S_addr!=INADDR_ANY && address.S_un.S_addr!=INADDR_BROADCAST; }
bool ParsePort(const std::string& text,unsigned short& result) {
    if(text.empty()||text.size()>5)return false;
    for(char c:text)if(c<'0'||c>'9')return false;
    const auto number=strtoul(text.c_str(),nullptr,10);
    if(number<1024||number>65535)return false;result=static_cast<unsigned short>(number);return true;
}
}
