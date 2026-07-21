#include "Triggerbot.hpp"
#include "HitboneList.hpp"
#include "../../../framework/settings/search.h"
#include <cmath>

void Core::Features::cTriggerbot::Shoot( int delay )
{
	if (option->param.triggerbot_delay_jitter > 0)
		delay += Utils::GenRandomInt(0, option->param.triggerbot_delay_jitter);

	if (option->param.triggerbot_curving &&
	    option->param.triggerbot_curve_strength > 0.001f) {
		float strength = option->param.triggerbot_curve_strength;
		int extra = static_cast<int>(delay * strength * 0.75f);
		delay += extra;
	}

	std::this_thread::sleep_for( std::chrono::milliseconds( delay ) );

	if (option->param.triggerbot_curving &&
	    option->param.triggerbot_curve_strength > 0.001f) {
		float strength = option->param.triggerbot_curve_strength;
		int steps = 6;
		int amp = static_cast<int>(std::round(strength * 4.f));
		if (amp < 1) amp = 1;
		for (int i = 0; i < steps; ++i) {
			float phase = (float)i / (float)(steps - 1) * 3.14159f;
			int dx = static_cast<int>(std::sin(phase) * amp);
			int dy = static_cast<int>(std::sin(phase * 2.f) * (amp / 2));
			mouse_event(MOUSEEVENTF_MOVE, dx, dy, 0, 0);
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}

	mouse_event( MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0 );

	int hold = option->param.triggerbot_shot_duration;
	hold += Utils::GenRandomInt(-hold / 4, hold / 4);
	if (hold < 20) hold = 20;
	std::this_thread::sleep_for( std::chrono::milliseconds( hold ) );
	mouse_event( MOUSEEVENTF_LEFTUP, 0, 0, 0, 0 );

	if (option->param.triggerbot_cooldown > 0)
		std::this_thread::sleep_for( std::chrono::milliseconds( option->param.triggerbot_cooldown ) );
}

void Core::Features::cTriggerbot::Start()
{
    while (!g_Variables.g_Unload)
    {
        try {
        if (!var->auth.authenticated) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        if (option->key.triggerbot_key && option->key.triggerbot_mode == 0 && GetForegroundWindow() == g_Variables.g_hGameWindow) {
            if (Utils::KeyPressedWithDelay(option->key.triggerbot_key, 250))
                option->param.enable_triggerbot = !option->param.enable_triggerbot;
        }

        bool triggerActive = option->param.enable_triggerbot;
        if (triggerActive) {
            if (option->key.triggerbot_key != 0) {
                if (option->key.triggerbot_mode == 1) {
                    triggerActive = (GetAsyncKeyState(option->key.triggerbot_key) & 0x8000) != 0;
                }
            } else {
                triggerActive = false;
            }
        }

        if (triggerActive && GetForegroundWindow() != g_Variables.g_hCheatWindow)
        {
            CPed* Ped = Core::SDK::Game::GetClosestPedEx(
                option->param.triggerbot_max_distance,
                option->param.triggerbot_ignore_npcs,
                option->param.triggerbot_check_visible,
                option->param.triggerbot_ignore_friends);

            if (!Ped)
                continue;

            if (option->param.triggerbot_target_combat_roll)
            {
                if (!Core::SDK::Game::IsPedDoingCombatRoll(Ped))
                    continue;
            }
            if (option->param.triggerbot_target_jump)
            {
                if (!Core::SDK::Game::IsPedJumping(Ped))
                    continue;
            }

            D3DXVECTOR3 TargetPos;
            bool hasSkeleton = false;
            do {
                uintptr_t FragInstNMGta = Mem.Read<uintptr_t>((uintptr_t)Ped + g_Offsets.m_FragInst);
                if (!FragInstNMGta) break;
                uintptr_t v9 = Mem.Read<uintptr_t>(FragInstNMGta + 0x68);
                if (!v9) break;

                Core::SDK::Game::cSkeleton_t Skeleton;
                Skeleton.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
                Skeleton.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
                Skeleton.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
                Skeleton.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
                Skeleton.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);
                if (!Skeleton.crSkeletonData.m_BoneIdTable_Slots) break;
                Skeleton.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
                Skeleton.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
                Skeleton.Arg2 = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);

                {
                    unsigned int selectedBone = Core::Features::GetHitboneId(option->param.triggerbot_hitbone);
                    TargetPos = Core::SDK::Game::GetBonePosComplex(Ped, selectedBone, Skeleton);
                }
                hasSkeleton = true;
            } while (false);

            if (!hasSkeleton) {
                TargetPos = Ped->GetBonePosDefault(0);
            }
            D3DXVECTOR2 ScreenTargetPos = Core::SDK::Game::WorldToScreen(TargetPos);

            if (Core::SDK::Game::IsOnScreen(ScreenTargetPos))
            {
                int Fov = std::hypot(
                    ScreenTargetPos.x - g_Variables.g_vGameWindowCenter.x,
                    ScreenTargetPos.y - g_Variables.g_vGameWindowCenter.y);

                if (Fov < option->param.triggerbot_fov_size)
                {
                    int roll = Utils::GenRandomInt(0, 100);
                    if (roll <= option->param.triggerbot_hit_chance) {
                        Shoot(option->param.triggerbot_delay);
                    }
                }
            }
        }
        } catch (...) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}
