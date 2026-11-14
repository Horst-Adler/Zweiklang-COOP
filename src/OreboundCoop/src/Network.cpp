#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "coop/Network.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <random>
#include <vector>
#include <map>
#include <deque>

#pragma comment(lib, "Ws2_32.lib")

namespace orebound { namespace net {
namespace {
using Bytes = std::vector<std::uint8_t>;
void Put16(Bytes& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value >> 8));
    out.push_back(static_cast<std::uint8_t>(value));
}
void Put32(Bytes& out, std::uint32_t value) {
    Put16(out, static_cast<std::uint16_t>(value >> 16));
    Put16(out, static_cast<std::uint16_t>(value));
}
void Put64(Bytes& out, std::uint64_t value) {
    Put32(out, static_cast<std::uint32_t>(value >> 32));
    Put32(out, static_cast<std::uint32_t>(value));
}
std::uint16_t Read16(const std::uint8_t* p) {
    return static_cast<std::uint16_t>((p[0] << 8) | p[1]);
}
std::uint32_t Read32(const std::uint8_t* p) {
    return (static_cast<std::uint32_t>(Read16(p)) << 16) | Read16(p + 2);
}
std::uint64_t Read64(const std::uint8_t* p) {
    return (static_cast<std::uint64_t>(Read32(p)) << 32) | Read32(p + 4);
}
void PutFloat(Bytes& out, float value) {
    std::uint32_t bits;
    static_assert(sizeof(value) == sizeof(bits), "32-bit float required");
    std::memcpy(&bits, &value, sizeof(bits));
    Put32(out, bits);
}
float ReadFloat(const std::uint8_t* p) {
    const std::uint32_t bits = Read32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
bool ValidSnapshot(const Snapshot& s) {
    return std::isfinite(s.x) && std::isfinite(s.y) && std::isfinite(s.z) && std::isfinite(s.yaw)
        && std::abs(s.x) <= 10000000.f && std::abs(s.y) <= 10000000.f
        && std::abs(s.z) <= 10000000.f && std::abs(s.yaw) <= 1000000.f;
}
bool SameAddress(const sockaddr_in& a, const sockaddr_in& b) {
    return a.sin_family == b.sin_family && a.sin_port == b.sin_port
        && a.sin_addr.s_addr == b.sin_addr.s_addr;
}
bool Newer(std::uint32_t value, std::uint32_t previous) {
    const std::uint32_t difference = value - previous;
    return value != 0 && difference != 0 && difference < 0x80000000u;
}
std::uint64_t RandomToken() {
    std::random_device random;
    std::uint64_t result = (static_cast<std::uint64_t>(random()) << 32) ^ random();
    return result ? result : 1;
}
Bytes CompatibilityPayload(const Compatibility& c) {
    Bytes out;
    Put32(out, c.engine); Put64(out, c.content); Put64(out, c.world);
    return out;
}
Compatibility ReadCompatibility(const std::uint8_t* p) {
    return Compatibility{ Read32(p), Read64(p + 4), Read64(p + 12) };
}
Rejection Mismatch(const Compatibility& a, const Compatibility& b) {
    if (a.engine != b.engine) return Rejection::Engine;
    if (a.content != b.content) return Rejection::Content;
    if (a.world != b.world) return Rejection::World;
    return static_cast<Rejection>(0);
}
const char* RejectionText(Rejection rejection) {
    switch (rejection) {
    case Rejection::Protocol: return "Rejected: different protocol version";
    case Rejection::Engine: return "Rejected: different Gothic engine";
    case Rejection::Content: return "Rejected: different content fingerprint";
    case Rejection::World: return "Rejected: different world";
    case Rejection::Busy: return "Rejected: host already has one guest";
    default: return "Rejected: unknown reason";
    }
}
std::uint64_t Elapsed(std::uint64_t now, std::uint64_t then) {
    return now >= then ? now - then : 0;
}
}

struct Network::Impl {
    SOCKET socket = INVALID_SOCKET;
    bool winsock = false;
    State state = State::Idle;
    Role role = Role::None;
    Compatibility compatibility;
    sockaddr_in peer{};
    std::uint16_t boundPort = 0;
    std::uint64_t token = 0, nonce = 0;
    std::uint64_t lastRx = 0, lastHello = 0, lastHeartbeat = 0, connectStarted = 0;
    std::uint64_t timeout = 8000;
    bool localSaving=false,peerSaving=false;
    std::uint64_t saveDeadline=0;
    std::uint32_t txSequence = 0, rxSequence = 0;
    bool hasSnapshot = false;
    Snapshot latest;
    std::string status = "Idle";
    NetworkStats stats;
    std::string playerName = "Spieler", peerName;
    Appearance appearance=Appearance::Nameless,peerAppearance=Appearance::Nameless;
    std::function<void(const std::string&)> trace;
    struct Pending { Bytes data;std::uint64_t first=0,last=0; };
    std::map<std::uint32_t,Pending> pending;
    std::map<std::uint32_t,Bytes> reorder;
    std::deque<Bytes> events,worldStates;
    std::uint32_t eventTx=0,eventExpected=1;
    void ClearEvents(){localSaving=peerSaving=false;saveDeadline=0;pending.clear();reorder.clear();events.clear();worldStates.clear();eventTx=0;eventExpected=1;}
    void Trace(const std::string& text) { if(trace)trace(text); }
    static std::string Endpoint(const sockaddr_in& value) {
        char address[INET_ADDRSTRLEN]{};InetNtopA(AF_INET,const_cast<in_addr*>(&value.sin_addr),address,sizeof(address));
        return std::string(address)+":"+std::to_string(ntohs(value.sin_port));
    }
    static bool ValidName(const std::string& name) {
        if(name.empty()||name.size()>32)return false;
        for(unsigned char c:name)if(c<32||c==127)return false;
        return MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,name.data(),static_cast<int>(name.size()),nullptr,0)>0;
    }

