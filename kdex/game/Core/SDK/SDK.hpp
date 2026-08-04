#pragma once
#include <Includes/Includes.hpp>
#include <Core/SDK/Memory.hpp>
#include <Core/Offsets.hpp>
#include <Core/Variables.hpp>
#include <Core/SDK/Structs/GameClasses.hpp>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <shared_mutex>
#include <chrono>

#include <d3d11.h>
#include <D3dx9math.h>
#pragma comment( lib, "d3d11.lib" )

namespace Core {

	namespace SDK {

		namespace Pointers {
			inline CPedFactory* pWorld = nullptr;
			inline CPed * pLocalPlayer = nullptr;
			inline CReplayInterFace* pReplayInterFace = nullptr;
			inline uintptr_t pBlipList = 0, pViewPort = 0, pCamGamePlayDirector = 0;
		}

		namespace Game
		{
			struct cSkeleton_t {

				std::unordered_map<unsigned int, int> StoredBonesIdx;

				struct crSkeletonData_t {
					uintptr_t Ptr, m_BoneIdTable;
					unsigned int m_Used;
					unsigned int m_NumBones;
					unsigned short m_BoneIdTable_Slots;
				} crSkeletonData;

				uintptr_t m_pSkeleton, Arg2;
				D3DXMATRIX Arg1;
			};

			struct NetworkInfo {
				std::string UserName;
				std::string DiscordId;
				std::string SteamId;
				int Ping = 0;
				bool IsAdmin = false;
			};

			struct EntityStruct {
				CPed* Ped;

				int Id;
				int Index;
				int PedType;
				int ScriptPedIndex;

				bool IsFriend;
				bool Visible;
				bool BoneVisible[40];
				bool FragmentVisible[32];
				bool IsPlayer;
				bool IsGodMode;

				D3DXVECTOR3 Pos;

				float Health;
				float MaxHealth;
				float Armor;
				float Distance;

				std::string WeaponName;

				NetworkInfo NetworkInfo;

				float HealthAnim;
			};

			struct EspAnim {
				bool CanFadeOut;
				float Health;
				float Armor;
				float Alpha;
			};

			struct VehicleStructure {
				CVehicle* Pointer;
				CPed* Driver;
				std::string Name;
				float Dist;
				bool IsLocked;
				D3DXVECTOR3 Pos;
			};

			struct ObjectStructure {
				uintptr_t Pointer;
				std::string Name;
				uint32_t ID;
				float Dist;
				D3DXVECTOR3 Pos;
			};

			struct PickupStructure {
				uintptr_t Pointer;
				std::string Name;
				float Dist;
				D3DXVECTOR3 Pos;
			};

			inline std::vector<EntityStruct> EntityList;
			inline std::vector<VehicleStructure> VehicleList;
			inline std::vector<ObjectStructure> ObjectList;
			inline std::vector<PickupStructure> PickupList;
			inline std::mutex EntityListMutex;
			inline std::mutex VehicleListMutex;
			inline std::mutex ObjectListMutex;
			inline std::mutex PickupListMutex;
			inline std::unordered_set<std::string> FriendSet;
			inline std::mutex FriendMapMutex;

			inline bool IsFriendName(const std::string& userName) {
				if (userName.empty()) return false;
				std::lock_guard<std::mutex> lock(FriendMapMutex);
				return FriendSet.find(userName) != FriendSet.end();
			}
			inline void SetFriendByName(const std::string& userName, bool isFriend) {
				if (userName.empty()) return;
				std::lock_guard<std::mutex> lock(FriendMapMutex);
				if (isFriend) FriendSet.insert(userName);
				else FriendSet.erase(userName);
			}
			inline CPed* TeleportBehindMarkerTarget = nullptr;
			inline CVehicle* PhysGunTarget = nullptr;

			inline static D3DXMATRIX s_cachedViewMatrix = {};
			inline static bool s_useCachedViewMatrix = false;

			inline void SetCachedViewMatrixForFrame(const D3DXMATRIX& viewMatrixTransposed) {
				s_cachedViewMatrix = viewMatrixTransposed;
				s_useCachedViewMatrix = true;
			}
			inline void ClearCachedViewMatrixForFrame() {
				s_useCachedViewMatrix = false;
			}

