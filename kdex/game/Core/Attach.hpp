#pragma once
#include "SetupOffsets.hpp"
#include "Threads/EntityList.hpp"
#include "Threads/VehicleList.hpp"
#include "Threads/ObjectList.hpp"
#include "Features/SilentAim.hpp"
#include "Features/MagicBullets.hpp"
#include <Includes/Utils.hpp>

namespace Core {

inline bool AttachWhenFiveMFound(HWND game_hwnd) {
  if (!game_hwnd || g_AttachedToGame)
    return g_AttachedToGame;
  Core::g_Variables.g_hGameWindow = game_hwnd;
  GetWindowThreadProcessId(game_hwnd, &Core::g_Variables.ProcIdFiveM);
  if (Core::g_Variables.ProcIdFiveM == 0)
    return false;
  auto WindowInfo = Utils::GetWindowPosAndSize(game_hwnd);
  Core::g_Variables.g_vGameWindowSize =
      { (float)WindowInfo.second.x, (float)WindowInfo.second.y };
  Core::g_Variables.g_vGameWindowPos =
      { (float)WindowInfo.first.x, (float)WindowInfo.first.y };
  Core::g_Variables.g_vGameWindowCenter = {Core::g_Variables.g_vGameWindowSize.x / 2.f,
                                     Core::g_Variables.g_vGameWindowSize.y / 2.f};

  g_Offsets = {};

  Core::SDK::Pointers::pWorld = nullptr;
  Core::SDK::Pointers::pLocalPlayer = nullptr;
  Core::SDK::Pointers::pReplayInterFace = nullptr;
  Core::SDK::Pointers::pViewPort = 0;
  Core::SDK::Pointers::pCamGamePlayDirector = 0;

  Features::g_SilentAim.SilentAimInitialized = false;
  Features::g_SilentAim.SilentAimHook = 0;
  Features::g_MagicBullets.Initialized = false;
  {
    std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
    Core::SDK::Game::EntityList.clear();
  }
  {
    std::lock_guard<std::mutex> lock(Core::SDK::Game::VehicleListMutex);
    Core::SDK::Game::VehicleList.clear();
    Threads::g_VehicleList.Reset();
  }
  {
    std::lock_guard<std::mutex> lock(Core::SDK::Game::ObjectListMutex);
    Core::SDK::Game::ObjectList.clear();
    Threads::g_ObjectList.Reset();
  }
  {
    std::lock_guard<std::mutex> lock(Core::SDK::Game::PickupListMutex);
    Core::SDK::Game::PickupList.clear();
  }

  if (!SetupOffsets()) {
    return false;
  }

  StartThreads();
  g_AttachedToGame = true;
  return true;
}

}