    ~Impl() { Close(); }
    void Close() {
        if (socket != INVALID_SOCKET) { closesocket(socket); socket = INVALID_SOCKET; }
        if (winsock) { WSACleanup(); winsock = false; }
        boundPort = 0;
    }
    bool Open(std::uint16_t port,const std::string& localAddress) {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            status = "WSAStartup failed"; state = State::Error; return false;
        }
        winsock = true;
        socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (socket == INVALID_SOCKET) return Fail("Cannot create UDP socket");
        sockaddr_in local{};
        local.sin_family = AF_INET; local.sin_addr.s_addr = INADDR_ANY; local.sin_port = htons(port);
        if(InetPtonA(AF_INET,localAddress.c_str(),&local.sin_addr)!=1||local.sin_addr.s_addr==INADDR_BROADCAST)
            return Fail("Local bind address must be numeric IPv4");
        if (bind(socket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) != 0)
            return Fail("Cannot bind local IP/UDP port: address not on this PC or port in use");
        u_long nonblocking = 1;
        if (ioctlsocket(socket, FIONBIO, &nonblocking) != 0) return Fail("Cannot set nonblocking UDP");
        int localBytes = sizeof(local);
        if (getsockname(socket, reinterpret_cast<sockaddr*>(&local), &localBytes) != 0)
            return Fail("Cannot query bound UDP port");
        boundPort = ntohs(local.sin_port);
        Trace("BIND local="+Endpoint(local));
        return true;
    }
    bool Fail(const std::string& reason) {
        Trace("ERROR reason="+reason+" wsa_error="+std::to_string(WSAGetLastError()));
        status = reason; state = State::Error; hasSnapshot = false; Close(); return false;
    }
    bool SendTo(const sockaddr_in& destination, Message type, const Bytes& payload,
                std::uint64_t packetToken, std::uint64_t packetNonce, std::uint32_t sequence = 0) {
        if (socket == INVALID_SOCKET || payload.size() + ProtocolHeaderBytes > MaximumDatagramBytes) return false;
        Bytes packet;
        packet.reserve(ProtocolHeaderBytes + payload.size());
        Put32(packet, ProtocolMagic); Put16(packet, ProtocolVersion);
        packet.push_back(static_cast<std::uint8_t>(type)); packet.push_back(0);
        Put16(packet, static_cast<std::uint16_t>(payload.size())); Put16(packet, 0);
        Put32(packet, sequence); Put64(packet, packetToken); Put64(packet, packetNonce);
        packet.insert(packet.end(), payload.begin(), payload.end());
        const int result = sendto(socket, reinterpret_cast<const char*>(packet.data()), static_cast<int>(packet.size()),
            0, reinterpret_cast<const sockaddr*>(&destination), sizeof(destination));
        const int error=result==SOCKET_ERROR?WSAGetLastError():0;
        Trace("TX to="+Endpoint(destination)+" type="+std::to_string(static_cast<int>(type))+" bytes="+std::to_string(packet.size())+" seq="+std::to_string(sequence)+" sent="+std::to_string(result)+" wsa_error="+std::to_string(error));
        return result == static_cast<int>(packet.size());
    }
    void Reject(const sockaddr_in& destination, std::uint64_t peerNonce, Rejection reason) {
        Trace("REJECT to="+Endpoint(destination)+" reason="+RejectionText(reason));
        SendTo(destination, Message::Reject, Bytes{ static_cast<std::uint8_t>(reason) }, 0, peerNonce);
    }
    void ForgetPeer(const char* reason) {
        ClearEvents();
        token = 0; nonce = 0; txSequence = 0; rxSequence = 0; hasSnapshot = false; peerName.clear();
        peerAppearance=Appearance::Nameless;
        if (role == Role::Host) { state = State::Listening; status = reason; }
        else { state = State::Error; status = reason; Close(); }
    }
    void Receive(const std::uint8_t* p, std::size_t bytes, const sockaddr_in& from, std::uint64_t now) {
        ++stats.received;
        Trace("RX from="+Endpoint(from)+" bytes="+std::to_string(bytes));
        if (bytes < ProtocolHeaderBytes || bytes > MaximumDatagramBytes || Read32(p) != ProtocolMagic
            || p[7] != 0 || Read16(p + 10) != 0 || Read16(p + 8) + ProtocolHeaderBytes != bytes) {
            ++stats.malformed; return;
        }
        const auto version = Read16(p + 4);
        const auto type = static_cast<Message>(p[6]);
        const auto length = Read16(p + 8);
        const auto sequence = Read32(p + 12);
        Trace("RX type="+std::to_string(p[6])+" seq="+std::to_string(sequence));
        const auto packetToken = Read64(p + 16), packetNonce = Read64(p + 24);
        const auto body = p + ProtocolHeaderBytes;
        if (type == Message::Hello && role == Role::Host) {
            if (length != 20 || packetToken != 0 || packetNonce == 0 || sequence != 0) { ++stats.malformed; return; }
            if (version != ProtocolVersion) { Reject(from, packetNonce, Rejection::Protocol); return; }
            const auto mismatch = Mismatch(compatibility, ReadCompatibility(body));
            if (mismatch != static_cast<Rejection>(0)) { Reject(from, packetNonce, mismatch); return; }
            if (state == State::Connected) {
                if (!SameAddress(from, peer) || packetNonce != nonce) { Reject(from, packetNonce, Rejection::Busy); return; }
                SendTo(peer, Message::Welcome, CompatibilityPayload(compatibility), token, nonce);
                SendTo(peer,Message::Appearance,Bytes{static_cast<std::uint8_t>(appearance)},token,nonce);
                return;
            }
            if (state != State::Listening) { ++stats.ignored; return; }
            peer = from; nonce = packetNonce; token = RandomToken();
            lastRx = now; lastHeartbeat = now; txSequence = 0; rxSequence = 0; hasSnapshot = false;
            state = State::Connected; status = "Connected: guest joined";
            SendTo(peer, Message::Welcome, CompatibilityPayload(compatibility), token, nonce);
            SendTo(peer,Message::Appearance,Bytes{static_cast<std::uint8_t>(appearance)},token,nonce);
            return;
        }
        if (version != ProtocolVersion || !SameAddress(peer, from) || packetNonce != nonce) {
            ++stats.ignored; return;
        }
        if (type == Message::Reject && role == Role::Guest && state == State::Connecting) {
            if (length != 1 || packetToken != 0 || sequence != 0) { ++stats.malformed; return; }
            Fail(RejectionText(static_cast<Rejection>(body[0]))); return;
        }
        if (type == Message::Welcome && role == Role::Guest && state == State::Connecting) {
            if (length != 20 || packetToken == 0 || sequence != 0) { ++stats.malformed; return; }
            const auto mismatch = Mismatch(compatibility, ReadCompatibility(body));
            if (mismatch != static_cast<Rejection>(0)) { Fail(RejectionText(mismatch)); return; }
            token = packetToken; state = State::Connected; lastRx = now; lastHeartbeat = now;
            txSequence = 0; rxSequence = 0; hasSnapshot = false; status = "Connected: joined host";
            SendTo(peer, Message::Heartbeat, {}, token, nonce);
            SendTo(peer, Message::PlayerName, Bytes(playerName.begin(),playerName.end()),token,nonce);
            SendTo(peer,Message::Appearance,Bytes{static_cast<std::uint8_t>(appearance)},token,nonce);
            return;
        }
        if (state != State::Connected || packetToken != token || token == 0) { ++stats.ignored; return; }
        switch (type) {
        case Message::WorldState:
            if(sequence!=0||length==0){++stats.malformed;return;}
            if(worldStates.size()>=256)worldStates.pop_front();
            worldStates.emplace_back(body,body+length);lastRx=now;return;
        case Message::EventAck:
            if(length!=0||sequence==0){++stats.malformed;return;}
            pending.erase(sequence);lastRx=now;return;
        case Message::ReliableEvent: {
            if(length==0||sequence==0){++stats.malformed;return;}
            // In-order delivery with bounded reordering; duplicates only get ACKed.
            if(sequence<eventExpected){SendTo(peer,Message::EventAck,{},token,nonce,sequence);++stats.ignored;return;}
            if(sequence-eventExpected>=64||events.size()+reorder.size()>=128){++stats.ignored;return;}
            reorder.emplace(sequence,Bytes(body,body+length));
            SendTo(peer,Message::EventAck,{},token,nonce,sequence);lastRx=now;
            while(reorder.count(eventExpected)){
                auto data=std::move(reorder[eventExpected]);reorder.erase(eventExpected++);
                // Transport-level ordered save controls work while engine queues pause.
                if(data.size()==2&&data[0]==63&&data[1]<=1){
                    peerSaving=data[1]!=0;saveDeadline=peerSaving?now+120000:0;
                    if(!peerSaving)for(auto& entry:pending)entry.second.first=entry.second.last=now;
                    Trace(peerSaving?"PEER_SAVE begin":"PEER_SAVE end");
                }else events.push_back(std::move(data));
            }
            return;
        }
        case Message::Appearance:
            if(sequence!=0||length!=1||body[0]>1){++stats.malformed;return;}
            peerAppearance=static_cast<Appearance>(body[0]);lastRx=now;return;
        case Message::PlayerName: {
            const std::string name(reinterpret_cast<const char*>(body),length);
            if(sequence!=0||!ValidName(name)){++stats.malformed;return;}
            peerName=name;lastRx=now;return;
        }
        case Message::Snapshot: {
            if (length != 16) { ++stats.malformed; return; }
            const Snapshot candidate{ ReadFloat(body), ReadFloat(body + 4), ReadFloat(body + 8), ReadFloat(body + 12) };
            if (!ValidSnapshot(candidate)) { ++stats.malformed; return; }
            if (!Newer(sequence, rxSequence)) { ++stats.ignored; return; }
            rxSequence = sequence; latest = candidate; hasSnapshot = true; lastRx = now; ++stats.snapshots;
            Trace("RX snapshot seq="+std::to_string(sequence)+" x="+std::to_string(candidate.x)+" y="+std::to_string(candidate.y)+" z="+std::to_string(candidate.z)+" yaw="+std::to_string(candidate.yaw));
            return;
        }
        case Message::Heartbeat:
            if (length != 0 || sequence != 0) { ++stats.malformed; return; }
            lastRx = now; return;
        case Message::Goodbye:
            if (length != 0 || sequence != 0) { ++stats.malformed; return; }
            ForgetPeer("Peer disconnected"); return;
        default: ++stats.ignored; return;
        }
    }
};

