#include "coop/NativeSave.h"
#include <fstream>
void TestNativeSavePaths(){
    namespace ns=zweiklang::native_save;
    const auto dir=recovery_test::Temp();
    const auto original=std::filesystem::current_path();
    struct RestoreCwd {std::filesystem::path value;~RestoreCwd(){std::error_code e;std::filesystem::current_path(value,e);}} restore{original};
    const auto base=dir+"\\save root",slot=ns::Slot(base,"savegame1\\");
    std::filesystem::current_path(dir);
    Check(ns::Slot(base,"savegame1\\")==slot,"native save path independent of engine working directory");
    ns::PrepareDirectory(slot);
    Check(GetFileAttributesA(slot.c_str())!=INVALID_FILE_ATTRIBUTES,"missing save parent and slot created before native write");
    Check(std::filesystem::is_empty(slot),"preflight leaves no disposable probe or game metadata");
    for(const auto& name:{"..","../outside","..\\outside","C:\\other","\\\\server\\share","savegame1:stream",""}){
        bool refused=false;try{ns::Slot(base,name);}catch(const std::exception&){refused=true;}
        Check(refused,"save directory traversal, absolute slot and alternate stream refused");
    }
    bool refused=false;try{ns::Slot("relative-save-root","savegame1");}catch(const std::exception&){refused=true;}
    Check(refused,"configured relative save base must not silently resolve against changing CWD");
    const auto file=slot+"SAVEDAT.SAV";
    auto before=ns::Capture(file);Check(!before.exists,"missing native character data detected");
    {std::ofstream f(file,std::ios::binary);f<<"test character data";}
    const auto written=ns::Capture(file);Check(ns::Advanced(before,written),"new nonempty character file accepted");
    Check(!ns::Advanced(written,ns::Capture(file)),"unchanged old character data cannot validate partial save");
    const auto metadata=slot+"SAVEINFO.SAV";{std::ofstream f(metadata);f<<"new thumbnail and metadata only";}
    Check(!ns::Advanced(written,ns::Capture(file)),"metadata-only update never proves native character write");
    {std::ofstream f(file,std::ios::binary|std::ios::trunc);}
    Check(!ns::Advanced(written,ns::Capture(file)),"empty/truncated native file cannot validate completed save");
    const auto blocked=dir+"\\file-is-not-a-directory";{std::ofstream f(blocked);f<<"preserve";}
    bool nativeCalled=false;refused=false;
    try{ns::PrepareDirectory(blocked+"\\savegame2");nativeCalled=true;}catch(const std::exception& e){refused=std::string(e.what()).find("win32=")!=std::string::npos;}
    Check(refused&&!nativeCalled,"failed destination preflight reports OS error before game manager mutation");
    {std::ifstream f(blocked);std::string text;f>>text;Check(text=="preserve","preflight never replaces an obstructing existing file");}
    const auto guard=dir+"\\saving-slot2.bin";
    ns::CancelGuard(guard);Check(!std::filesystem::exists(guard),"cancelling a missing save guard is idempotent");
    HANDLE locked=CreateFileA(guard.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    Check(locked!=INVALID_HANDLE_VALUE,"create isolated locked save guard");
    refused=false;try{ns::CancelGuard(guard);}catch(const std::exception& e){refused=std::string(e.what()).find("win32=")!=std::string::npos;}
    Check(refused&&std::filesystem::exists(guard),"failed guard cancellation remains visible instead of silently poisoning later loads");
    CloseHandle(locked);ns::CancelGuard(guard);Check(!std::filesystem::exists(guard),"cancelled preflight guard removed once writable");
    std::filesystem::current_path(original);
    std::filesystem::remove_all(dir);
}
