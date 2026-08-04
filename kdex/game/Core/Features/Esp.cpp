#define IMGUI_DEFINE_MATH_OPERATORS
#include "Esp.hpp"
#include "HitboneList.hpp"
#include "ObjectNames.hpp"
#include "Exploits/Core/Exploits.hpp"
#include "../Threads/EntityList.hpp"
#include "../Variables.hpp"
#include "../../../framework/settings/search.h"
#include "../../../framework/settings/variables.h"
#include "../../../framework/settings/colors.h"
#include "../../../framework/settings/functions.h"
#include "framework/Globals.hpp"
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <utility>

namespace Core {
    namespace Features {
        static inline void DrawTextOutlined(ImDrawList* dl, ImFont* font, float fontSize, ImVec2 pos, ImU32 col, const char* text)
        {
            const ImU32 shadow = IM_COL32(0, 0, 0, 255);
            dl->AddText(font, fontSize, ImVec2(pos.x - 1.0f, pos.y),        shadow, text);
            dl->AddText(font, fontSize, ImVec2(pos.x + 1.0f, pos.y),        shadow, text);
            dl->AddText(font, fontSize, ImVec2(pos.x,        pos.y - 1.0f), shadow, text);
            dl->AddText(font, fontSize, ImVec2(pos.x,        pos.y + 1.0f), shadow, text);
            dl->AddText(font, fontSize, pos, col, text);
        }

        static inline void DrawTextOutlined(ImDrawList* dl, ImVec2 pos, ImU32 col, const char* text)
        {
            const ImU32 shadow = IM_COL32(0, 0, 0, 255);
            dl->AddText(ImVec2(pos.x - 1.0f, pos.y),        shadow, text);
            dl->AddText(ImVec2(pos.x + 1.0f, pos.y),        shadow, text);
            dl->AddText(ImVec2(pos.x,        pos.y - 1.0f), shadow, text);
            dl->AddText(ImVec2(pos.x,        pos.y + 1.0f), shadow, text);
            dl->AddText(pos, col, text);
        }

        static const unsigned int g_espSkeletonBoneIds[40] = {
            SKEL_Head, SKEL_L_Foot, SKEL_R_Foot, SKEL_L_Hand, SKEL_R_Hand, SKEL_Pelvis, SKEL_L_Calf, SKEL_R_Calf,
            SKEL_L_Thigh, SKEL_R_Thigh, SKEL_L_Forearm, SKEL_R_Forearm, SKEL_L_UpperArm, SKEL_R_UpperArm, SKEL_Spine3,
            SKEL_Neck_1, SKEL_L_Clavicle, SKEL_R_Clavicle, SKEL_L_Toe0, SKEL_R_Toe0,
            SKEL_L_Finger00, SKEL_L_Finger02, SKEL_L_Finger10, SKEL_L_Finger12, SKEL_L_Finger20, SKEL_L_Finger22,
            SKEL_L_Finger30, SKEL_L_Finger32, SKEL_L_Finger40, SKEL_L_Finger42,
            SKEL_R_Finger00, SKEL_R_Finger02, SKEL_R_Finger10, SKEL_R_Finger12, SKEL_R_Finger20, SKEL_R_Finger22,
            SKEL_R_Finger30, SKEL_R_Finger32, SKEL_R_Finger40, SKEL_R_Finger42
        };
        static const int g_dotBoneToBatchIndex[20] = { 0, 15, 14, 5, 12, 10, 3, 13, 11, 4, 8, 6, 1, 18, 9, 7, 2, 19, 16, 17 };

        static D3DXVECTOR3 GetHitbonePosForPed(CPed* ped, int hitbone) {
            uintptr_t frag = Core::Mem.Read<uintptr_t>((uintptr_t)ped + Core::g_Offsets.m_FragInst);
            if (frag) {
                uintptr_t v9 = Core::Mem.Read<uintptr_t>(frag + 0x68);
                if (v9) {
                    Core::SDK::Game::cSkeleton_t sk;
                    sk.m_pSkeleton = Core::Mem.Read<uintptr_t>(v9 + 0x178);
                    sk.crSkeletonData.Ptr = Core::Mem.Read<uintptr_t>(sk.m_pSkeleton);
                    sk.crSkeletonData.m_BoneIdTable_Slots = Core::Mem.Read<unsigned short>(sk.crSkeletonData.Ptr + 0x18);
                    if (sk.crSkeletonData.m_BoneIdTable_Slots) {
                        sk.crSkeletonData.m_Used = Core::Mem.Read<unsigned int>(sk.crSkeletonData.Ptr + 0x1A);
                        sk.crSkeletonData.m_NumBones = Core::Mem.Read<unsigned int>(sk.crSkeletonData.Ptr + 0x5E);
                        sk.crSkeletonData.m_BoneIdTable = Core::Mem.Read<uintptr_t>(sk.crSkeletonData.Ptr + 0x10);
                        sk.Arg1 = Core::Mem.Read<D3DXMATRIX>(Core::Mem.Read<uintptr_t>(sk.m_pSkeleton + 0x8));
                        sk.Arg2 = Core::Mem.Read<uintptr_t>(sk.m_pSkeleton + 0x18);
                        unsigned int bone = Core::Features::GetHitboneId(hitbone);
                        return Core::SDK::Game::GetBonePosComplex(ped, bone, sk);
                    }
                }
            }
            return ped->GetBonePosDefault(0);
        }

        struct BulletImpact {
            D3DXVECTOR3 start_pos;
            D3DXVECTOR3 end_pos;
            std::chrono::time_point<std::chrono::steady_clock> start_time;
            float initial_alpha = 255.0f;
            float current_alpha = 255.0f;
        };

        static std::vector<BulletImpact> g_BulletImpacts;
        static std::mutex g_BulletImpactsMtx;

        static void UpdateImpacts()
        {
            auto now = std::chrono::steady_clock::now();
            float duration = option->param.tracer_duration;
            if (duration < 0.1f) duration = 0.1f;

            for (auto& impact : g_BulletImpacts)
            {
                float elapsed = std::chrono::duration<float>(now - impact.start_time).count();
                float remaining = duration - elapsed;

                if (remaining < 0.5f) {
                    impact.current_alpha = (remaining / 0.5f) * 255.0f;
                } else {
                    impact.current_alpha = 255.0f;
                }

                if (impact.current_alpha < 0.0f) impact.current_alpha = 0.0f;
            }

            g_BulletImpacts.erase(std::remove_if(g_BulletImpacts.begin(), g_BulletImpacts.end(), [&](const BulletImpact& bi) {
                float elapsed = std::chrono::duration<float>(now - bi.start_time).count();
                return elapsed >= duration;
            }), g_BulletImpacts.end());
        }

        void StartTracer()
        {
            static D3DXVECTOR3 last_impact = { 0.f, 0.f, 0.f };
            while (!g_Variables.g_Unload)
            {
                try {
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
                if (!option->param.tracer)
                    continue;

                CPed* ped = Core::SDK::Pointers::pLocalPlayer;
                if (!ped)
                    continue;

                {
                    std::lock_guard<std::mutex> lock_impacts(g_BulletImpactsMtx);
                    UpdateImpacts();
                }

                if (!ped->HasFlag(ePedConfigFlag::IsShooting))
                    continue;

                auto* weaponManager = ped->GetWeaponManager();
                if (!weaponManager)
                    continue;

                D3DXVECTOR3 end_pos = Mem.Read<D3DXVECTOR3>((uintptr_t)weaponManager + 0x1B0);
                if (end_pos.x == 0.f && end_pos.y == 0.f && end_pos.z == 0.f)
                    continue;

                if (end_pos.x == last_impact.x && end_pos.y == last_impact.y && end_pos.z == last_impact.z)
                    continue;

                D3DXVECTOR3 start_pos = Mem.Read<D3DXVECTOR3>((uintptr_t)ped + 0x90);
                uintptr_t FragInst = Mem.Read<uintptr_t>((uintptr_t)ped + g_Offsets.m_FragInst);
                if (FragInst)
                {
                    uintptr_t v9 = Mem.Read<uintptr_t>(FragInst + 0x68);
                    if (v9)
                    {
                        Core::SDK::Game::cSkeleton_t Skeleton;
                        Skeleton.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
                        Skeleton.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
                        Skeleton.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
                        Skeleton.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
                        Skeleton.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);

                        if (Skeleton.crSkeletonData.m_BoneIdTable_Slots)
                        {
                            Skeleton.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
                            Skeleton.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
                            Skeleton.Arg2 = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);

                            D3DXVECTOR3 rightHandPos = Core::SDK::Game::GetBonePosComplex(ped, SKEL_R_Hand, Skeleton);
                            if (rightHandPos.x != 0 || rightHandPos.y != 0 || rightHandPos.z != 0)
                                start_pos = rightHandPos;
                        }
                    }
                }

                D3DXVECTOR3 dir = end_pos - start_pos;
                float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
                if (len > 0.1f) {
                    float invLen = 1.0f / len;
                    start_pos.x += dir.x * invLen * 0.85f;
                    start_pos.y += dir.y * invLen * 0.85f;
                    start_pos.z += dir.z * invLen * 0.85f + 0.08f;
                }

                {
                    std::lock_guard<std::mutex> lock_impacts(g_BulletImpactsMtx);
                    g_BulletImpacts.push_back({ start_pos, end_pos, std::chrono::steady_clock::now() });
                }
                last_impact = end_pos;
                } catch (...) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
    }
}