Network::Network() : impl_(new Impl) {}
Network::~Network() { Stop(); }
bool Network::PeerSaving() const {return impl_->peerSaving;}
bool Network::SavingReady() const {return impl_->state==State::Connected&&impl_->localSaving&&!impl_->peerSaving&&impl_->pending.empty();}
bool Network::SetSaving(bool saving,std::uint64_t now){
    if(impl_->state!=State::Connected)return false;
    if(impl_->localSaving==saving)return true;
    if(saving&&impl_->pending.size()>=126)return false; // reserve ordered resume capacity
    if(!SendEvent(Bytes{63,static_cast<std::uint8_t>(saving)},now))return false;
    impl_->localSaving=saving;
    if(!saving){impl_->lastRx=now;for(auto& p:impl_->pending)p.second.first=p.second.last=now;}
    impl_->Trace(saving?"LOCAL_SAVE begin":"LOCAL_SAVE end");return true;
}

bool Network::StartHost(std::uint16_t port, Compatibility compatibility, std::uint64_t nowMs,const std::string& localAddress) {
    Stop();
    impl_->compatibility = compatibility; impl_->stats = NetworkStats{};
    impl_->role = Role::Host; impl_->lastRx = nowMs;
    if (!impl_->Open(port,localAddress)) return false;
    impl_->state = State::Listening; impl_->status = "Hosting: waiting for one guest";
    return true;
}

