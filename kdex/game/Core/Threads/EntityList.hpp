#pragma once
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Offsets.hpp>
#include "UpdateNames.hpp"
#include "../../Globals.hpp"
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <cstdio>

namespace Core
{
    namespace Threads
    {
        struct visibleCache { bool is_visible; bool bone_visible[40]; bool fragment_visible[32]; uint32_t query_id; std::chrono::steady_clock::time_point last_check; };
        static constexpr auto kVisibleCacheMs = std::chrono::milliseconds(10);

        class cEntityList
        {
        private:
            CPedInterFace* CPedInterFace = nullptr;
            CPedList* CPedList = nullptr;
        public:
            std::unordered_map<CPed*, Core::SDK::Game::EntityStruct> CachedEntities;
        public:
            struct HealthCacheEntry {
                CPed* lastPed = nullptr;
                float bestHealth = 0.0f;
                float bestMaxHealth = 0.0f;
                int zeroStreak = 0;
                std::chrono::steady_clock::time_point ts{};
            };

            void Update()
            {
                static std::unordered_map<uintptr_t, visibleCache> visibility_cache;
                static std::unordered_map<uintptr_t, uint32_t> altered_ped_flags;
                static std::unordered_map<int, HealthCacheEntry> player_health_cache;
                auto getVisibilityBase = []() -> uintptr_t {
                    static uintptr_t address = 0;
                    static DWORD last_pid = 0;

                    if (last_pid != Core::g_Variables.ProcIdFiveM) {
                        address = 0;
                        last_pid = Core::g_Variables.ProcIdFiveM;
                    }

                    if (address) return address;

                    address = Core::Mem.PatternScan(Core::Mem.Pattern2Vector(xorstr("48 8D 05 ? ? ? ? 48 C1 E1 ? 48 03 C8 0F 29 4C 24")), 7);

                    return address;
                };
                while (!Core::g_Variables.g_Unload)
                {
                    try {
                    std::this_thread::sleep_for(std::chrono::milliseconds(15));

                    if (!g_MenuInfo.IsLogged && !g_Variables.g_bPassedByThisVerify)
                        continue;

                    if (!Core::g_AttachedToGame)
                        continue;

                    if (!Core::SDK::Pointers::pReplayInterFace)
                        continue;

                    CPedInterFace = Core::SDK::Pointers::pReplayInterFace->InterfacePed();
                    if (!CPedInterFace)
                        continue;
                    CPedList = CPedInterFace->PedList();

                    if (!CPedList)
                        continue;

                    const int PedCount = CPedInterFace->MaxPed();
                    if (PedCount <= 0 || PedCount > 10000)
                        continue;

                    std::unordered_set<CPed*> currentValidPeds;
                    currentValidPeds.reserve(static_cast<size_t>(PedCount));

                    auto now = std::chrono::steady_clock::now();
                    uintptr_t base_offset = getVisibilityBase();

                    CPed* localPedCached = Core::SDK::Pointers::pLocalPlayer;
                    D3DXVECTOR3 localPosCached = localPedCached ? localPedCached->GetPos() : D3DXVECTOR3(0, 0, 0);

                    std::vector<Core::SDK::Game::EntityStruct> updatedList;
                    updatedList.reserve(static_cast<size_t>(PedCount));

                    for (int i = 0; i < PedCount; i++)
                    {
                        CPed* CurrentPed = CPedList->Ped(i);

                        if (!CurrentPed)
                            continue;

                        currentValidPeds.insert(CurrentPed);

                        uintptr_t ped_addr = reinterpret_cast<uintptr_t>(CurrentPed);

                        Core::SDK::Game::EntityStruct Entity;
                        {
                            auto it = CachedEntities.find(CurrentPed);
                            if (it != CachedEntities.end())
                                Entity = it->second;
                        }

                        Entity.Ped = CurrentPed;
                        Entity.Id = CurrentPed->GetID();
                        Entity.Index = i;
                        Entity.Pos = CurrentPed->GetPos();

                        {
                            std::lock_guard<std::mutex> flock(Core::SDK::Game::FriendMapMutex);
                            auto itf = Core::SDK::Game::FriendMap.find(CurrentPed);
                            Entity.IsFriend = (itf != Core::SDK::Game::FriendMap.end()) ? itf->second : false;
                        }

                        Entity.MaxHealth = CurrentPed->GetMaxHealth();
                        Entity.Health = CurrentPed->GetHealth();
                        Entity.Armor = CurrentPed->GetArmor();
                        Entity.PedType = CurrentPed->GetPedType();

                        uintptr_t net_obj = Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(CurrentPed) + 0xD0);
                        uint16_t net_id = net_obj ? Core::Mem.Read<uint16_t>(net_obj + 0xA) : 0;
                        uintptr_t player_info = Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(CurrentPed) + Core::g_Offsets.m_PlayerInfo);
                        Entity.IsPlayer = (net_id != 0 && player_info != 0);
                        Entity.IsGodMode = false;
                        if (Entity.IsPlayer) {
                            DWORD pedFlags = Core::Mem.Read<DWORD>(reinterpret_cast<uintptr_t>(CurrentPed) + 0x188);
                            Entity.IsGodMode = (pedFlags & (1 << 9)) != 0;
                        }
                        if (Entity.IsPlayer && player_info) {
                            CPlayerInfo* pPlayerInfo = reinterpret_cast<CPlayerInfo*>(player_info);
                            Entity.ScriptPedIndex = pPlayerInfo->PlayerID();
                        } else {
                            Entity.ScriptPedIndex = 0;
                        }

                        if (Entity.IsPlayer && Entity.Id > 0) {
                            auto& hc = player_health_cache[Entity.Id];
                            bool samePedStub = (hc.lastPed == CurrentPed);
                            float readHealth = Entity.Health;
                            float readMax = Entity.MaxHealth;

                            if (readHealth > 0.0f) {
                                hc.bestHealth = readHealth;
                                hc.zeroStreak = 0;
                            } else {
                                hc.zeroStreak = samePedStub ? (hc.zeroStreak + 1) : 1;
                                const int kConfirmDeathStreak = 10;
                                if (hc.zeroStreak >= kConfirmDeathStreak) {
                                    hc.bestHealth = 0.0f;
                                } else if (hc.bestHealth > 0.0f) {
                                    Entity.Health = hc.bestHealth;
                                }
                            }

                            if (readMax > 0.0f) {
                                hc.bestMaxHealth = readMax;
                            } else if (hc.bestMaxHealth > 0.0f) {
                                Entity.MaxHealth = hc.bestMaxHealth;
                            }

                            hc.lastPed = CurrentPed;
                            hc.ts = now;
                        }

                        Entity.Distance = CurrentPed->GetDistance(localPosCached, Entity.Pos);
                        static const std::string kUnknown = xorstr("Unknown");
                        auto* weaponManager = CurrentPed->GetWeaponManager();
                        if (weaponManager) {
                            auto* weaponInfo = weaponManager->GetWeaponInfo();
                            Entity.WeaponName = weaponInfo ? weaponInfo->GetName() : kUnknown;
                        } else {
                            Entity.WeaponName = kUnknown;
                        }

                        bool is_visible = false;
                        bool from_cache = false;
                        {
                            auto itc = visibility_cache.find(ped_addr);
                            if (itc != visibility_cache.end()) {
                                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - itc->second.last_check);
                                if (elapsed < kVisibleCacheMs) {
                                    is_visible = itc->second.is_visible;
                                    for (int b = 0; b < 32; ++b) Entity.FragmentVisible[b] = itc->second.fragment_visible[b];
                                    for (int b = 0; b < 40; ++b) Entity.BoneVisible[b] = itc->second.bone_visible[b];
                                    from_cache = true;
                                }
                            }
                        }
                        if (!from_cache) {
                            auto it_altered = altered_ped_flags.find(ped_addr);
                            if (it_altered == altered_ped_flags.end()) {
                                uint32_t original_flags = Core::Mem.Read<uint32_t>(ped_addr + 0xC0);
                                if (!(original_flags & (1u << 29))) {
                                    Core::Mem.Write<uint32_t>(ped_addr + 0xC0, original_flags | (1u << 29));
                                    altered_ped_flags[ped_addr] = original_flags;
                                }
                            }
                            uint64_t fx_draw_data = Core::Mem.Read<uint64_t>(reinterpret_cast<uintptr_t>(CurrentPed) + 0x48);
                            if (fx_draw_data && base_offset) {
                                uint32_t query_id = static_cast<uint32_t>(Core::Mem.Read<uint8_t>(fx_draw_data + 0x2B)) & 0xFFu;
                                if (query_id > 0 && query_id <= 1000) {
                                    int visibility_value = Core::Mem.Read<int>(base_offset + ((query_id - 1) * 0x80) + 0x78);
                                    is_visible = visibility_value > 100;

                                    uint16_t node_count = Core::Mem.Read<uint16_t>(fx_draw_data + 0x20);
                                    uint64_t nodes_ptr = Core::Mem.Read<uint64_t>(fx_draw_data + 0x18);

                                    for (int b = 0; b < 32; ++b) {
                                        Entity.FragmentVisible[b] = is_visible;
                                    }

                                    if (node_count > 1 && nodes_ptr && node_count < 100) {
                                        for (int n = 0; n < node_count && n < 32; ++n) {
                                            uint64_t node = Core::Mem.Read<uint64_t>(nodes_ptr + (n * 0x8));
                                            if (node) {
                                                uint32_t node_qid = static_cast<uint32_t>(Core::Mem.Read<uint8_t>(node + 0x2B)) & 0xFFu;
                                                if (node_qid > 0 && node_qid <= 1000) {
                                                    int node_vis = Core::Mem.Read<int>(base_offset + ((node_qid - 1) * 0x80) + 0x78);
                                                    Entity.FragmentVisible[n] = (node_vis > 100);
                                                }
                                            }
                                        }
                                    }

                                    uint64_t skeleton_ptr = Core::Mem.Read<uint64_t>(reinterpret_cast<uintptr_t>(CurrentPed) + 0x20);
                                    uint64_t frag_mapping_ptr = 0;
                                    if (skeleton_ptr) {
                                        frag_mapping_ptr = Core::Mem.Read<uint64_t>(skeleton_ptr + 0x48);
                                    }

                                    for (int b = 0; b < 40; ++b) {
                                        int fragIdx = -1;
                                        if (frag_mapping_ptr) {
                                            fragIdx = Core::Mem.Read<uint8_t>(frag_mapping_ptr + b);
                                        }

                                        if (fragIdx < 0 || fragIdx >= node_count) {
                                            if (b == 0 || b == 15) fragIdx = 1;
                                            else if (b == 1 || b == 6 || b == 8 || b == 18) fragIdx = 4;
                                            else if (b == 2 || b == 7 || b == 9 || b == 19) fragIdx = 5;
                                            else if (b == 3 || b == 10 || b == 12 || b == 16 || (b >= 20 && b <= 29)) fragIdx = 2;
                                            else if (b == 4 || b == 11 || b == 13 || b == 17 || (b >= 30 && b <= 39)) fragIdx = 3;
                                            else fragIdx = 0;
                                        }

                                        if (fragIdx >= 0 && fragIdx < node_count) {
                                            Entity.BoneVisible[b] = Entity.FragmentVisible[fragIdx];
                                        } else {
                                            Entity.BoneVisible[b] = is_visible;
                                        }
                                    }

                                    auto& vc = visibility_cache[ped_addr];
                                    vc.is_visible = is_visible;
                                    vc.query_id = query_id;
                                    vc.last_check = now;
                                    for (int b = 0; b < 32; ++b) vc.fragment_visible[b] = Entity.FragmentVisible[b];
                                    for (int b = 0; b < 40; ++b) vc.bone_visible[b] = Entity.BoneVisible[b];
                                } else {
                                    BYTE vis_flag = Core::Mem.Read<BYTE>(ped_addr + 0x145C);
                                    is_visible = (vis_flag != 36 && vis_flag != 0 && vis_flag != 4);
                                    for (int b = 0; b < 32; ++b) Entity.FragmentVisible[b] = is_visible;
                                }
                            } else {
                                BYTE vis_flag = Core::Mem.Read<BYTE>(ped_addr + 0x145C);
                                is_visible = (vis_flag != 36 && vis_flag != 0 && vis_flag != 4);
                                for (int b = 0; b < 32; ++b) Entity.FragmentVisible[b] = is_visible;
                            }
                        }
                        Entity.Visible = is_visible;

                        static const std::string kNPC = xorstr("NPC");
                        std::string previousUserName = Entity.NetworkInfo.UserName;
                        Entity.NetworkInfo.UserName = (!Entity.IsPlayer) ? kNPC : "";

                        if (Entity.IsPlayer) {
                            try {
                                Core::SDK::Game::NetworkInfo net{};
                                bool haveNet = Core::Threads::g_UpdateNames.TryGetNetworkInfo(Entity.Id, net);

                                if (haveNet) {
                                    Entity.NetworkInfo.SteamId  = net.SteamId;
                                    Entity.NetworkInfo.DiscordId = net.DiscordId;

                                    if (!net.UserName.empty()) {
                                        Entity.NetworkInfo.UserName = Utils::StringToFirstUpperCase(net.UserName);
                                    } else if (!previousUserName.empty() && previousUserName != xorstr("NPC")) {
                                        Entity.NetworkInfo.UserName = previousUserName;
                                    }
                                } else if (!previousUserName.empty() && previousUserName != xorstr("NPC")) {
                                    Entity.NetworkInfo.UserName = previousUserName;
                                }

                                if (Entity.NetworkInfo.UserName.empty() || Entity.NetworkInfo.UserName == xorstr("NPC") || Entity.NetworkInfo.UserName == xorstr("Loading...")) {
                                    std::string fallbackName = CurrentPed->GetPedName(CurrentPed);
                                    if (!fallbackName.empty() && fallbackName.find(xorstr("ID:")) == std::string::npos) {
                                        Entity.NetworkInfo.UserName = Utils::StringToFirstUpperCase(fallbackName);
                                    } else if (!fallbackName.empty()) {
                                        Entity.NetworkInfo.UserName = fallbackName;
                                    }
                                }
                            }
                            catch (...) {
                                if (!previousUserName.empty() && previousUserName != xorstr("NPC"))
                                    Entity.NetworkInfo.UserName = previousUserName;
                            }
                        }

                        CachedEntities[CurrentPed] = Entity;
                        updatedList.push_back(Entity);
                    }

