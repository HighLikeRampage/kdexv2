#include "SilentAim.hpp"
#include "HitboneList.hpp"
#include "../../../../Globals.hpp"
#include "../../../framework/settings/search.h"
#include <Security/xorstr.hpp>
#include <cmath>
#include <algorithm>

bool Core::Features::cSilentAim::SilentAimInitialized;
std::uintptr_t Core::Features::cSilentAim::StartAddy;
std::uintptr_t Core::Features::cSilentAim::BulletHandlerStartAddy;
std::uintptr_t Core::Features::cSilentAim::BulletInstanceStartAddy;
static int lastHitbox = 0;

static bool IsMovssPattern(uintptr_t addr) {
    if (!addr) return false;
    auto b = Core::Mem.ReadBytes(addr, 5);
    return b.size() == 5 &&
           b[0] == 0xF3 && b[1] == 0x41 &&
           b[2] == 0x0F && b[3] == 0x10 &&
           (b[4] & 0xC7) == 0x01;
}

static bool IsLeaDisp8Pattern(uintptr_t addr) {
    if (!addr) return false;
    auto b = Core::Mem.ReadBytes(addr, 3);
    return b.size() == 3 && b[0] == 0x48 && b[1] == 0x8D && b[2] == 0x45;
}

void Core::Features::cSilentAim::InitializeSilentAim() {
    if (g_Offsets.m_SilentAim) {
        if (!IsMovssPattern(g_Offsets.m_SilentAim) &&
            !IsLeaDisp8Pattern(g_Offsets.m_SilentAim))
            g_Offsets.m_SilentAim = 0;
    }

    if (!g_Offsets.m_SilentAim) {
        uintptr_t found = Mem.FindSignatureStr(
            xorstr("F3 41 0F 10 19 F3 41 0F 10 41 04 F3 41 0F 10 51 08"));
        if (!found)
            found = Mem.FindSignatureStr(
                xorstr("F3 41 0F 10 19 F3 41 0F 10 41 04"));
        if (!found)
            found = g_Offsets.m_BulletHandler;
        if (!found)
            found = g_Offsets.m_BulletInstance;
        if (found)
            g_Offsets.m_SilentAim = found;
        else
            return;
    }

    StartAddy = g_Offsets.m_SilentAim;
    BulletHandlerStartAddy = g_Offsets.m_BulletHandler;
    BulletInstanceStartAddy = g_Offsets.m_BulletInstance;

    auto initHook = [this](uintptr_t& startAddy, uintptr_t& hook, std::vector<uint8_t>& shell,
                          std::vector<uint8_t>& leaShell, std::vector<uint8_t>& original,
                          HookMode& mode, const std::vector<uint8_t>& baseShell,
                          const std::vector<uint8_t>& baseLeaShell) {
        if (!startAddy) return;

        if (IsMovssPattern(startAddy)) {
            mode = HookMode::Xmm;
            shell = baseShell;
            auto gameBytes = Mem.ReadBytes(startAddy, 20);
            if (gameBytes.size() < 17) { mode = HookMode::None; return; }

            uint8_t regX = (gameBytes[4] & 0x38);
            uint8_t regY = (gameBytes[9] & 0x38);

            size_t thirdLen = 6;
            uint8_t regZ = 0x10;
            if (gameBytes[11] == 0xF3 && gameBytes[12] == 0x41 &&
                gameBytes[13] == 0x0F && gameBytes[14] == 0x10) {
                regZ = (gameBytes[15] & 0x38);
                thirdLen = 6;
            } else if (gameBytes[11] == 0xF3 && gameBytes[12] == 0x0F &&
                       gameBytes[13] == 0x10) {
                regZ = (gameBytes[14] & 0x38);
                thirdLen = 5;
            }

            size_t totalSkip = 5 + 6 + thirdLen;

            shell[3]  = regX | 0x05;
            shell[11] = regY | 0x05;
            shell[19] = regZ | 0x05;

            original = Mem.ReadBytes(startAddy, 38);
            uintptr_t backAddr = startAddy + totalSkip;
            memcpy(shell.data() + 30, &backAddr, sizeof(backAddr));

            if (!hook) hook = Mem.CreateCodeCave(500);
            if (hook) Mem.WriteBytes(hook, shell);
            if (hook) Mem.HookJMP(startAddy, hook);
        } else if (IsLeaDisp8Pattern(startAddy)) {
            mode = HookMode::Lea;
            leaShell = baseLeaShell;
            original = Mem.ReadBytes(startAddy, 38);
            uint8_t disp = original[3];
            leaShell[12] = disp;
            leaShell[25] = static_cast<uint8_t>(disp + 4);
            leaShell[38] = static_cast<uint8_t>(disp + 8);
            uintptr_t backAddr = startAddy + 0x26;
            memcpy(leaShell.data() + 45, &backAddr, sizeof(backAddr));

            if (!hook) hook = Mem.CreateCodeCave(500);
            if (hook) Mem.WriteBytes(hook, leaShell);
            if (hook) Mem.HookJMP(startAddy, hook);
        }
    };

    initHook(StartAddy, SilentAimHook, SilentAimShell, LeaShell, OriginalFuncTable, m_HookMode, SilentAimShell, LeaShell);
    if (BulletHandlerStartAddy && BulletHandlerStartAddy != StartAddy) {
        initHook(BulletHandlerStartAddy, BulletHandlerHook, BulletHandlerShell, BulletHandlerLeaShell, OriginalBulletHandlerTable, m_BulletHandlerMode, SilentAimShell, LeaShell);
    }
    if (BulletInstanceStartAddy && BulletInstanceStartAddy != StartAddy && BulletInstanceStartAddy != BulletHandlerStartAddy) {
        initHook(BulletInstanceStartAddy, BulletInstanceHook, BulletInstanceShell, BulletInstanceLeaShell, OriginalBulletInstanceTable, m_BulletInstanceMode, SilentAimShell, LeaShell);
    }
}

