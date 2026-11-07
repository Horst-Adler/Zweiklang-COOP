#pragma once
#include <cstddef>
#include <string>

namespace zweiklang {
// Gothic keeps a packed active prefix; unused channel slots are not valid
// pointers and need not be null. Never inspect them, even for a null check.
template<class Active, std::size_t Capacity, class ReadName>
std::string CombatAnimationName(Active* const (&channels)[Capacity], int count, ReadName readName) {
    if(count < 0 || static_cast<std::size_t>(count) > Capacity) return {};
    std::string fallback;
    for(int i=0; i<count; ++i) {
        if(!channels[i]) continue;
        const std::string name=readName(channels[i]);
        if(name.empty() || name.size()>=64) continue;
        if(fallback.empty()) fallback=name;
        if(name.find("ATTACK")!=std::string::npos || name.find("SHOOT")!=std::string::npos ||
           name.find("CAST")!=std::string::npos || name.find("DEAD")!=std::string::npos ||
           name.find("UNCON")!=std::string::npos) return name;
    }
    return fallback;
}
}
