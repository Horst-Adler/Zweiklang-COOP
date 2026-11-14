#pragma once
#include <windows.h>
#include <string>
#include "coop/Localization.h"
namespace orebound {
struct Options {
    std::string host="127.0.0.1",client="0.0.0.0",name="Player";
    i18n::Language language=i18n::Language::English;
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
