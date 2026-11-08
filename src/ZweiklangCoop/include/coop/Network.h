#pragma once

#include "coop/Protocol.h"
#include <memory>
#include <string>
#include <functional>
#include <vector>

namespace zweiklang { namespace net {

enum class State { Idle, Listening, Connecting, Connected, Error };
enum class Role { None, Host, Guest };

struct NetworkStats {
    std::uint64_t received = 0;
    std::uint64_t malformed = 0;
    std::uint64_t ignored = 0;
    std::uint64_t snapshots = 0;
};

// This class belongs to one thread. Poll it from the engine thread; no worker
// thread, asynchronous engine access or background callback is used.
class Network {
public:
    Network();
    ~Network();
    Network(const Network&) = delete;
    Network& operator=(const Network&) = delete;

    bool StartHost(std::uint16_t port, Compatibility compatibility, std::uint64_t nowMs,
                   const std::string& localAddress="0.0.0.0");
    bool Connect(const std::string& host, std::uint16_t port,
                 Compatibility compatibility, std::uint64_t nowMs,
                 const std::string& localAddress="0.0.0.0");
    void Poll(std::uint64_t nowMs);
    bool SendSnapshot(const Snapshot& snapshot, std::uint64_t nowMs);
    void Stop();
    bool SetSaving(bool saving, std::uint64_t nowMs);
    bool PeerSaving() const;

    State GetState() const;
    Role GetRole() const;
    bool ConsumeSnapshot(Snapshot& snapshot);
    std::uint16_t BoundPort() const;
    const std::string& LastStatus() const;
    const NetworkStats& Stats() const;
    void SetTimeoutMs(std::uint64_t timeoutMs);
    bool SetPlayerName(const std::string& name);
    const std::string& PeerName() const;
    bool SetAppearance(Appearance appearance);
    Appearance PeerAppearance() const;
    void SetTraceSink(std::function<void(const std::string&)> sink);
    bool SendWorldState(const std::vector<std::uint8_t>& payload);
    bool ConsumeWorldState(std::vector<std::uint8_t>& payload);
    bool SendEvent(const std::vector<std::uint8_t>& payload,std::uint64_t nowMs);
    bool ConsumeEvent(std::vector<std::uint8_t>& payload);
    std::size_t PendingEvents() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} }
