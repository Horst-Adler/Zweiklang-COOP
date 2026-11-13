#pragma once
#include <windows.h>
#include <string>
#include <stdexcept>
#include <filesystem>
namespace zweiklang::native_save {
inline std::runtime_error Error(const char* operation,const std::string& path,DWORD code=GetLastError()) {
    return std::runtime_error(std::string(operation)+" path="+path+" win32="+std::to_string(code));
}
inline bool Absolute(const std::string& p) {
    return p.size()>2&&((p[0]=='\\'&&p[1]=='\\')||(p[1]==':'&&(p[2]=='\\'||p[2]=='/')));
}
inline std::string FullPath(const std::string& input) {
    if(!Absolute(input))throw Error("Save path is not absolute",input,ERROR_BAD_PATHNAME);
    char out[MAX_PATH];const auto n=GetFullPathNameA(input.c_str(),MAX_PATH,out,nullptr);
    if(!n||n>=MAX_PATH)throw Error("Cannot resolve save path",input,n>=MAX_PATH?ERROR_FILENAME_EXCED_RANGE:GetLastError());
    std::string p=out;while(p.size()>3&&p.back()=='\\')p.pop_back();return p;
}
inline std::string Slot(const std::string& absoluteBase,std::string relative) {
    const auto base=FullPath(absoluteBase);
    while(!relative.empty()&&(relative.back()=='\\'||relative.back()=='/'))relative.pop_back();
    if(relative.empty()||relative=="."||relative==".."||relative.find_first_of("\\/:\r\n")!=std::string::npos)
        throw Error("Invalid native save slot",relative,ERROR_BAD_PATHNAME);
    return FullPath(base+"\\"+relative)+"\\";
}
inline void PrepareDirectory(const std::string& input) {
    const auto dir=FullPath(input);
    std::error_code error;std::filesystem::create_directories(dir,error);
    if(error)throw Error("Cannot create save checkpoint directory",dir,static_cast<DWORD>(error.value()));
    const auto attr=GetFileAttributesA(dir.c_str());
    if(attr==INVALID_FILE_ATTRIBUTES||!(attr&FILE_ATTRIBUTE_DIRECTORY))throw Error("Save destination is not a directory",dir,ERROR_DIRECTORY);
    // A unique disposable probe touches no existing save data. Run before the
    // game manager can change metadata, thumbnails or script sidecars.
    char file[MAX_PATH];if(!GetTempFileNameA(dir.c_str(),"ZK",0,file))throw Error("Save destination is not writable",dir);
    if(!DeleteFileA(file))throw Error("Cannot remove save preflight probe",file);
}
struct Stamp {bool exists=false;DWORD high=0,low=0,sizeHigh=0,sizeLow=0;};
inline void CancelGuard(const std::string& path){
    if(DeleteFileA(path.c_str()))return;const auto code=GetLastError();
    if(code!=ERROR_FILE_NOT_FOUND&&code!=ERROR_PATH_NOT_FOUND)throw Error("Cannot cancel save checkpoint guard",path,code);
}
inline Stamp Capture(const std::string& path) {
    WIN32_FILE_ATTRIBUTE_DATA a{};if(!GetFileAttributesExA(path.c_str(),GetFileExInfoStandard,&a))return {};
    return {!(a.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY),a.ftLastWriteTime.dwHighDateTime,a.ftLastWriteTime.dwLowDateTime,a.nFileSizeHigh,a.nFileSizeLow};
}
inline bool Advanced(const Stamp& before,const Stamp& after) {
    return after.exists&&(after.sizeHigh||after.sizeLow)&&(!before.exists||before.high!=after.high||before.low!=after.low||before.sizeHigh!=after.sizeHigh||before.sizeLow!=after.sizeLow);
}
}
