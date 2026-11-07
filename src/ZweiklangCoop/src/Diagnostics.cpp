#include <windows.h>
#include "coop/Diagnostics.h"
#include <algorithm>
#include <share.h>
namespace zweiklang {
Diagnostics::~Diagnostics() { if (file_) fclose(file_); }
std::string Diagnostics::Escape(const std::string& text) {
    std::string out;
    const char* hex = "0123456789abcdef";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c < 32 || c == 127) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += c;
    }
    return out;
}
void Diagnostics::Open(const std::string& directory, unsigned int maxBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_) { fclose(file_); file_ = nullptr; }
    sequence_=0;lastFlush_=0;
    CreateDirectoryA(directory.c_str(), nullptr);
    SYSTEMTIME t{}; GetLocalTime(&t);
    char fileName[140]{};
    sprintf_s(fileName, "\\session-%04u%02u%02u-%02u%02u%02u-%lu.jsonl",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());
    path_ = directory + fileName;
    maxBytes_ = (std::max)(1024u, maxBytes);
    file_=_fsopen(path_.c_str(),"ab",_SH_DENYNO);
    if (file_) setvbuf(file_,nullptr,_IOFBF,65536);
}
void Diagnostics::Configure(int level, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_); level_ = (std::max)(0,(std::min)(2,level)); name_=name;
}
void Diagnostics::Role(const std::string& role) { std::lock_guard<std::mutex> lock(mutex_); role_=role; }
void Diagnostics::Rotate() {
    if (!file_ || ftell(file_) < static_cast<long>(maxBytes_)) return;
    fclose(file_); file_=nullptr;
    DeleteFileA((path_+".4").c_str());
    for(int i=3;i>=1;--i) MoveFileExA((path_+"."+std::to_string(i)).c_str(),
        (path_+"."+std::to_string(i+1)).c_str(),MOVEFILE_REPLACE_EXISTING);
    MoveFileExA(path_.c_str(),(path_+".1").c_str(),MOVEFILE_REPLACE_EXISTING);
    file_=_fsopen(path_.c_str(),"ab",_SH_DENYNO);
    if(file_) setvbuf(file_,nullptr,_IOFBF,65536);
}
void Diagnostics::Write(int level, const std::string& event, const std::string& detail) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_ || level_==0 || level>level_) return;
    Rotate(); if(!file_ || ftell(file_)>=static_cast<long>(maxBytes_)) return;
    SYSTEMTIME t{},utc{}; GetLocalTime(&t);GetSystemTime(&utc); char timestamp[80]{},utcTimestamp[80]{};
    sprintf_s(timestamp,"%04u-%02u-%02uT%02u:%02u:%02u.%03u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
    sprintf_s(utcTimestamp,"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",utc.wYear,utc.wMonth,utc.wDay,utc.wHour,utc.wMinute,utc.wSecond,utc.wMilliseconds);
    fprintf(file_,"{\"time_local\":\"%s\",\"time_utc\":\"%s\",\"seq\":%llu,\"tick_ms\":%llu,\"pid\":%lu,\"thread\":%lu,\"role\":\"%s\",\"name\":\"%s\",\"level\":%d,\"event\":\"%s\",\"detail\":\"%s\"}\n",
        timestamp,utcTimestamp,++sequence_,GetTickCount64(),GetCurrentProcessId(),GetCurrentThreadId(),Escape(role_).c_str(),Escape(name_).c_str(),level,Escape(event).c_str(),Escape(detail.substr(0,4096)).c_str());
    const auto now=GetTickCount64();
    if(level<=1 || now-lastFlush_>=1000) { fflush(file_);lastFlush_=now; }
}
void Diagnostics::Flush() { std::lock_guard<std::mutex> lock(mutex_); if(file_)fflush(file_); }
}
