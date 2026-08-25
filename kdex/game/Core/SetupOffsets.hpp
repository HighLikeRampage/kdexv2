#pragma once
#include "ProcessUtils.hpp"
#include <Auth/lazyimporter.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>
#include <Security/AntiCrack.hpp>
#include <Security/anticrack/WindowCheck.hpp>
#include <Security/xorstr.hpp>
#include <algorithm>
#include <fstream>
#include <string>

namespace Core {

inline bool SetupOffsets() {
  g_Offsets.CurrentBuild = GetBuild();

  std::string rawProcName =
      AntiCrack::WindowCheck::get_process_name(Core::g_Variables.ProcIdFiveM);
  Mem.ProcName = AntiCrack::WindowCheck::widen(rawProcName.c_str());
  Mem.ProcId = Core::g_Variables.ProcIdFiveM;

  if (g_Offsets.CurrentBuild <= 0) {
    std::string pName = rawProcName;
    std::transform(pName.begin(), pName.end(), pName.begin(), ::tolower);

    size_t bPos = pName.find(xorstr("_b"));
    if (bPos != std::string::npos) {
      std::string buildStr = "";
      for (size_t i = bPos + 2; i < pName.size() && isdigit(pName[i]); ++i) {
        buildStr += pName[i];
      }
      if (!buildStr.empty()) {
        g_Offsets.CurrentBuild = std::stoi(buildStr);
      }
    }
  }

  if (!Mem.OpenProcByPid())
    return false;

  if (!Mem.ModBase || !Mem.ModBaseSize || !Core::g_Variables.ProcIdFiveM)
    return false;
  g_Offsets.m_RocketSpeed = 0x58;
  g_Offsets.m_Range = 0x28;
  g_Offsets.m_NoReload = 0x134;
  g_Offsets.m_BulletDamage = 0xB0;

  auto p2v = [&](const std::string &s) { return Mem.Pattern2Vector(s); };
  auto scanRel = [&](const std::string &s, int len) -> uintptr_t {
    uintptr_t a = Mem.FindSignature(Mem.Pattern2Vector(s));
    if (a) {
      int32_t rel = Mem.Read<int32_t>(a + (len - 4));
      return a + len + rel;
    }
    return 0;
  };

  enum class type { offset, relative };

  auto resolvePattern = [&](const std::string &pattern, int offset,
                            type t) -> uintptr_t {
    uintptr_t a = Mem.FindSignature(p2v(pattern));
    if (!a)
      return 0;

    if (t == type::offset) {
      return Mem.Read<int>(a + offset);
    } else if (t == type::relative) {
      int32_t rel = Mem.Read<int32_t>(a + offset);
      return a + offset + 4 + rel;
    }
    return 0;
  };

  if (!g_Offsets.m_FragInst)
    g_Offsets.m_FragInst = resolvePattern(
        xorstr("48 3b 83 ? ? ? ? 74 ? 48 8b 83 ? ? ? ? 48 85 c0 74 ? b9"), 3,
        type::offset);

  if (!g_Offsets.m_World)
    g_Offsets.m_World = scanRel(
        xorstr("48 8B 05 ? ? ? ? 33 D2 48 8B 40 08 8A CA 48 85 C0 74 16 48 8B"),
        7);
  if (!g_Offsets.m_World)
    g_Offsets.m_World = scanRel(
        xorstr("48 8B 0D ? ? ? ? 45 33 C0 48 8B D7 E8 ? ? ? ? EB 0A"), 7);
  if (!g_Offsets.m_World)
    g_Offsets.m_World = scanRel(
        xorstr("48 8B 0D ? ? ? ? 45 33 C0 48 8B D7 E8 ? ? ? ? EB 1B"), 7);
  if (!g_Offsets.m_World)
    g_Offsets.m_World =
        scanRel(xorstr("48 8B 0D ? ? ? ? 45 33 C0 48 8B D3 44 8A F0"), 7);
  if (!g_Offsets.m_ViewPort)
    g_Offsets.m_ViewPort = scanRel(
        xorstr("48 8B 15 ?? ?? ?? ?? 48 8D 2D ?? ?? ?? ?? 48 8B CD"), 7);
  if (!g_Offsets.m_Camera)
    g_Offsets.m_Camera =
        scanRel(xorstr("48 8B 05 ? ? ? ? 48 8B 98 ? ? ? ? EB"), 7);
  if (!g_Offsets.m_Camera)
    g_Offsets.m_Camera = scanRel(xorstr("4C 8B 35 ? ? ? ? 33 FF 32 DB"), 7);
  if (!g_Offsets.m_ReplayInterFace) {
    g_Offsets.m_ReplayInterFace = scanRel(
        xorstr("48 8B 05 ?? ?? ?? ?? 66 89 0D ?? ?? ?? ?? 4C 89 2C D0"), 7);
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = scanRel(
          xorstr("48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 4C 24 ?? E8 ?? ?? "
                 "?? ?? 48 8D 4C 24 ?? E8 ?? ?? ?? ?? 48 8B 9C 24"),
          7);
  }
  if (!g_Offsets.m_SwapChain)
    g_Offsets.m_SwapChain = scanRel(
        xorstr("48 8B 0D ? ? ? ? 48 8B 01 44 8D 43 01 33 D2 FF 50 40 8B C8"),
        7);
  if (!g_Offsets.m_PointerToHandle)
    g_Offsets.m_PointerToHandle = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 8B "
                   "15 ? ? ? ? 48 8B F9 48 83 C1 10 33 DB")));
  if (!g_Offsets.m_TriggerbotFunc)
    g_Offsets.m_TriggerbotFunc = Mem.FindSignature(
        p2v(xorstr("48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B 0D ?? "
                   "?? ?? ?? 48 85 C9 74 05 E8 ?? ?? ?? ?? 8A CB")));
  if (!g_Offsets.m_BoneMaskFunc)
    g_Offsets.m_BoneMaskFunc = Mem.FindSignature(p2v(xorstr(
        "48 89 5C 24 ?? 48 89 6C 24 ?? 48 89 74 24 ?? 57 48 83 EC 60 48 8B "
        "01 41 8B E8 48 8B F2 48 8B F9 33 DB")));
  if (!g_Offsets.m_PlayerInfo) {
    uintptr_t a = Mem.FindSignature(p2v(
        xorstr("48 8B 83 ? ? ? ? 83 B8 ? ? ? ? ? 0F 84 ? ? ? ? 48 8B 48 20")));
    if (a)
      g_Offsets.m_PlayerInfo = Mem.Read<int>(a + 3);
  }
  if (!g_Offsets.m_PlayerInfo) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 8b 89 ? ? ? ? 48 85 c9 74 ? 48 8d 81 ? ? ? ? c3")));
    if (a)
      g_Offsets.m_PlayerInfo = Mem.Read<int>(a + 3);
  }
  if (!g_Offsets.m_Handling) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 8B 83 ? ? ? ? 8B D5 48 89 44 24 ? 4C 89 74 24")));
    if (a)
      g_Offsets.m_Handling = Mem.Read<int>(a + 3);
  }
  g_Offsets.m_InfiniteCombatRoll =
      Mem.FindSignatureStr(xorstr("89 81 00 00 00 00 8B 87 00 00 00 00 F7 D0"));
  g_Offsets.m_GiveWeapon =
      Mem.FindSignatureStr(xorstr("48 89 5C 24 00 48 89 6C 24 00 "
                                  "48 89 74 24 00 57 48 83 EC 00 "
                                  "41 8B F0 8B FA 48 8B D9 E8"));
  g_Offsets.m_InfiniteAmmo0 =
      Mem.FindSignatureStr(xorstr("41 2B C9 3B C8 0F 4D C8"));
  g_Offsets.m_InfiniteAmmo1 = Mem.FindSignatureStr(xorstr("41 2B D1 E8"));
  g_Offsets.m_SilentAim =
      Mem.FindSignatureStr(xorstr("48 8D 45 00 F3 0F 10 00 F3 "
                                  "0F 10 48 00 F3 0F 11 45"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(
        xorstr("48 8D 85 00 00 00 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 45"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(
        xorstr("48 8D 45 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 85"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(
        xorstr("48 8D 85 00 00 00 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 85"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(
        xorstr("48 8D 4D 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 45"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(
        xorstr("48 8D 4D 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 4D"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(
        xorstr("48 8D 44 24 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 44 24"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(xorstr(
        "48 8D 84 24 00 00 00 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 44 24"));
  if (!g_Offsets.m_SilentAim)
    g_Offsets.m_SilentAim = Mem.FindSignatureStr(xorstr(
        "48 8D 84 24 00 00 00 00 F3 0F 10 00 F3 0F 10 48 00 F3 0F 11 84 24"));
  {
    uintptr_t recoilAddr =
        Mem.FindSignatureStr(xorstr("F3 0F 10 B0 00 00 00 00 49 8B 87"));
    g_Offsets.m_Recoil = recoilAddr ? Mem.Read<int>(recoilAddr + 4) : 0;
  }
  {
    uintptr_t spreadAddr =
        Mem.FindSignatureStr(xorstr("F3 41 0F 10 B0 00 00 00 00 "
                                    "48 8B CA"));
    g_Offsets.m_Spread = spreadAddr ? Mem.Read<int>(spreadAddr + 4) : 0;
  }
  if (!g_Offsets.m_WeaponManager) {
    uintptr_t a = Mem.FindSignature(p2v(xorstr("48 8b 83 ? ? ? ? 44 8a fe")));
    if (a)
      g_Offsets.m_WeaponManager = Mem.Read<int>(a + 3);
    if (!g_Offsets.m_WeaponManager) {
      uintptr_t a = Mem.FindSignature(
          p2v(xorstr("48 8B 88 ? ? ? ? 48 83 79 ? ? 74 ? 45 33 C9")));
      if (a)
        g_Offsets.m_WeaponManager = Mem.Read<int>(a + 3);
    }
    if (!g_Offsets.m_WeaponManager) {
      uintptr_t b = Mem.FindSignature(
          p2v(xorstr("0F 84 ? ? ? ? 48 8B 8F ? ? ? ? E8 ? ? ? ? 48 8B F0")));
      if (b)
        g_Offsets.m_WeaponManager = Mem.Read<int>(b + 9);
    }
  }
  {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("8B 82 ? ? ? ? C1 E0 ? C1 F8 ? 41 2B C4")));
    if (a && !g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = Mem.Read<int>(a + 2);
  }
  {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("8B 86 ? ? ? ? C1 E8 ? A8 ? 74 ? 48 8B 97")));
    if (a && !g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = Mem.Read<int>(a + 2);
  }
  {
    uintptr_t a =
        Mem.FindSignature(p2v(xorstr("48 8B 82 ? ? ? ? F3 0F 10 83 ? ? ? ?")));
    if (a && !g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = Mem.Read<int>(a + 11);
  }
  if (!g_Offsets.m_Armor) {
    uintptr_t a =
        Mem.FindSignature(p2v(xorstr("f3 0f 10 8b ? ? ? ? f3 0f 2c d1")));
    if (a)
      g_Offsets.m_Armor = Mem.Read<int>(a + 4);
  }
  if (!g_Offsets.m_VehicleDriver) {
    uintptr_t a = Mem.FindSignatureStr(
        xorstr("4c 8b 94 c7 00 00 00 00 eb 00 4d 8b d6 41 8a 82"));
    if (a)
      g_Offsets.m_VehicleDriver = Mem.Read<int>(a + 4);
  }
  g_Offsets.m_WeaponInfo =
      Mem.FindSignatureStr(xorstr("48 8B 8B 00 00 00 00 48 8B "
                                  "D7 E8 00 00 00 00 48 8B 8B "
                                  "00 00 00 00 48 8B D7 E8"));

  if (g_Offsets.CurrentBuild != 0) {
    {
      g_Offsets.m_GiveWeapon =
          Mem.FindSignatureStr(xorstr("48 89 5c 24 00 48 89 6c 24 00 "
                                      "48 89 74 24 00 57 48 83 ec 00 "
                                      "41 8b f0 8b fa 48 8b d9 e8"));
      {
        uintptr_t tmp = Mem.FindSignatureStr(xorstr("0F 29 4F 00 83 8F 00 00 "
                                                    "00 00 00 48 8B 4F"));
        if (tmp)
          g_Offsets.m_MagicBulletsPatch = tmp;
      }
      g_Offsets.m_ArmsKinematics =
          Mem.FindSignatureStr(xorstr("E8 00 00 00 00 48 83 C3 60 48 FF "
                                      "CF 75 E6 48 8B 5C 24"));
      g_Offsets.m_LegsKinematics =
          Mem.FindSignatureStr(xorstr("E8 00 00 00 00 48 83 C3 60 48 "
                                      "FF CF 75 DF 48 8B 5C 24 00 48 "
                                      "8B 6C 24 00 48 8B 74 24"));
      {
        uintptr_t tmp =
            Mem.FindSignatureStr(xorstr("48 8D 45 00 F3 0F 10 00 F3 0F 10 "
                                        "48 00 F3 0F 11 45"));
        if (tmp)
          g_Offsets.m_SilentAim = tmp;
      }
      g_Offsets.m_InfiniteCombatRoll =
          Mem.FindSignatureStr(xorstr("89 81 00 00 00 00 8B 87 "
                                      "00 00 00 00 F7 D0"));
      g_Offsets.m_InfiniteAmmo0 =
          Mem.FindSignatureStr(xorstr("41 2B C9 3B C8 0F 4D C8"));
      g_Offsets.m_InfiniteAmmo1 = Mem.FindSignatureStr(xorstr("41 2B D1 E8"));
      g_Offsets.m_AimCPedPatternResult =
          Mem.FindSignatureStr(xorstr("48 8D 0D 00 00 00 00 E8 00 00 00 "
                                      "00 48 8B 0D 00 00 00 00 48 85 C9 "
                                      "74 05 E8 00 00 00 00 8A CB"));
      uintptr_t SpeedSigAddr =
          Mem.FindSignatureStr(xorstr("0f 2f c2 77 00 f3 0f 10 "
                                      "83 00 00 00 00 48 83 c4"));
      if (SpeedSigAddr)
        g_Offsets.m_Speed = Mem.Read<int>(SpeedSigAddr + 9);
      uintptr_t SpreadSigAddr =
          Mem.FindSignatureStr(xorstr("f3 41 0f 10 b0 00 00 00 "
                                      "00 48 8b ca"));
      if (SpreadSigAddr)
        g_Offsets.m_Spread = Mem.Read<int>(SpreadSigAddr + 5);

      uintptr_t SpreadHSigAddr =
          Mem.FindSignatureStr(xorstr("F3 0F 59 70 ? 0F 28 C6"));
      if (SpreadHSigAddr)
        g_Offsets.m_Spread = Mem.Read<int>(SpreadHSigAddr + 4);

      if (!g_Offsets.m_Spread) {
        uintptr_t SpreadVSigAddr =
            Mem.FindSignatureStr(xorstr("F3 0F 10 78 ? 44 38 85"));
        if (SpreadVSigAddr)
          g_Offsets.m_Spread = Mem.Read<int>(SpreadVSigAddr + 4);
      }

      uintptr_t RecoilSigAddr =
          Mem.FindSignatureStr(xorstr("f3 0f 10 b0 00 00 00 00 49 8b 87"));
      if (RecoilSigAddr)
        g_Offsets.m_Recoil = Mem.Read<int>(RecoilSigAddr + 4);
      uintptr_t ObjectSigAddr =
          Mem.FindSignatureStr(xorstr("4c 8b 41 00 4d 85 c9"));
      if (ObjectSigAddr)
        g_Offsets.m_CObject = (int)Mem.Read<BYTE>(ObjectSigAddr + 3);
      uintptr_t CWeaponSigAddr =
          Mem.FindSignatureStr(xorstr("48 8b 8b 00 00 00 00 48 8b "
                                      "d7 e8 00 00 00 00 48 8b 8b "
                                      "00 00 00 00 48 8b d7 e8"));
      if (CWeaponSigAddr)
        g_Offsets.m_CWeapon = Mem.Read<int>(CWeaponSigAddr + 3);
      uintptr_t RagDollSigAddr =
          Mem.FindSignatureStr(xorstr("8b 81 00 00 00 00 83 e0 00 c1 e0 "
                                      "00 3d 00 00 00 00 0f 8f"));
      if (RagDollSigAddr)
        g_Offsets.m_NoRagDoll = Mem.Read<int>(RagDollSigAddr + 2);
      uintptr_t SeatBealtAddr =
          Mem.FindSignatureStr(xorstr("f6 81 00 00 00 00 00 75 00 8b 83"));
      if (SeatBealtAddr)
        g_Offsets.m_SeatBealt = Mem.Read<int>(SeatBealtAddr + 2);
      uintptr_t VehicleDoorsLockAddr =
          Mem.FindSignatureStr(xorstr("74 00 83 b9 00 00 00 00 "
                                      "00 75 00 45 84 f6"));
      if (VehicleDoorsLockAddr)
        g_Offsets.m_VehicleDoorsLockState =
            Mem.Read<int>(VehicleDoorsLockAddr + 4);
      uintptr_t HandlingSigAddr =
          Mem.FindSignatureStr(xorstr("48 8b 83 00 00 00 00 8b d5 48 89 "
                                      "44 24 00 4c 89 74 24"));
      if (HandlingSigAddr)
        g_Offsets.m_Handling = Mem.Read<int>(HandlingSigAddr + 3);
    }
  }

  if (!g_Offsets.m_IsPedFalling) {
    g_Offsets.m_IsPedFalling = Mem.FindSignatureStr(xorstr("48 8B 81 ? ? ? ? 8B 80 ? ? ? ? C1 E8 ? 83 E0 ? C3 48 8B C4"));
    if (!g_Offsets.m_IsPedFalling) {
      g_Offsets.m_IsPedFalling = Mem.FindSignatureStr(xorstr("48 8B 81 ? ? ? ? 8B 80 ? ? ? ? C1 E8 ? 83 E0 ? C3 48 8B C4 48 89 58 ? 48 89 70"));
    }
  }
  if (!g_Offsets.m_GetPedAmmoByType) {
    g_Offsets.m_GetPedAmmoByType = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1"));
  }
  if (!g_Offsets.m_GetAmmoInPedWeapon) {
    g_Offsets.m_GetAmmoInPedWeapon = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1"));
  }
  if (!g_Offsets.m_GetSelectedPedWeapon) {
    g_Offsets.m_GetSelectedPedWeapon = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1"));
  }
  if (!g_Offsets.m_GetBestPedWeapon) {
    g_Offsets.m_GetBestPedWeapon = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1"));
  }
  if (!g_Offsets.m_GetCurrentPedWeapon) {
    g_Offsets.m_GetCurrentPedWeapon = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1"));
  }
  if (!g_Offsets.m_SHOOT_SINGLE_BULLET_BETWEEN_COORDS) {
    g_Offsets.m_SHOOT_SINGLE_BULLET_BETWEEN_COORDS = Mem.FindSignatureStr(xorstr("40 53 48 83 EC ? 48 8B 49 ? 33 C0"));
  }
  if (!g_Offsets.m_RequestAnimDict) {
    g_Offsets.m_RequestAnimDict = Mem.FindSignatureStr(xorstr("48 83 EC ? 48 8B D1 33 C9 E8 ? ? ? ? 48 8D 54 24 ? 48 8D 4C 24 ? 89 44 24 ? E8 ? ? ? ? 83 7C 24 ? ? 74 ? E8 ? ? ? ? 48 85 C0"));
  }
  if (!g_Offsets.m_HasAnimDictLoaded) {
    g_Offsets.m_HasAnimDictLoaded = Mem.FindSignatureStr(xorstr("48 83 EC ? 48 8B D1 33 C9 E8 ? ? ? ? 48 8D 54 24 ? 48 8D 4C 24 ? 89 44 24 ? E8 ? ? ? ? 83 7C 24 ? ? 74 ? E8 ? ? ? ? E8"));
  }
  if (!g_Offsets.m_TaskPlayAnim) {
    g_Offsets.m_TaskPlayAnim = Mem.FindSignatureStr(xorstr("48 81 EC ? ? ? ? 44 8B 8C 24"));
  }
  if (!g_Offsets.m_RageAtHashCompute) {
    g_Offsets.m_RageAtHashCompute = Mem.FindSignatureStr(xorstr("48 63 C1 48 8B CA 4C 8D 04 C0 48 8D 05 ? ? ? ? 4A FF 24 C0"));
  }
  if (!g_Offsets.m_FindAnimDictSlot) {
    g_Offsets.m_FindAnimDictSlot = Mem.FindSignatureStr(xorstr("40 53 48 83 EC 20 8B 02 48 8B D9 48 8B D1 48 8D 0D ? ? ? ? 4C 8D 44 24 30 89 44 24 30"));
  }
  if (!g_Offsets.m_StreamingRequestBuild) {
    g_Offsets.m_StreamingRequestBuild = Mem.FindSignatureStr(xorstr("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC 30 41 8B D8 8B FA 48 8B F1 E8 ? ? ? ? BA 0D 00 00 00"));
  }
  if (!g_Offsets.m_SetEntityHealthNative) {
    g_Offsets.m_SetEntityHealthNative = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 0F 29 74 24 ? 0F 28 F1 48 8B F1 0F 2F B1"));
  }
  if (!g_Offsets.m_CPedIntelligenceClearTasks) {
    g_Offsets.m_CPedIntelligenceClearTasks = Mem.FindSignatureStr(xorstr("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 56 41 57 48 81 EC C0 00 00 00 45 33 FF 45 8A F0 48 8B D9 84 D2"));
  }
  if (!g_Offsets.m_CPedIntelligenceFlushImmediately) {
    g_Offsets.m_CPedIntelligenceFlushImmediately = Mem.FindSignatureStr(xorstr("48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 41 54 41 55 41 56 41 57 48 81 EC 90 00 00 00 48 8B 81 ? ? ? ? 45 8A F9 45 8A E0"));
  }
  if (!g_Offsets.m_CPedSetIsCrouching) {
    g_Offsets.m_CPedSetIsCrouching = Mem.FindSignatureStr(xorstr("48 89 5C 24 08 57 48 83 EC 20 8B 81 ? ? ? ? 48 8B D9 C1 E8 04 A8 01 0F 85"));
  }
  if (!g_Offsets.m_CPhysicalDetachFromParent) {
    g_Offsets.m_CPhysicalDetachFromParent = Mem.FindSignatureStr(xorstr("48 8B C4 48 89 58 10 48 89 70 18 48 89 78 20 55 41 54 41 55 41 56 41 57 48 8D A8 98 F6 FF FF 48 81 EC 40 0A 00 00 80 79 28 03"));
  }
  if (!g_Offsets.m_CommandIsEntityAttached) {
    uintptr_t wrapper = Mem.FindSignatureStr(xorstr("40 53 48 83 EC ? 48 8B 41 ? 48 8B D9 8B 08 E8 ? ? ? ? 0F B6 D0 48 8B 03 89 10"));
    if (wrapper) {
      g_Offsets.m_CommandIsEntityAttached = wrapper + 20 + Mem.Read<int>(wrapper + 16);
    }
  }
  if (!g_Offsets.m_CommandNetworkRequestControlOfEntity) {
    uintptr_t wrapper = Mem.FindSignatureStr(xorstr("40 53 48 83 EC ? 48 8B 41 ? 48 8B D9 8B 08 E8 ? ? ? ? 0F B6 D0 48 8B 03 89 10"));
    if (wrapper) {
      uintptr_t wrapper2 = Mem.FindSignatureStr(xorstr("40 53 48 83 EC ? 48 8B 41 ? 48 8B D9 8B 08 E8 ? ? ? ? 0F B6 D0 48 8B 03 89 10"), wrapper + 1);
      if (wrapper2) {
        g_Offsets.m_CommandNetworkRequestControlOfEntity = wrapper2 + 20 + Mem.Read<int>(wrapper2 + 16);
      }
    }
  }
  if (!g_Offsets.m_CommandDetachEntity) {
    uintptr_t wrapper = Mem.FindSignatureStr(xorstr("48 8B 41 ? 83 78 ? ? 8B 08 0F 95 C2 83 78 ? ? 41 0F 95 C0 E9 ? ? ? ?"));
    if (wrapper) {
      g_Offsets.m_CommandDetachEntity = wrapper + 25 + Mem.Read<int>(wrapper + 21);
    }
  }
  if (!g_Offsets.m_SetPedInVehicle) {
    g_Offsets.m_SetPedInVehicle = Mem.FindSignatureStr(xorstr("48 8B C4 44 89 48 ? 44 89 40 ? 48 89 50 ? 48 89 48 ? 55 53 56 57"));
  }
  if (!g_Offsets.m_SetVehicleNumberPlateText) {
    g_Offsets.m_SetVehicleNumberPlateText = Mem.FindSignatureStr(xorstr("48 89 5C 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? F3 0F 10 02"));
  }
  if (!g_Offsets.m_SoloSessionStart) {
    g_Offsets.m_SoloSessionStart = Mem.FindSignatureStr(xorstr("48 83 EC ? E8 ? ? ? ? 48 85 C0 74 ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? E8 ? ? ? ? 48 83 C4"));
  }
  if (!g_Offsets.m_SoloSessionEnd) {
    g_Offsets.m_SoloSessionEnd = Mem.FindSignatureStr(xorstr("48 83 EC ? E8 ? ? ? ? 48 85 C0 74 ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 80 B9"));
  }
  if (!g_Offsets.m_SetEntityRotation) {
    g_Offsets.m_SetEntityRotation = Mem.FindSignatureStr(
        xorstr("48 89 5C 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? F3 0F 10 02"));
  }
  if (!g_Offsets.m_GetSerialisePackedAddress) {
    g_Offsets.m_GetSerialisePackedAddress = Mem.FindSignatureStr(
        xorstr("48 89 5C 24 08 57 48 83 EC 20 F3 0F 10 0A"));
  }
  if (!g_Offsets.m_PatchButt) {
    g_Offsets.m_PatchButt = Mem.FindSignatureStr(
        xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 20 41 8A F0 8B DA 8B F9"));
  }
  if (!g_Offsets.m_SilentPatchModule) {
    g_Offsets.m_SilentPatchModule = Mem.FindSignatureStr(
        xorstr("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 78 ? 4C 89 70 ? 55 48 8B EC 48 83 EC ? 33 DB 0F 29 70"));
  }
  if (!g_Offsets.m_GetPedBoneCoords) {
    g_Offsets.m_GetPedBoneCoords = Mem.FindSignatureStr(
        xorstr("55 48 8D 2D ? ? ? ? 48 87 2C 24 C3 48 8D 64 24 ? 48 8B 4C 24 ? 48 8D 64 24 ? 48 8B 44 24"));
  }
  if (!g_Offsets.m_EntityPool) {
    uintptr_t entityPoolSig = Mem.FindSignatureStr(
        xorstr("4C 8B 0D ? ? ? ? 44 8B C1 49 8B 41"));
    if (entityPoolSig) {
      int32_t ripOffset = Mem.Read<int32_t>(entityPoolSig + 3);
      g_Offsets.m_EntityPool = entityPoolSig + 7 + ripOffset;
    }
  }
  if (!g_Offsets.m_SetForwardSpeed) {
    g_Offsets.m_SetForwardSpeed = Mem.FindSignatureStr(
        xorstr("48 83 EC 38 0F 29 74 24 ? 0F 28 F1 E8 ? ? ? ? 48 85 C0 74 ? F6 40"));
  }
  if (!g_Offsets.m_IsNetworkTutorialSession) {
    g_Offsets.m_IsNetworkTutorialSession = Mem.FindSignatureStr(
        xorstr("40 53 48 83 EC ? 33 DB E8 ? ? ? ? 48 85 C0 74 ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 80 B9"));
  }
  if (!g_Offsets.m_IsNetworkTutorialSession2) {
    g_Offsets.m_IsNetworkTutorialSession2 = Mem.FindSignatureStr(
        xorstr("40 53 48 83 EC ? 33 DB E8 ? ? ? ? 48 85 C0 74 ? 48 8B 80 ? ? ? ? 48 85 C0 74 ? 80 B8"));
  }
  if (!g_Offsets.m_GetPedLastWeaponImpactCoord) {
    g_Offsets.m_GetPedLastWeaponImpactCoord = Mem.FindSignatureStr(
        xorstr("48 89 5C 24 ? 57 48 83 EC ? 33 DB 48 8B FA 48 89 5A"));
  }
  if (!g_Offsets.m_HasEntityBeenDamagedByWeapon) {
    g_Offsets.m_HasEntityBeenDamagedByWeapon = Mem.FindSignatureStr(
        xorstr("48 89 5C 24 08 48 89 74 24 10 57 48 83 EC ? 41 8B F8 8B F2 E8"));
  }
  if (!g_Offsets.m_ClonedSeatShuffleSerialise) {
    g_Offsets.m_ClonedSeatShuffleSerialise = Mem.FindSignatureStr(
        xorstr("48 89 5C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B FA 48 8B F1 E8 ? ? ? ? 48 8B 07 48 8D 56 30 45 33 C0 48 8B CF FF 50 38 80 7E 30 00 75"));
  }
  if (!g_Offsets.m_NetSyncPattern) {
    g_Offsets.m_NetSyncPattern = Mem.FindSignatureStr(
        xorstr("0F B7 CA 83 F9 07 7F 5E"));
  }
  if (!g_Offsets.m_BulletHandler) {
    g_Offsets.m_BulletHandler =
        Mem.FindSignatureStr(xorstr("F3 41 0F 10 19 F3 41 0F 10 41 04"));
  }
  if (!g_Offsets.m_CurrentPedWeapon) {
    g_Offsets.m_CurrentPedWeapon = Mem.FindSignatureStr(
        xorstr("40 53 48 83 EC ? 48 8B DA E8 ? ? ? ? 33 C9"));
  }
  if (!g_Offsets.m_IsCutscene) {
    g_Offsets.m_IsCutscene =
        Mem.FindSignature(p2v(xorstr("8A 81 63 0C 00 00 C3")));
  }
  if (!g_Offsets.m_GetRenderingCam) {
    g_Offsets.m_GetRenderingCam =
        Mem.FindSignatureStr(xorstr("48 89 5C 24 08 57 48 83 EC 20 83 CF FF E8 "
                                    "? ? ? ? 48 85 C0 74 ? 48 8B 0D"));
  }
  if (!g_Offsets.m_FreecamNop) {
    g_Offsets.m_FreecamNop = Mem.FindSignatureStr(
        xorstr("F3 0F 11 47 ?? F3 0F 10 44 24 ?? F3 0F 11 4F ?? F3 0F 10 4C 24 "
               "?? F3 0F 11 47 ?? F3 0F 11 4F ?? E8"));
  }

  if (!g_Offsets.m_BlipList)
    g_Offsets.m_BlipList = scanRel(xorstr("4C 8D 05 ? ? ? ? 0F B7 C1"), 7);
  if (!g_Offsets.m_BlipList)
    g_Offsets.m_BlipList =
        scanRel(xorstr("4C 8D 35 ? ? ? ? 3B 35 ? ? ? ? 74 ? 49 8B 3E"), 7);
  if (!g_Offsets.m_CamGameplayDirector) {
    g_Offsets.m_CamGameplayDirector =
        scanRel(xorstr("48 8B 05 ? ? ? ? 38 98 ? ? ? ? 8A C3"), 7);
  }
  if (!g_Offsets.m_Room)
    g_Offsets.m_Room =
        scanRel(xorstr("48 8B 05 ? ? ? ? 48 8B 98 ? ? ? ? EB"), 7);
  if (!g_Offsets.m_Timecycle) {
    g_Offsets.m_Timecycle = scanRel(
        xorstr("48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D "
               "0D ? ? ? ? E8 ? ? ? ? 33 C0 48 8D 0D ? ? ? ? 89 05 ? ? ? ?"),
        7);
    if (!g_Offsets.m_Timecycle) {
      g_Offsets.m_Timecycle =
          scanRel(xorstr("48 8D 0D ? ? ? ? 33 D2 48 83 C4 28 E9 ? ? ? ? 48 83 "
                         "EC 28 0F B6 4A 2D"),
                  7);
    }
  }
  if (!g_Offsets.m_BulletInstance)
    g_Offsets.m_BulletInstance =
        Mem.FindSignature(p2v(xorstr("F3 41 0F 10 19 F3 41 0F 10 41 04")));

  if (!g_Offsets.m_GameplayCamHolder) {
    g_Offsets.m_GameplayCamHolder =
        scanRel(xorstr("48 8B 05 ? ? ? ? 48 8B 98 ? ? ? ? EB"), 7);
  }
  if (!g_Offsets.m_GameplayCamTarget) {
    g_Offsets.m_GameplayCamTarget = scanRel(
        xorstr("48 83 EC 38 0F 29 74 24 ? 0F 28 F0 0F 2F 35 ? ? ? ? 73"), 19);
  }
  if (!g_Offsets.m_GetGameplayCamRot) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 08 57 48 83 EC 30 8B DA 48 8B F9 E8 ? ? ? ?")));
    if (!a)
      a = Mem.FindSignature(p2v(
          xorstr("? ? ? ? 8B DA 48 8B F9 E8 ? ? ? ? 48 8D 4C 24 ? 48 8D 90")));
    if (a)
      g_Offsets.m_GetGameplayCamRot = a;
  }
  if (!g_Offsets.m_GetFinalRenderedCamRot) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 08 57 48 83 EC 30 8B DA 48 8B F9 E8 "
                   "? ? ? ? 48 8D 4C 24 20 44 8B C3 48 8B D0")));
    if (a)
      g_Offsets.m_GetFinalRenderedCamRot = a;
  }
  if (!g_Offsets.m_GetGameplayCamCoord) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 F8 48 8D 64 24 F8 48 83 EC 60")));
    if (!a)
      a = Mem.FindSignature(
          p2v(xorstr("40 53 48 83 EC ?? 48 8B D9 E8 ?? ?? ?? ?? 8B 90")));
    if (a)
      g_Offsets.m_GetGameplayCamCoord = a;
  }
  if (!g_Offsets.m_GetPedBoneIndex) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 ? 57 48 83 EC ? 8B FA 83 CB")));
    if (a)
      g_Offsets.m_GetPedBoneIndex = a;
  }
  if (!g_Offsets.m_CreateObject) {
    uintptr_t a = Mem.FindSignature(p2v(xorstr(
        "4C 8B DC 48 83 EC ? 8A 84 24 ? ? ? ? F3 0F 10 5A ? F3 0F 10 52 ?")));
    if (a)
      g_Offsets.m_CreateObject = a;
  }
  if (!g_Offsets.m_AttachEntityToEntity) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 57 41 54 41 55 "
                   "41 56 41 57 48 83 EC 70 0F 29 70 C8 45 0F B7 E1 45 0F B7 "
                   "E8 4C 8B FA 4C 8B F1 E8")));
    if (a)
      g_Offsets.m_AttachEntityToEntity = a;
  }
  if (!g_Offsets.m_HasEntityClearLosToEntity) {
    uintptr_t a =
        Mem.FindSignature(p2v(xorstr("48 8B C4 48 89 58 ? 48 89 70 ? 48 89 "
                                     "78 ? 4C 89 70 ? 55 48 8D A8 ? ? ? ?")));
    if (!a)
      a = Mem.FindSignature(p2v(
          xorstr("57 48 83 EC 60 48 8B 01 41 8B E8 48 8B F2 48 8B F9 33 DB")));
    if (a)
      g_Offsets.m_HasEntityClearLosToEntity = a;
  }
  if (!g_Offsets.m_ClearPedTasks) {
    uintptr_t a = Mem.FindSignature(p2v(
        xorstr("40 53 48 83 EC 30 E8 ?? ?? ?? ?? 48 8B D8 48 85 C0 0F 84")));
    if (a)
      g_Offsets.m_ClearPedTasks = a;
  }
  if (!g_Offsets.m_SetCurrentPedWeapon) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC ? 41 8A "
                   "F0 8B FA E8 ? ? ? ? 48 8B D8")));
    if (a)
      g_Offsets.m_SetCurrentPedWeapon = a;
  }
  if (!g_Offsets.m_ExplodeVehicle) {
    uintptr_t a =
        Mem.FindSignature(p2v(xorstr("48 8B 41 ? 83 78 ? ? 8B 08 0F 95 C2 "
                                     "83 78 ? ? 41 0F 95 C0 E9 ? ? ? ?")));
    if (a)
      g_Offsets.m_ExplodeVehicle = a;
  }
  if (!g_Offsets.m_GetHandleByPointer) {
    uintptr_t a = Mem.FindSignature(p2v(
        xorstr("48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 8B 15 ? ? ? ? "
               "48 8B F9 48 83 C1 10 33 DB")));
    if (a)
      g_Offsets.m_GetHandleByPointer = a;
  }
  if (!g_Offsets.m_ExplodeVehicleRemote) {
    uintptr_t a = Mem.FindSignatureStr(xorstr(
        "40 53 48 83 EC ? 8A DA E8 ? ? ? ? 48 85 C0 74 ? 0F BA B0 ? ? ? ? ? 8B 0D"));
    if (a)
      g_Offsets.m_ExplodeVehicleRemote = a;
  }
  if (!g_Offsets.m_AddWeapon) {
    uintptr_t a = Mem.FindSignature(
        p2v(xorstr("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ?")));
    if (!a)
      a = Mem.FindSignature(
          p2v(xorstr("48 8B C4 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 90")));
    if (a)
      g_Offsets.m_AddWeapon = a;
  }
  if (!g_Offsets.m_CreateVehicle) {
    uintptr_t a =
        Mem.FindSignature(p2v(xorstr("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 "
                                     "41 57 48 8B EC 48 83 EC 50")));
    if (a)
      g_Offsets.m_CreateVehicle = a;
  }
  if (!g_Offsets.m_SkySettings) {
  auto p2v = [&](const std::string &s) { return Mem.Pattern2Vector(s); };
  auto scanRel = [&](const std::string &s, int len) -> uintptr_t {
    uintptr_t a = Mem.FindSignature(p2v(s));
    if (a) {
      int32_t rel = Mem.Read<int32_t>(a + (len - 4));
      return a + len + rel;
    }
    return 0;
  };

  g_Offsets.m_SkySettings = scanRel(
      xorstr(
          "48 8D 0D ? ? ? ? E8 ? ? ? ? 83 25 ? ? ? ? 00 48 8D 0D ? ? ? ? F3"),
      7);

  uintptr_t skyInit = Mem.FindSignature(
      p2v(xorstr("E8 ? ? ? ? 48 8B CE E8 ? ? ? ? 41 8D 4E 06")));
  if (skyInit) {
    uintptr_t skyUpdate =
        Mem.FindSignature(p2v(xorstr("48 83 EC 18 48 8B 0D ? ? ? ?")));
    if (skyUpdate) {
      g_Offsets.m_SkySettings = Mem.ResolveRelativeAddress(skyUpdate + 4, 7);
    }
  }

  g_Offsets.m_bRenderPhaseSeeThrough =
      scanRel(xorstr("38 1D ? ? ? 01 F3 0F 10 85 80"), 6);
  g_Offsets.m_ColorFar =
      scanRel(xorstr("66 0F 6E 05 ? ? ? ? F3 0F 11 8F C8 02"), 8);
  g_Offsets.m_ColorNear =
      scanRel(xorstr("66 0F 6E 05 ? ? ? ? F3 0F 11 8F BC 02"), 8);
  g_Offsets.m_PlayerColorOutline =
      scanRel(xorstr("66 0F 6E 05 ? ? ? ? F3 0F 11 8F D8 02"), 8);
  g_Offsets.m_PlayerColorInside =
      scanRel(xorstr("66 0F 6E 05 ? ? ? ? F3 0F 11 8F E8 02"), 8);
  g_Offsets.m_PostFXPass = scanRel(xorstr("8B 05 ? ? ? ? 83 F8 66"), 6);
}

  if (g_Offsets.CurrentBuild == 2060) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x24C8858;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1EC3828;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1F6A7E0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1F6B940;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1F4F940;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1414;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x2A0;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x10B8;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0x68;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xCD0;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x218;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x2657B70;

    g_Offsets.CurrentBuild = 2060;
  } else if (g_Offsets.CurrentBuild == 2189) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x24E6D90;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1EE18A8;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1F888C0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1F89768;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1F6EF80;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1414;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x2A0;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x10B8;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0x68;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xCD0;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x218;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26761A0;

    g_Offsets.CurrentBuild = 2189;
  } else if (g_Offsets.CurrentBuild == 2372) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x252DCD8;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F05208;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1F9E9F0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1F9F898;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1F9FFA0;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1414;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x2A0;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x10B8;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x218;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xCD0;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26AB9F0;

    g_Offsets.CurrentBuild = 2372;
  } else if (g_Offsets.CurrentBuild == 2545) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x25667E8;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F2E7A8;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1FD6F70;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1FD7E18;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1FDF560;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x2A0;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x10B8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1450;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x1530;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1464;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x218;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xCD0;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26E5710;

    g_Offsets.CurrentBuild = 2545;
  } else if (g_Offsets.CurrentBuild == 2612) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x2567DB0;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F77EF0;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1FD8570;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1FD9418;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1FDDD20;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x10B8;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1450;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x1530;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1464;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x218;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xCD0;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26E6F10;

    g_Offsets.CurrentBuild = 2612;
  } else if (g_Offsets.CurrentBuild == 2699) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x26684D8;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x20304C8;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x20D8C90;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x20D9B38;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x20E1420;
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x10EF174;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x10B8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1450;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x1530;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1464;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x218;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xCD0;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x27e7a70;

    g_Offsets.CurrentBuild = 2699;
  } else if (g_Offsets.CurrentBuild == 2802) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x254D448;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F5B820;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1FBC100;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1FBCFA8;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1FBD6E0;
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x1120D30;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10C8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x270;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26BB8B0;

    g_Offsets.CurrentBuild = 2802;
  } else if (g_Offsets.CurrentBuild == 2944) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x257BEA0;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F42068;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x1FEAAC0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x1FEB968;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x1FF3130;
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x1101B50;
    if (!g_Offsets.m_CameraX)
      g_Offsets.m_CameraX = 0x275A2B;
    if (!g_Offsets.m_CameraY)
      g_Offsets.m_CameraY = 0x275A36;
    if (!g_Offsets.m_CameraZ)
      g_Offsets.m_CameraZ = 0x275A41;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x270;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26EA160;

    g_Offsets.CurrentBuild = 2944;

  } else if (g_Offsets.CurrentBuild == 3095) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x2593320;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F58B58;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x20019E0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x2002888;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x2002FA0;
    if (!g_Offsets.m_CameraX)
      g_Offsets.m_CameraX = 0x2796B3;
    if (!g_Offsets.m_CameraY)
      g_Offsets.m_CameraY = 0x2796BE;
    if (!g_Offsets.m_CameraZ)
      g_Offsets.m_CameraZ = 0x2796C9;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x118786C;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x270;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x26EA160;

    g_Offsets.CurrentBuild = 3095;
  } else if (g_Offsets.CurrentBuild == 3258) {
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x1198558;
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x25B14B0;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1FBD4F0;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x201DBA0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x201ED50;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x2023400;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x1198558;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x0270;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_Velocity)
      g_Offsets.m_Velocity = 0x320;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x27028C0 + 0x60;

    g_Offsets.CurrentBuild = 3258;
  } else if (g_Offsets.CurrentBuild == 3323) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x25C15B0;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F85458;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x202DC50;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x202EB48;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x20333E0;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_WeaponObject)
      g_Offsets.m_WeaponObject = 0x78;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x27028C0 + 0x60;

    g_Offsets.CurrentBuild = 3323;
  } else if (g_Offsets.CurrentBuild == 3407) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x25D7108;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F9A9D8;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x20431C0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x20440C8;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x2047D50;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x27211F0 + 0x60;

    g_Offsets.CurrentBuild = 3407;
  } else if (g_Offsets.CurrentBuild == 3751) {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x2603908;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1FC38A8;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x206C060;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x206D1C0;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x206D600;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x27211F0 + 0x60;

    g_Offsets.CurrentBuild = 3751;
  } else if (g_Offsets.CurrentBuild == 3570) {
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x270;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_WeaponDamageModifier)
      g_Offsets.m_WeaponDamageModifier = 0xD60;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;

    g_Offsets.CurrentBuild = 3570;
  } else {
    if (!g_Offsets.m_World)
      g_Offsets.m_World = Mem.ModBase + 0x257BEA0;
    if (!g_Offsets.m_ReplayInterFace)
      g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1FBD4F0;
    if (!g_Offsets.m_ViewPort)
      g_Offsets.m_ViewPort = Mem.ModBase + 0x201DBA0;
    if (!g_Offsets.m_CamGameplayDirector)
      g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x201ED50;
    if (!g_Offsets.m_BlipList)
      g_Offsets.m_BlipList = Mem.ModBase + 0x2023400;
    if (!g_Offsets.m_OffsetPool)
      g_Offsets.m_OffsetPool = 0x1101B50;
    if (!g_Offsets.m_CameraX)
      g_Offsets.m_CameraX = 0x275A2B;
    if (!g_Offsets.m_CameraY)
      g_Offsets.m_CameraY = 0x275A36;
    if (!g_Offsets.m_CameraZ)
      g_Offsets.m_CameraZ = 0x275A41;
    if (!g_Offsets.m_MaxHealth)
      g_Offsets.m_MaxHealth = 0x284;
    if (!g_Offsets.m_WeaponManager)
      g_Offsets.m_WeaponManager = 0x10B8;
    if (!g_Offsets.m_EntityType)
      g_Offsets.m_EntityType = 0x1098;
    if (!g_Offsets.m_PlayerInfo)
      g_Offsets.m_PlayerInfo = 0x10A8;
    if (!g_Offsets.m_FragInst)
      g_Offsets.m_FragInst = 0x1430;
    if (!g_Offsets.m_PlayerId)
      g_Offsets.m_PlayerId = 0xE8;
    if (!g_Offsets.m_Armor)
      g_Offsets.m_Armor = 0x150C;
    if (!g_Offsets.m_PedFlag)
      g_Offsets.m_PedFlag = 0x1444;
    if (!g_Offsets.m_FrameFlag)
      g_Offsets.m_FrameFlag = 0x270;
    if (!g_Offsets.m_LastVehicle)
      g_Offsets.m_LastVehicle = 0xD10;
    if (!g_Offsets.m_Speed)
      g_Offsets.m_Speed = 0xD40;
    if (!g_Offsets.m_CamFollowOffset)
      g_Offsets.m_CamFollowOffset = 0x2C0;
    if (!g_Offsets.m_CamViewOffset)
      g_Offsets.m_CamViewOffset = 0x40;
    if (!g_Offsets.m_CamPosOffset)
      g_Offsets.m_CamPosOffset = 0x60;
    if (!g_Offsets.m_CamDataOffset)
      g_Offsets.m_CamDataOffset = 0x3D0;
    if (!g_Offsets.m_CamFovOffset)
      g_Offsets.m_CamFovOffset = 0x30;
    if (!g_Offsets.m_SkySettings)
      g_Offsets.m_SkySettings = Mem.ModBase + 0x27211F0 + 0x60;
    if (!g_Offsets.m_WeaponDamageModifier)
      g_Offsets.m_WeaponDamageModifier = 0xD60;

    g_Offsets.CurrentBuild = 3570;
  }

  if (!g_Offsets.m_CamFollowOffset)
    g_Offsets.m_CamFollowOffset = 0x2C0;
  if (!g_Offsets.m_CamViewOffset)
    g_Offsets.m_CamViewOffset = 0x40;
  if (!g_Offsets.m_CamPosOffset)
    g_Offsets.m_CamPosOffset = 0x60;
  if (!g_Offsets.m_CamDataOffset)
    g_Offsets.m_CamDataOffset = 0x3D0;
  if (!g_Offsets.m_CamFovOffset)
    g_Offsets.m_CamFovOffset = 0x30;
  if (!g_Offsets.m_CamDistanceOffset)
    g_Offsets.m_CamDistanceOffset = 0x30;

  if (!g_Offsets.m_CObject)
    g_Offsets.m_CObject = 0x20;
  if (!g_Offsets.m_CWeapon)
    g_Offsets.m_CWeapon = 0x20;
  if (!g_Offsets.m_FragInst)
    g_Offsets.m_FragInst = 0x1430;
  if (!g_Offsets.m_WeaponManager)
    g_Offsets.m_WeaponManager = 0x10B8;
  if (!g_Offsets.m_EntityType)
    g_Offsets.m_EntityType = 0x1098;
  if (!g_Offsets.m_PlayerInfo)
    g_Offsets.m_PlayerInfo = 0x10A8;
  if (!g_Offsets.m_MaxHealth)
    g_Offsets.m_MaxHealth = 0x284;
  if (!g_Offsets.m_PedFlag)
    g_Offsets.m_PedFlag = 0x1444;
  if (!g_Offsets.m_FrameFlag)
    g_Offsets.m_FrameFlag = 0x270;
  if (!g_Offsets.m_Speed)
    g_Offsets.m_Speed = 0xD40;
  if (!g_Offsets.m_LastVehicle)
    g_Offsets.m_LastVehicle = 0xD10;
  if (!g_Offsets.m_Armor)
    g_Offsets.m_Armor = 0x150C;
  if (!g_Offsets.m_PlayerId)
    g_Offsets.m_PlayerId = 0xE8;

  return true;
}

}
