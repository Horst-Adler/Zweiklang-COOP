#pragma once
#include <windows.h>

namespace zweiklang {
// Gothic supplies DirectInput scan codes. Translate text with the current
// Windows layout instead of Gothic's fixed language table. No layout is changed.
inline unsigned char MenuCharacter(unsigned int scan, HKL layout, const BYTE* state) {
    if(!layout||scan==0||scan>=0x100)return 0;
    const UINT windowsScan=(scan&0x80)?(0xE000|(scan&0x7f)):scan;
    const UINT key=MapVirtualKeyExW(windowsScan,MAPVK_VSC_TO_VK_EX,layout);
    if(!key)return 0;
    WCHAR text[4]{};
    // Bit 2 prevents this translation from altering Windows' dead-key state.
    if(ToUnicodeEx(key,windowsScan,state,text,4,4,layout)!=1||text[0]<32)return 0;
    char result[4]{};BOOL substituted=FALSE;
    const int count=WideCharToMultiByte(CP_ACP,WC_NO_BEST_FIT_CHARS,text,1,result,4,nullptr,&substituted);
    return count==1&&!substituted?static_cast<unsigned char>(result[0]):0;
}
}
