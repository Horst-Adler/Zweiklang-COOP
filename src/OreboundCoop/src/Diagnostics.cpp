#include <windows.h>
#include "coop/Diagnostics.h"
#include <algorithm>
#include <share.h>
namespace orebound {
Diagnostics::~Diagnostics() { if (file_) fclose(file_);if(events_)fclose(events_);if(beginning_)fclose(beginning_); }
std::string Diagnostics::Escape(const std::string& text) {
    std::string normalized=text;
    // Native Gothic strings use the Windows codepage; JSON must remain UTF-8.
    // Already valid UTF-8 (including player names) is preserved byte for byte.
    if(!text.empty()&&!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0)){
        const int count=MultiByteToWideChar(CP_ACP,0,text.data(),static_cast<int>(text.size()),nullptr,0);
        if(count>0){std::wstring wide(count,L'\0');MultiByteToWideChar(CP_ACP,0,text.data(),static_cast<int>(text.size()),&wide[0],count);const int bytes=WideCharToMultiByte(CP_UTF8,0,wide.data(),count,nullptr,0,nullptr,nullptr);if(bytes>0){normalized.resize(bytes);WideCharToMultiByte(CP_UTF8,0,wide.data(),count,&normalized[0],bytes,nullptr,nullptr);}}
    }
    std::string out;
    const char* hex = "0123456789abcdef";
    for (unsigned char c : normalized) {
        if (c == '"' || c == '\\') { out += '\\'; out += c; }
        else if (c < 32 || c == 127) { out += "\\u00"; out += hex[c >> 4]; out += hex[c & 15]; }
        else out += c;
    }
    return out;
}
void Diagnostics::Open(const std::string& directory, unsigned int maxBytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_) { fclose(file_); file_ = nullptr; }
    if(events_){fclose(events_);events_=nullptr;}if(beginning_){fclose(beginning_);beginning_=nullptr;}
    sequence_=0;lastFlush_=0;rotationRetryAfter_=0;
    CreateDirectoryA(directory.c_str(), nullptr);
    SYSTEMTIME t{}; GetLocalTime(&t);
    char fileName[140]{};
    sprintf_s(fileName, "\\session-%04u%02u%02u-%02u%02u%02u-%lu.jsonl",
        t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,GetCurrentProcessId());
    path_ = directory + fileName;
    maxBytes_ = (std::max)(1024u, maxBytes);
    file_=_fsopen(path_.c_str(),"ab",_SH_DENYNO);
    if (file_) {setvbuf(file_,nullptr,_IOFBF,65536);fseek(file_,0,SEEK_END);}
    events_=_fsopen((path_+".events.jsonl").c_str(),"ab",_SH_DENYNO);
    beginning_=_fsopen((path_+".begin.jsonl").c_str(),"ab",_SH_DENYNO);
    if(events_)fseek(events_,0,SEEK_END);if(beginning_)fseek(beginning_,0,SEEK_END);
}
void Diagnostics::Configure(int level, const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_); level_ = (std::max)(0,(std::min)(2,level)); name_=name;
}
void Diagnostics::Role(const std::string& role) { std::lock_guard<std::mutex> lock(mutex_); role_=role; }
void Diagnostics::Rotate() {
    if(file_&&ftell(file_)>=0&&ftell(file_)<static_cast<long>(maxBytes_))return;
    if(!file_&&GetTickCount64()<rotationRetryAfter_)return;
    if(file_){fclose(file_);file_=nullptr;}
    WIN32_FILE_ATTRIBUTE_DATA attr{};
    if(GetFileAttributesExA(path_.c_str(),GetFileExInfoStandard,&attr)&&(attr.nFileSizeHigh||attr.nFileSizeLow>=maxBytes_)){
        DeleteFileA((path_+".4").c_str());
        for(int i=3;i>=1;--i)MoveFileExA((path_+"."+std::to_string(i)).c_str(),(path_+"."+std::to_string(i+1)).c_str(),MOVEFILE_REPLACE_EXISTING);
        if(!MoveFileExA(path_.c_str(),(path_+".1").c_str(),MOVEFILE_REPLACE_EXISTING)){rotationRetryAfter_=GetTickCount64()+1000;return;}
    }
    file_=_fsopen(path_.c_str(),"ab",_SH_DENYNO);
    if(file_){setvbuf(file_,nullptr,_IOFBF,65536);fseek(file_,0,SEEK_END);rotationRetryAfter_=0;}
    else rotationRetryAfter_=GetTickCount64()+1000;
}
void Diagnostics::Write(int level, const std::string& event, const std::string& detail) {
    std::lock_guard<std::mutex> lock(mutex_);
    if ((!file_&&!events_&&!beginning_) || level_==0 || level>level_) return;
    Rotate();
    SYSTEMTIME t{},utc{}; GetLocalTime(&t);GetSystemTime(&utc); char timestamp[80]{},utcTimestamp[80]{};
    sprintf_s(timestamp,"%04u-%02u-%02uT%02u:%02u:%02u.%03u",t.wYear,t.wMonth,t.wDay,t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
    sprintf_s(utcTimestamp,"%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",utc.wYear,utc.wMonth,utc.wDay,utc.wHour,utc.wMinute,utc.wSecond,utc.wMilliseconds);
    char header[512];sprintf_s(header,"{\"time_local\":\"%s\",\"time_utc\":\"%s\",\"seq\":%llu,\"tick_ms\":%llu,\"pid\":%lu,\"thread\":%lu,",
        timestamp,utcTimestamp,++sequence_,GetTickCount64(),GetCurrentProcessId(),GetCurrentThreadId());
    const std::string line=std::string(header)+"\"role\":\""+Escape(role_)+"\",\"name\":\""+Escape(name_)+"\",\"level\":"+std::to_string(level)+",\"event\":\""+Escape(event)+"\",\"detail\":\""+Escape(detail.substr(0,4096))+"\"}\n";
    if(file_&&ftell(file_)>=0&&ftell(file_)<static_cast<long>(maxBytes_))fwrite(line.data(),1,line.size(),file_);
    // Packet floods rotate independently of essential events and the immutable
    // beginning of a session. All streams share the original event sequence.
    if(beginning_&&ftell(beginning_)<1024*1024)fwrite(line.data(),1,line.size(),beginning_);
    if(events_&&level<=1){
        const auto eventPath=path_+".events.jsonl";
        if(ftell(events_)>=16*1024*1024){
            fclose(events_);events_=nullptr;DeleteFileA((eventPath+".4").c_str());
            for(int i=3;i>=1;--i)MoveFileExA((eventPath+"."+std::to_string(i)).c_str(),(eventPath+"."+std::to_string(i+1)).c_str(),MOVEFILE_REPLACE_EXISTING);
            MoveFileExA(eventPath.c_str(),(eventPath+".1").c_str(),MOVEFILE_REPLACE_EXISTING);events_=_fsopen(eventPath.c_str(),"ab",_SH_DENYNO);if(events_)fseek(events_,0,SEEK_END);
        }
        if(events_&&ftell(events_)>=0&&ftell(events_)<16*1024*1024){fwrite(line.data(),1,line.size(),events_);fflush(events_);}
    }
    const auto now=GetTickCount64();
    if(level<=1 || now-lastFlush_>=1000) { if(file_)fflush(file_);if(beginning_)fflush(beginning_);lastFlush_=now; }
}
void Diagnostics::Flush() { std::lock_guard<std::mutex> lock(mutex_); if(file_)fflush(file_);if(events_)fflush(events_);if(beginning_)fflush(beginning_); }
}
