#include <winsock2.h>
#include <windows.h>
#include "coop/Diagnostics.h"
#include "coop/Settings.h"
#include "coop/MenuLayout.h"
#include <fstream>
#include <stdexcept>
#include <iostream>
void RunDiagnosticsTests() {
    using namespace zweiklang;
    auto check=[](bool value,const char* reason){if(!value)throw std::runtime_error(reason);};
    unsigned short port=0;
    {
        const std::vector<MenuRow> rows={{2000,200},{2400,200},{2800,200},{3200,200},{3600,200},{4000,200}};
        const auto saved=rows;
        const int y=FindMenuSlot(rows,200);
        check(y>=4240,"new menu row below all existing rows, including disabled");
        for(size_t i=0;i<rows.size();++i){
            check(rows[i].y==saved[i].y,"original menu positions preserved");
            check(y>=rows[i].y+rows[i].height||y+200<=rows[i].y,"no overlap with existing row");
        }
        check(FindMenuSlot({{0,7800}},200)==-1,"full menu fails safely instead of overlapping");
        check(FindMenuSlot({},200)==-1,"missing menu layout fails safely");
    }
    check(ParsePort("12345",port)&&port==12345,"port accepted");
    check(!ParsePort("0",port)&&!ParsePort("65536",port)&&!ParsePort("12x",port),"bad ports rejected");
    check(ValidHost("192.0.2.1")&&!ValidHost("hello")&&!ValidHost("0.0.0.0"),"host validation");
    check(ValidName("Anna")&&ValidName("M\xc3\xbcller")&&!ValidName("bad\nname")&&!ValidName("")&&!ValidName(std::string(33,'x'))&&!ValidName("\xff"),"UTF8 names validated");
    check(Diagnostics::Escape("a\n\"\\") == "a\\u000a\\\"\\\\","JSON escaping");
    char temp[MAX_PATH]{};GetTempPathA(MAX_PATH,temp);
    const auto folder=std::string(temp)+"ZweiklangLogTest-"+std::to_string(GetCurrentProcessId());
    std::string path;
    {
        Diagnostics logger;logger.Open(folder,1024);path=logger.Path();
        logger.Configure(1,"Anna");logger.Role("CLIENT");
        logger.Write(2,"debug_hidden","should not appear");logger.Write(1,"normal","first\nsecond");logger.Flush();
        std::ifstream file(path);std::string content((std::istreambuf_iterator<char>(file)),{});
        file.close();
        check(content.find("debug_hidden")==std::string::npos&&content.find("CLIENT")!=std::string::npos&&content.find("first\\u000asecond")!=std::string::npos,"level and context");
        check(content.find("\"time_utc\":\"")!=std::string::npos&&content.find("Z\"")!=std::string::npos,"UTC timestamp available for cross-PC log correlation");
        check(content.find("\"seq\":1,")!=std::string::npos,"first persisted event has monotonic sequence 1");
        logger.Write(1,"second_event","next");logger.Flush();
        {std::ifstream next(path);std::string second((std::istreambuf_iterator<char>(next)),{});check(second.find("\"seq\":2,")!=std::string::npos,"suppressed debug entries do not consume sequence; next event increments");}
        logger.Configure(2,"Anna");
        for(int i=0;i<100;++i)logger.Write(2,"rotation",std::string(200,'x'));
        logger.Flush();check(GetFileAttributesA((path+".1").c_str())!=INVALID_FILE_ATTRIBUTES,"log rotates");
        {std::ifstream events(path+".events.jsonl");std::string savedEvents((std::istreambuf_iterator<char>(events)),{});
         check(savedEvents.find("first\\u000asecond")!=std::string::npos&&savedEvents.find("rotation")==std::string::npos,"essential events survive packet/debug rotation independently");}
        {std::ifstream beginning(path+".begin.jsonl");std::string savedBeginning((std::istreambuf_iterator<char>(beginning)),{});
         check(savedBeginning.find("\"seq\":1,")!=std::string::npos,"session beginning preserved after rotating main log");}
        logger.Write(1,"test_marker","step=C09");logger.Flush();
        {std::ifstream events(path+".events.jsonl");std::string savedMarkers((std::istreambuf_iterator<char>(events)),{});
         check(savedMarkers.find("step=C09")!=std::string::npos,"test markers durably retained in essential event stream");}
        logger.Configure(0,"Anna");logger.Write(1,"disabled","hidden");logger.Flush();
        std::ifstream tail(path);std::string final((std::istreambuf_iterator<char>(tail)),{});
        check(final.find("disabled")==std::string::npos,"logging off respected");
    }
    for(int i=1;i<=4;++i)DeleteFileA((path+"."+std::to_string(i)).c_str());
    DeleteFileA(path.c_str());RemoveDirectoryA(folder.c_str());
    DeleteFileA((path+".events.jsonl").c_str());DeleteFileA((path+".begin.jsonl").c_str());RemoveDirectoryA(folder.c_str());
    std::cout<<"PASS: settings validation and diagnostic log levels, JSON escaping, rotation, role context\n";
}