			inline D3DXVECTOR2 WorldToScreen(D3DXVECTOR3 World)
			{
				D3DXMATRIX ViewMatrix;
				if (s_useCachedViewMatrix) {
					ViewMatrix = s_cachedViewMatrix;
				} else {
					if (!Core::SDK::Pointers::pViewPort) return D3DXVECTOR2(0, 0);
					ViewMatrix = Mem.Read<D3DXMATRIX>(Core::SDK::Pointers::pViewPort + 0x24C);
					D3DXMatrixTranspose(&ViewMatrix, &ViewMatrix);
				}

				auto VecX = D3DXVECTOR4(ViewMatrix._21, ViewMatrix._22, ViewMatrix._23, ViewMatrix._24),
					VecY = D3DXVECTOR4(ViewMatrix._31, ViewMatrix._32, ViewMatrix._33, ViewMatrix._34),
					VecZ = D3DXVECTOR4(ViewMatrix._41, ViewMatrix._42, ViewMatrix._43, ViewMatrix._44);

				D3DXVECTOR3 ScreenPos = D3DXVECTOR3(
					(VecX.x * World.x) + (VecX.y * World.y) + (VecX.z * World.z) + VecX.w,
					(VecY.x * World.x) + (VecY.y * World.y) + (VecY.z * World.z) + VecY.w,
					(VecZ.x * World.x) + (VecZ.y * World.y) + (VecZ.z * World.z) + VecZ.w
				);

				if (ScreenPos.z <= 0.1f)
					return D3DXVECTOR2(-1000, -1000);

				ScreenPos.z = 1.0f / ScreenPos.z;
				ScreenPos.x *= ScreenPos.z;
				ScreenPos.y *= ScreenPos.z;

				ScreenPos.x = (g_Variables.g_vGameWindowSize.x / 2) + float(0.5f * ScreenPos.x * g_Variables.g_vGameWindowSize.x + 0.5f);
				ScreenPos.y = (g_Variables.g_vGameWindowSize.y / 2) - float(0.5f * ScreenPos.y * g_Variables.g_vGameWindowSize.y + 0.5f);
				return D3DXVECTOR2(ScreenPos.x, ScreenPos.y);
			}

			inline std::unordered_map<uintptr_t, std::unordered_map<unsigned int, int>> StoredBonesIdx;
			inline std::shared_mutex BonesCache;

			inline bool GetBoneIndex(cSkeleton_t cSkeleton, unsigned int boneId, int& outBoneIdx)
			{
				{
					std::shared_lock<std::shared_mutex> lock(BonesCache);
					auto it = StoredBonesIdx.find(cSkeleton.crSkeletonData.Ptr);
					if (it != StoredBonesIdx.end()) {
						auto& cache = it->second;
						auto cit = cache.find(boneId);
						if (cit != cache.end()) {
							outBoneIdx = cit->second;
							return true;
						}
					}
				}
				{
					std::unique_lock<std::shared_mutex> lock(BonesCache);
					auto& StoredBonesIdxCache = StoredBonesIdx[cSkeleton.crSkeletonData.Ptr];
					{
						auto cit2 = StoredBonesIdxCache.find(boneId);
						if (cit2 != StoredBonesIdxCache.end()) {
							outBoneIdx = cit2->second;
							return true;
						}
					}
					if (cSkeleton.crSkeletonData.m_Used != 0 && cSkeleton.crSkeletonData.m_BoneIdTable_Slots != 0)
					{
						unsigned short m_BoneIdTable_Slots = cSkeleton.crSkeletonData.m_BoneIdTable_Slots;
						std::uintptr_t m_BoneIdTable_Hash = Mem.Read<std::uintptr_t>(cSkeleton.crSkeletonData.m_BoneIdTable + 0x8 * (boneId % (unsigned int)m_BoneIdTable_Slots));
						for (std::uintptr_t i = m_BoneIdTable_Hash; i != 0; i = Mem.Read<std::uintptr_t>(i + 0x8))
						{
							int i_key = Mem.Read<int>(i);
							if (boneId == i_key)
							{
								int p_Data = Mem.Read<int>(i + 0x4);
								if (p_Data)
								{
									outBoneIdx = p_Data;
									StoredBonesIdxCache[boneId] = p_Data;
									return true;
								}
							}
						}
					}
					else if (boneId < cSkeleton.crSkeletonData.m_NumBones)
					{
						outBoneIdx = boneId;
						StoredBonesIdxCache[boneId] = boneId;
						return true;
					}
					return false;
				}
			}