bool Network::Connect(const std::string& host, std::uint16_t port,
                      Compatibility compatibility, std::uint64_t nowMs,const std::string& localAddress) {
    Stop();
    impl_->compatibility = compatibility; impl_->stats = NetworkStats{};
    impl_->role = Role::Guest;
    if (port == 0 || host.empty()) return impl_->Fail("Host and nonzero UDP port required");
    addrinfo hints{};
    hints.ai_family = AF_INET; hints.ai_socktype = SOCK_DGRAM; hints.ai_protocol = IPPROTO_UDP;
    // Connecting happens on the engine thread. Never perform a potentially
    // blocking DNS lookup there; alpha accepts numeric IPv4 and localhost.
    hints.ai_flags = AI_NUMERICHOST;
    if (!impl_->Open(0,localAddress)) return false;
    addrinfo* addresses = nullptr;
    const std::string service = std::to_string(port);
    const std::string numericHost = host == "localhost" ? "127.0.0.1" : host;
    const int resolved = getaddrinfo(numericHost.c_str(), service.c_str(), &hints, &addresses);
    if (resolved != 0 || !addresses) return impl_->Fail("A numeric IPv4 host address is required");
    impl_->peer = *reinterpret_cast<sockaddr_in*>(addresses->ai_addr);
    freeaddrinfo(addresses);
    impl_->nonce = RandomToken(); impl_->connectStarted = nowMs;
    impl_->lastRx = nowMs; impl_->lastHello = nowMs; impl_->state = State::Connecting;
    impl_->status = "Connecting to host";
    impl_->SendTo(impl_->peer, Message::Hello, CompatibilityPayload(compatibility), 0, impl_->nonce);
    return true;
}

