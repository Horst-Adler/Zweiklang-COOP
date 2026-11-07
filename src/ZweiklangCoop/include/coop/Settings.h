#pragma once
#include <windows.h>
#include <string>
namespace zweiklang {
struct Options {
    std::string host="127.0.0.1",client="0.0.0.0",name="Spieler";
    bool asHost=true;
    bool female=false;
    bool friendlyFire=false;
    unsigned short port=12345;
    int logLevel=1;
    bool engineLog=true;
};
bool ValidName(const std::string& name);
bool ValidHost(const std::string& host);
bool ParsePort(const std::string& text, unsigned short& result);
}