			inline D3DXVECTOR3 GetBonePosInstFrag(std::uintptr_t InstFrag, unsigned int BoneID, D3DXMATRIX Arg1, uintptr_t Arg2)
			{
				D3DXMATRIX v4;
				D3DXMATRIX Result;

				v4 = Arg1;
				if (!v4) { return D3DXVECTOR3(0, 0, 0); }

				Result = Mem.Read<D3DXMATRIX>(Arg2 + ((unsigned __int64)BoneID << 6));
				if (!Result) { return D3DXVECTOR3(0, 0, 0); }

				D3DXVECTOR3 vec1(v4._11, v4._12, v4._13);
				D3DXVECTOR3 vec2(v4._21, v4._22, v4._23);
				D3DXVECTOR3 vec3(v4._31, v4._32, v4._33);
				D3DXVECTOR3 vec4(v4._41, v4._42, v4._43);
				D3DXVECTOR3 vec5(Result._41, Result._42, Result._43);
				return D3DXVECTOR3(
					vec1.x * vec5.x + vec4.x + vec2.x * vec5.y + vec3.x * vec5.z,
					vec1.y * vec5.x + vec4.y + vec2.y * vec5.y + vec3.y * vec5.z,
					vec1.z * vec5.x + vec4.z + vec2.z * vec5.y + vec3.z * vec5.z
				);
			}

			inline void GetBonePositionsBatch(CPed* Ped, cSkeleton_t& skel, const unsigned int* boneIds, size_t count, D3DXVECTOR3* outPositions)
			{
				constexpr size_t kMaxBones = 64;
				int indices[kMaxBones];
				for (size_t i = 0; i < count && i < kMaxBones; ++i) indices[i] = -1;
				{
					std::unique_lock<std::shared_mutex> lock(BonesCache);
					auto& cache = StoredBonesIdx[skel.crSkeletonData.Ptr];
					for (size_t i = 0; i < count && i < kMaxBones; ++i) {
						unsigned int boneId = boneIds[i];
						auto it = cache.find(boneId);
						if (it != cache.end()) {
							indices[i] = it->second;
						} else if (skel.crSkeletonData.m_Used != 0 && skel.crSkeletonData.m_BoneIdTable_Slots != 0) {
							unsigned short slots = skel.crSkeletonData.m_BoneIdTable_Slots;
							std::uintptr_t hash = Mem.Read<std::uintptr_t>(skel.crSkeletonData.m_BoneIdTable + 0x8 * (boneId % (unsigned int)slots));
							for (std::uintptr_t j = hash; j != 0; j = Mem.Read<std::uintptr_t>(j + 0x8)) {
								if (Mem.Read<int>(j) == (int)boneId) {
									int p_Data = Mem.Read<int>(j + 0x4);
									if (p_Data) { cache[boneId] = p_Data; indices[i] = p_Data; break; }
								}
							}
						} else if (boneId < skel.crSkeletonData.m_NumBones) {
							cache[boneId] = (int)boneId;
							indices[i] = (int)boneId;
						}
					}
				}
				D3DXVECTOR3 fallback = Ped->GetPos();
				for (size_t i = 0; i < count && i < kMaxBones; ++i) {
					if (indices[i] >= 0)
						outPositions[i] = GetBonePosInstFrag(skel.m_pSkeleton, (unsigned int)indices[i], skel.Arg1, skel.Arg2);
					else
						outPositions[i] = fallback;
				}
			}

			inline D3DXVECTOR3 GetBonePosComplex(CPed* Ped, unsigned int Mask, cSkeleton_t cSkeleton_t)
			{
				bool result = false;

				int BoneId = 0;

				if (GetBoneIndex(cSkeleton_t, Mask, BoneId)) {
					return GetBonePosInstFrag(cSkeleton_t.m_pSkeleton, BoneId, cSkeleton_t.Arg1, cSkeleton_t.Arg2);
				}

				return Ped->GetPos();
			}

			inline std::string GetPedName(int id, CPed* Ped)
			{
				for (int i = 0; i < 256; i++)
				{
					uintptr_t NetPointer = Mem.Read<uintptr_t>(g_Offsets.m_NetIdToNamesEntry + (i * 0x8));

					if (!NetPointer) continue;

					int Id = Mem.Read<int>(NetPointer + 0x10);

					if (Id == id || (short)Id == (short)id) {
						std::string name = Mem.ReadString(NetPointer + 0x18);
						if (!name.empty())
							return name;
					}
				}

				return "";
			}