void Network::Poll(std::uint64_t nowMs) {
    if (impl_->socket == INVALID_SOCKET) return;
    for (unsigned int i = 0; i < 32 && impl_->socket != INVALID_SOCKET; ++i) {
        std::array<std::uint8_t, MaximumDatagramBytes + 1> buffer{};
        sockaddr_in from{}; int fromBytes = sizeof(from);
        const int result = recvfrom(impl_->socket, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()),
            0, reinterpret_cast<sockaddr*>(&from), &fromBytes);
        if (result == SOCKET_ERROR) {
            const auto error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) break;
            if (error == WSAEMSGSIZE) { ++impl_->stats.malformed; continue; }
            if (error == WSAECONNRESET) continue;
            impl_->Fail("UDP receive failed"); break;
        }
        if (fromBytes != sizeof(sockaddr_in) || from.sin_family != AF_INET) { ++impl_->stats.malformed; continue; }
        impl_->Receive(buffer.data(), static_cast<std::size_t>(result), from, nowMs);
    }
    if (impl_->state == State::Connecting) {
        if (Elapsed(nowMs, impl_->connectStarted) >= impl_->timeout) { impl_->Fail("Connection timed out"); return; }
        if (Elapsed(nowMs, impl_->lastHello) >= 500) {
            impl_->SendTo(impl_->peer, Message::Hello, CompatibilityPayload(impl_->compatibility), 0, impl_->nonce);
            impl_->lastHello = nowMs;
        }
    } else if (impl_->state == State::Connected) {
        if(impl_->peerSaving&&nowMs>=impl_->saveDeadline){impl_->ForgetPeer("Peer save exceeded 120 seconds");return;}
        for(auto& entry:impl_->pending){
            auto& p=entry.second;
            if(Elapsed(nowMs,p.first)>=((impl_->localSaving||(impl_->peerSaving&&nowMs<impl_->saveDeadline))?120000:impl_->timeout)){impl_->Fail("Combat event acknowledgement timed out; reconnect");impl_->ClearEvents();return;}
            if(Elapsed(nowMs,p.last)>=150){impl_->SendTo(impl_->peer,Message::ReliableEvent,p.data,impl_->token,impl_->nonce,entry.first);p.last=nowMs;}
        }
        if (Elapsed(nowMs, impl_->lastRx) >= ((impl_->localSaving||(impl_->peerSaving&&nowMs<impl_->saveDeadline))?120000:impl_->timeout)) { impl_->ForgetPeer("Peer timed out"); return; }
        if (Elapsed(nowMs, impl_->lastHeartbeat) >= 1000) {
            impl_->SendTo(impl_->peer, Message::Heartbeat, {}, impl_->token, impl_->nonce);
            impl_->SendTo(impl_->peer,Message::PlayerName,Bytes(impl_->playerName.begin(),impl_->playerName.end()),impl_->token,impl_->nonce);
            impl_->SendTo(impl_->peer,Message::Appearance,Bytes{static_cast<std::uint8_t>(impl_->appearance)},impl_->token,impl_->nonce);
            impl_->lastHeartbeat = nowMs;
        }
    }
}

