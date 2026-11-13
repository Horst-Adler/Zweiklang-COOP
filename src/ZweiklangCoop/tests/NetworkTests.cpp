#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "coop/Network.h"
#include "coop/CombatProtocol.h"
#include "coop/CharacterPolicy.h"
#include "coop/ActiveAnimations.h"
#include "coop/NetworkMotion.h"
#include "coop/LootProtocol.h"
#include "coop/Recovery.h"
#include "coop/WorldProtocol.h"
#include "coop/SessionProtocol.h"
#include "coop/CavalornPolicy.h"
#include "coop/QolPolicy.h"
#include <filesystem>
#include "coop/KeyboardText.h"
#include "coop/LocalAddress.h"
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

using namespace zweiklang::net;
namespace {
int assertions = 0;
void Check(bool condition, const char* message) {
    ++assertions;
    if (!condition) throw std::runtime_error(message);
}
Compatibility Compatible() { return Compatibility{4, HashString("g2-notr-test"), HashString("NEWWORLD.ZEN")}; }
void Pump(Network& host, Network& guest, std::uint64_t now = 1000) {
    for (unsigned int i = 0; i < 12; ++i) { host.Poll(now); guest.Poll(now); }
}
void ConnectedPair(Network& host, Network& guest, Compatibility c = Compatible()) {
    Check(host.StartHost(0, c, 1000), "host start");
    Check(host.BoundPort() != 0, "ephemeral port reported");
    Check(guest.Connect("127.0.0.1", host.BoundPort(), c, 1000), "guest start");
    Pump(host, guest);
    Check(host.GetState() == State::Connected && guest.GetState() == State::Connected, "loopback handshake");
}
void TestConnectionAndSnapshots() {
    Network host, guest;
    ConnectedPair(host, guest);
    Snapshot incoming;
    Check(!host.ConsumeSnapshot(incoming), "no invented snapshot before delivery");
    Check(guest.SendSnapshot(Snapshot{12, 34, 56, 90}, 1010), "guest sends snapshot");
    Check(host.SendSnapshot(Snapshot{-15, 22, 17, -45}, 1010), "host sends snapshot");
    Pump(host, guest, 1010);
    Check(host.ConsumeSnapshot(incoming) && incoming.x == 12 && incoming.y == 34 && incoming.z == 56 && incoming.yaw == 90,
          "guest snapshot decoded exactly");
    Check(!host.ConsumeSnapshot(incoming), "snapshot consumed once");
    Check(guest.ConsumeSnapshot(incoming) && incoming.x == -15 && incoming.yaw == -45, "host snapshot decoded exactly");
    Check(!guest.SendSnapshot(Snapshot{std::numeric_limits<float>::infinity(), 0, 0, 0}, 1011), "reject infinite local position");
    Check(!guest.SendSnapshot(Snapshot{0, 0, 0, std::numeric_limits<float>::quiet_NaN()}, 1011), "reject NaN local orientation");
    Check(host.Stats().snapshots == 1 && guest.Stats().snapshots == 1, "snapshot counters");
}
void TestCompatibility() {
    const auto base = Compatible();
    for (unsigned int type = 0; type < 3; ++type) {
        Network host, guest;
        auto other = base;
        if (type == 0) other.engine = 1;
        if (type == 1) ++other.content;
        if (type == 2) ++other.world;
        Check(host.StartHost(0, base, 1000), "mismatch host start");
        Check(guest.Connect("127.0.0.1", host.BoundPort(), other, 1000), "mismatch guest start");
        Pump(host, guest);
        Check(host.GetState() == State::Listening, "mismatch leaves host available");
        Check(guest.GetState() == State::Error, "mismatch rejected");
        const char* word = type == 0 ? "engine" : (type == 1 ? "content" : "world");
        Check(guest.LastStatus().find(word) != std::string::npos, "mismatch reason is useful");
    }
}
void TestBusyDisconnectReconnect() {
    Network host, guest, third;
    ConnectedPair(host, guest);
    Check(third.Connect("127.0.0.1", host.BoundPort(), Compatible(), 1020), "third guest starts");
    Pump(host, third, 1020);
    Check(third.GetState() == State::Error && third.LastStatus().find("one guest") != std::string::npos, "third guest rejected");
    Check(host.GetState() == State::Connected && guest.GetState() == State::Connected, "existing pair preserved");
    guest.Stop();
    host.Poll(1030);
    Check(host.GetState() == State::Listening, "goodbye releases guest slot");
    Check(third.Connect("127.0.0.1", host.BoundPort(), Compatible(), 1040), "reconnect starts");
    Pump(host, third, 1040);
    Check(host.GetState() == State::Connected && third.GetState() == State::Connected, "reconnect succeeds");
    host.Stop(); third.Poll(1050);
    Check(third.GetState() == State::Error, "host goodbye disconnects guest");
}
void TestTimeouts() {
    Network host, guest;
    ConnectedPair(host, guest);
    host.SetTimeoutMs(100);
    host.Poll(1101);
    Check(host.GetState() == State::Listening, "silent guest times out and releases slot");
    Check(host.LastStatus().find("timed out") != std::string::npos, "timeout status");
    guest.SetTimeoutMs(100);
    guest.Poll(1101);
    Check(guest.GetState() == State::Error, "silent host timeout");
    Check(guest.Connect("127.0.0.1", host.BoundPort(), Compatible(), 1200), "reconnect after timeout");
    Pump(host, guest, 1200);
    Check(host.GetState() == State::Connected && guest.GetState() == State::Connected, "timeout recovery succeeds");

    Network unresponsive;
    Network portHolder;
    Check(portHolder.StartHost(0, Compatible(), 2000), "unresponsive port holder");
    unresponsive.SetTimeoutMs(100);
    Check(unresponsive.Connect("127.0.0.1", portHolder.BoundPort(), Compatible(), 2000), "connect to unpolled host");
    unresponsive.Poll(2101);
    Check(unresponsive.GetState() == State::Error && unresponsive.LastStatus().find("timed out") != std::string::npos,
          "initial handshake times out");
}

using Bytes = std::vector<std::uint8_t>;
void U16(Bytes& b, std::uint16_t v) { b.push_back(static_cast<std::uint8_t>(v >> 8)); b.push_back(static_cast<std::uint8_t>(v)); }
void U32(Bytes& b, std::uint32_t v) { U16(b, static_cast<std::uint16_t>(v >> 16)); U16(b, static_cast<std::uint16_t>(v)); }
void U64(Bytes& b, std::uint64_t v) { U32(b, static_cast<std::uint32_t>(v >> 32)); U32(b, static_cast<std::uint32_t>(v)); }
void Float(Bytes& b, float v) { std::uint32_t bits; std::memcpy(&bits, &v, 4); U32(b, bits); }
std::uint64_t Read64(const std::uint8_t* p) { std::uint64_t v = 0; for (int i = 0; i < 8; ++i) v = (v << 8) | p[i]; return v; }
Bytes Packet(Message type, const Bytes& body, std::uint64_t token, std::uint64_t nonce,
             std::uint32_t sequence = 0, std::uint16_t version = ProtocolVersion) {
    Bytes b;
    U32(b, ProtocolMagic); U16(b, version); b.push_back(static_cast<std::uint8_t>(type)); b.push_back(0);
    U16(b, static_cast<std::uint16_t>(body.size())); U16(b, 0); U32(b, sequence); U64(b, token); U64(b, nonce);
    b.insert(b.end(), body.begin(), body.end()); return b;
}
Bytes Hello(std::uint64_t nonce, std::uint16_t version = ProtocolVersion) {
    const auto c = Compatible(); Bytes body;
    U32(body, c.engine); U64(body, c.content); U64(body, c.world);
    return Packet(Message::Hello, body, 0, nonce, 0, version);
}
Bytes Position(std::uint64_t token, std::uint64_t nonce, std::uint32_t sequence, float x = 42) {
    Bytes body; Float(body, x); Float(body, 2); Float(body, 3); Float(body, 90);
    return Packet(Message::Snapshot, body, token, nonce, sequence);
}
struct RawPeer {
    SOCKET socket = INVALID_SOCKET;
    sockaddr_in destination{};
    explicit RawPeer(std::uint16_t port) {
        socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        Check(socket != INVALID_SOCKET, "raw socket created");
        sockaddr_in local{}; local.sin_family = AF_INET; local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        Check(bind(socket, reinterpret_cast<sockaddr*>(&local), sizeof(local)) == 0, "raw socket bound");
        u_long enabled = 1; ioctlsocket(socket, FIONBIO, &enabled);
        destination.sin_family = AF_INET; destination.sin_port = htons(port);
        destination.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    }
    ~RawPeer() { if (socket != INVALID_SOCKET) closesocket(socket); }
    void Send(const Bytes& b) {
        Check(sendto(socket, reinterpret_cast<const char*>(b.data()), static_cast<int>(b.size()), 0,
            reinterpret_cast<sockaddr*>(&destination), sizeof(destination)) == static_cast<int>(b.size()), "raw datagram sent");
    }
    std::uint16_t BoundPort() const {
        sockaddr_in local{}; int bytes = sizeof(local);
        Check(getsockname(socket, reinterpret_cast<sockaddr*>(&local), &bytes) == 0, "raw bound port lookup");
        return ntohs(local.sin_port);
    }
    void SendTo(const Bytes& b, const sockaddr_in& to) {
        Check(sendto(socket, reinterpret_cast<const char*>(b.data()), static_cast<int>(b.size()), 0,
            reinterpret_cast<const sockaddr*>(&to), sizeof(to)) == static_cast<int>(b.size()), "raw response sent");
    }
    Bytes Receive(sockaddr_in* sender = nullptr) {
        std::array<std::uint8_t, 1024> b{};
        sockaddr_in from{}; int fromBytes = sizeof(from);
        const int size = recvfrom(socket, reinterpret_cast<char*>(b.data()), static_cast<int>(b.size()), 0,
            reinterpret_cast<sockaddr*>(&from), &fromBytes);
        if (size <= 0) return {};
        if (sender) *sender = from;
        return Bytes(b.begin(), b.begin() + size);
    }
};
void TestHandshakePacketLoss() {
    // Act as a controlled lossy host: discard the first HELLO and answer only
    // its retry. This exercises timed retransmission over a real UDP socket.
    RawPeer lossyHost(0);
    Network guest;
    Check(guest.Connect("127.0.0.1", lossyHost.BoundPort(), Compatible(), 1000), "lossy guest starts");
    sockaddr_in guestAddress{};
    const auto droppedHello = lossyHost.Receive(&guestAddress);
    Check(droppedHello.size() == ProtocolHeaderBytes + 20, "first hello deliberately dropped");
    guest.Poll(1499);
    Check(lossyHost.Receive().empty(), "hello is not retransmitted too early");
    guest.Poll(1500);
    const auto retriedHello = lossyHost.Receive(&guestAddress);
    Check(retriedHello == droppedHello, "lost hello retransmitted with stable nonce");
    const auto nonce = Read64(retriedHello.data() + 24);
    const auto c = Compatible(); Bytes compatibility;
    U32(compatibility, c.engine); U64(compatibility, c.content); U64(compatibility, c.world);
    lossyHost.SendTo(Packet(Message::Welcome, compatibility, 777, nonce), guestAddress);
    guest.Poll(1500);
    Check(guest.GetState() == State::Connected, "connection recovers after first handshake packet loss");
    lossyHost.SendTo(Position(777, nonce, 1, 321), guestAddress);
    guest.Poll(1501);
    Snapshot snapshot;
    Check(guest.ConsumeSnapshot(snapshot) && snapshot.x == 321, "recovered connection transports movement");
}
void TestWireValidationAndReplay() {
    Network host;
    Check(host.StartHost(0, Compatible(), 1000), "wire host start");
    RawPeer peer(host.BoundPort());
    peer.Send(Bytes{1, 2, 3}); host.Poll(1000);
    peer.Send(Bytes(300, 0xFF)); host.Poll(1000);
    Check(host.GetState() == State::Listening && host.Stats().malformed == 2, "truncated and oversized packets rejected");
    peer.Send(Hello(90, static_cast<std::uint16_t>(ProtocolVersion + 1))); host.Poll(1001);
    auto reject = peer.Receive();
    Check(reject.size() == ProtocolHeaderBytes + 1 && reject[6] == static_cast<std::uint8_t>(Message::Reject)
        && reject.back() == static_cast<std::uint8_t>(Rejection::Protocol), "different protocol explicitly rejected");
    Check(host.GetState() == State::Listening, "wrong protocol does not occupy guest slot");
    const std::uint64_t nonce = 1234567;
    peer.Send(Hello(nonce)); host.Poll(1002);
    auto welcome = peer.Receive();
    Check(welcome.size() == ProtocolHeaderBytes + 20 && welcome[6] == static_cast<std::uint8_t>(Message::Welcome), "raw handshake welcome");
    const auto token = Read64(welcome.data() + 16);
    Check(token != 0, "nonzero session token");
    const auto appearance=peer.Receive();
    Check(appearance.size()==ProtocolHeaderBytes+1&&appearance[6]==static_cast<std::uint8_t>(Message::Appearance),"appearance follows welcome");
    peer.Send(Hello(nonce)); host.Poll(1003);
    const auto duplicateWelcome = peer.Receive();
    Check(duplicateWelcome == welcome, "retry after lost welcome reuses same session");
    Check(peer.Receive()==appearance,"retry also repeats appearance");
    peer.Send(Packet(Message::Appearance,Bytes{1},token,nonce));host.Poll(1003);
    Check(host.PeerAppearance()==Appearance::Velaya,"valid wire appearance accepted");
    const auto malformedBefore=host.Stats().malformed;
    peer.Send(Packet(Message::Appearance,Bytes{2},token,nonce));
    peer.Send(Packet(Message::Appearance,Bytes{},token,nonce));
    host.Poll(1003);
    Check(host.Stats().malformed==malformedBefore+2&&host.PeerAppearance()==Appearance::Velaya,"invalid appearance payloads rejected without changing visual");

    Snapshot snapshot;
    peer.Send(Position(token, nonce, 5)); host.Poll(1004);
    Check(host.ConsumeSnapshot(snapshot) && snapshot.x == 42, "valid session packet accepted");
    peer.Send(Position(token, nonce, 4, 99)); host.Poll(1005);
    Check(!host.ConsumeSnapshot(snapshot), "out of order snapshot ignored");
    peer.Send(Position(token, nonce, 5, 99)); host.Poll(1006);
    Check(!host.ConsumeSnapshot(snapshot), "replayed snapshot ignored");
    peer.Send(Position(token, nonce, 6, std::numeric_limits<float>::quiet_NaN())); host.Poll(1007);
    Check(!host.ConsumeSnapshot(snapshot), "wire NaN rejected");
    peer.Send(Position(token ^ 1, nonce, 6, 99)); host.Poll(1008);
    Check(!host.ConsumeSnapshot(snapshot), "foreign session token ignored");
    peer.Send(Position(token, nonce + 1, 6, 99)); host.Poll(1009);
    Check(!host.ConsumeSnapshot(snapshot), "foreign nonce ignored");
    RawPeer intruder(host.BoundPort());
    intruder.Send(Position(token, nonce, 6, 99)); host.Poll(1010);
    Check(!host.ConsumeSnapshot(snapshot), "foreign sender address ignored even with matching token");
    auto corrupted = Position(token, nonce, 6);
    corrupted[8] = 0xFF;
    peer.Send(corrupted); host.Poll(1011);
    Check(!host.ConsumeSnapshot(snapshot), "false length rejected");
    peer.Send(Position(token, nonce, 6, -19)); host.Poll(1012);
    Check(host.ConsumeSnapshot(snapshot) && snapshot.x == -19, "malformed traffic cannot consume valid sequence number");
    Check(host.GetState() == State::Connected, "malformed traffic does not crash or drop session");
}
}