void Core::Features::cEsp::Draw()
{
    if (option->key.enable_esp_key && GetForegroundWindow() == g_Variables.g_hGameWindow) {
        if (Utils::KeyPressedWithDelay(option->key.enable_esp_key, 250))
            option->param.enable_esp = !option->param.enable_esp;
    }

    bool espActive = option->param.enable_esp;

    if (!espActive)
        return;

    CPed* aimbotTarget = nullptr;
    bool aimActive = false;
    if (option->key.enable_aimbot_key != 0) {
        if (option->key.enable_aimbot_mode == 0) {
            aimActive = option->param.enable_aimbot;
        } else if (option->key.enable_aimbot_mode == 1) {
            aimActive = (GetAsyncKeyState(option->key.enable_aimbot_key) & 0x8000) != 0;
        }
    }
    aimActive = aimActive && GetForegroundWindow() != g_Variables.g_hCheatWindow;

    if (aimActive) {
        aimbotTarget = Core::SDK::Game::GetClosestPed(option->param.aim_distance, option->param.ignore_npcs, option->param.check_visible);
        if (aimbotTarget) {
            D3DXVECTOR3 hp = GetHitbonePosForPed(aimbotTarget, option->param.hitbone);
            D3DXVECTOR2 hps = Core::SDK::Game::WorldToScreen(hp);
            if (Core::SDK::Game::IsOnScreen(hps)) {
                float adx = hps.x - g_Variables.g_vGameWindowCenter.x, ady = hps.y - g_Variables.g_vGameWindowCenter.y;
                float afov = (float)option->param.fov_size;
                if (afov > 0 && adx*adx + ady*ady >= afov * afov) aimbotTarget = nullptr;
            } else {
                aimbotTarget = nullptr;
            }
        }
    }

    auto drawTargetLine = [&](CPed* target, int hitbone, float fovRadius, const float* color) {
        if (!target) return;
        D3DXVECTOR3 bonePos = GetHitbonePosForPed(target, hitbone);
        D3DXVECTOR2 boneScr = Core::SDK::Game::WorldToScreen(bonePos);
        if (!Core::SDK::Game::IsOnScreen(boneScr)) return;
        ImDrawList* dl = ImGui::GetBackgroundDrawList();
        ImVec2 center(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowCenter.y);
        ImVec2 end(boneScr.x, boneScr.y);
        if (fovRadius > 0.0f) {
            float dx = end.x - center.x;
            float dy = end.y - center.y;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist > fovRadius) {
                float inv = 1.0f / dist;
                end.x = center.x + dx * inv * fovRadius;
                end.y = center.y + dy * inv * fovRadius;
            }
        }
        dl->AddLine(center, end, ImGui::GetColorU32(ImVec4(color[0], color[1], color[2], color[3])), 2.0f);
    };

    if (option->param.target_line && aimbotTarget) {
        float fovR = option->param.fov_size > 0 ? (float)option->param.fov_size : 0.0f;
        drawTargetLine(aimbotTarget, option->param.hitbone, fovR, option->param.visualize_aimbot_color);
    }

    if (option->param.silent_target_line) {
        bool silentActive = option->param.silent_aim;
        if (silentActive && option->key.silent_aim_key != 0) {
            if (option->key.silent_aim_mode == 1)
                silentActive = (GetAsyncKeyState(option->key.silent_aim_key) & 0x8000) != 0;
        } else if (!option->key.silent_aim_key) {
            silentActive = false;
        }
        if (silentActive && GetForegroundWindow() != g_Variables.g_hCheatWindow) {
            CPed* silentTarget = Core::SDK::Game::GetClosestPed(option->param.aim_distance, option->param.silent_ignore_npcs, option->param.silent_check_visible);
            if (silentTarget) {
                D3DXVECTOR3 sp = GetHitbonePosForPed(silentTarget, option->param.silent_hitbone);
                D3DXVECTOR2 ss = Core::SDK::Game::WorldToScreen(sp);
                if (Core::SDK::Game::IsOnScreen(ss)) {
                    float sdx = ss.x - g_Variables.g_vGameWindowCenter.x, sdy = ss.y - g_Variables.g_vGameWindowCenter.y;
                    float sfovLimit = (float)option->param.silent_fov_size;
                    if (sdx*sdx + sdy*sdy < sfovLimit * sfovLimit)
                        drawTargetLine(silentTarget, option->param.silent_hitbone, sfovLimit, option->param.silent_fov_color);
                }
            }
        }
    }

    if (option->param.triggerbot_target_line) {
        bool trigActive = option->param.enable_triggerbot;
        if (trigActive && option->key.triggerbot_key != 0) {
            if (option->key.triggerbot_mode == 1)
                trigActive = (GetAsyncKeyState(option->key.triggerbot_key) & 0x8000) != 0;
        } else if (!option->key.triggerbot_key) {
            trigActive = false;
        }
        if (trigActive && GetForegroundWindow() != g_Variables.g_hCheatWindow) {
            CPed* trigTarget = Core::SDK::Game::GetClosestPedEx(option->param.aim_distance, option->param.triggerbot_ignore_npcs, option->param.triggerbot_check_visible, option->param.triggerbot_ignore_friends);
            if (trigTarget) {
                D3DXVECTOR3 tp = GetHitbonePosForPed(trigTarget, option->param.triggerbot_hitbone);
                D3DXVECTOR2 ts = Core::SDK::Game::WorldToScreen(tp);
                if (Core::SDK::Game::IsOnScreen(ts)) {
                    float tdx = ts.x - g_Variables.g_vGameWindowCenter.x, tdy = ts.y - g_Variables.g_vGameWindowCenter.y;
                    float tfovLimit = (float)option->param.triggerbot_fov_size;
                    if (tdx*tdx + tdy*tdy < tfovLimit * tfovLimit)
                        drawTargetLine(trigTarget, option->param.triggerbot_hitbone, tfovLimit, option->param.triggerbot_fov_color);
                }
            }
        }
    }

    if (option->param.tracer)
    {
        auto DrawList = ImGui::GetBackgroundDrawList();
        std::lock_guard<std::mutex> lock_draw(Core::Features::g_BulletImpactsMtx);
        Core::Features::UpdateImpacts();
        for (const auto& impact : Core::Features::g_BulletImpacts)
        {
            D3DXVECTOR2 s0 = Core::SDK::Game::WorldToScreen(impact.start_pos);
            D3DXVECTOR2 s1 = Core::SDK::Game::WorldToScreen(impact.end_pos);
            if (Core::SDK::Game::IsOnScreen(s0) && Core::SDK::Game::IsOnScreen(s1))
            {
                float a = (option->param.tracer_color[3]) * (impact.current_alpha / 255.0f);
                ImU32 col = ImGui::GetColorU32(ImVec4(option->param.tracer_color[0], option->param.tracer_color[1], option->param.tracer_color[2], a));
                DrawList->AddLine(ImVec2(s0.x, s0.y), ImVec2(s1.x, s1.y), col, option->param.tracer_thickness);
            }
        }
    }

    static std::unordered_map<CPed*, Core::SDK::Game::EspAnim> vEspAnimations;
    static std::chrono::steady_clock::time_point last_anim_cleanup = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_anim_cleanup).count() > 10) {
        std::lock_guard<std::mutex> Guard(Core::SDK::Game::EntityListMutex);
        std::unordered_set<CPed*> liveSet;
        liveSet.reserve(Core::SDK::Game::EntityList.size());
        for (const auto& e : Core::SDK::Game::EntityList) liveSet.insert(e.Ped);
        for (auto it = vEspAnimations.begin(); it != vEspAnimations.end(); )
            it = liveSet.count(it->first) ? ++it : vEspAnimations.erase(it);
        last_anim_cleanup = std::chrono::steady_clock::now();
    }

    CPed* teleportBehindMarkerTarget = nullptr;
    if (option->param.teleport_behind_enemy && Core::SDK::Pointers::pLocalPlayer) {
      CPed* localPed = Core::SDK::Pointers::pLocalPlayer;
      float maxRange = option->param.teleport_behind_enemy_range > 1.0f
        ? option->param.teleport_behind_enemy_range
        : 1.0f;
      float maxFov = option->param.teleport_behind_enemy_fov;
      if (maxFov < 5.0f)
        maxFov = 20.0f;
      float maxFovSq = maxFov * maxFov;
      float bestDistSq = FLT_MAX;

      std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
      for (auto& candidate : Core::SDK::Game::EntityList) {
        CPed* ped = candidate.Ped;
        if (!ped || ped == localPed)
          continue;
        if (!candidate.IsPlayer)
          continue;
        if (std::abs(candidate.Health) <= 1.0f)
          continue;
        if (candidate.IsFriend)
          continue;
        if (candidate.Distance > maxRange)
          continue;

        D3DXVECTOR2 candidateScreen = Core::SDK::Game::WorldToScreen(candidate.Pos);
        if (!Core::SDK::Game::IsOnScreen(candidateScreen))
          continue;

        float dx = candidateScreen.x - g_Variables.g_vGameWindowCenter.x;
        float dy = candidateScreen.y - g_Variables.g_vGameWindowCenter.y;
        float distSq = dx * dx + dy * dy;
        if (distSq > maxFovSq)
          continue;

        if (distSq < bestDistSq) {
          bestDistSq = distSq;
          teleportBehindMarkerTarget = ped;
        }
      }
    }

    Core::SDK::Game::TeleportBehindMarkerTarget = teleportBehindMarkerTarget;

    const size_t MAX_ESP_ENTITIES = 512;
    static std::vector<Core::SDK::Game::EntityStruct> s_cached_draw_entities;
    static int s_esp_order_tick = 0;
    if (++s_esp_order_tick >= 2) {
        s_esp_order_tick = 0;
        s_cached_draw_entities.clear();

        std::lock_guard<std::mutex> Guard(Core::SDK::Game::EntityListMutex);
        s_cached_draw_entities.reserve(Core::SDK::Game::EntityList.size());
        for (const auto& e : Core::SDK::Game::EntityList) {
            if (!option->param.show_local_player && e.Ped == Core::SDK::Pointers::pLocalPlayer) continue;
            if (e.Distance > option->param.esp_max_distance) continue;

            if (option->param.esp_ignore_npcs && !e.IsPlayer && e.Ped != Core::SDK::Pointers::pLocalPlayer) continue;

            if (option->param.esp_ignore_dead && e.Ped != Core::SDK::Pointers::pLocalPlayer) {
                if (e.IsPlayer) { if (e.Health > 400.f || e.Health <= 1.0f) continue; }
                else { if (e.Health > 200.f || e.Health <= 1.f) continue; }
            }
            s_cached_draw_entities.push_back(e);
        }
        std::sort(s_cached_draw_entities.begin(), s_cached_draw_entities.end(), [](const auto& a, const auto& b) { return a.Distance < b.Distance; });
        if (s_cached_draw_entities.size() > MAX_ESP_ENTITIES)
            s_cached_draw_entities.resize(MAX_ESP_ENTITIES);
    }
    ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

    if (!Core::SDK::Pointers::pViewPort) return;
    D3DXMATRIX ViewMatrix = Mem.Read<D3DXMATRIX>(Core::SDK::Pointers::pViewPort + 0x24C);
    D3DXMatrixTranspose(&ViewMatrix, &ViewMatrix);
    Core::SDK::Game::SetCachedViewMatrixForFrame(ViewMatrix);

    auto VecX = D3DXVECTOR4(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23, ViewMatrix._24),
         VecZ = D3DXVECTOR4(ViewMatrix._41, ViewMatrix._42, ViewMatrix._43, ViewMatrix._44);

    for (size_t j = 0; j < s_cached_draw_entities.size(); j++)
    {
        Core::SDK::Game::EntityStruct& Entity = s_cached_draw_entities[j];
        CPed* Ped = Entity.Ped;
        if (!Ped)
            continue;

        auto [it_anim, inserted_anim] = vEspAnimations.emplace(Ped, Core::SDK::Game::EspAnim{});
        (void)inserted_anim;
        auto& CurrentESPAnim = it_anim->second;
        CurrentESPAnim.CanFadeOut = false;

        float Distance = Entity.Distance;
        float Health = Entity.Health;
        int PedType = Entity.PedType;
        bool IsFriend = Entity.IsFriend;
        bool IsLocalPlayer = Ped == Core::SDK::Pointers::pLocalPlayer;

        if (!option->param.show_local_player && IsLocalPlayer)
            continue;

        D3DXVECTOR3 HeadPos;
        D3DXVECTOR2 EntityTop;
        D3DXVECTOR2 EntityBottom;
        float BoxLeft = 0.0f;
        float BoxRight = 0.0f;
        bool dynamicBoxCalculated = false;
        bool hasValidSkeleton = false;
        Core::SDK::Game::cSkeleton_t Skeleton = {};
        D3DXVECTOR3 batchPositions[40] = {};

        auto DrawArrow = [&](ImVec2 pos, float ang, ImColor col, float size) {
            ImVec2 dir = ImVec2(std::cos(ang), std::sin(ang));
            ImVec2 ort = ImVec2(-dir.y, dir.x);
            ImVec2 p1 = ImVec2(pos.x + dir.x * size, pos.y + dir.y * size);
            ImVec2 p2 = ImVec2(pos.x - dir.x * size * 0.6f + ort.x * size * 0.6f, pos.y - dir.y * size * 0.6f + ort.y * size * 0.6f);
            ImVec2 p3 = ImVec2(pos.x - dir.x * size * 0.6f - ort.x * size * 0.6f, pos.y - dir.y * size * 0.6f - ort.y * size * 0.6f);
            DrawList->AddTriangleFilled(p1, p2, p3, col);
        };

        uintptr_t FragInst = Mem.Read<uintptr_t>((uintptr_t)Ped + g_Offsets.m_FragInst);
        if (FragInst) {
            uintptr_t v9 = Mem.Read<uintptr_t>(FragInst + 0x68);
            if (v9) {
                Skeleton.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
                Skeleton.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
                Skeleton.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
                Skeleton.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
                Skeleton.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);

                if (Skeleton.crSkeletonData.m_BoneIdTable_Slots) {
                    Skeleton.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
                    Skeleton.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
                    Skeleton.Arg2 = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);
                    hasValidSkeleton = true;

                    Core::SDK::Game::GetBonePositionsBatch(Ped, Skeleton, g_espSkeletonBoneIds, 40, batchPositions);
                    HeadPos = batchPositions[0];

                    float minX = FLT_MAX, minY = FLT_MAX;
                    float maxX = -FLT_MAX, maxY = -FLT_MAX;
                    bool anyValid = false;
                    for (int i = 0; i < 15; ++i) {
                        const D3DXVECTOR3& pos = batchPositions[i];
                        if (pos.x == 0 && pos.y == 0 && pos.z == 0) continue;
                        D3DXVECTOR2 screen = Core::SDK::Game::WorldToScreen(pos);
                        if (!Core::SDK::Game::IsOnScreen(screen)) continue;
                        if (screen.x < minX) minX = screen.x;
                        if (screen.x > maxX) maxX = screen.x;
                        if (screen.y < minY) minY = screen.y;
                        if (screen.y > maxY) maxY = screen.y;
                        anyValid = true;
                    }

                    if (anyValid) {
                        float width = maxX - minX;
                        float height = maxY - minY;
                        if (width < 5.0f) width = 5.0f;
                        if (height < 5.0f) height = 5.0f;

                        float padX = width * 0.26f;
                        float padY_Top = height * 0.20f;
                        float padY_Bottom = height * 0.16f;

                        float finalWidth = width + padX * 2.0f;
                        float minWidth = height * 0.28f;
                        float extra = 0.0f;
                        if (finalWidth < minWidth) extra = (minWidth - finalWidth) * 0.5f;

                        BoxLeft = minX - padX - extra;
                        BoxRight = maxX + padX + extra;

                        EntityTop = D3DXVECTOR2((BoxLeft + BoxRight) * 0.5f, minY - padY_Top);
                        EntityBottom = D3DXVECTOR2((BoxLeft + BoxRight) * 0.5f, maxY + padY_Bottom);

                        dynamicBoxCalculated = true;
                    }
                }
            }
        }

        if (option->param.arrows && !IsLocalPlayer)
        {
            if (!hasValidSkeleton)
                HeadPos = Ped->GetBonePosDefault(0);
            D3DXVECTOR2 headScreen = Core::SDK::Game::WorldToScreen(HeadPos);
            ImVec2 center = ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowCenter.y);

            ImGuiIO& io2 = ImGui::GetIO();
            float defaultFovR = (std::min)(io2.DisplaySize.x, io2.DisplaySize.y) * 0.33f;

            float fovR = 0.0f;
            if (option->param.fov_circle) fovR = (std::max)(fovR, option->param.fov_size > 0.0f ? option->param.fov_size : defaultFovR);
            if (option->param.silent_dual_fov && option->param.silent_aim) {
                float nearR = option->param.silent_fov_near;
                float farR = option->param.silent_fov_far;
                if (nearR > 0.0f || farR > 0.0f) {
                    fovR = (std::max)(fovR, (std::max)(nearR, farR));
                }
            } else if (option->param.silent_fov_circle) {
                fovR = (std::max)(fovR, option->param.silent_fov_size > 0.0f ? option->param.silent_fov_size : defaultFovR);
            }
            if (option->param.triggerbot_fov_circle) fovR = (std::max)(fovR, option->param.triggerbot_fov_size > 0.0f ? option->param.triggerbot_fov_size : defaultFovR);

            if (fovR <= 0.0f) {
                fovR = defaultFovR;
            }

            bool onScreen = Core::SDK::Game::IsOnScreen(headScreen);
            float dx = headScreen.x - center.x;
            float dy = headScreen.y - center.y;
            float dist = std::hypot(dx, dy);
            if (!onScreen || dist > fovR)
            {
                float relX = (VecX.x * HeadPos.x) + (VecX.y * HeadPos.y) + (VecX.z * HeadPos.z) + VecX.w;
                float relZ = (VecZ.x * HeadPos.x) + (VecZ.y * HeadPos.y) + (VecZ.z * HeadPos.z) + VecZ.w;

                float ang = std::atan2(relX, relZ) - 1.57079632679f;
                float gap = option->param.arrows_size * 0.9f + 10.0f;
                ImVec2 pos = ImVec2(center.x + std::cos(ang) * (fovR + gap), center.y + std::sin(ang) * (fovR + gap));
                ImColor arrowCol = ImColor(option->param.arrows_color[0], option->param.arrows_color[1], option->param.arrows_color[2], option->param.arrows_color[3]);
                if (option->param.team_check && IsFriend)
                    arrowCol = ImColor(option->param.team_check_color[0], option->param.team_check_color[1], option->param.team_check_color[2], option->param.team_check_color[3]);
                else if (option->param.visible_check && Entity.Visible)
                    arrowCol = ImColor(option->param.visible_check_color[0], option->param.visible_check_color[1], option->param.visible_check_color[2], option->param.visible_check_color[3]);
                DrawArrow(pos, ang, arrowCol, option->param.arrows_size);
            }
        }

        if (!dynamicBoxCalculated) {
            HeadPos = Ped->GetBonePosDefault(0);
            EntityTop = Core::SDK::Game::WorldToScreen(HeadPos + D3DXVECTOR3(0, 0, 0.55));
            EntityBottom = Core::SDK::Game::WorldToScreen(
                Ped->GetBonePosDefault(8) - (g_Offsets.CurrentBuild >= 2802 ? D3DXVECTOR3(0, 0, 2.3) : D3DXVECTOR3(0, 0, 1.5))
            );

            if (EntityBottom.y < EntityTop.y)
                std::swap(EntityTop, EntityBottom);

            float height = EntityBottom.y - EntityTop.y;
            float width = height * 0.28f;
            if (width < 5.0f) width = 5.0f;
            float halfWidth = width * 0.5f;
            BoxLeft = EntityTop.x - halfWidth;
            BoxRight = EntityTop.x + halfWidth;
        }

        if (!Core::SDK::Game::IsOnScreen(EntityTop) || !Core::SDK::Game::IsOnScreen(EntityBottom))
            continue;

        float Height = EntityBottom.y - EntityTop.y;

        ImVec2 BoxMin(BoxLeft, EntityTop.y);
        ImVec2 BoxMax(BoxRight, EntityBottom.y);
        ImVec2 BoxCenter = ImVec2((BoxMin.x + BoxMax.x) * 0.5f, (BoxMin.y + BoxMax.y) * 0.5f);

        float FirstTextBoxTop = BoxMin.y - 18;
        float spacingBase = Height * 0.06f;
        if (spacingBase < 2.0f) spacingBase = 2.0f;
        if (spacingBase > 10.0f) spacingBase = 10.0f;
        float FirstTextBoxBottom = BoxMax.y + spacingBase;
        float SecondTextBoxBottom = BoxMax.y + (4 * 2) + 8;
        float ThirdTextBoxBottom = BoxMax.y + (4 * 3) + (8 * 2);

        float barOffsetUnit = Height * 0.05f;
        if (barOffsetUnit < 4.0f) barOffsetUnit = 4.0f;
        if (barOffsetUnit > 12.0f) barOffsetUnit = 12.0f;
        int bottomBars = (option->param.health_bar_pos == 2) + (option->param.armor_bar_pos == 2);
        if (bottomBars > 0) {
            float add = barOffsetUnit * bottomBars;
            FirstTextBoxBottom += add;
            SecondTextBoxBottom += add;
            ThirdTextBoxBottom += add;
            float farFactor = 0.0f;
            if (Height < 120.0f) {
                farFactor = (120.0f - Height) / 120.0f;
                if (farFactor < 0.0f) farFactor = 0.0f;
                if (farFactor > 1.0f) farFactor = 1.0f;
            }
            float extra = (4.0f + 6.0f * farFactor) * bottomBars;
            FirstTextBoxBottom += extra;
            SecondTextBoxBottom += extra;
            ThirdTextBoxBottom += extra;
        }

        float topBarOffsetUnit = Height * 0.04f;
        if (topBarOffsetUnit < 2.0f) topBarOffsetUnit = 2.0f;
        if (topBarOffsetUnit > 8.0f) topBarOffsetUnit = 8.0f;
        if (option->param.health_bar_pos == 0) FirstTextBoxTop -= topBarOffsetUnit;
        if (option->param.armor_bar_pos == 0) FirstTextBoxTop -= topBarOffsetUnit;

        if (var->friends_tab.enable_setfriend_keybind && !IsLocalPlayer)
        {
            float radius = 2 / Distance;
            float FovRadius = var->friends_tab.setfriend_fov;
            const static int Delay = 300;
            int Key = var->friends_tab.setfriend_key;

            float FovSize = std::hypot(BoxCenter.x - g_Variables.g_vGameWindowCenter.x, BoxCenter.y - g_Variables.g_vGameWindowCenter.y);

            ImU32 dot_col = IsFriend
                ? ImColor(clr->base_colors.accent_clr)
                : ImColor(255, 255, 255);
            DrawList->AddCircleFilled(ImVec2(BoxCenter.x, BoxCenter.y), radius + Distance < 1 ? 1 : 4, ImColor(0, 0, 0), 999);
            DrawList->AddCircleFilled(ImVec2(BoxCenter.x, BoxCenter.y), radius + Distance < 1 ? 0 : 3, dot_col, 999);

            if (FovSize < FovRadius)
            {
                const char* label = IsFriend ? xorstr("Remove Friend") : xorstr("Add Friend");
                ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, label);
                DrawTextOutlined(DrawList, var->font.instrument_medium[0], var->font.instrument_medium[0]->FontSize, ImVec2(BoxCenter.x - (text_size.x / 2), BoxCenter.y - 30), ImColor(255, 255, 255), label);

                if (option->param.teleport_behind_enemy &&
                    Core::SDK::Game::TeleportBehindMarkerTarget == Ped)
                {
                    const char* teleportLabel = xorstr("TP Behind");
                    ImVec2 teleportTextSize = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, teleportLabel);
                    DrawTextOutlined(DrawList, var->font.instrument_medium[0], var->font.instrument_medium[0]->FontSize,
                                     ImVec2(BoxCenter.x - (teleportTextSize.x * 0.5f), BoxCenter.y + 10), ImColor(255, 255, 255), teleportLabel);
                }

                if (Utils::KeyPressedWithDelay(Key, Delay))
                {
                    bool newState = !IsFriend;
                    IsFriend = newState;
                    Core::SDK::Game::SetFriendByName(Entity.NetworkInfo.UserName, newState);
                }
            }
        }

        if (IsFriend && !option->param.team_check)
            continue;

        if (option->param.snaplines)
        {
            DrawList->AddLine(ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y), ImVec2(BoxCenter.x, BoxMax.y), ImColor(0, 0, 0), 2.5);
            DrawList->AddLine(ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y), ImVec2(BoxCenter.x, BoxMax.y), ImColor(option->param.snaplines_color[0], option->param.snaplines_color[1], option->param.snaplines_color[2], option->param.snaplines_color[3]), 1.5);
        }

        ImColor activeColBox = ImColor(option->param.box_color[0], option->param.box_color[1], option->param.box_color[2], option->param.box_color[3]);
        ImColor activeColSkel = ImColor(option->param.skeleton_color[0], option->param.skeleton_color[1], option->param.skeleton_color[2], option->param.skeleton_color[3]);

        if (option->param.team_check && IsFriend) {
            activeColBox = ImColor(option->param.team_check_color[0], option->param.team_check_color[1], option->param.team_check_color[2], option->param.team_check_color[3]);
            activeColSkel = activeColBox;
        }
        else if (option->param.visible_check && Entity.Visible) {
            activeColBox = ImColor(option->param.visible_check_color[0], option->param.visible_check_color[1], option->param.visible_check_color[2], option->param.visible_check_color[3]);
            activeColSkel = activeColBox;
        }

        if (option->param.admin_check_esp && Entity.IsPlayer && Entity.IsGodMode) {
            activeColBox = ImColor(option->param.admin_check_esp_color[0], option->param.admin_check_esp_color[1], option->param.admin_check_esp_color[2], option->param.admin_check_esp_color[3]);
            activeColSkel = activeColBox;
        }

        if (!IsLocalPlayer && var->friends_tab.selected_player_ped != nullptr &&
            Entity.Ped == var->friends_tab.selected_player_ped)
        {
            float t = (float)ImGui::GetTime();

            float scale = 1.1f - (Distance / 250.0f);
            if (scale < 0.55f) scale = 0.55f;
            if (scale > 1.2f)  scale = 1.2f;

            float bob = sinf(t * 4.5f) * 3.0f * scale;
            float ax = (BoxLeft + BoxRight) * 0.5f;
            float ay_tip = EntityTop.y - 10.0f * scale - bob;
            float half_w = 9.0f * scale;
            float height = 14.0f * scale;

            ImVec2 tip(ax, ay_tip);
            ImVec2 tl(ax - half_w, ay_tip - height);
            ImVec2 tr(ax + half_w, ay_tip - height);

            ImVec4 acc = ImVec4(var->gui.accent_clr[0], var->gui.accent_clr[1],
                                var->gui.accent_clr[2], 1.0f);
            ImU32 col_fill = ImGui::GetColorU32(ImVec4(acc.x, acc.y, acc.z, 0.90f));
            ImU32 col_shadow = ImGui::GetColorU32(ImVec4(0.f, 0.f, 0.f, 0.55f));
            ImU32 col_glow = ImGui::GetColorU32(ImVec4(acc.x, acc.y, acc.z, 0.25f));

            DrawList->AddTriangleFilled(
                tip + ImVec2(0, 1), tl + ImVec2(0, 1), tr + ImVec2(0, 1),
                col_shadow);

            float glow_scale = 1.35f;
            ImVec2 gc(ax, ay_tip - height * 0.5f);
            ImVec2 g_tip(ax, ay_tip + (tip.y - gc.y) * (glow_scale - 1.0f) + (ay_tip - gc.y) * glow_scale + gc.y - ay_tip);
            ImVec2 g_tl(ax - half_w * glow_scale, gc.y + (tl.y - gc.y) * glow_scale);
            ImVec2 g_tr(ax + half_w * glow_scale, gc.y + (tr.y - gc.y) * glow_scale);
            ImVec2 g_tip2(ax, gc.y + (tip.y - gc.y) * glow_scale);
            DrawList->AddTriangleFilled(g_tip2, g_tl, g_tr, col_glow);

            DrawList->AddTriangleFilled(tip, tl, tr, col_fill);

            ImU32 col_stroke = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.85f));
            DrawList->AddTriangle(tip, tl, tr, col_stroke, 1.4f);
        }

        if (option->param.bounding_box)
        {
            if (option->param.box_style == 1)
            {
                float midY = (EntityTop.y + EntityBottom.y) * 0.5f;
                float r = ((ImVec4)activeColBox).x;
                float g = ((ImVec4)activeColBox).y;
                float b = ((ImVec4)activeColBox).z;
                float a = ((ImVec4)activeColBox).w;

                float aTop = a;
                float aMid = a * 0.2f;
                float aBot = a;

                ImColor colTop(r, g, b, aTop);
                ImColor colMid(r, g, b, aMid);
                ImColor colBot(r, g, b, aBot);

                DrawList->AddRectFilledMultiColor(
                    ImVec2(BoxLeft, EntityTop.y), ImVec2(BoxRight, midY),
                    colTop, colTop, colMid, colMid
                );
                DrawList->AddRectFilledMultiColor(
                    ImVec2(BoxLeft, midY), ImVec2(BoxRight, EntityBottom.y),
                    colMid, colMid, colBot, colBot
                );
            }

            float w = BoxRight - BoxLeft;
            float h = EntityBottom.y - EntityTop.y;
            float lineW = w * 0.25f;
            float lineH = h * 0.25f;

            DrawList->AddLine({ BoxLeft, EntityTop.y }, { BoxLeft + lineW, EntityTop.y }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxLeft, EntityTop.y }, { BoxLeft, EntityTop.y + lineH }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxRight, EntityTop.y }, { BoxRight - lineW, EntityTop.y }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxRight, EntityTop.y }, { BoxRight, EntityTop.y + lineH }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxLeft, EntityBottom.y }, { BoxLeft + lineW, EntityBottom.y }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxLeft, EntityBottom.y }, { BoxLeft, EntityBottom.y - lineH }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxRight, EntityBottom.y }, { BoxRight - lineW, EntityBottom.y }, activeColBox, 1.5f);
            DrawList->AddLine({ BoxRight, EntityBottom.y }, { BoxRight, EntityBottom.y - lineH }, activeColBox, 1.5f);
        }

        bool isTarget = (aimbotTarget && Ped == aimbotTarget);
        float rainbowT = 0.0f;
        if (isTarget) {
            static auto rainbow_start = std::chrono::steady_clock::now();
            rainbowT = std::chrono::duration<float>(std::chrono::steady_clock::now() - rainbow_start).count();
        }

        if (option->param.skeleton && hasValidSkeleton)
        {
            ImColor skeletonCol = activeColSkel;
            if (isTarget) {
                float t = rainbowT;
                float h = std::fmod(t * 0.25f, 120.0f);
                float r, g, b;
                ImGui::ColorConvertHSVtoRGB(h, 1.0f, 1.0f, r, g, b);
                skeletonCol = ImColor(r, g, b, 1.0f);
            }
            const D3DXVECTOR3& PelvisPos = batchPositions[5];
            const D3DXVECTOR3& NeckPos = batchPositions[15];
            const D3DXVECTOR3& LeftUperarmPos = batchPositions[12];
            const D3DXVECTOR3& RightUperarmPos = batchPositions[13];
            const D3DXVECTOR3& RightFormArmPos = batchPositions[11];
            const D3DXVECTOR3& LeftFormArmPos = batchPositions[10];
            const D3DXVECTOR3& RightHandPos = batchPositions[4];
            const D3DXVECTOR3& LeftHandPos = batchPositions[3];
            const D3DXVECTOR3& LeftThighPos = batchPositions[8];
            const D3DXVECTOR3& LeftCalfPos = batchPositions[6];
            const D3DXVECTOR3& RightThighPos = batchPositions[9];
            const D3DXVECTOR3& RightCalfPos = batchPositions[7];
            const D3DXVECTOR3& LfootPos = batchPositions[1];
            const D3DXVECTOR3& RfootPos = batchPositions[2];
            const D3DXVECTOR3& LToePos = batchPositions[18];
            const D3DXVECTOR3& RToePos = batchPositions[19];
            const D3DXVECTOR3& LClaviclePos = batchPositions[16];
            const D3DXVECTOR3& RClaviclePos = batchPositions[17];

            D3DXVECTOR2 Pelvis = Core::SDK::Game::WorldToScreen(PelvisPos);
            D3DXVECTOR2 Neck = Core::SDK::Game::WorldToScreen(NeckPos);
            D3DXVECTOR2 LeftUperarm = Core::SDK::Game::WorldToScreen(LeftUperarmPos);
            D3DXVECTOR2 RightUperarm = Core::SDK::Game::WorldToScreen(RightUperarmPos);
            D3DXVECTOR2 RightFormArm = Core::SDK::Game::WorldToScreen(RightFormArmPos);
            D3DXVECTOR2 LeftFormArm = Core::SDK::Game::WorldToScreen(LeftFormArmPos);
            D3DXVECTOR2 RightHand = Core::SDK::Game::WorldToScreen(RightHandPos);
            D3DXVECTOR2 LeftHand = Core::SDK::Game::WorldToScreen(LeftHandPos);
            D3DXVECTOR2 LeftThigh = Core::SDK::Game::WorldToScreen(LeftThighPos);
            D3DXVECTOR2 LeftCalf = Core::SDK::Game::WorldToScreen(LeftCalfPos);
            D3DXVECTOR2 RightThigh = Core::SDK::Game::WorldToScreen(RightThighPos);
            D3DXVECTOR2 RightCalf = Core::SDK::Game::WorldToScreen(RightCalfPos);
            D3DXVECTOR2 Lfoot = Core::SDK::Game::WorldToScreen(LfootPos);
            D3DXVECTOR2 Rfoot = Core::SDK::Game::WorldToScreen(RfootPos);
            D3DXVECTOR2 LToe = Core::SDK::Game::WorldToScreen(LToePos);
            D3DXVECTOR2 RToe = Core::SDK::Game::WorldToScreen(RToePos);
            D3DXVECTOR2 LClavicle = Core::SDK::Game::WorldToScreen(LClaviclePos);
            D3DXVECTOR2 RClavicle = Core::SDK::Game::WorldToScreen(RClaviclePos);

            if (!Core::SDK::Game::IsOnScreen(Lfoot) || !Core::SDK::Game::IsOnScreen(Rfoot) || !Core::SDK::Game::IsOnScreen(Pelvis) ||
                !Core::SDK::Game::IsOnScreen(Neck) || !Core::SDK::Game::IsOnScreen(LeftUperarm) || !Core::SDK::Game::IsOnScreen(RightUperarm) ||
                !Core::SDK::Game::IsOnScreen(RightFormArm) || !Core::SDK::Game::IsOnScreen(LeftFormArm) || !Core::SDK::Game::IsOnScreen(RightHand) ||
                !Core::SDK::Game::IsOnScreen(LeftHand) || !Core::SDK::Game::IsOnScreen(LeftThigh) || !Core::SDK::Game::IsOnScreen(RightThigh) ||
                !Core::SDK::Game::IsOnScreen(RightCalf) || !Core::SDK::Game::IsOnScreen(LToe) || !Core::SDK::Game::IsOnScreen(RToe) ||
                !Core::SDK::Game::IsOnScreen(LClavicle) || !Core::SDK::Game::IsOnScreen(RClavicle))
                continue;

            auto ScreenHead = Core::SDK::Game::WorldToScreen(HeadPos);
            DrawList->AddLine(ImVec2(ScreenHead.x, ScreenHead.y), ImVec2(Neck.x, Neck.y), skeletonCol, 1.5f);

            DrawList->AddLine(ImVec2(Neck.x, Neck.y), ImVec2(LClavicle.x, LClavicle.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(Neck.x, Neck.y), ImVec2(RClavicle.x, RClavicle.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(LClavicle.x, LClavicle.y), ImVec2(LeftUperarm.x, LeftUperarm.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(RClavicle.x, RClavicle.y), ImVec2(RightUperarm.x, RightUperarm.y), skeletonCol, 1.5f);

            {
                static const int leftFingerBatchIdx[5][2] = { {20,21}, {22,23}, {24,25}, {26,27}, {28,29} };
                for (int fi = 0; fi < 5; ++fi) {
                    D3DXVECTOR2 s1 = Core::SDK::Game::WorldToScreen(batchPositions[leftFingerBatchIdx[fi][0]]);
                    D3DXVECTOR2 s2 = Core::SDK::Game::WorldToScreen(batchPositions[leftFingerBatchIdx[fi][1]]);
                    if (Core::SDK::Game::IsOnScreen(s1) && Core::SDK::Game::IsOnScreen(s2)) {
                        DrawList->AddLine(ImVec2(LeftHand.x, LeftHand.y), ImVec2(s1.x, s1.y), skeletonCol, 1.5f);
                        DrawList->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), skeletonCol, 1.5f);
                    }
                }
            }

            {
                static const int rightFingerBatchIdx[5][2] = { {30,31}, {32,33}, {34,35}, {36,37}, {38,39} };
                for (int fi = 0; fi < 5; ++fi) {
                    D3DXVECTOR2 s1 = Core::SDK::Game::WorldToScreen(batchPositions[rightFingerBatchIdx[fi][0]]);
                    D3DXVECTOR2 s2 = Core::SDK::Game::WorldToScreen(batchPositions[rightFingerBatchIdx[fi][1]]);
                    if (Core::SDK::Game::IsOnScreen(s1) && Core::SDK::Game::IsOnScreen(s2)) {
                        DrawList->AddLine(ImVec2(RightHand.x, RightHand.y), ImVec2(s1.x, s1.y), skeletonCol, 1.5f);
                        DrawList->AddLine(ImVec2(s1.x, s1.y), ImVec2(s2.x, s2.y), skeletonCol, 1.5f);
                    }
                }
            }

            DrawList->AddLine(ImVec2(RightUperarm.x, RightUperarm.y), ImVec2(RightFormArm.x, RightFormArm.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(LeftUperarm.x, LeftUperarm.y), ImVec2(LeftFormArm.x, LeftFormArm.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(RightFormArm.x, RightFormArm.y), ImVec2(RightHand.x, RightHand.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(LeftFormArm.x, LeftFormArm.y), ImVec2(LeftHand.x, LeftHand.y), skeletonCol, 1.5f);

            DrawList->AddLine(ImVec2(Neck.x, Neck.y), ImVec2(Pelvis.x, Pelvis.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(Pelvis.x, Pelvis.y), ImVec2(LeftThigh.x, LeftThigh.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(Pelvis.x, Pelvis.y), ImVec2(RightThigh.x, RightThigh.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(LeftThigh.x, LeftThigh.y), ImVec2(LeftCalf.x, LeftCalf.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(RightThigh.x, RightThigh.y), ImVec2(RightCalf.x, RightCalf.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(LeftCalf.x, LeftCalf.y), ImVec2(Lfoot.x, Lfoot.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(RightCalf.x, RightCalf.y), ImVec2(Rfoot.x, Rfoot.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(Lfoot.x, Lfoot.y), ImVec2(LToe.x, LToe.y), skeletonCol, 1.5f);
            DrawList->AddLine(ImVec2(Rfoot.x, Rfoot.y), ImVec2(RToe.x, RToe.y), skeletonCol, 1.5f);
        }

        {
            if (option->param.dotbones && hasValidSkeleton)
            {
                ImColor baseDotCol = ImColor(option->param.dotbones_color[0], option->param.dotbones_color[1], option->param.dotbones_color[2], option->param.dotbones_color[3]);
                ImColor teamCol = ImColor(option->param.team_check_color[0], option->param.team_check_color[1], option->param.team_check_color[2], option->param.team_check_color[3]);
                ImColor visCol = ImColor(option->param.visible_check_color[0], option->param.visible_check_color[1], option->param.visible_check_color[2], option->param.visible_check_color[3]);
                bool closeAimOnTarget = (isTarget && Distance < 12.0f);
                if (isTarget) {
                    float t2 = rainbowT;
                    float h2 = std::fmod(t2 * 0.25f, 120.0f);
                    float r2, g2, b2;
                    ImGui::ColorConvertHSVtoRGB(h2, 1.0f, 1.0f, r2, g2, b2);
                    baseDotCol = ImColor(r2, g2, b2, 1.0f);
                    visCol = baseDotCol;
                }

                if (Distance <= 40.0f) {
                    float maxDist = option->param.esp_max_distance > 1.0f ? option->param.esp_max_distance : 1.0f;
                    float tdist = Distance / maxDist;
                    if (tdist > 1.0f) tdist = 1.0f;
                    if (tdist < 0.0f) tdist = 0.0f;
                    float radius = 2.6f - 1.2f * tdist;
                    if (radius < 1.3f) radius = 1.3f;
                    if (radius > 2.6f) radius = 2.6f;
                    for (int i = 0; i < 20; ++i) {
                        if (!option->param.dotbones_select[i]) continue;
                        if (i == 2) continue;
                        if ((i == 18 || i == 19) && !closeAimOnTarget) continue;
                        const D3DXVECTOR3& pos = batchPositions[g_dotBoneToBatchIndex[i]];
                        D3DXVECTOR2 sp = Core::SDK::Game::WorldToScreen(pos);
                        if (!Core::SDK::Game::IsOnScreen(sp)) continue;
                        ImColor dotCol = baseDotCol;
                        if (option->param.team_check && IsFriend) dotCol = teamCol;
                        else if (option->param.visible_check && Entity.Visible) dotCol = visCol;
                        ImVec4 dv = (ImVec4)dotCol;
                        dv.w *= (1.0f - 0.3f * tdist);
                        dotCol = ImColor(dv);
                        DrawList->AddCircleFilled(ImVec2(sp.x, sp.y), radius, dotCol, 16);
                    }
                }
            }

        }

        const auto DrawHealthBarV = [&DrawList](ImVec2 pos, ImVec2 dim, ImColor col, int background) {
            if (background == 1) {
                DrawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + dim.x, pos.y - (dim.y + 1)), col);
            }
            else {
                DrawList->AddRectFilled(ImVec2(pos.x - 1, pos.y + 1), ImVec2(pos.x + dim.x + 1, pos.y - (dim.y + 2)), ImColor(0, 0, 0, 255));
                DrawList->AddRectFilled(ImVec2(pos.x, pos.y - 1), ImVec2(pos.x + dim.x, pos.y - (dim.y + 2)), ImColor(80, 80, 80, 125));
            }
            };

        const auto DrawHealthBarH = [&DrawList](ImVec2 pos, ImVec2 dim, ImColor col, int background) {
            if (background == 1) {
                DrawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + dim.y, pos.y + dim.x), col);
            }
            else {
                DrawList->AddRectFilled(ImVec2(pos.x - 1, pos.y - 1), ImVec2(pos.x + dim.y + 1, pos.y + dim.x + 1), ImColor(0, 0, 0, 255));
                DrawList->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + dim.y, pos.y + dim.x), ImColor(80, 80, 80, 125));
            }
            };

        if (option->param.health_bar)
        {
            float MaxHealth = Entity.MaxHealth;
            CurrentESPAnim.Health = ImLerp(CurrentESPAnim.Health, Health, ImGui::GetIO().DeltaTime * 4);
            float AnimHealth = CurrentESPAnim.Health;

            float FullHealthBar = Height / 100 * 100;
            float DecreaseHealthBar = FullHealthBar * (AnimHealth / MaxHealth);

            float Width2 = BoxRight - BoxLeft;
            float FullHealthBarH = Width2 / 100 * 100;
            float DecreaseHealthBarH = FullHealthBarH * (AnimHealth / MaxHealth);

            if (DecreaseHealthBarH > FullHealthBarH)
                DecreaseHealthBarH = FullHealthBarH;

            if (DecreaseHealthBar > FullHealthBar)
                DecreaseHealthBar = FullHealthBar;

            ImColor BarColor;
            ImColor FullHealth = ImVec4(ImColor(80, 80, 80, 200));

            if (Health > (MaxHealth / 2))
                BarColor = ImVec4(ImColor(66, 245, 132, 255));
            else if (Health <= (MaxHealth / 2) && (MaxHealth == 200 ? Health > 50 : Health > 150))
                BarColor = ImVec4(ImColor(245, 135, 66, 255));
            else
                BarColor = ImVec4(ImColor(245, 66, 66, 255));

            switch (option->param.health_bar_pos)
            {
            case 0:
                DrawHealthBarH(ImVec2(BoxMin.x, BoxMin.y - 4), ImVec2(2, FullHealthBarH), FullHealth, 0);
                DrawHealthBarH(ImVec2(BoxMin.x, BoxMin.y - 4), ImVec2(2, DecreaseHealthBarH), BarColor, 1);
                break;
            case 1:
                DrawHealthBarV(ImVec2(BoxMax.x + 6, BoxMax.y), ImVec2(2, FullHealthBar), FullHealth, 0);
                DrawHealthBarV(ImVec2(BoxMax.x + 6, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                break;
            case 2:
                DrawHealthBarH(ImVec2(BoxMin.x, BoxMax.y + 6), ImVec2(2, FullHealthBarH), FullHealth, 0);
                DrawHealthBarH(ImVec2(BoxMin.x, BoxMax.y + 6), ImVec2(2, DecreaseHealthBarH), BarColor, 1);
                break;
            case 3:
                DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, FullHealthBar), FullHealth, 0);
                DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                break;
            default:
                DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, FullHealthBar), FullHealth, 0);
                DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                break;
            }
        }

        if (option->param.armor_bar)
        {
            float MaxHealth = Entity.MaxHealth;
            float Armor = Entity.Armor;

            if (Armor > 0)
            {
                MaxHealth = MaxHealth / 2;
                CurrentESPAnim.Armor = ImLerp(CurrentESPAnim.Armor, Armor, ImGui::GetIO().DeltaTime * 4);
                float AnimArmor = CurrentESPAnim.Armor;

                float FullArmorBar = Height / 100 * 100;
                float DecreaseHealthBar = FullArmorBar * (AnimArmor / MaxHealth);

                float Width2 = BoxRight - BoxLeft;
                float FullArmorBarH = Width2 / 100 * 100;
                float DecreaseArmorBarH = FullArmorBarH * (AnimArmor / MaxHealth);

                if (DecreaseHealthBar > FullArmorBar)
                    DecreaseHealthBar = FullArmorBar;

                if (DecreaseArmorBarH > FullArmorBarH)
                    DecreaseArmorBarH = FullArmorBarH;

                ImColor BarColor;
                ImColor FullArmor = ImVec4(ImColor(80, 80, 80, 200));

                if (Armor > (MaxHealth / 2))
                    BarColor = ImVec4(ImColor(85, 128, 200, 255));
                else if (Armor <= (MaxHealth / 2) && (MaxHealth == 200 ? Armor > 50 : Armor > 150))
                    BarColor = ImVec4(ImColor(69, 102, 157, 255));
                else
                    BarColor = ImVec4(ImColor(45, 62, 92, 255));

                switch (option->param.armor_bar_pos)
                {
                case 0:
                    if (option->param.health_bar && option->param.health_bar_pos == option->param.armor_bar_pos)
                    {
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMin.y - 8), ImVec2(2, FullArmorBarH), FullArmor, 0);
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMin.y - 8), ImVec2(2, DecreaseArmorBarH), BarColor, 1);
                    }
                    else
                    {
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMin.y - 4), ImVec2(2, FullArmorBarH), FullArmor, 0);
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMin.y - 4), ImVec2(2, DecreaseArmorBarH), BarColor, 1);
                    }
                    break;
                case 1:
                    if (option->param.health_bar && option->param.health_bar_pos == option->param.armor_bar_pos)
                    {
                        DrawHealthBarV(ImVec2(BoxMax.x + 12, BoxMax.y), ImVec2(2, FullArmorBar), FullArmor, 0);
                        DrawHealthBarV(ImVec2(BoxMax.x + 12, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                    }
                    else
                    {
                        DrawHealthBarV(ImVec2(BoxMax.x + 6, BoxMax.y), ImVec2(2, FullArmorBar), FullArmor, 0);
                        DrawHealthBarV(ImVec2(BoxMax.x + 6, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                    }
                    break;
                case 2:
                    if (option->param.health_bar && option->param.health_bar_pos == option->param.armor_bar_pos)
                    {
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMax.y + 12), ImVec2(2, FullArmorBarH), FullArmor, 0);
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMax.y + 12), ImVec2(2, DecreaseArmorBarH), BarColor, 1);
                    }
                    else
                    {
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMax.y + 6), ImVec2(2, FullArmorBarH), FullArmor, 0);
                        DrawHealthBarH(ImVec2(BoxMin.x, BoxMax.y + 6), ImVec2(2, DecreaseArmorBarH), BarColor, 1);
                    }
                    break;
                case 3:
                    if (option->param.health_bar && option->param.health_bar_pos == option->param.armor_bar_pos)
                    {
                        DrawHealthBarV(ImVec2(BoxMin.x - 12, BoxMax.y), ImVec2(2, FullArmorBar), FullArmor, 0);
                        DrawHealthBarV(ImVec2(BoxMin.x - 12, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                    }
                    else
                    {
                        DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, FullArmorBar), FullArmor, 0);
                        DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                    }
                    break;
                default:
                    DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, FullArmorBar), FullArmor, 0);
                    DrawHealthBarV(ImVec2(BoxMin.x - 6, BoxMax.y), ImVec2(2, DecreaseHealthBar), BarColor, 1);
                    break;
                }
            }
        }

        {
            ImFont* font = var->font.instrument_medium[0];
            float maxDist = option->param.esp_max_distance > 1.0f ? option->param.esp_max_distance : 1.0f;
            float t = Distance / maxDist;
            if (t > 1.0f) t = 1.0f;
            if (t < 0.0f) t = 0.0f;
            float textScale = 1.0f - 0.2f * t;
            float baseFontSize = option->param.font_size > 0.0f ? option->param.font_size : font->FontSize;
            float fontSizeScaled = baseFontSize * textScale;
            float lineH = fontSizeScaled - 2.0f;
            float textYOffset = 0.0f;
            {
                CPed* lp = Core::SDK::Pointers::pLocalPlayer;
                if (lp) {
                    D3DXVECTOR3 lpos = lp->GetPos();
                    float dz = Entity.Pos.z - lpos.z;
                    if (dz > 0.8f) {
                        float shift = Height * 0.06f;
                        if (shift < 4.0f) shift = 4.0f;
                        if (shift > 14.0f) shift = 14.0f;
                        textYOffset = shift;
                    }
                }
            }

            struct Line { std::string text; ImColor col; bool isDistance; bool isWeapon; ID3D11ShaderResourceView* icon; };
            std::vector<Line> topLines;
            std::vector<Line> bottomLines;
            std::vector<Line> leftLines;
            std::vector<Line> rightLines;

            auto pushByPos = [&](int pos, const Line& l) {
                switch (pos) {
                case 0: topLines.push_back(l); break;
                case 1: rightLines.push_back(l); break;
                case 2: bottomLines.push_back(l); break;
                case 3: leftLines.push_back(l); break;
                default: bottomLines.push_back(l); break;
                }
            };

            if (option->param.names)
            {
                std::string name = Entity.NetworkInfo.UserName;
                ImColor nameColor = ImColor(option->param.names_color[0], option->param.names_color[1], option->param.names_color[2], option->param.names_color[3]);
                if (option->param.admin_check_esp && Entity.IsPlayer && Entity.IsGodMode) {
                    name = xorstr("[ADMIN] ") + name;
                    nameColor = ImColor(option->param.admin_check_esp_color[0], option->param.admin_check_esp_color[1], option->param.admin_check_esp_color[2], option->param.admin_check_esp_color[3]);
                }
                Line l{ name, nameColor, false, false, nullptr };
                pushByPos(option->param.names_pos, l);
            }

            std::string weaponName = Entity.WeaponName;
            if (option->param.weapon_name && !weaponName.empty() && weaponName != xorstr(""))
            {
                std::transform(weaponName.begin(), weaponName.end(), weaponName.begin(), ::tolower);
                if (!weaponName.empty()) weaponName[0] = std::toupper(weaponName[0]);
                Line l{ weaponName, ImColor(option->param.weapon_name_color[0], option->param.weapon_name_color[1], option->param.weapon_name_color[2], option->param.weapon_name_color[3]), false, true, nullptr };
                pushByPos(option->param.weapon_name_pos, l);
            }

            if (option->param.distance)
            {
                std::string dist = std::to_string((int)Distance) + xorstr("m");
                Line l{ dist, ImColor(option->param.distance_color[0], option->param.distance_color[1], option->param.distance_color[2], option->param.distance_color[3]), true, false, nullptr };
                pushByPos(option->param.distance_pos, l);
            }

            auto ReorderWeaponInStack = [](std::vector<Line>& lines, bool weaponToEnd) {
                if (lines.empty())
                    return;
                if (weaponToEnd)
                    std::stable_partition(lines.begin(), lines.end(), [](const Line& l) { return !l.isWeapon; });
                else
                    std::stable_partition(lines.begin(), lines.end(), [](const Line& l) { return l.isWeapon; });
            };

            bool allTop = true;
            if (option->param.names && option->param.names_pos != 0) allTop = false;
            if (option->param.distance && option->param.distance_pos != 0) allTop = false;
            if (option->param.weapon_name && option->param.weapon_name_pos != 0) allTop = false;

            if (!allTop)
                ReorderWeaponInStack(topLines, true);
            ReorderWeaponInStack(bottomLines, true);
            ReorderWeaponInStack(leftLines, true);
            ReorderWeaponInStack(rightLines, true);

            auto GetIconDims = [&](float& outW, float& outH) {
                float distScale = 1.0f;
                if (Distance > 20.0f) {
                    distScale = 1.0f - (std::min(Distance - 20.0f, 80.0f) / 80.0f) * 0.5f;
                }
                float baseIconScale = option->param.icon_size > 0.0f ? option->param.icon_size : 1.0f;
                outH = fontSizeScaled * 3.0f * distScale * baseIconScale;
                outW = outH * 1.5f;
            };

            auto RenderLines = [&](const std::vector<Line>& lines, float startY, bool isTop) {
                float curY = isTop ? (startY - 3.0f + textYOffset) : (startY - 2.0f + textYOffset);

                for (int i = 0; i < (int)lines.size(); ++i)
                {
                    const auto& l = lines[i];
                    float itemW = 0.0f;
                    float itemH = lineH;
                    if (l.icon) {
                        GetIconDims(itemW, itemH);
                        itemH += 1.0f;
                    }

                    float y = 0.0f;
                    if (isTop) {
                        curY -= itemH;
                        y = curY;
                    } else {
                        y = curY;
                        curY += itemH;
                    }

                    if (l.icon)
                    {
                        float iconW = itemW;
                        float iconH = itemH - 1.0f;
                        float x = std::floor(BoxCenter.x - (iconW * 0.5f));

                        float yOffsetExtra = 4.0f;
                        ImVec2 min = ImVec2(x, y - (iconH * 0.2f) + yOffsetExtra);
                        ImVec2 max = ImVec2(x + iconW, y + iconH - (iconH * 0.2f) + yOffsetExtra);

                        DrawList->AddImage((ImTextureID)l.icon, min, max, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
                    }
                    else
                    {
                        ImVec2 ts = font->CalcTextSizeA(fontSizeScaled, FLT_MAX, 0.0f, l.text.c_str());
                        float x = std::floor(BoxCenter.x - (ts.x * 0.5f));
                        DrawTextOutlined(DrawList, font, fontSizeScaled, ImVec2(x, y), l.col, l.text.c_str());
                    }
                }
            };

            RenderLines(topLines, FirstTextBoxTop, true);
            RenderLines(bottomLines, FirstTextBoxBottom, false);

            float sidePad = 4.0f;
            float rightBarMaxX = BoxMax.x;
            if (option->param.health_bar && option->param.health_bar_pos == 1)
                rightBarMaxX = (std::max)(rightBarMaxX, BoxMax.x + 6.0f + 3.0f);
            if (option->param.armor_bar && option->param.armor_bar_pos == 1) {
                float armorX = BoxMax.x + 6.0f;
                if (option->param.health_bar && option->param.health_bar_pos == option->param.armor_bar_pos)
                    armorX = BoxMax.x + 12.0f;
                rightBarMaxX = (std::max)(rightBarMaxX, armorX + 3.0f);
            }
            float rightTextX = BoxMax.x + 6.0f;
            if (rightBarMaxX > BoxMax.x)
                rightTextX = rightBarMaxX + sidePad;

            float leftBarMinX = BoxMin.x;
            if (option->param.health_bar && option->param.health_bar_pos == 3)
                leftBarMinX = (std::min)(leftBarMinX, BoxMin.x - 6.0f - 1.0f);
            if (option->param.armor_bar && option->param.armor_bar_pos == 3) {
                float armorX = BoxMin.x - 6.0f;
                if (option->param.health_bar && option->param.health_bar_pos == option->param.armor_bar_pos)
                    armorX = BoxMin.x - 12.0f;
                leftBarMinX = (std::min)(leftBarMinX, armorX - 1.0f);
            }
            float leftTextRightX = BoxMin.x - 6.0f;
            if (leftBarMinX < BoxMin.x)
                leftTextRightX = leftBarMinX - sidePad;

            float rightCurY = BoxMin.y + textYOffset;
            for (int k = 0; k < (int)rightLines.size(); ++k)
            {
                const auto& l = rightLines[k];
                float y = rightCurY;
                float x = rightTextX;
                if (l.icon) {
                    float iconW = 0.0f;
                    float iconH = 0.0f;
                    GetIconDims(iconW, iconH);

                    float yOffsetExtra = 4.0f;
                    ImVec2 min = ImVec2(x, y + yOffsetExtra);
                    ImVec2 max = ImVec2(x + iconW, y + iconH + yOffsetExtra);

                    DrawList->AddImage((ImTextureID)l.icon, min, max, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
                    rightCurY += iconH + 1.0f;
                } else {
                    DrawTextOutlined(DrawList, font, fontSizeScaled, ImVec2(x, y), l.col, l.text.c_str());
                    rightCurY += lineH;
                }
            }

            float leftCurY = BoxMin.y + textYOffset;
            for (int m = 0; m < (int)leftLines.size(); ++m)
            {
                const auto& l = leftLines[m];
                float y = leftCurY;
                if (l.icon) {
                    float iconW = 0.0f;
                    float iconH = 0.0f;
                    GetIconDims(iconW, iconH);
                    float x = leftTextRightX - iconW;

                    float yOffsetExtra = 4.0f;
                    ImVec2 min = ImVec2(x, y + yOffsetExtra);
                    ImVec2 max = ImVec2(x + iconW, y + iconH + yOffsetExtra);

                    DrawList->AddImage((ImTextureID)l.icon, min, max, ImVec2(0, 0), ImVec2(1, 1), ImColor(255, 255, 255, 255));
                    leftCurY += iconH + 1.0f;
                } else {
                    ImVec2 ts = font->CalcTextSizeA(fontSizeScaled, FLT_MAX, 0.0f, l.text.c_str());
                    float x = leftTextRightX - ts.x;
                    DrawTextOutlined(DrawList, font, fontSizeScaled, ImVec2(x, y), l.col, l.text.c_str());
                    leftCurY += lineH;
                }
            }
        }
    }
}