bool Network::SendSnapshot(const Snapshot& snapshot, std::uint64_t) {
    if (impl_->state != State::Connected || !ValidSnapshot(snapshot)) return false;
    impl_->Trace("TX snapshot x="+std::to_string(snapshot.x)+" y="+std::to_string(snapshot.y)+" z="+std::to_string(snapshot.z)+" yaw="+std::to_string(snapshot.yaw));
    Bytes payload;
    PutFloat(payload, snapshot.x); PutFloat(payload, snapshot.y); PutFloat(payload, snapshot.z); PutFloat(payload, snapshot.yaw);
    if (++impl_->txSequence == 0) ++impl_->txSequence;
    return impl_->SendTo(impl_->peer, Message::Snapshot, payload, impl_->token, impl_->nonce, impl_->txSequence);
}

void Network::Stop() {
    if (impl_->state == State::Connected)
        impl_->SendTo(impl_->peer, Message::Goodbye, {}, impl_->token, impl_->nonce);
    impl_->Close(); impl_->state = State::Idle; impl_->role = Role::None;
    impl_->token = 0; impl_->nonce = 0; impl_->txSequence = 0; impl_->rxSequence = 0;
    impl_->hasSnapshot = false; impl_->peer = sockaddr_in{}; impl_->status = "Stopped";
    impl_->peerName.clear();
    impl_->peerAppearance=Appearance::Nameless;
    impl_->ClearEvents();
}

