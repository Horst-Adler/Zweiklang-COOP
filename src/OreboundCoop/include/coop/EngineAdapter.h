#pragma once

#include <windows.h>

namespace orebound {
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