void Core::Features::cEsp::DrawVehicle()
{
    if (!option->param.vehicle_esp)
        return;

    std::vector<Core::SDK::Game::VehicleStructure> vehicleSnapshot;
    {
        std::lock_guard<std::mutex> lock(Core::SDK::Game::VehicleListMutex);
        vehicleSnapshot = Core::SDK::Game::VehicleList;
    }
    for (auto Entity : vehicleSnapshot)
    {
        CVehicle* Vehicle = Entity.Pointer;
        if (!Vehicle)
            continue;
        auto DrawList = ImGui::GetBackgroundDrawList();

        float Distance = Entity.Dist;
        if (Distance > option->param.vehicle_max_distance)
            continue;

        D3DXVECTOR2 VehicleLocation = Core::SDK::Game::WorldToScreen(Vehicle->GetPos());
        if (!VehicleLocation || !SDK::Game::IsOnScreen(VehicleLocation))
            continue;

        if (option->param.vehicle_snaplines)
        {
            DrawList->AddLine(ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y), ImVec2(VehicleLocation.x, VehicleLocation.y + 30), ImColor(0, 0, 0), 2.5);
            DrawList->AddLine(ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowPos.y + g_Variables.g_vGameWindowSize.y), ImVec2(VehicleLocation.x, VehicleLocation.y + 30), ImColor(option->param.vehicle_snaplines_color[0], option->param.vehicle_snaplines_color[1], option->param.vehicle_snaplines_color[2], option->param.vehicle_snaplines_color[3]), 1.5);
        }

        float genRadius = 3.5f;
        ImColor genColor = ImColor(option->param.vehicle_snaplines_color[0], option->param.vehicle_snaplines_color[1], option->param.vehicle_snaplines_color[2], option->param.vehicle_snaplines_color[3]);
        DrawList->AddCircleFilled(ImVec2(VehicleLocation.x, VehicleLocation.y), genRadius + 1.0f, ImColor(0, 0, 0, 220), 12);
        DrawList->AddCircleFilled(ImVec2(VehicleLocation.x, VehicleLocation.y), genRadius, genColor, 12);

        if (option->param.vehicle_name)
        {
            ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, Entity.Name.c_str());
            DrawTextOutlined(DrawList, ImVec2(VehicleLocation.x - (text_size.x / 2), VehicleLocation.y + 6), ImColor(255, 255, 255), Entity.Name.c_str());
        }

        if (option->param.vehicle_lock || option->param.bring_vehicle || option->param.warp_into_vehicle)
        {
            float radius = 3.5f;
            ImVec2 dotPos = ImVec2(VehicleLocation.x, VehicleLocation.y - 12);

            ImColor dotColor = Entity.IsLocked ? ImColor(250, 72, 62) : ImColor(102, 255, 133);

            DrawList->AddCircleFilled(dotPos, radius + 1.0f, ImColor(0, 0, 0, 220), 12);
            DrawList->AddCircleFilled(dotPos, radius, dotColor, 12);

            auto GetKeyName = [](int vk) -> std::string {
                if (vk == 0) return xorstr("NONE");
                char name[64];
                if (GetKeyNameTextA(MapVirtualKeyA(vk, MAPVK_VK_TO_VSC) << 16, name, sizeof(name)))
                    return std::string(name);
                return std::to_string(vk);
            };

            int FovSize = std::hypot(VehicleLocation.x - g_Variables.g_vGameWindowCenter.x, VehicleLocation.y - g_Variables.g_vGameWindowCenter.y);
            const static int fovRadius = 40;

            if (FovSize < fovRadius )
            {
                std::string lockKeyName = GetKeyName(option->key.vehicle_lock_key);
                std::string bringKeyName = GetKeyName(option->key.bring_vehicle_key);
                std::string warpKeyName = GetKeyName(option->key.warp_into_vehicle_key);

                std::string lockText = (Entity.IsLocked ? xorstr("Unlock [") : xorstr("Lock [")) + lockKeyName + xorstr("]");
                std::string bringText = xorstr("Bring [") + bringKeyName + xorstr("]");
                std::string warpText = xorstr("Warp Into [") + warpKeyName + xorstr("]");

                float currentY = dotPos.y - 10;

                if (option->param.warp_into_vehicle && option->key.warp_into_vehicle_key != 0)
                {
                    ImVec2 warpSize = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, warpText.c_str());
                    currentY -= warpSize.y;
                    DrawTextOutlined(DrawList, ImVec2(dotPos.x - (warpSize.x / 2), currentY), ImColor(255, 255, 255), warpText.c_str());
                    currentY -= 2;
                }

                if (option->param.bring_vehicle && option->key.bring_vehicle_key != 0)
                {
                    ImVec2 bringSize = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, bringText.c_str());
                    currentY -= bringSize.y;
                    DrawTextOutlined(DrawList, ImVec2(dotPos.x - (bringSize.x / 2), currentY), ImColor(255, 255, 255), bringText.c_str());
                    currentY -= 2;
                }

                if (option->param.vehicle_lock && option->key.vehicle_lock_key != 0)
                {
                    ImVec2 lockSize = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, lockText.c_str());
                    currentY -= lockSize.y;
                    DrawTextOutlined(DrawList, ImVec2(dotPos.x - (lockSize.x / 2), currentY), ImColor(255, 255, 255), lockText.c_str());
                }

                if (option->param.vehicle_lock && option->key.vehicle_lock_key != 0) {
                    if (option->key.vehicle_lock_mode == 0) {
                        if (Utils::KeyPressedWithDelay(option->key.vehicle_lock_key, 500))
                            Vehicle->DoorState(Entity.IsLocked);
                    } else {
                        if (GetAsyncKeyState(option->key.vehicle_lock_key) & 0x8000) {
                            static ULONGLONG last_lock = 0;
                            ULONGLONG now = GetTickCount64();
                            if (now - last_lock > 500) {
                                Vehicle->DoorState(Entity.IsLocked);
                                last_lock = now;
                            }
                        }
                    }
                }

                if (option->param.bring_vehicle && option->key.bring_vehicle_key != 0) {
                    if (option->key.bring_vehicle_mode == 0) {
                        if (Utils::KeyPressedWithDelay(option->key.bring_vehicle_key, 1000))
                            Core::Features::Exploits::BringVehicle(Vehicle);
                    } else {
                        if (GetAsyncKeyState(option->key.bring_vehicle_key) & 0x8000) {
                            static ULONGLONG last_bring = 0;
                            ULONGLONG now = GetTickCount64();
                            if (now - last_bring > 1000) {
                                Core::Features::Exploits::BringVehicle(Vehicle);
                                last_bring = now;
                            }
                        }
                    }
                }

                if (option->param.warp_into_vehicle && option->key.warp_into_vehicle_key != 0) {
                    if (option->key.warp_into_vehicle_mode == 0) {
                        if (Utils::KeyPressedWithDelay(option->key.warp_into_vehicle_key, 1000))
                            Core::Features::Exploits::WarpIntoVehicle(Vehicle);
                    } else {
                        if (GetAsyncKeyState(option->key.warp_into_vehicle_key) & 0x8000) {
                            static ULONGLONG last_warp = 0;
                            ULONGLONG now = GetTickCount64();
                            if (now - last_warp > 1000) {
                                Core::Features::Exploits::WarpIntoVehicle(Vehicle);
                                last_warp = now;
                            }
                        }
                    }
                }
            }
        }

        if (option->param.vehicle_distance)
        {
            std::string vehicleDistance = std::to_string(int(Distance)) + xorstr("m");
            ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, vehicleDistance.c_str());
            float yOffset = option->param.vehicle_name ? 20.f : 6.f;
            DrawTextOutlined(DrawList, ImVec2(VehicleLocation.x - (text_size.x / 2), VehicleLocation.y + yOffset), ImColor(255, 255, 255), vehicleDistance.c_str());
        }
    }
}