void Core::Features::cSilentAim::RestoreSilent() {
    auto restore = [](uintptr_t startAddy, const std::vector<uint8_t>& original) {
        if (startAddy && !original.empty()) {
            Mem.WriteBytes(startAddy, original);
        }
    };
    restore(StartAddy, OriginalFuncTable);
    restore(BulletHandlerStartAddy, OriginalBulletHandlerTable);
    restore(BulletInstanceStartAddy, OriginalBulletInstanceTable);
    m_HookMode = HookMode::None;
    m_BulletHandlerMode = HookMode::None;
    m_BulletInstanceMode = HookMode::None;
}

void Core::Features::cSilentAim::HookSilent() {
    while (!g_Variables.g_Unload) {
        if (!var->auth.authenticated) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        if (option->key.silent_aim_key && option->key.silent_aim_mode == 0 && GetForegroundWindow() == g_Variables.g_hGameWindow) {
            if (Utils::KeyPressedWithDelay(option->key.silent_aim_key, 250))
                option->param.silent_aim = !option->param.silent_aim;
        }

        bool silentActive = option->param.silent_aim;
        if (silentActive) {
            if (option->key.silent_aim_key != 0) {
                if (option->key.silent_aim_mode == 1) {
                    silentActive = (GetAsyncKeyState(option->key.silent_aim_key) & 0x8000) != 0;
                }
            } else {
                silentActive = false;
            }
        }

        if (silentActive && GetForegroundWindow() != g_Variables.g_hCheatWindow) {
            CPed *Ped = Core::SDK::Game::GetClosestPed(
                option->param.aim_distance, option->param.silent_ignore_npcs,
                option->param.silent_check_visible);
            if (!Ped) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            if (option->param.silent_target_combat_roll) {
                if (!Core::SDK::Game::IsPedDoingCombatRoll(Ped)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
            }
            if (option->param.silent_target_jump) {
                if (!Core::SDK::Game::IsPedJumping(Ped)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }
            }

            D3DXVECTOR3 TargetPos;

            uintptr_t FragInstNMGta =
                Mem.Read<uintptr_t>((uintptr_t)Ped + g_Offsets.m_FragInst);
            if (!FragInstNMGta) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }
            uintptr_t v9 = Mem.Read<uintptr_t>(FragInstNMGta + 0x68);
            if (!v9) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            Core::SDK::Game::cSkeleton_t Skeleton;
            Skeleton.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
            Skeleton.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
            Skeleton.crSkeletonData.m_Used =
                Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
            Skeleton.crSkeletonData.m_NumBones =
                Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
            Skeleton.crSkeletonData.m_BoneIdTable_Slots =
                Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);
            if (!Skeleton.crSkeletonData.m_BoneIdTable_Slots) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            Skeleton.crSkeletonData.m_BoneIdTable =
                Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
            Skeleton.Arg1 =
                Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
            Skeleton.Arg2 = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);

            {
                unsigned int selectedBone = Core::Features::GetHitboneId(option->param.silent_hitbone);
                TargetPos = Core::SDK::Game::GetBonePosComplex(Ped, selectedBone, Skeleton);
                if (selectedBone == SKEL_Head)
                    lastHitbox = 0;
                else if (selectedBone == SKEL_Pelvis)
                    lastHitbox = 2;
                else
                    lastHitbox = 1;
            }

            D3DXVECTOR2 Screen = Core::SDK::Game::WorldToScreen(TargetPos);

            if (Core::SDK::Game::IsOnScreen(Screen)) {
                int Fov = std::hypot(Screen.x - g_Variables.g_vGameWindowCenter.x,
                                     Screen.y - g_Variables.g_vGameWindowCenter.y);

                float fovLimit = option->param.silent_fov_size;
                if (option->param.silent_smart_fov) {
                    float dist = 0.0f;
                    if (Core::SDK::Pointers::pLocalPlayer) {
                        dist = Ped->GetDistance(Core::SDK::Pointers::pLocalPlayer->GetPos(), TargetPos);
                    }
                    float maxD = (float)option->param.silent_dual_fov_distance;
                    float t = maxD > 1.f ? std::clamp(dist / maxD, 0.0f, 1.0f) : 0.0f;
                    float a = (std::max)(option->param.silent_fov_near, option->param.silent_fov_far);
                    float b = (std::min)(option->param.silent_fov_near, option->param.silent_fov_far);
                    fovLimit = a + (b - a) * t;
                } else if (option->param.silent_dual_fov) {
                    float dist = 0.0f;
                    if (Core::SDK::Pointers::pLocalPlayer) {
                        dist = Ped->GetDistance(Core::SDK::Pointers::pLocalPlayer->GetPos(), TargetPos);
                    }
                    fovLimit = (dist >= static_cast<float>(option->param.silent_dual_fov_distance))
                                   ? option->param.silent_fov_far
                                   : option->param.silent_fov_near;
                }

                if (Fov < fovLimit) {
                    bool Miss = option->param.miss_chance >= Utils::GenRandomInt(0, 100);
                    D3DXVECTOR3 HitOffset;
                    if (Miss) {
                        HitOffset = D3DXVECTOR3(0.0f, 0.4f, 0.0f);
                    } else {
                        static D3DXVECTOR3 lastJitter = { 0, 0, 0 };
                        D3DXVECTOR3 currentJitter;
                        float range = option->param.silent_jitter;

                        int attempts = 0;
                        do {
                            currentJitter.x = Utils::GenRandomFloat(-range, range);
                            currentJitter.y = Utils::GenRandomFloat(-range, range);
                            currentJitter.z = Utils::GenRandomFloat(-range, range);
                            attempts++;
                        } while (attempts < 5 &&
                                 std::abs(currentJitter.x - lastJitter.x) < 0.0001f &&
                                 std::abs(currentJitter.y - lastJitter.y) < 0.0001f &&
                                 std::abs(currentJitter.z - lastJitter.z) < 0.0001f);

                        lastJitter = currentJitter;

                        switch (lastHitbox) {
                        case 0:
                            HitOffset = D3DXVECTOR3(currentJitter.x, currentJitter.y, 0.08f + currentJitter.z);
                            break;
                        case 1:
                            HitOffset = currentJitter;
                            break;
                        case 2:
                            HitOffset = D3DXVECTOR3(currentJitter.x, currentJitter.y, -0.1f + currentJitter.z);
                            break;
                        default:
                            HitOffset = D3DXVECTOR3(currentJitter.x, currentJitter.y, 0.08f + currentJitter.z);
                            break;
                        }
                    }

                    auto FinalPos = TargetPos + HitOffset;

                    auto updateShellcodes = [&](bool init) {

                        auto updateXmm = [&](std::vector<uint8_t>& shell, uintptr_t hook, HookMode mode) {
                            if (init || mode == HookMode::Xmm) {
                                memcpy(shell.data() + 38, &FinalPos.x, sizeof(float));
                                memcpy(shell.data() + 42, &FinalPos.y, sizeof(float));
                                memcpy(shell.data() + 46, &FinalPos.z, sizeof(float));
                                if (!init && hook)
                                    Mem.WriteRaw(hook + 38, shell.data() + 38, shell.size() - 38);
                            }
                        };

                        auto updateLea = [&](std::vector<uint8_t>& shell, uintptr_t hook, HookMode mode) {
                            if (init || mode == HookMode::Lea) {
                                memcpy(shell.data() + 53, &FinalPos.x, sizeof(float));
                                memcpy(shell.data() + 57, &FinalPos.y, sizeof(float));
                                memcpy(shell.data() + 61, &FinalPos.z, sizeof(float));
                                if (!init && hook)
                                    Mem.WriteRaw(hook + 53, shell.data() + 53, shell.size() - 53);
                            }
                        };

                        updateXmm(SilentAimShell, SilentAimHook, m_HookMode);
                        updateXmm(BulletHandlerShell, BulletHandlerHook, m_BulletHandlerMode);
                        updateXmm(BulletInstanceShell, BulletInstanceHook, m_BulletInstanceMode);
                        updateLea(LeaShell, SilentAimHook, m_HookMode);
                        updateLea(BulletHandlerLeaShell, BulletHandlerHook, m_BulletHandlerMode);
                        updateLea(BulletInstanceLeaShell, BulletInstanceHook, m_BulletInstanceMode);
                    };

                    if (!SilentAimInitialized) {

                        updateShellcodes(true);
                        InitializeSilentAim();
                        SilentAimInitialized = (m_HookMode != HookMode::None ||
                                               m_BulletHandlerMode != HookMode::None ||
                                               m_BulletInstanceMode != HookMode::None);
                    } else {

                        updateShellcodes(false);
                    }

                } else {
                    if (SilentAimInitialized) {
                        RestoreSilent();
                        SilentAimInitialized = false;
                    }
                }
            } else {
                if (SilentAimInitialized) {
                    RestoreSilent();
                    SilentAimInitialized = false;
                }
            }

        } else {
            if (SilentAimInitialized) {
                RestoreSilent();
                SilentAimInitialized = false;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
