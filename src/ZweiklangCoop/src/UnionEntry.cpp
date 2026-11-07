// Zweiklang Coop: original Union integration, 2026.
// The Union/Gothic API headers are third-party SDK dependencies.
#include <winsock2.h>
#include <windows.h>
#include <crtversion.h>
#include "UnionAfx.h"
#include "coop/EngineAdapter.h"

namespace zweiklang { HMODULE moduleHandle = nullptr; }

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        zweiklang::moduleHandle = instance;
        DisableThreadLibraryCalls(instance);
        Union.DefineCRTVersion(_VC_CRT_MAJOR_VERSION, _VC_CRT_MINOR_VERSION,
                               _VC_CRT_BUILD_VERSION, _VC_CRT_RBUILD_VERSION);
    }
    return TRUE;
}