void RunDiagnosticsTests();
void TestPrimaryAddress() {
    using namespace zweiklang;
    auto require=[](bool value){if(!value)throw std::runtime_error("primary IPv4 selection failed");};
    require(SelectPrimaryIPv4({},0).empty());
    require(!UsableLocalIPv4("127.0.0.1")&&!UsableLocalIPv4("0.0.0.0")&&!UsableLocalIPv4("169.254.1.4"));
    require(!UsableLocalIPv4("255.255.255.255")&&!UsableLocalIPv4("224.0.0.1")&&!UsableLocalIPv4("::1"));
    const std::vector<LocalIPv4> addresses={{"192.168.2.3",8,30},{"10.0.0.2",9,10},{"127.0.0.1",10,0}};
    require(SelectPrimaryIPv4(addresses,8)=="192.168.2.3");
    require(SelectPrimaryIPv4(addresses,0)=="10.0.0.2");
    require(SelectPrimaryIPv4(addresses,10)=="10.0.0.2");
    const auto local=PrimaryIPv4();
    require(local.empty()||UsableLocalIPv4(local));
    std::cout<<"PASS: primary IPv4 route priority, metric fallback, invalid/loopback/APIPA filtering; local="<<local<<"\n";
}
void TestAppearance() {
    Network host,guest;
    Check(host.SetAppearance(Appearance::Velaya),"host chooses female before session");
    ConnectedPair(host,guest);
    Check(guest.PeerAppearance()==Appearance::Velaya&&host.PeerAppearance()==Appearance::Nameless,"different local genders propagate at handshake");
    Check(!guest.SetAppearance(static_cast<Appearance>(2)),"unknown appearance rejected locally");
    guest.SetAppearance(Appearance::Velaya);host.SetAppearance(Appearance::Nameless);Pump(host,guest);
    Check(host.PeerAppearance()==Appearance::Velaya&&guest.PeerAppearance()==Appearance::Nameless,"appearance changes propagate bidirectionally");
    guest.Stop();host.Poll(1001);
    Check(host.PeerAppearance()==Appearance::Nameless,"departed peer appearance reset");
    Check(guest.Connect("127.0.0.1",host.BoundPort(),Compatible(),1002),"reconnect after avatar selection");Pump(host,guest,1002);
    Check(host.PeerAppearance()==Appearance::Velaya,"local choice survives reconnect");
}
void TestLocalAddresses() {
    Network host,guest;
    const auto profile=Compatible();
    Check(host.StartHost(0,profile,1000,"127.0.0.1"),"host binds selected local IP");
    Check(guest.Connect("127.0.0.1",host.BoundPort(),profile,1000,"127.0.0.1"),"client binds selected local IP");
    Pump(host,guest);
    Check(host.GetState()==State::Connected&&guest.GetState()==State::Connected,"explicit host/client IP handshake");
    host.Stop();guest.Stop();
    Check(!host.StartHost(0,profile,1000,"invalid"),"reject invalid host bind IP");
    Check(host.GetState()==State::Error&&host.BoundPort()==0,"failed host bind cleaned up");
    Check(!guest.Connect("127.0.0.1",12345,profile,1000,"255.255.255.255"),"reject broadcast client bind IP");
    Check(guest.GetState()==State::Error&&guest.BoundPort()==0,"failed client bind cleaned up");
    Check(host.StartHost(0,profile,1000,"0.0.0.0"),"wildcard host after bind failure");
    Check(guest.Connect("127.0.0.1",host.BoundPort(),profile,1000,"0.0.0.0"),"automatic client interface");
    Pump(host,guest);
    Check(host.GetState()==State::Connected&&guest.GetState()==State::Connected,"automatic binding handshake");
}
void TestKeyboardLayout() {
    const HKL original=GetKeyboardLayout(0);
    const HKL de=LoadKeyboardLayoutW(L"00000407",0);
    const HKL us=LoadKeyboardLayoutW(L"00000409",0);
    if(!de||!us)throw std::runtime_error("German/US keyboard layouts unavailable for text tests");
    BYTE state[256]{};
    auto require=[](bool ok){if(!ok)throw std::runtime_error("keyboard text translation failed");};
    require(zweiklang::MenuCharacter(0x15,de,state)=='z');
    require(zweiklang::MenuCharacter(0x2C,de,state)=='y');
    require(zweiklang::MenuCharacter(0x15,us,state)=='y');
    require(zweiklang::MenuCharacter(0x2C,us,state)=='z');
    require(zweiklang::MenuCharacter(0x34,de,state)=='.');
    require(zweiklang::MenuCharacter(0x02,de,state)=='1');
    const char umlaut=static_cast<char>(zweiklang::MenuCharacter(0x28,de,state));
    WCHAR decoded=0;
    require(MultiByteToWideChar(CP_ACP,0,&umlaut,1,&decoded,1)==1&&decoded==L'\x00E4');
    state[VK_CONTROL]=state[VK_MENU]=0x80;
    require(zweiklang::MenuCharacter(0x10,de,state)=='@');
    state[VK_CONTROL]=state[VK_MENU]=0;
    state[VK_SHIFT]=0x80;
    require(zweiklang::MenuCharacter(0x15,de,state)=='Z');
    require(zweiklang::MenuCharacter(0x02,de,state)=='!');
    state[VK_SHIFT]=0;state[VK_CAPITAL]=1;
    require(zweiklang::MenuCharacter(0x2C,de,state)=='Y');
    require(GetKeyboardLayout(0)==original);
    std::cout<<"PASS: German/US Y/Z, umlaut, AltGr, Shift, Caps Lock, IPv4 punctuation and unchanged active keyboard layout\n";
}
#include "CombatTests.inl"
#include "CharacterTests.inl"
#include "LootTests.inl"
#include "RecoveryTests.inl"
#include "NativeSaveTests.inl"
#include "WorldTests.inl"
#include "SessionTests.inl"
#include "QolTests.inl"
int main(int argc,char** argv) {
    const bool headless=argc==2&&std::strcmp(argv[1],"--headless")==0;
    if(argc>2||(argc==2&&!headless)){
        std::cerr<<"Usage: ZweiklangNetworkTests [--headless]\n";
        return 2;
    }
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) return 2;
    int result = 0;
    try {
        if(headless)std::cout<<"SKIP: interactive keyboard layout checks (explicit --headless service-session run)\n";
        else TestKeyboardLayout();
        TestPrimaryAddress();
        TestAppearance();
        TestLocalAddresses();
        TestConnectionAndSnapshots();
        for(int mode=0;mode<=7;++mode){
            Check(zweiklang::combat::BlockPartnerFocus(false,mode)==(mode!=0),"friendly-fire off: partner name focus only with sheathed weapon");
            Check(!zweiklang::combat::BlockPartnerFocus(true,mode),"friendly-fire on: original partner targeting stays available");
        }
        TestCombatCodec();
        TestActiveAnimationPrefix();
        TestNetworkMovement();
        TestLootProtocol();
        TestLootLedger();
        TestLootSnapshotAndUdp();
        TestLootConservation();
        TestRecoveryWire();
        TestRecoveryStore();
        TestInterruptedJournal();
        TestRecoveryCheckpoints();
        TestNativeSavePaths();
        TestRecoveryCrashMatrix();
        TestDepositLedger();
        TestWorldCodecAndLifecycle();
        TestDualActivation();
        TestWorldTransport();
        TestTradeRecoveryMatrix();
        TestLocalDropRecovery();
        TestLegacyRecoveryMigration();
        TestQolCompatibility();
        TestSessionReadiness();
        TestSaveTransport();
        TestSaveAnnouncementLoss();
        TestSessionCodecsAndTransport();
        TestCavalornRewards();
        TestCharacterRules();
        TestCombatTransport();
        TestReliableReorderingAndLoss();
        TestCompatibility();
        TestBusyDisconnectReconnect();
        TestTimeouts();
        TestHandshakePacketLoss();
        TestWireValidationAndReplay();
        {
            Network host,guest;ConnectedPair(host,guest);
            Check(host.SetPlayerName("HostName"),"host name accepted");
            Check(guest.SetPlayerName("Anna"),"guest name accepted");
            Pump(host,guest,2001);
            Check(host.PeerName()=="Anna"&&guest.PeerName()=="HostName","names delivered bidirectionally");
            Check(!guest.SetPlayerName("bad\nname")&&!guest.SetPlayerName(std::string(33,'x')),"invalid wire names rejected");
            guest.Stop();Check(guest.PeerName().empty(),"peer name cleared on stop");
        }
        RunDiagnosticsTests();
        std::cout << "PASS: " << assertions << " network assertions (real loopback UDP, no games launched)\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL after " << assertions << " assertions: " << error.what() << "\n";
        result = 1;
    }
    WSACleanup();
    return result;
}