bool Network::SendWorldState(const Bytes& payload){
    return impl_->state==State::Connected&&!payload.empty()&&payload.size()<=MaximumDatagramBytes-ProtocolHeaderBytes&&impl_->SendTo(impl_->peer,Message::WorldState,payload,impl_->token,impl_->nonce);
}
bool Network::ConsumeWorldState(Bytes& payload){if(impl_->worldStates.empty())return false;payload=std::move(impl_->worldStates.front());impl_->worldStates.pop_front();return true;}
bool Network::SendEvent(const Bytes& payload,std::uint64_t now){
    if(impl_->state!=State::Connected||payload.empty()||payload.size()>MaximumDatagramBytes-ProtocolHeaderBytes||impl_->pending.size()>=64||impl_->eventTx==UINT32_MAX)return false;
    const auto id=++impl_->eventTx;impl_->pending.emplace(id,Impl::Pending{payload,now,now});
    impl_->Trace("EVENT enqueue id="+std::to_string(id));
    impl_->SendTo(impl_->peer,Message::ReliableEvent,payload,impl_->token,impl_->nonce,id);return true;
}
bool Network::ConsumeEvent(Bytes& payload){if(impl_->events.empty())return false;payload=std::move(impl_->events.front());impl_->events.pop_front();return true;}
std::size_t Network::PendingEvents()const{return impl_->pending.size();}

State Network::GetState() const { return impl_->state; }
Role Network::GetRole() const { return impl_->role; }
bool Network::ConsumeSnapshot(Snapshot& snapshot) {
    if (!impl_->hasSnapshot) return false;
    snapshot = impl_->latest; impl_->hasSnapshot = false; return true;
}
std::uint16_t Network::BoundPort() const { return impl_->boundPort; }
const std::string& Network::LastStatus() const { return impl_->status; }
const NetworkStats& Network::Stats() const { return impl_->stats; }
void Network::SetTimeoutMs(std::uint64_t timeoutMs) { impl_->timeout = (std::max)(100ull, timeoutMs); }
bool Network::SetPlayerName(const std::string& name) {
    if(!Impl::ValidName(name))return false;
    impl_->playerName=name;
    if(impl_->state==State::Connected)impl_->SendTo(impl_->peer,Message::PlayerName,Bytes(name.begin(),name.end()),impl_->token,impl_->nonce);
    return true;
}
const std::string& Network::PeerName() const { return impl_->peerName; }
bool Network::SetAppearance(Appearance value) {
    if(value!=Appearance::Nameless&&value!=Appearance::Velaya)return false;
    impl_->appearance=value;
    if(impl_->state==State::Connected)impl_->SendTo(impl_->peer,Message::Appearance,Bytes{static_cast<std::uint8_t>(value)},impl_->token,impl_->nonce);
    return true;
}
Appearance Network::PeerAppearance() const {return impl_->peerAppearance;}
void Network::SetTraceSink(std::function<void(const std::string&)> sink) { impl_->trace=std::move(sink); }

} }