void Core::Features::cEsp::DrawObjects()
{
    if (!option->param.object_esp)
        return;

    auto DrawList = ImGui::GetBackgroundDrawList();

    {
        std::lock_guard<std::mutex> Guard(Core::SDK::Game::ObjectListMutex);
        for (const auto& Entity : Core::SDK::Game::ObjectList)
        {
            if (Entity.Dist > option->param.object_max_distance)
                continue;

            D3DXVECTOR2 ScreenLocation = Core::SDK::Game::WorldToScreen(Entity.Pos);
            if (!ScreenLocation || !SDK::Game::IsOnScreen(ScreenLocation))
                continue;

            ImColor color = ImColor(option->param.object_name_color[0], option->param.object_name_color[1], option->param.object_name_color[2], option->param.object_name_color[3]);

            float radius = 3.5f;
            DrawList->AddCircleFilled(ImVec2(ScreenLocation.x, ScreenLocation.y), radius + 1.0f, ImColor(0, 0, 0, 220), 12);
            DrawList->AddCircleFilled(ImVec2(ScreenLocation.x, ScreenLocation.y), radius, color, 12);

            if (option->param.object_name)
            {
                if (object_names.find(Entity.ID) == object_names.end())
                    continue;
                const char* objName = object_names[Entity.ID].c_str();
                ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, objName);
                DrawTextOutlined(DrawList, ImVec2(ScreenLocation.x - (text_size.x / 2), ScreenLocation.y + 6), color, objName);
            }

            if (option->param.object_distance)
            {
                std::string distStr = std::to_string(int(Entity.Dist)) + xorstr(" m");
                ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, distStr.c_str());
                float yOffset = option->param.object_name ? 20.f : 6.f;
                DrawTextOutlined(DrawList, ImVec2(ScreenLocation.x - (text_size.x / 2), ScreenLocation.y + yOffset), color, distStr.c_str());
            }
        }
    }

    {
        std::lock_guard<std::mutex> Guard(Core::SDK::Game::PickupListMutex);
        for (const auto& Entity : Core::SDK::Game::PickupList)
        {
            if (Entity.Dist > option->param.object_max_distance)
                continue;

            D3DXVECTOR2 ScreenLocation = Core::SDK::Game::WorldToScreen(Entity.Pos);
            if (!ScreenLocation || !SDK::Game::IsOnScreen(ScreenLocation))
                continue;

            ImColor color = ImColor(option->param.object_name_color[0], option->param.object_name_color[1], option->param.object_name_color[2], option->param.object_name_color[3]);

            float radius = 3.5f;
            DrawList->AddCircleFilled(ImVec2(ScreenLocation.x, ScreenLocation.y), radius + 1.0f, ImColor(0, 0, 0, 220), 12);
            DrawList->AddCircleFilled(ImVec2(ScreenLocation.x, ScreenLocation.y), radius, color, 12);

            if (option->param.object_name)
            {
                ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, Entity.Name.c_str());
                DrawTextOutlined(DrawList, ImVec2(ScreenLocation.x - (text_size.x / 2), ScreenLocation.y + 6), color, Entity.Name.c_str());
            }

            if (option->param.object_distance)
            {
                std::string distStr = std::to_string(int(Entity.Dist)) + xorstr(" m");
                ImVec2 text_size = var->font.instrument_medium[0]->CalcTextSizeA(var->font.instrument_medium[0]->FontSize, FLT_MAX, 0.0f, distStr.c_str());
                float yOffset = option->param.object_name ? 20.f : 6.f;
                DrawTextOutlined(DrawList, ImVec2(ScreenLocation.x - (text_size.x / 2), ScreenLocation.y + yOffset), color, distStr.c_str());
            }
        }
    }
}