            inline bool IsPedDoingCombatRoll(CPed* Ped)
            {
                if (!Ped) return false;
                uintptr_t FragInst = Mem.Read<uintptr_t>((uintptr_t)Ped + Core::g_Offsets.m_FragInst);
                if (!FragInst) return false;
                uintptr_t v9 = Mem.Read<uintptr_t>(FragInst + 0x68);
                if (!v9) return false;

                cSkeleton_t Skel;
                Skel.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
                Skel.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skel.m_pSkeleton);
                Skel.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skel.crSkeletonData.Ptr + 0x1A);
                Skel.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skel.crSkeletonData.Ptr + 0x5E);
                Skel.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skel.crSkeletonData.Ptr + 0x18);
                Skel.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skel.crSkeletonData.Ptr + 0x10);
                Skel.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skel.m_pSkeleton + 0x8));
                Skel.Arg2 = Mem.Read<uintptr_t>(Skel.m_pSkeleton + 0x18);

                D3DXVECTOR3 head = GetBonePosComplex(Ped, SKEL_Head, Skel);
                D3DXVECTOR3 pelvis = GetBonePosComplex(Ped, SKEL_Pelvis, Skel);
                D3DXVECTOR3 lfoot = GetBonePosComplex(Ped, SKEL_L_Foot, Skel);
                D3DXVECTOR3 rfoot = GetBonePosComplex(Ped, SKEL_R_Foot, Skel);

                float feet_z = (std::min)(lfoot.z, rfoot.z);

                bool head_near_feet = std::abs(head.z - feet_z) < 0.55f;
                bool head_below_pelvis = (head.z <= pelvis.z + 0.22f);
                return head_near_feet && head_below_pelvis;
            }

            inline bool IsPedJumping(CPed* Ped)
            {
                if (!Ped) return false;
                uintptr_t FragInst = Mem.Read<uintptr_t>((uintptr_t)Ped + Core::g_Offsets.m_FragInst);
                if (!FragInst) return false;
                uintptr_t v9 = Mem.Read<uintptr_t>(FragInst + 0x68);
                if (!v9) return false;

                cSkeleton_t Skel;
                Skel.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
                Skel.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skel.m_pSkeleton);
                Skel.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skel.crSkeletonData.Ptr + 0x1A);
                Skel.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skel.crSkeletonData.Ptr + 0x5E);
                Skel.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skel.crSkeletonData.Ptr + 0x18);
                Skel.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skel.crSkeletonData.Ptr + 0x10);
                Skel.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skel.m_pSkeleton + 0x8));
                Skel.Arg2 = Mem.Read<uintptr_t>(Skel.m_pSkeleton + 0x18);

                D3DXVECTOR3 head = GetBonePosComplex(Ped, SKEL_Head, Skel);
                D3DXVECTOR3 pelvis = GetBonePosComplex(Ped, SKEL_Pelvis, Skel);
                D3DXVECTOR3 lfoot = GetBonePosComplex(Ped, SKEL_L_Foot, Skel);
                D3DXVECTOR3 rfoot = GetBonePosComplex(Ped, SKEL_R_Foot, Skel);

                float feet_avg = 0.5f * (lfoot.z + rfoot.z);
                float feet_min = (std::min)(lfoot.z, rfoot.z);

                static std::mutex s_jump_mutex;
                static std::unordered_map<CPed*, std::pair<float, std::pair<float, std::chrono::steady_clock::time_point>>> s_last;
                static std::unordered_map<CPed*, std::pair<bool, std::pair<std::chrono::steady_clock::time_point, std::chrono::steady_clock::time_point>>> s_jump;
                static std::unordered_map<CPed*, std::pair<float, std::chrono::steady_clock::time_point>> s_feet_base;

                std::lock_guard<std::mutex> lock(s_jump_mutex);
                auto now = std::chrono::steady_clock::now();
                float vel_p = 0.0f, vel_f = 0.0f;
                float prev_pelvis = pelvis.z;
                float prev_feet = feet_avg;
                auto it = s_last.find(Ped);
                if (it != s_last.end()) {
                    prev_pelvis = it->second.first;
                    prev_feet = it->second.second.first;
                    float dt = std::chrono::duration<float>(now - it->second.second.second).count();
                    if (dt > 1e-4f) {
                        vel_p = (pelvis.z - prev_pelvis) / dt;
                        vel_f = (feet_avg - prev_feet) / dt;
                    }
                }

                bool upright = (head.z - pelvis.z) > 0.25f;
                bool not_rolling = !(std::abs(head.z - (std::min)(lfoot.z, rfoot.z)) < 0.55f && head.z <= pelvis.z + 0.22f);
                bool rising_fast = (vel_p > 0.6f) || (vel_f > 0.5f);

                auto it2 = s_jump.find(Ped);
                if (it2 == s_jump.end()) {
                    s_jump[Ped] = { false, { now, now } };
                    it2 = s_jump.find(Ped);
                }
                bool in = it2->second.first;
                auto start = it2->second.second.first;
                auto last_ok = it2->second.second.second;

                bool base = upright && not_rolling;
                static const float kLiftThreshold = 0.10f;
                static const float kGroundStickEps = 0.06f;
                auto itb = s_feet_base.find(Ped);
                if (itb == s_feet_base.end()) itb = s_feet_base.emplace(Ped, std::make_pair(feet_min, now)).first;
                if (vel_p > 0.2f) {
                    if (std::chrono::duration_cast<std::chrono::milliseconds>(now - itb->second.second).count() > 250) {
                        itb->second.second = now;
                        itb->second.first = feet_min;
                    }
                } else {
                    itb->second.second = now;
                    itb->second.first = feet_min;
                }
                if (base && rising_fast) {
                    bool feet_lifted = (feet_min - itb->second.first) > kLiftThreshold;
                    bool still_on_ground = std::abs(feet_min - itb->second.first) < kGroundStickEps;
                    if (feet_lifted && !still_on_ground) {
                        in = true;
                        start = now;
                        last_ok = now;
                    }
                    else if ((pelvis.z - prev_pelvis) > 0.08f && vel_p > 0.4f && (feet_avg - prev_feet) > 0.05f) {
                        in = true;
                        start = now;
                        last_ok = now;
                    }
                } else if (in) {
                    bool within = (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() < 800);
                    bool slight_descent = (vel_p > -0.5f) || (vel_f > -0.45f);
                    if (base && within && slight_descent) {
                        last_ok = now;
                    } else {
                        bool stale = (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_ok).count() > 250) || !within || !base;
                        if (stale) in = false;
                    }
                }

                it2->second = { in, { start, last_ok } };
                s_last[Ped] = { pelvis.z, { feet_avg, now } };
                return base && in;
            }

			inline CPed* GetClosestPed(int MaxDistance, bool IgnoreNpc, bool VisibleCheck)
			{
				CPed* ClosestPed = nullptr;
				float ClosestDistSq = FLT_MAX;

				D3DXMATRIX ViewMatrix;
				if (s_useCachedViewMatrix) {
					ViewMatrix = s_cachedViewMatrix;
				} else {
					if (!Core::SDK::Pointers::pViewPort) return nullptr;
					ViewMatrix = Mem.Read<D3DXMATRIX>(Core::SDK::Pointers::pViewPort + 0x24C);
					D3DXMatrixTranspose(&ViewMatrix, &ViewMatrix);
				}
				float vxx = ViewMatrix._21, vxy = ViewMatrix._22, vxz = ViewMatrix._23, vxw = ViewMatrix._24;
				float vyx = ViewMatrix._31, vyy = ViewMatrix._32, vyz = ViewMatrix._33, vyw = ViewMatrix._34;
				float vzx = ViewMatrix._41, vzy = ViewMatrix._42, vzz = ViewMatrix._43, vzw = ViewMatrix._44;
				float winW = g_Variables.g_vGameWindowSize.x;
				float winH = g_Variables.g_vGameWindowSize.y;
				float cx = g_Variables.g_vGameWindowCenter.x;
				float cy = g_Variables.g_vGameWindowCenter.y;

				std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
				for (auto& Entity : Core::SDK::Game::EntityList)
				{
					CPed* Ped = Entity.Ped;

					if (Ped == Core::SDK::Pointers::pLocalPlayer)
						continue;

					if (std::abs(Entity.Health) <= 1.0f)
						continue;

					if (IgnoreNpc && !Entity.IsPlayer)
						continue;

					if (Entity.IsFriend)
						continue;

					if (VisibleCheck && !Entity.Visible)
						continue;

					if (Entity.Distance > MaxDistance)
						continue;

					float wx = Entity.Pos.x, wy = Entity.Pos.y, wz = Entity.Pos.z;
					float sx = vxx * wx + vxy * wy + vxz * wz + vxw;
					float sy = vyx * wx + vyy * wy + vyz * wz + vyw;
					float sz = vzx * wx + vzy * wy + vzz * wz + vzw;

					float screenX, screenY;
					if (sz <= 0.1f) {
						screenX = -1000.0f;
						screenY = -1000.0f;
					} else {
						float inv = 1.0f / sz;
						screenX = (winW / 2) + float(0.5f * (sx * inv) * winW + 0.5f);
						screenY = (winH / 2) - float(0.5f * (sy * inv) * winH + 0.5f);
					}

					float dx = screenX - cx;
					float dy = screenY - cy;
					float AimDistSq = dx * dx + dy * dy;

					if (AimDistSq < ClosestDistSq)
					{
						ClosestDistSq = AimDistSq;
						ClosestPed = Ped;
					}
				}

				return ClosestPed;
			}

			inline CPed* GetClosestPedEx(int MaxDistance, bool IgnoreNpc, bool VisibleCheck, bool IgnoreFriends)
			{
				CPed* ClosestPed = nullptr;
				float ClosestDistSq = FLT_MAX;

				D3DXMATRIX ViewMatrix;
				if (s_useCachedViewMatrix) {
					ViewMatrix = s_cachedViewMatrix;
				} else {
					if (!Core::SDK::Pointers::pViewPort) return nullptr;
					ViewMatrix = Mem.Read<D3DXMATRIX>(Core::SDK::Pointers::pViewPort + 0x24C);
					D3DXMatrixTranspose(&ViewMatrix, &ViewMatrix);
				}
				float vxx = ViewMatrix._21, vxy = ViewMatrix._22, vxz = ViewMatrix._23, vxw = ViewMatrix._24;
				float vyx = ViewMatrix._31, vyy = ViewMatrix._32, vyz = ViewMatrix._33, vyw = ViewMatrix._34;
				float vzx = ViewMatrix._41, vzy = ViewMatrix._42, vzz = ViewMatrix._43, vzw = ViewMatrix._44;
				float winW = g_Variables.g_vGameWindowSize.x;
				float winH = g_Variables.g_vGameWindowSize.y;
				float cx = g_Variables.g_vGameWindowCenter.x;
				float cy = g_Variables.g_vGameWindowCenter.y;

				std::lock_guard<std::mutex> lock(Core::SDK::Game::EntityListMutex);
				for (auto& Entity : Core::SDK::Game::EntityList)
				{
					CPed* Ped = Entity.Ped;

					if (Ped == Core::SDK::Pointers::pLocalPlayer)
						continue;

					if (std::abs(Entity.Health) <= 1.0f)
						continue;

					if (IgnoreNpc && !Entity.IsPlayer)
						continue;

					if (IgnoreFriends && Entity.IsFriend)
						continue;

					if (VisibleCheck && !Entity.Visible)
						continue;

					if (Entity.Distance > MaxDistance)
						continue;

					float wx = Entity.Pos.x, wy = Entity.Pos.y, wz = Entity.Pos.z;
					float sx = vxx * wx + vxy * wy + vxz * wz + vxw;
					float sy = vyx * wx + vyy * wy + vyz * wz + vyw;
					float sz = vzx * wx + vzy * wy + vzz * wz + vzw;

					float screenX, screenY;
					if (sz <= 0.1f) {
						screenX = -1000.0f;
						screenY = -1000.0f;
					} else {
						float inv = 1.0f / sz;
						screenX = (winW / 2) + float(0.5f * (sx * inv) * winW + 0.5f);
						screenY = (winH / 2) - float(0.5f * (sy * inv) * winH + 0.5f);
					}

					float dx = screenX - cx;
					float dy = screenY - cy;
					float AimDistSq = dx * dx + dy * dy;

					if (AimDistSq < ClosestDistSq)
					{
						ClosestDistSq = AimDistSq;
						ClosestPed = Ped;
					}
				}

				return ClosestPed;
			}

			inline bool IsOnScreen(D3DXVECTOR2 Pos)
			{
				if (Pos.x <= 0.1f || Pos.y <= 0.1f) return false;
				if (Pos.x >= g_Variables.g_vGameWindowSize.x - 0.1f || Pos.y >= g_Variables.g_vGameWindowSize.y - 0.1f) return false;
				return true;
			}

		}
	}

}