                    {
                        std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
                        Core::SDK::Game::EntityList = std::move(updatedList);
                    }

                    {
                        static auto lastPrint = std::chrono::steady_clock::now();
                        static size_t lastPlayerCount = 0;
                        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastPrint).count();
                        if (elapsed >= 3) {
                            size_t curPlayers = 0;
                            for (const auto& e : updatedList) if (e.IsPlayer) ++curPlayers;
                            if (elapsed >= 15 || curPlayers != lastPlayerCount) {
                                lastPrint = now;
                                lastPlayerCount = curPlayers;
                                if (curPlayers > 0) {
                                    printf(xorstr("\n[PlayerList] %zu players detected:\n"), curPlayers);
                                    for (const auto& e : updatedList) {
                                        if (!e.IsPlayer) continue;
                                        printf(xorstr("  %-20s | Ped: 0x%llX | Id: %d | Dist: %.1fm | HP: %.0f/%.0f | GodMode: %s | Discord: %s | Steam: %s\n"),
                                            e.NetworkInfo.UserName.c_str(),
                                            (unsigned long long)(uintptr_t)e.Ped,
                                            e.Id,
                                            e.Distance,
                                            e.Health, e.MaxHealth,
                                            e.IsGodMode ? xorstr("YES") : xorstr("NO"),
                                            e.NetworkInfo.DiscordId.empty() ? xorstr("n/a") : e.NetworkInfo.DiscordId.c_str(),
                                            e.NetworkInfo.SteamId.empty() ? xorstr("n/a") : e.NetworkInfo.SteamId.c_str());
                                    }
                                }
                            }
                        }
                    }

                    for (auto it = CachedEntities.begin(); it != CachedEntities.end();) {
                        if (currentValidPeds.find(it->first) == currentValidPeds.end())
                            it = CachedEntities.erase(it);
                        else
                            ++it;
                    }

                    for (auto it = altered_ped_flags.begin(); it != altered_ped_flags.end();) {
                        CPed* pedPtr = reinterpret_cast<CPed*>(it->first);
                        if (currentValidPeds.find(pedPtr) == currentValidPeds.end()) {
                            Core::Mem.Write<uint32_t>(it->first + 0xC0, it->second);
                            it = altered_ped_flags.erase(it);
                        }
                        else {
                            ++it;
                        }
                    }

                    {
                        static auto last_cleanup = std::chrono::steady_clock::now();
                        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_cleanup).count() > 5) {
                            for (auto it = visibility_cache.begin(); it != visibility_cache.end();) {
                                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - it->second.last_check);
                                if (elapsed > kVisibleCacheMs * 10)
                                    it = visibility_cache.erase(it);
                                else
                                    ++it;
                            }
                            last_cleanup = now;
                        }
                    }
                    } catch (...) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
            }
        };

        inline cEntityList g_EntityList;
    }
}
