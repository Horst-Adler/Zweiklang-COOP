#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <string>
#include <vector>
#pragma comment(lib,"Iphlpapi.lib")
namespace orebound {
struct LocalIPv4 {
    std::string address;
    unsigned long interfaceIndex=0,metric=0;
};
inline bool UsableLocalIPv4(const std::string& value) {
    IN_ADDR a{};
    if(InetPtonA(AF_INET,value.c_str(),&a)!=1)return false;
    const auto ip=ntohl(a.s_addr);
    return ip!=0&&(ip>>24)!=127&&(ip>>16)!=0xA9FE&&(ip>>24)<224;
}
inline std::string SelectPrimaryIPv4(const std::vector<LocalIPv4>& addresses,unsigned long preferred) {
    const LocalIPv4* best=nullptr;
    for(const auto& candidate:addresses){
        if(!UsableLocalIPv4(candidate.address))continue;
        const bool isPreferred=preferred&&candidate.interfaceIndex==preferred;
        const bool bestPreferred=best&&preferred&&best->interfaceIndex==preferred;
        if(!best||(isPreferred&&!bestPreferred)||(isPreferred==bestPreferred&&candidate.metric<best->metric))best=&candidate;
    }
    return best?best->address:std::string{};
}
inline std::string PrimaryIPv4() {
    // This reads Windows' routing table; it sends no traffic to this address.
    DWORD preferred=0;
    if(GetBestInterface(htonl(0x01010101),&preferred)!=NO_ERROR)preferred=0;
    ULONG size=16384;std::vector<unsigned char> buffer(size);
    ULONG result=ERROR_BUFFER_OVERFLOW;
    for(int attempt=0;attempt<3&&result==ERROR_BUFFER_OVERFLOW;++attempt){
        buffer.resize(size);
        result=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_ANYCAST|GAA_FLAG_SKIP_MULTICAST|GAA_FLAG_SKIP_DNS_SERVER,nullptr,
            reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()),&size);
    }
    if(result!=NO_ERROR)return {};
    std::vector<LocalIPv4> addresses;
    for(auto* a=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());a;a=a->Next){
        if(a->OperStatus!=IfOperStatusUp||a->IfType==IF_TYPE_SOFTWARE_LOOPBACK)continue;
        for(auto* u=a->FirstUnicastAddress;u;u=u->Next){
            if(!u->Address.lpSockaddr||u->Address.lpSockaddr->sa_family!=AF_INET||u->DadState!=IpDadStatePreferred)continue;
            auto* ip=reinterpret_cast<sockaddr_in*>(u->Address.lpSockaddr);
            char text[INET_ADDRSTRLEN]{};
            if(InetNtopA(AF_INET,&ip->sin_addr,text,sizeof(text)))addresses.push_back({text,a->IfIndex,a->Ipv4Metric});
        }
    }
    return SelectPrimaryIPv4(addresses,preferred);
}
}
