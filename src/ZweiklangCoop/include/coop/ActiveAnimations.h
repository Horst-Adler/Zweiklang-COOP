#pragma once
#include <cstddef>
#include <string>

namespace zweiklang {
// Gothic keeps a packed active prefix; unused channel slots are not valid
// pointers and need not be null. Never inspect them, even for a null check.
template<class Active, std::size_t Capacity, class ReadName>
std::string CombatAnimationName(Active* const (&channels)[Capacity], int count, ReadName readName) {
    if(count < 0 || static_cast<std::size_t>(count) > Capacity) return {};
    std::string fallback;int best=-1;
    for(int i=0; i<count; ++i) {
        if(!channels[i]) continue;
        const std::string name=readName(channels[i]);
        if(name.empty() || name.size()>=64) continue;
        int priority=0;
        if(name.find("DEAD")!=std::string::npos||name.find("UNCON")!=std::string::npos)priority=120;
        else if(name.find("CLIMB")!=std::string::npos||name.find("LADDER")!=std::string::npos||name.find("SWIM")!=std::string::npos||name.find("DIVE")!=std::string::npos||name.find("JUMP")!=std::string::npos||name.find("FALL")!=std::string::npos)priority=100;
        else if(name.find("ATTACK")!=std::string::npos||name.find("SHOOT")!=std::string::npos||name.find("CAST")!=std::string::npos)priority=80;
        if(priority>best){best=priority;fallback=name;}
    }
    return fallback;
}
}
