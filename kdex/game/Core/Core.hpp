#pragma once
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>
#include <Includes/Includes.hpp>
#include <Security/anticrack/WindowCheck.hpp>
#include <Security/xorstr.hpp>
#include "SetupOffsets.hpp"
#include "../../framework/settings/search.h"
#include "Threads/EntityList.hpp"
#include "Threads/ScreenResolution.hpp"
#include "Threads/UpdateNames.hpp"
#include "Threads/UpdatePointers.hpp"
#include "Threads/VehicleList.hpp"
#include "Threads/ObjectList.hpp"

#include "Features/Aimbot.hpp"
#include "Features/ESP.hpp"
#include "Features/MagicBullets.hpp"
#include "Features/SilentAim.hpp"
#include "Features/Triggerbot.hpp"

#include "Features/Exploits/Exploits.hpp"
#include "Features/Exploits/ResourceList.hpp"
#include "Features/Exploits/FakeFps.hpp"
#include <Auth/lazyimporter.hpp>

#include "SDK/Natives/NativeCaller.hpp"

#include "SDK/Natives/CitizenNativeCore.hpp"
#include "SDK/Natives/Natives.hpp"

#include "../../../test/ResourceManagerV2.hpp"
#include "../../../test/LuaExecutor.hpp"

#include <Windows.h>
#include <functional>
#include <regex>
#include <vector>

namespace Core {

inline static bool ThreadsStarted = false;

inline void StartNativeCaller() {
  while (!g_Variables.g_Unload) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (!Core::g_AttachedToGame) continue;
    NativeCaller::g_NativeCaller.EnsureReady();
    ResourceV2::g_ResourceManagerV2.Init();
    LuaExec::g_LuaExecutor.Init();
  }
}

inline void StartFakeFps() {
  FakeFps::g_FakeFps.Start();
  while (!g_Variables.g_Unload) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (!Core::g_AttachedToGame) continue;

    if (option->param.fake_fps_enabled) {
      if (option->param.fake_fps_range_mode) {
        FakeFps::g_FakeFps.SetFpsRange(option->param.fake_fps_min, option->param.fake_fps_max);
      } else {
        FakeFps::g_FakeFps.SetFps(option->param.fake_fps_target);
      }
    } else {
      FakeFps::g_FakeFps.Disable();
    }
  }
}

inline void StartThreads() {
  if (ThreadsStarted)
    return;
  ThreadsStarted = true;

  g_Variables.g_Unload = false;

  std::thread(&Threads::cScreenResolution::Update, &Threads::g_ScreenResolution).detach();
  std::thread(&Threads::cUpdatePtrs::Update, &Threads::g_UpdatePtrs).detach();
  std::thread(&Threads::cEntityList::Update, &Threads::g_EntityList).detach();
  std::thread(&Threads::cVehicleList::Update, &Threads::g_VehicleList).detach();
  std::thread(&Threads::cObjectList::Update, &Threads::g_ObjectList).detach();
  std::thread(&Threads::cUpdateNames::Update, &Threads::g_UpdateNames).detach();
  std::thread(&Features::Exploits::cResourceList::List, &Features::Exploits::g_ResourceList).detach();
  std::thread(&Features::Exploits::RunThread, std::ref(Features::Exploits::g_Exploits)).detach();
  std::thread(&Features::cAimbot::Start, Features::g_Aimbot).detach();
  std::thread(&Features::cSilentAim::HookSilent, Features::g_SilentAim).detach();
  std::thread(&Features::cTriggerbot::Start, Features::g_Triggerbot).detach();
  std::thread(&Features::cMagicBullets::Start, Features::g_MagicBullets).detach();
  std::thread(&Features::StartTracer).detach();
  std::thread(&StartNativeCaller).detach();

  std::thread(&StartFakeFps).detach();
}
}

#include "Attach.hpp"
