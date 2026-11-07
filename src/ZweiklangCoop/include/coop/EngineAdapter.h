#pragma once

#include <windows.h>

namespace zweiklang {
extern HMODULE moduleHandle;
namespace engine {
void Initialize();
void Shutdown();
void Frame();
void MenuFrame();
void BeforeSave();
void AfterSave();
void BeforeLoad();
void AfterLoad();
}
}