void Core::Features::cEsp::DrawRadar()
{
    if (!option->param.radar)
        return;

    CPed* pLocal = Core::SDK::Pointers::pLocalPlayer;
    if (!pLocal)
        return;

    const float radar_width = 300.f;
    const float radar_height = 180.f;
    static ImVec2 s_radar_pos = ImVec2(-1, -1);

    ImGuiIO& io = ImGui::GetIO();
    float display_y = io.DisplaySize.y;
    if (s_radar_pos.x < 0 || s_radar_pos.y < 0)
        s_radar_pos = ImVec2(SCALE(24.f), display_y - SCALE(24.f));

    ImGui::SetNextWindowSize(ImVec2(radar_width, radar_height), ImGuiCond_Always);
    ImGui::SetNextWindowPos(s_radar_pos, ImGuiCond_FirstUseEver, ImVec2(0.f, 1.f));

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.f, 0.f, 0.f, 0.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, SCALE(14.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    if (ImGui::Begin(xorstr("##kdex_radar"), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar))
    {
        ImVec2 win_pos = ImGui::GetWindowPos();
        ImVec2 win_size = ImGui::GetWindowSize();
        s_radar_pos = win_pos;

        ImVec2 radar_center = ImVec2(win_pos.x + win_size.x * 0.5f, win_pos.y + win_size.y * 0.5f);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        const float rounding = SCALE(14.f);
        const ImU32 bg_dark = draw->get_clr(ImVec4(0.00f, 0.00f, 0.00f, 0.45f));
        const ImU32 border_subtle = draw->get_clr(ImVec4(1.f, 1.f, 1.f, 0.08f));

        draw->rect_filled(dl, win_pos, ImVec2(win_pos.x + win_size.x, win_pos.y + win_size.y), bg_dark, rounding);
        dl->AddRect(ImVec2(win_pos.x + 1, win_pos.y + 1), ImVec2(win_pos.x + win_size.x - 1, win_pos.y + win_size.y - 1), border_subtle, rounding, 0, 1.f);

        const float pad = 8.f;
        ImVec2 inner_min = ImVec2(win_pos.x + pad, win_pos.y + pad);
        ImVec2 inner_max = ImVec2(win_pos.x + win_size.x - pad, win_pos.y + win_size.y - pad);
        float half_w = (inner_max.x - inner_min.x) * 0.5f;
        float half_h = (inner_max.y - inner_min.y) * 0.5f;
        float target_range = option->param.esp_max_distance > 10.f ? (option->param.esp_max_distance * 0.2f) : 30.f;
        static float s_range = target_range;
        s_range = ImLerp(s_range, target_range, ImGui::GetIO().DeltaTime * 5.f);
        float scale_x = half_w / s_range;
        float scale_y = half_h / s_range;
        D3DXVECTOR3 local_pos = Mem.Read<D3DXVECTOR3>((uintptr_t)pLocal + 0x90);
        float heading;
        uintptr_t camFollow = Core::SDK::Pointers::pCamGamePlayDirector ? Mem.Read<uintptr_t>(Core::SDK::Pointers::pCamGamePlayDirector + g_Offsets.m_CamFollowOffset) : 0;
        if (camFollow) {
            D3DXVECTOR3 view_dir = Mem.Read<D3DXVECTOR3>(camFollow + g_Offsets.m_CamViewOffset);
            float len_xy = std::sqrt(view_dir.x * view_dir.x + view_dir.y * view_dir.y);
            if (len_xy > 0.001f)
                heading = std::atan2(-view_dir.x, view_dir.y);
            else {
                float ped_h = Mem.Read<float>((uintptr_t)pLocal + 0x70);
                heading = ped_h;
            }
        } else
            heading = Mem.Read<float>((uintptr_t)pLocal + 0x70);
        float cos_h = std::cos(heading);
        float sin_h = std::sin(heading);

        static float s_crosshair_anim = 0.f;
        s_crosshair_anim += ImGui::GetIO().DeltaTime * 1.8f;
        if (s_crosshair_anim > 6.283185f) s_crosshair_anim -= 6.283185f;
        float pulse = 0.88f + 0.12f * std::sin(s_crosshair_anim);
        float ch_len = 7.f * pulse;
        float ch_thick = 2.f;
        ImU32 cross_col = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.82f));
        dl->AddLine(ImVec2(radar_center.x - ch_len, radar_center.y), ImVec2(radar_center.x + ch_len, radar_center.y), cross_col, ch_thick);
        dl->AddLine(ImVec2(radar_center.x, radar_center.y - ch_len), ImVec2(radar_center.x, radar_center.y + ch_len), cross_col, ch_thick);
        float diag = ch_len * 0.65f;
        ImU32 diag_col = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 0.35f));
        dl->AddLine(ImVec2(radar_center.x - diag, radar_center.y - diag), ImVec2(radar_center.x + diag, radar_center.y + diag), diag_col, 1.2f);
        dl->AddLine(ImVec2(radar_center.x + diag, radar_center.y - diag), ImVec2(radar_center.x - diag, radar_center.y + diag), diag_col, 1.2f);

        std::vector<Core::SDK::Game::EntityStruct> entitySnapshot;
        {
            std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
            entitySnapshot = Core::SDK::Game::EntityList;
        }
        for (const auto& Entity : entitySnapshot)
        {
            if (!Entity.Ped || Entity.Distance > option->param.esp_max_distance)
                continue;
            if (option->param.esp_ignore_npcs && !Entity.IsPlayer && Entity.Ped != pLocal)
                continue;
            if (option->param.esp_ignore_dead && Entity.Ped != pLocal)
            {
                if (Entity.IsPlayer) { if (Entity.Health > 400.f || Entity.Health <= 1.0f) continue; }
                else { if (Entity.Health > 200.f || Entity.Health <= 1.f) continue; }
            }

            float dx = Entity.Pos.x - local_pos.x;
            float dy = Entity.Pos.y - local_pos.y;
            float rel_x = dx * cos_h + dy * sin_h;
            float rel_y = -dx * sin_h + dy * cos_h;
            float dist_2d = std::sqrt(rel_x * rel_x + rel_y * rel_y);
            if (dist_2d > s_range - 1.f)
                continue;
            float px = radar_center.x + rel_x * scale_x;
            float py = radar_center.y - rel_y * scale_y;
            if (px < inner_min.x || px > inner_max.x || py < inner_min.y || py > inner_max.y)
                continue;

            ImU32 dot_color;
            if (Entity.Ped == pLocal)
                dot_color = ImGui::GetColorU32(ImVec4(1.f, 1.f, 1.f, 1.f));
            else if (option->param.team_check && Entity.IsFriend)
                dot_color = ImGui::GetColorU32(ImVec4(option->param.team_check_color[0], option->param.team_check_color[1], option->param.team_check_color[2], option->param.team_check_color[3]));
            else if (option->param.visible_check && Entity.Visible)
                dot_color = ImGui::GetColorU32(ImVec4(option->param.visible_check_color[0], option->param.visible_check_color[1], option->param.visible_check_color[2], option->param.visible_check_color[3]));
            else
                dot_color = ImGui::GetColorU32(ImVec4(option->param.box_color[0], option->param.box_color[1], option->param.box_color[2], option->param.box_color[3]));

            float dot_r = (Entity.Ped == pLocal) ? 5.5f : 4.5f;
            dl->AddCircleFilled(ImVec2(px, py), dot_r + 1.0f, ImColor(0, 0, 0, 220));
            dl->AddCircleFilled(ImVec2(px, py), dot_r, dot_color);
        }

    }
    ImGui::End();

    Core::SDK::Game::ClearCachedViewMatrixForFrame();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(2);
}
