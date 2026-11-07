#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace zweiklang { namespace net {

constexpr std::uint16_t ProtocolVersion = 9;
constexpr std::uint32_t ProtocolMagic = 0x5A434F50u;
constexpr std::size_t ProtocolHeaderBytes = 32;
constexpr std::size_t MaximumDatagramBytes = 512;

// Stable content identifiers, not cryptographic signatures.
inline std::uint64_t HashString(const std::string& value) {
    std::uint64_t hash = 14695981039346656037ull;
    for (unsigned char byte : value) {
        hash ^= byte;
        hash *= 1099511628211ull;
    }
    return hash;
}

struct Compatibility {
    std::uint32_t engine = 0;
    std::uint64_t content = 0;
    std::uint64_t world = 0;
};

struct Snapshot {
    float x = 0;
    float y = 0;
    float z = 0;
    float yaw = 0;
};

enum class Message : std::uint8_t {
    Hello = 1, Welcome = 2, Reject = 3, Snapshot = 4, Heartbeat = 5, Goodbye = 6, PlayerName = 7, Appearance = 8,
    WorldState = 9, ReliableEvent = 10, EventAck = 11
};
enum class Appearance : std::uint8_t { Nameless = 0, Velaya = 1 };

enum class Rejection : std::uint8_t {
    Protocol = 1, Engine = 2, Content = 3, World = 4, Busy = 5
};

} }
