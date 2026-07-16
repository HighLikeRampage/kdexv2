#pragma once
#include <filesystem>
#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <Security/Api/json.hpp>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#include <Includes/Utils.hpp>
#include <Security/xorstr.hpp>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <Security/LazyImporter.hpp>
#include "../../framework/settings/config_theme.h"

namespace fs = std::filesystem;

inline std::string CitizenFX;

namespace Core {

	class Config {
	public:

		struct General {
		public:
			inline static bool StreamProof = false;
			inline static bool WaterMark;
			inline static bool WaterMarkCol;
			inline static bool ArrayList;
			inline static bool ArrayListCol;
			inline static bool VSync = true;
			inline static bool SecondMonitorDisplay = false;
			inline static bool enableRGBParticles = false;
			inline static int ProcessPriority = 0;
            inline static int MenuKey = VK_RSHIFT;
		} *General;

		struct Aimbot {
			enum HitboxType {
				HEAD = 0,
				BODY = 1,
				LEG = 2,
				RANDOM = 3
			};
		public:
            inline static bool Enabled = true;
			inline static bool ShowFov;
			inline static bool OnlyVisible;
			inline static bool IgnoreNPCs = true;
			inline static int HitBox = 0;
			inline static int FOV = 180;
			inline static int MaxDistance = 240;
			inline static int SmoothHorizontal = 12;
			inline static int SmoothVertical = 12;
			inline static int AimbotSpeed = 12;
			inline static bool AimCurving = true;
			inline static float CurveStrength = 0.3f;
			inline static bool InVehicleAimbot = true;
			inline static bool UsePrediction = false;
			inline static float PredictionTime = 0.0f;
			inline static float RandomizeAngle = 0.0f;
			inline static int KeyBind;
			inline static int Hitbox = HEAD;
			inline static ImColor FovColor{ 255, 255, 255 };
		} *Aimbot;

		struct TriggerBot {
		public:
            inline static bool Enabled = true;
			inline static bool ShowFov;
			inline static bool OnlyVisible;
			inline static bool IgnoreNPCs = true;
			inline static bool SmartTrigger;
			inline static int FOV = 20;
			inline static int MaxDistance = 200;
			inline static int Delay = 0;
			inline static int KeyBind;
			inline static ImColor FovColor { 255, 255, 255 };
		} *TriggerBot;

		struct SilentAim {
		public:
			enum HitboxType {
				HEAD = 0,
				BODY = 1,
				LEG = 2,
				RANDOM = 3
			};

		public:
            inline static bool Enabled = true;
			inline static bool ShowFov = true;
			inline static bool OnlyVisible;
			inline static bool MagicBullets;
			inline static bool IgnoreNPCs = true;
			inline static int FOV = 40;
			inline static int MissChance = 0;
			inline static int MaxDistance = 200;
			inline static int KeyBind;
			inline static int Hitbox = HEAD;
			inline static ImColor FovColor{ 224, 94, 103, 200 };
		} *SilentAim;

		struct ESP {
		public:
			inline static bool UpdateCfgESP;
            inline static bool Enabled = true;
			inline static bool Box;
			inline static bool FilledBox;
			inline static int BoxState = 0;
			inline static bool Skeleton;
			inline static bool HealthBar;
			inline static ImVec2 HealthBarPos;
			inline static int HealthBarState = 2;
			inline static bool ArmorBar;
			inline static ImVec2 ArmorBarPos;
			inline static int ArmorBarState = 2;
			inline static bool WeaponName;
			inline static ImVec2 WeaponNamePos;
			inline static int WeaponNameState = 1;
			inline static bool SnapLines;
			inline static bool UserNames;
			inline static ImVec2 UserNamesPos;
			inline static int UserNamesState = 1;
			inline static bool HeadCircle;
			inline static bool IgnoreNPCs = true;
			inline static bool ShowLocalPlayer;
			inline static bool HighlightVisible;
			inline static bool IgnoreDead;
			inline static bool DistanceFromMe;
			inline static bool FriendsMarker;
			inline static int FriendsMarkerBind;
			inline static ImVec2 DistanceFromMePos;
			inline static int DistanceFromMeState = 1;
			inline static int MaxDistance = 2500;
			inline static ImColor DistanceCol { 230, 230, 230, 255 };
			inline static ImColor UserNamesCol { 230, 230, 230, 255 };
			inline static ImColor WeaponNameCol { 230, 230, 230, 255 };
			inline static ImColor SkeletonCol { 255, 255, 255, 200 };
			inline static ImColor BoxCol { 255, 255, 255, 200 };
			inline static ImColor FilledBoxCol { 0, 0, 0, 40 };
			inline static ImColor SnapLinesCol { 255, 255, 255, 200 };
			inline static ImColor FriendCol { 255, 204, 0, 255 };
			inline static float TextSize = 13.0f;
			inline static float IconSize = 1.0f;
			inline static bool Tracer;
			inline static float TracerThickness = 1.5f;
			inline static float TracerDuration = 0.6f;
			inline static ImColor TracerColor { 255, 255, 255, 255 };
			inline static bool Coordinates;
			inline static int KeyBind;
		} *ESP;

        struct VehicleESP {
        public:
            inline static bool Enabled = true;
            inline static bool SnapLines;
            inline static bool ShowLockUnlock;
            inline static bool VehName;
            inline static bool DistanceFromMe;
            inline static int MaxDistance = 200;
            inline static ImColor SnapLinesCol { 255, 255, 255, 200 };
            inline static int KeyBind;
        } *VehicleESP;

        struct ObjectESP {
        public:
            inline static bool Enabled = true;
            inline static bool ObjName = true;
            inline static bool DistanceFromMe = true;
            inline static int MaxDistance = 150;
            inline static ImColor NameCol { 255, 255, 255, 255 };
        } *ObjectESP;

        struct Vehicle {
        public:
            inline static int PrimaryColorIdx = 0;
            inline static int SecondaryColorIdx = 0;
        } *Vehicle;

		struct Player {
		public:
			inline static float CurrentHealthValue;
			inline static float CurrentArmorValue;
			inline static bool FastRun;
			inline static float RunSpeed = 1.f;
			inline static bool InfiniteStamina;
			inline static bool WeaponOptions;
			inline static bool NoRecoilEnabled;
			inline static float RecoilValue;
			inline static bool NoSpreadEnabled;
			inline static float SpreadValue;
			inline static bool InfiniteAmmoEnabled;
			inline static bool NoReloadEnabled;
			inline static bool NoClipEnabled;
			inline static bool HandlingEditor;
			inline static int NoClipKey;
			inline static float NoClipSpeed = 2.0f;
			inline static bool InfiniteCombatRoll;
			inline static bool EnableGodMode;
			inline static bool VehicleGodMode;
			inline static bool SeatBelt;
            inline static bool ForceWeaponWheel;
            inline static bool ShrinkEnabled;
            inline static float ShrinkScale = 1.0f;
            inline static bool BigPedEnabled;
            inline static float BigPedScale = 2.0f;
            inline static bool NoRagDollEnabled;
			inline static bool AntiHSEnabled;
			inline static bool AntiBubbleAll;
			inline static bool KillAllPlayers;

			inline static bool StealCarEnabled;

			inline static bool FreeCam;
			inline static float FreeCamSpeed = 1.f;
			inline static int FreeCamKey;
			inline static bool FreeCamTeleport;
			inline static int FreeCamTeleportKey;

			inline static int GodModeKey;

			inline static bool SpectateEnabled;
			inline static int SpectateTargetIndex = -1;

			inline static bool FreeCamLockControls = true;
			inline static int FreeCamLockControlsKey = 0;

			inline static bool ExplosiveAmmoEnabled;
			inline static bool FireAmmoEnabled;
			inline static bool DamageBoost;
			inline static float Boost = 1.f;

			inline static bool BeastJumpEnabled;
			inline static bool SuperJumpEnabled;
			inline static bool SuperFistEnabled;
			inline static bool ExplosiveFistEnabled;

			inline static bool InvisibleEnabled;
			inline static bool AutoHealEnabled;
			inline static float AutoHealThreshold = 75.f;
			inline static bool AutoArmorEnabled;
			inline static float AutoArmorThreshold = 75.f;

			inline static bool CustomFovEnabled;
			inline static float FovValue = 70.f;

			inline static float SwimSpeed = 1.0f;
		} *Player;

		void Initialize() {
			static bool inited = false;
			if (inited) return;
			General = new struct General();
			Aimbot = new struct Aimbot();
			TriggerBot = new struct TriggerBot();
			SilentAim = new struct SilentAim();
			ESP = new struct ESP();
			VehicleESP = new struct VehicleESP();
			ObjectESP = new struct ObjectESP();
			Player = new struct Player();
			Vehicle = new struct Vehicle();
			inited = true;
		}

		nlohmann::json ImColToJson( const ImColor & Col ) {
			return nlohmann::json::array( { Col.Value.x, Col.Value.y, Col.Value.z, Col.Value.w } );
		}

		ImColor JsonToImCol( const nlohmann::json & JsonCol ) {
			if ( JsonCol.is_array( ) && JsonCol.size( ) == 4 ) {
				float r = JsonCol[ 0 ];
				float g = JsonCol[ 1 ];
				float b = JsonCol[ 2 ];
				float a = JsonCol[ 3 ];
				return ImColor( r, g, b, a );
			}
			else {
				return ImColor( 0.0f, 0.0f, 0.0f, 1.0f );
			}
		}

		ImVec2 JsonToImVec2( const nlohmann::json & j ) {
			if ( j.is_array( ) && j.size( ) >= 2 )
				return ImVec2( (float)j[ 0 ], (float)j[ 1 ] );
			return ImVec2( 0, 0 );
		}

		static std::vector<std::string> GetConfigs() {
			static bool created_cfg = true;
			std::vector<std::string> configs;

			if (created_cfg) {
				char path[MAX_PATH];
				if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
					CitizenFX = std::string(path) + xorstr("\\discord\\");
					CreateDirectoryA(CitizenFX.c_str(), nullptr);
				}
				created_cfg = false;
			}

			for (const auto& entry : fs::directory_iterator(CitizenFX)) {
				if (entry.is_regular_file() && entry.path().extension() == xorstr(".cfg")) {
					configs.push_back(entry.path().filename().string());
				}
			}

			return configs;
		}

		static std::string GetConfigFolder() {
			static bool inited = false;
			if (!inited) {
				char path[MAX_PATH];
				if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path))) {
					CitizenFX = std::string(path) + xorstr("\\discord\\");
					CreateDirectoryA(CitizenFX.c_str(), nullptr);
				}
				inited = true;
			}
			return CitizenFX;
		}

		static int GetLastConfigId() {
			try {
				std::string path = GetConfigFolder() + xorstr("kdex_last_config.txt");
				std::ifstream f(path);
				if (!f.is_open()) return 0;
				int id = 0;
				f >> id;
				return id > 0 ? id : 0;
			} catch (...) {}
			return 0;
		}
		static void SetLastConfigId(int id) {
			if (id <= 0) return;
			try {
				std::string path = GetConfigFolder() + xorstr("kdex_last_config.txt");
				std::ofstream f(path);
				if (f.is_open()) f << id;
			} catch (...) {}
		}

		void OpenConfigFolder() {
			std::wstring str = std::wstring(CitizenFX.begin(), CitizenFX.end());

			PIDLIST_ABSOLUTE pidl;
			if (SUCCEEDED(SHParseDisplayName(str.c_str(), 0, &pidl, 0, 0))) {
				ITEMIDLIST idNull = { 0 };
				LPCITEMIDLIST pidlNull[1] = { &idNull };
				LI_FN(SHOpenFolderAndSelectItems)(pidl, 1, pidlNull, 0);
			}
		}

		void CreateConfig(const std::string& configname) {
			if (configname.empty()) return;

			std::string fullPath = CitizenFX + configname + xorstr(".cfg");

			std::ifstream test_file(fullPath);
			if (test_file.good()) {
				test_file.close();
				return;
			}
			test_file.close();

			fs::path pathObj(fullPath);
			fs::create_directories(pathObj.parent_path());

			SaveCurrentConfigToFile(configname);
		}

		void ResetConfig() {
		}

		void SaveCurrentConfigToFile(const std::string& name, const nlohmann::json* merge_extra = nullptr) {
			try {
				std::string cleanName = name;
				if (cleanName.size() > 4 &&
					cleanName.substr(cleanName.size() - 4) == xorstr(".cfg")) {
					cleanName = cleanName.substr(0, cleanName.size() - 4);
				}

				std::string fullPath = CitizenFX + cleanName + xorstr(".cfg");

				nlohmann::json CfgJson;

				auto& GeneralCfg = CfgJson["General"];
				auto& FeaturesCfg = CfgJson["Features"];

			GeneralCfg["StreamProof"] = Config::General->StreamProof;
			GeneralCfg["WaterMark"] = Config::General->WaterMark;
			GeneralCfg["WaterMarkCol"] = Config::General->WaterMarkCol;
			GeneralCfg["ArrayList"] = Config::General->ArrayList;
			GeneralCfg["ArrayListCol"] = Config::General->ArrayListCol;
			GeneralCfg["VSync"] = Config::General->VSync;
			GeneralCfg["SecondMonitorDisplay"] = Config::General->SecondMonitorDisplay;
			GeneralCfg["enableRGBParticles"] = Config::General->enableRGBParticles;
			GeneralCfg["ProcessPriority"] = Config::General->ProcessPriority;
			GeneralCfg["MenuKey"] = Config::General->MenuKey;

				FeaturesCfg["Aimbot"]["Enabled"] = Config::Aimbot->Enabled;
				FeaturesCfg["Aimbot"]["ShowFov"] = Config::Aimbot->ShowFov;
				FeaturesCfg["Aimbot"]["OnlyVisible"] = Config::Aimbot->OnlyVisible;
				FeaturesCfg["Aimbot"]["IgnoreNPCs"] = Config::Aimbot->IgnoreNPCs;
				FeaturesCfg["Aimbot"]["HitBox"] = Config::Aimbot->HitBox;
				FeaturesCfg["Aimbot"]["Hitbox"] = Config::Aimbot->Hitbox;
				FeaturesCfg["Aimbot"]["FOV"] = Config::Aimbot->FOV;
				FeaturesCfg["Aimbot"]["MaxDistance"] = Config::Aimbot->MaxDistance;
				FeaturesCfg["Aimbot"]["SmoothHorizontal"] = Config::Aimbot->SmoothHorizontal;
				FeaturesCfg["Aimbot"]["SmoothVertical"] = Config::Aimbot->SmoothVertical;
				FeaturesCfg["Aimbot"]["AimbotSpeed"] = Config::Aimbot->AimbotSpeed;
				FeaturesCfg["Aimbot"]["AimCurving"] = Config::Aimbot->AimCurving;
				FeaturesCfg["Aimbot"]["CurveStrength"] = Config::Aimbot->CurveStrength;
				FeaturesCfg["Aimbot"]["KeyBind"] = Config::Aimbot->KeyBind;
				FeaturesCfg["Aimbot"]["FovColor"] = ImColToJson(Config::Aimbot->FovColor);
				FeaturesCfg["Aimbot"]["InVehicleAimbot"] = Config::Aimbot->InVehicleAimbot;
				FeaturesCfg["Aimbot"]["UsePrediction"] = Config::Aimbot->UsePrediction;
				FeaturesCfg["Aimbot"]["PredictionTime"] = Config::Aimbot->PredictionTime;
				FeaturesCfg["Aimbot"]["RandomizeAngle"] = Config::Aimbot->RandomizeAngle;

				FeaturesCfg["TriggerBot"]["Enabled"] = Config::TriggerBot->Enabled;
				FeaturesCfg["TriggerBot"]["ShowFov"] = Config::TriggerBot->ShowFov;
				FeaturesCfg["TriggerBot"]["OnlyVisible"] = Config::TriggerBot->OnlyVisible;
				FeaturesCfg["TriggerBot"]["IgnoreNPCs"] = Config::TriggerBot->IgnoreNPCs;
				FeaturesCfg["TriggerBot"]["SmartTrigger"] = Config::TriggerBot->SmartTrigger;
				FeaturesCfg["TriggerBot"]["FOV"] = Config::TriggerBot->FOV;
				FeaturesCfg["TriggerBot"]["MaxDistance"] = Config::TriggerBot->MaxDistance;
				FeaturesCfg["TriggerBot"]["Delay"] = Config::TriggerBot->Delay;
				FeaturesCfg["TriggerBot"]["KeyBind"] = Config::TriggerBot->KeyBind;
				FeaturesCfg["TriggerBot"]["FovColor"] = ImColToJson(Config::TriggerBot->FovColor);

				FeaturesCfg["SilentAim"]["Enabled"] = Config::SilentAim->Enabled;
				FeaturesCfg["SilentAim"]["ShowFov"] = Config::SilentAim->ShowFov;
				FeaturesCfg["SilentAim"]["OnlyVisible"] = Config::SilentAim->OnlyVisible;
				FeaturesCfg["SilentAim"]["MagicBullets"] = Config::SilentAim->MagicBullets;
				FeaturesCfg["SilentAim"]["IgnoreNPCs"] = Config::SilentAim->IgnoreNPCs;
				FeaturesCfg["SilentAim"]["FOV"] = Config::SilentAim->FOV;
				FeaturesCfg["SilentAim"]["MissChance"] = Config::SilentAim->MissChance;
				FeaturesCfg["SilentAim"]["MaxDistance"] = Config::SilentAim->MaxDistance;
				FeaturesCfg["SilentAim"]["KeyBind"] = Config::SilentAim->KeyBind;
				FeaturesCfg["SilentAim"]["HitBox"] = Config::SilentAim->Hitbox;
				FeaturesCfg["SilentAim"]["FovColor"] = ImColToJson(Config::SilentAim->FovColor);

				FeaturesCfg["ESP"]["Enabled"] = Config::ESP->Enabled;
				FeaturesCfg["ESP"]["Box"] = Config::ESP->Box;
				FeaturesCfg["ESP"]["FilledBox"] = Config::ESP->FilledBox;
				FeaturesCfg["ESP"]["BoxState"] = Config::ESP->BoxState;
				FeaturesCfg["ESP"]["Skeleton"] = Config::ESP->Skeleton;
				FeaturesCfg["ESP"]["HealthBar"] = Config::ESP->HealthBar;
				FeaturesCfg["ESP"]["HealthBarPos"] = nlohmann::json::array({ Config::ESP->HealthBarPos.x, Config::ESP->HealthBarPos.y });
				FeaturesCfg["ESP"]["HealthBarState"] = Config::ESP->HealthBarState;
				FeaturesCfg["ESP"]["ArmorBar"] = Config::ESP->ArmorBar;
				FeaturesCfg["ESP"]["ArmorBarPos"] = nlohmann::json::array({ Config::ESP->ArmorBarPos.x, Config::ESP->ArmorBarPos.y });
				FeaturesCfg["ESP"]["ArmorBarState"] = Config::ESP->ArmorBarState;
				FeaturesCfg["ESP"]["WeaponName"] = Config::ESP->WeaponName;
				FeaturesCfg["ESP"]["WeaponNamePos"] = nlohmann::json::array({ Config::ESP->WeaponNamePos.x, Config::ESP->WeaponNamePos.y });
				FeaturesCfg["ESP"]["WeaponNameState"] = Config::ESP->WeaponNameState;
				FeaturesCfg["ESP"]["SnapLines"] = Config::ESP->SnapLines;
				FeaturesCfg["ESP"]["UserNames"] = Config::ESP->UserNames;
				FeaturesCfg["ESP"]["UserNamesPos"] = nlohmann::json::array({ Config::ESP->UserNamesPos.x, Config::ESP->UserNamesPos.y });
				FeaturesCfg["ESP"]["UserNamesState"] = Config::ESP->UserNamesState;
				FeaturesCfg["ESP"]["HeadCircle"] = Config::ESP->HeadCircle;
				FeaturesCfg["ESP"]["IgnoreNPCs"] = Config::ESP->IgnoreNPCs;
				FeaturesCfg["ESP"]["ShowLocalPlayer"] = Config::ESP->ShowLocalPlayer;
				FeaturesCfg["ESP"]["HighlightVisible"] = Config::ESP->HighlightVisible;
				FeaturesCfg["ESP"]["IgnoreDead"] = Config::ESP->IgnoreDead;
				FeaturesCfg["ESP"]["DistanceFromMe"] = Config::ESP->DistanceFromMe;
				FeaturesCfg["ESP"]["DistanceFromMePos"] = nlohmann::json::array({ Config::ESP->DistanceFromMePos.x, Config::ESP->DistanceFromMePos.y });
				FeaturesCfg["ESP"]["DistanceFromMeState"] = Config::ESP->DistanceFromMeState;
				FeaturesCfg["ESP"]["MaxDistance"] = Config::ESP->MaxDistance;
				FeaturesCfg["ESP"]["FriendsMarker"] = Config::ESP->FriendsMarker;
				FeaturesCfg["ESP"]["FriendsMarkerBind"] = Config::ESP->FriendsMarkerBind;
				FeaturesCfg["ESP"]["DistanceCol"] = ImColToJson(Config::ESP->DistanceCol);
				FeaturesCfg["ESP"]["UserNamesCol"] = ImColToJson(Config::ESP->UserNamesCol);
				FeaturesCfg["ESP"]["WeaponNameCol"] = ImColToJson(Config::ESP->WeaponNameCol);
				FeaturesCfg["ESP"]["SkeletonCol"] = ImColToJson(Config::ESP->SkeletonCol);
				FeaturesCfg["ESP"]["BoxCol"] = ImColToJson(Config::ESP->BoxCol);
				FeaturesCfg["ESP"]["FilledBoxCol"] = ImColToJson(Config::ESP->FilledBoxCol);
				FeaturesCfg["ESP"]["SnapLinesCol"] = ImColToJson(Config::ESP->SnapLinesCol);
				FeaturesCfg["ESP"]["FriendCol"] = ImColToJson(Config::ESP->FriendCol);
				FeaturesCfg["ESP"]["Tracer"] = Config::ESP->Tracer;
				FeaturesCfg["ESP"]["TracerThickness"] = Config::ESP->TracerThickness;
				FeaturesCfg["ESP"]["TracerDuration"] = Config::ESP->TracerDuration;
				FeaturesCfg["ESP"]["TracerColor"] = ImColToJson(Config::ESP->TracerColor);
				FeaturesCfg["ESP"]["Coordinates"] = Config::ESP->Coordinates;
				FeaturesCfg["ESP"]["KeyBind"] = Config::ESP->KeyBind;

                FeaturesCfg["VehicleESP"]["Enabled"] = Config::VehicleESP->Enabled;
                FeaturesCfg["VehicleESP"]["SnapLines"] = Config::VehicleESP->SnapLines;
                FeaturesCfg["VehicleESP"]["ShowLockUnlock"] = Config::VehicleESP->ShowLockUnlock;
                FeaturesCfg["VehicleESP"]["VehName"] = Config::VehicleESP->VehName;
                FeaturesCfg["VehicleESP"]["DistanceFromMe"] = Config::VehicleESP->DistanceFromMe;
                FeaturesCfg["VehicleESP"]["MaxDistance"] = Config::VehicleESP->MaxDistance;
                FeaturesCfg["VehicleESP"]["SnapLinesCol"] = ImColToJson(Config::VehicleESP->SnapLinesCol);
                FeaturesCfg["VehicleESP"]["KeyBind"] = Config::VehicleESP->KeyBind;

                FeaturesCfg["ObjectESP"]["Enabled"] = Config::ObjectESP->Enabled;
                FeaturesCfg["ObjectESP"]["ObjName"] = Config::ObjectESP->ObjName;
                FeaturesCfg["ObjectESP"]["DistanceFromMe"] = Config::ObjectESP->DistanceFromMe;
                FeaturesCfg["ObjectESP"]["MaxDistance"] = Config::ObjectESP->MaxDistance;
                FeaturesCfg["ObjectESP"]["NameCol"] = ImColToJson(Config::ObjectESP->NameCol);

                FeaturesCfg["Vehicle"]["PrimaryColorIdx"] = Config::Vehicle->PrimaryColorIdx;
                FeaturesCfg["Vehicle"]["SecondaryColorIdx"] = Config::Vehicle->SecondaryColorIdx;

				FeaturesCfg["Player"]["FastRun"] = Config::Player->FastRun;
				FeaturesCfg["Player"]["RunSpeed"] = Config::Player->RunSpeed;
				FeaturesCfg["Player"]["InfiniteStamina"] = Config::Player->InfiniteStamina;
				FeaturesCfg["Player"]["WeaponOptions"] = Config::Player->WeaponOptions;
				FeaturesCfg["Player"]["NoRecoilEnabled"] = Config::Player->NoRecoilEnabled;
				FeaturesCfg["Player"]["RecoilValue"] = Config::Player->RecoilValue;
				FeaturesCfg["Player"]["NoSpreadEnabled"] = Config::Player->NoSpreadEnabled;
				FeaturesCfg["Player"]["SpreadValue"] = Config::Player->SpreadValue;
				FeaturesCfg["Player"]["InfiniteAmmoEnabled"] = Config::Player->InfiniteAmmoEnabled;
				FeaturesCfg["Player"]["NoReloadEnabled"] = Config::Player->NoReloadEnabled;
				FeaturesCfg["Player"]["NoClipEnabled"] = Config::Player->NoClipEnabled;
				FeaturesCfg["Player"]["NoClipKey"] = Config::Player->NoClipKey;
				FeaturesCfg["Player"]["NoClipSpeed"] = Config::Player->NoClipSpeed;
				FeaturesCfg["Player"]["HandlingEditor"] = Config::Player->HandlingEditor;
				FeaturesCfg["Player"]["InfiniteCombatRoll"] = Config::Player->InfiniteCombatRoll;
				FeaturesCfg["Player"]["EnableGodMode"] = Config::Player->EnableGodMode;
				FeaturesCfg["Player"]["GodModeKey"] = Config::Player->GodModeKey;
				FeaturesCfg["Player"]["VehicleGodMode"] = Config::Player->VehicleGodMode;
				FeaturesCfg["Player"]["SeatBelt"] = Config::Player->SeatBelt;
				FeaturesCfg["Player"]["ForceWeaponWheel"] = Config::Player->ForceWeaponWheel;
				FeaturesCfg["Player"]["ShrinkEnabled"] = Config::Player->ShrinkEnabled;
				FeaturesCfg["Player"]["ShrinkScale"] = Config::Player->ShrinkScale;
				FeaturesCfg["Player"]["BigPedEnabled"] = Config::Player->BigPedEnabled;
				FeaturesCfg["Player"]["BigPedScale"] = Config::Player->BigPedScale;
				FeaturesCfg["Player"]["NoRagDollEnabled"] = Config::Player->NoRagDollEnabled;
				FeaturesCfg["Player"]["AntiHSEnabled"] = Config::Player->AntiHSEnabled;
				FeaturesCfg["Player"]["AntiBubbleAll"] = Config::Player->AntiBubbleAll;
				FeaturesCfg["Player"]["KillAllPlayers"] = Config::Player->KillAllPlayers;
				FeaturesCfg["Player"]["StealCarEnabled"] = Config::Player->StealCarEnabled;
				FeaturesCfg["Player"]["FreeCam"] = Config::Player->FreeCam;
				FeaturesCfg["Player"]["FreeCamSpeed"] = Config::Player->FreeCamSpeed;
				FeaturesCfg["Player"]["FreeCamKey"] = Config::Player->FreeCamKey;
				FeaturesCfg["Player"]["FreeCamTeleport"] = Config::Player->FreeCamTeleport;
				FeaturesCfg["Player"]["FreeCamTeleportKey"] = Config::Player->FreeCamTeleportKey;
				FeaturesCfg["Player"]["FreeCamLockControls"] = Config::Player->FreeCamLockControls;
				FeaturesCfg["Player"]["FreeCamLockControlsKey"] = Config::Player->FreeCamLockControlsKey;
				FeaturesCfg["Player"]["SpectateEnabled"] = Config::Player->SpectateEnabled;
				FeaturesCfg["Player"]["SpectateTargetIndex"] = Config::Player->SpectateTargetIndex;
				FeaturesCfg["Player"]["ExplosiveAmmoEnabled"] = Config::Player->ExplosiveAmmoEnabled;
				FeaturesCfg["Player"]["FireAmmoEnabled"] = Config::Player->FireAmmoEnabled;
				FeaturesCfg["Player"]["DamageBoost"] = Config::Player->DamageBoost;
				FeaturesCfg["Player"]["Boost"] = Config::Player->Boost;
				FeaturesCfg["Player"]["BeastJumpEnabled"] = Config::Player->BeastJumpEnabled;
				FeaturesCfg["Player"]["SuperJumpEnabled"] = Config::Player->SuperJumpEnabled;
				FeaturesCfg["Player"]["SuperFistEnabled"] = Config::Player->SuperFistEnabled;
				FeaturesCfg["Player"]["ExplosiveFistEnabled"] = Config::Player->ExplosiveFistEnabled;
				FeaturesCfg["Player"]["InvisibleEnabled"] = Config::Player->InvisibleEnabled;
				FeaturesCfg["Player"]["AutoHealEnabled"] = Config::Player->AutoHealEnabled;
				FeaturesCfg["Player"]["AutoHealThreshold"] = Config::Player->AutoHealThreshold;
				FeaturesCfg["Player"]["AutoArmorEnabled"] = Config::Player->AutoArmorEnabled;
				FeaturesCfg["Player"]["AutoArmorThreshold"] = Config::Player->AutoArmorThreshold;
				FeaturesCfg["Player"]["CustomFovEnabled"] = Config::Player->CustomFovEnabled;
				FeaturesCfg["Player"]["FovValue"] = Config::Player->FovValue;
				FeaturesCfg["Player"]["SwimSpeed"] = Config::Player->SwimSpeed;

				if (merge_extra && !merge_extra->empty())
					CfgJson.update(*merge_extra);

				std::string jsonStr = CfgJson.dump();
				std::ofstream out(fullPath);
				if (!out.is_open())
					throw std::runtime_error(xorstr("Error opening file for writing."));
				out << jsonStr;
				out.close();

				std::cout << xorstr("Config saved successfully: ") << cleanName << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << xorstr("Error saving config: ") << e.what() << std::endl;
			}
		}

		void LoadConfigFromFile(const std::string& name, nlohmann::json* out_parsed = nullptr) {
			try {
				std::string fullPath = CitizenFX + name;
				if (fullPath.size() <= 4 ||
					fullPath.substr(fullPath.size() - 4) != xorstr(".cfg")) {
					fullPath += xorstr(".cfg");
				}

				std::ifstream in(fullPath);
				if (!in.is_open())
					throw std::runtime_error(xorstr("File not found."));

				std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
				in.close();

				std::string jsonStr;
				if (content.size() >= 8 && content.compare(0, 8, xorstr("[vpx] - ")) == 0) {
					jsonStr = Utils::DecodeB64(content.substr(8));
				} else {
					jsonStr = content;
				}

				nlohmann::json CfgJson = nlohmann::json::parse(jsonStr);

				if (CfgJson.contains("General")) {
					auto& g = CfgJson["General"];
					if (g.contains("StreamProof")) Config::General->StreamProof = g["StreamProof"];
					if (g.contains("WaterMark")) Config::General->WaterMark = g["WaterMark"];
					if (g.contains("WaterMarkCol")) Config::General->WaterMarkCol = g["WaterMarkCol"];
					if (g.contains("ArrayList")) Config::General->ArrayList = g["ArrayList"];
					if (g.contains("ArrayListCol")) Config::General->ArrayListCol = g["ArrayListCol"];
					if (g.contains("VSync")) Config::General->VSync = g["VSync"];
					if (g.contains("SecondMonitorDisplay")) Config::General->SecondMonitorDisplay = g["SecondMonitorDisplay"];
					if (g.contains("enableRGBParticles")) Config::General->enableRGBParticles = g["enableRGBParticles"];
					if (g.contains("ProcessPriority")) Config::General->ProcessPriority = g["ProcessPriority"];
					if (g.contains("MenuKey")) Config::General->MenuKey = g["MenuKey"];
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("Aimbot")) {
					auto& a = CfgJson["Features"]["Aimbot"];
					if (a.contains("Enabled")) Config::Aimbot->Enabled = a["Enabled"];
					if (a.contains("ShowFov")) Config::Aimbot->ShowFov = a["ShowFov"];
					if (a.contains("OnlyVisible")) Config::Aimbot->OnlyVisible = a["OnlyVisible"];
					if (a.contains("IgnoreNPCs")) Config::Aimbot->IgnoreNPCs = a["IgnoreNPCs"];
					if (a.contains("HitBox")) Config::Aimbot->HitBox = a["HitBox"];
					if (a.contains("Hitbox")) Config::Aimbot->Hitbox = a["Hitbox"];
					if (a.contains("FOV")) Config::Aimbot->FOV = a["FOV"];
					if (a.contains("MaxDistance")) Config::Aimbot->MaxDistance = a["MaxDistance"];
					if (a.contains("SmoothHorizontal")) Config::Aimbot->SmoothHorizontal = a["SmoothHorizontal"];
					if (a.contains("SmoothVertical")) Config::Aimbot->SmoothVertical = a["SmoothVertical"];
					if (a.contains("AimbotSpeed")) Config::Aimbot->AimbotSpeed = a["AimbotSpeed"];
					if (a.contains("AimCurving")) Config::Aimbot->AimCurving = a["AimCurving"];
					if (a.contains("CurveStrength")) Config::Aimbot->CurveStrength = a["CurveStrength"];
					if (a.contains("KeyBind")) Config::Aimbot->KeyBind = a["KeyBind"];
					if (a.contains("FovColor")) Config::Aimbot->FovColor = JsonToImCol(a["FovColor"]);
					if (a.contains("InVehicleAimbot")) Config::Aimbot->InVehicleAimbot = a["InVehicleAimbot"];
					if (a.contains("UsePrediction")) Config::Aimbot->UsePrediction = a["UsePrediction"];
					if (a.contains("PredictionTime")) Config::Aimbot->PredictionTime = a["PredictionTime"];
					if (a.contains("RandomizeAngle")) Config::Aimbot->RandomizeAngle = a["RandomizeAngle"];
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("TriggerBot")) {
					auto& t = CfgJson["Features"]["TriggerBot"];
					if (t.contains("Enabled")) Config::TriggerBot->Enabled = t["Enabled"];
					if (t.contains("ShowFov")) Config::TriggerBot->ShowFov = t["ShowFov"];
					if (t.contains("OnlyVisible")) Config::TriggerBot->OnlyVisible = t["OnlyVisible"];
					if (t.contains("IgnoreNPCs")) Config::TriggerBot->IgnoreNPCs = t["IgnoreNPCs"];
					if (t.contains("SmartTrigger")) Config::TriggerBot->SmartTrigger = t["SmartTrigger"];
					if (t.contains("FOV")) Config::TriggerBot->FOV = t["FOV"];
					if (t.contains("MaxDistance")) Config::TriggerBot->MaxDistance = t["MaxDistance"];
					if (t.contains("Delay")) Config::TriggerBot->Delay = t["Delay"];
					if (t.contains("KeyBind")) Config::TriggerBot->KeyBind = t["KeyBind"];
					if (t.contains("FovColor")) Config::TriggerBot->FovColor = JsonToImCol(t["FovColor"]);
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("SilentAim")) {
					auto& s = CfgJson["Features"]["SilentAim"];
					if (s.contains("Enabled")) Config::SilentAim->Enabled = s["Enabled"];
					if (s.contains("ShowFov")) Config::SilentAim->ShowFov = s["ShowFov"];
					if (s.contains("OnlyVisible")) Config::SilentAim->OnlyVisible = s["OnlyVisible"];
					if (s.contains("MagicBullets")) Config::SilentAim->MagicBullets = s["MagicBullets"];
					if (s.contains("IgnoreNPCs")) Config::SilentAim->IgnoreNPCs = s["IgnoreNPCs"];
					if (s.contains("FOV")) Config::SilentAim->FOV = s["FOV"];
					if (s.contains("MissChance")) Config::SilentAim->MissChance = s["MissChance"];
					if (s.contains("MaxDistance")) Config::SilentAim->MaxDistance = s["MaxDistance"];
					if (s.contains("KeyBind")) Config::SilentAim->KeyBind = s["KeyBind"];
					if (s.contains("HitBox")) Config::SilentAim->Hitbox = s["HitBox"];
					if (s.contains("FovColor")) Config::SilentAim->FovColor = JsonToImCol(s["FovColor"]);
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("ESP")) {
					auto& e = CfgJson["Features"]["ESP"];
					if (e.contains("Enabled")) Config::ESP->Enabled = e["Enabled"];
					if (e.contains("Box")) Config::ESP->Box = e["Box"];
					if (e.contains("FilledBox")) Config::ESP->FilledBox = e["FilledBox"];
					if (e.contains("BoxState")) Config::ESP->BoxState = e["BoxState"];
					if (e.contains("Skeleton")) Config::ESP->Skeleton = e["Skeleton"];
					if (e.contains("HealthBar")) Config::ESP->HealthBar = e["HealthBar"];
					if (e.contains("HealthBarPos")) Config::ESP->HealthBarPos = JsonToImVec2(e["HealthBarPos"]);
					if (e.contains("HealthBarState")) Config::ESP->HealthBarState = e["HealthBarState"];
					if (e.contains("ArmorBar")) Config::ESP->ArmorBar = e["ArmorBar"];
					if (e.contains("ArmorBarPos")) Config::ESP->ArmorBarPos = JsonToImVec2(e["ArmorBarPos"]);
					if (e.contains("ArmorBarState")) Config::ESP->ArmorBarState = e["ArmorBarState"];
					if (e.contains("WeaponName")) Config::ESP->WeaponName = e["WeaponName"];
					if (e.contains("WeaponNamePos")) Config::ESP->WeaponNamePos = JsonToImVec2(e["WeaponNamePos"]);
					if (e.contains("WeaponNameState")) Config::ESP->WeaponNameState = e["WeaponNameState"];
					if (e.contains("SnapLines")) Config::ESP->SnapLines = e["SnapLines"];
					if (e.contains("UserNames")) Config::ESP->UserNames = e["UserNames"];
					if (e.contains("UserNamesPos")) Config::ESP->UserNamesPos = JsonToImVec2(e["UserNamesPos"]);
					if (e.contains("UserNamesState")) Config::ESP->UserNamesState = e["UserNamesState"];
					if (e.contains("HeadCircle")) Config::ESP->HeadCircle = e["HeadCircle"];
					if (e.contains("IgnoreNPCs")) Config::ESP->IgnoreNPCs = e["IgnoreNPCs"];
					if (e.contains("ShowLocalPlayer")) Config::ESP->ShowLocalPlayer = e["ShowLocalPlayer"];
					if (e.contains("HighlightVisible")) Config::ESP->HighlightVisible = e["HighlightVisible"];
					if (e.contains("IgnoreDead")) Config::ESP->IgnoreDead = e["IgnoreDead"];
					if (e.contains("DistanceFromMe")) Config::ESP->DistanceFromMe = e["DistanceFromMe"];
					if (e.contains("DistanceFromMePos")) Config::ESP->DistanceFromMePos = JsonToImVec2(e["DistanceFromMePos"]);
					if (e.contains("DistanceFromMeState")) Config::ESP->DistanceFromMeState = e["DistanceFromMeState"];
					if (e.contains("MaxDistance")) Config::ESP->MaxDistance = e["MaxDistance"];
					if (e.contains("FriendsMarker")) Config::ESP->FriendsMarker = e["FriendsMarker"];
					if (e.contains("FriendsMarkerBind")) Config::ESP->FriendsMarkerBind = e["FriendsMarkerBind"];
					if (e.contains("DistanceCol")) Config::ESP->DistanceCol = JsonToImCol(e["DistanceCol"]);
					if (e.contains("UserNamesCol")) Config::ESP->UserNamesCol = JsonToImCol(e["UserNamesCol"]);
					if (e.contains("WeaponNameCol")) Config::ESP->WeaponNameCol = JsonToImCol(e["WeaponNameCol"]);
					if (e.contains("SkeletonCol")) Config::ESP->SkeletonCol = JsonToImCol(e["SkeletonCol"]);
					if (e.contains("BoxCol")) Config::ESP->BoxCol = JsonToImCol(e["BoxCol"]);
					if (e.contains("FilledBoxCol")) Config::ESP->FilledBoxCol = JsonToImCol(e["FilledBoxCol"]);
					if (e.contains("SnapLinesCol")) Config::ESP->SnapLinesCol = JsonToImCol(e["SnapLinesCol"]);
					if (e.contains("FriendCol")) Config::ESP->FriendCol = JsonToImCol(e["FriendCol"]);
					if (e.contains("Tracer")) Config::ESP->Tracer = e["Tracer"];
					if (e.contains("TracerThickness")) Config::ESP->TracerThickness = e["TracerThickness"];
					if (e.contains("TracerDuration")) Config::ESP->TracerDuration = e["TracerDuration"];
					if (e.contains("TracerColor")) Config::ESP->TracerColor = JsonToImCol(e["TracerColor"]);
					if (e.contains("KeyBind")) Config::ESP->KeyBind = e["KeyBind"];
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("VehicleESP")) {
					auto& v = CfgJson["Features"]["VehicleESP"];
					if (v.contains("Enabled")) Config::VehicleESP->Enabled = v["Enabled"];
					if (v.contains("SnapLines")) Config::VehicleESP->SnapLines = v["SnapLines"];
					if (v.contains("ShowLockUnlock")) Config::VehicleESP->ShowLockUnlock = v["ShowLockUnlock"];
					if (v.contains("VehName")) Config::VehicleESP->VehName = v["VehName"];
					if (v.contains("DistanceFromMe")) Config::VehicleESP->DistanceFromMe = v["DistanceFromMe"];
					if (v.contains("MaxDistance")) Config::VehicleESP->MaxDistance = v["MaxDistance"];
					if (v.contains("SnapLinesCol")) Config::VehicleESP->SnapLinesCol = JsonToImCol(v["SnapLinesCol"]);
					if (v.contains("KeyBind")) Config::VehicleESP->KeyBind = v["KeyBind"];
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("ObjectESP")) {
					auto& o = CfgJson["Features"]["ObjectESP"];
					if (o.contains("Enabled")) Config::ObjectESP->Enabled = o["Enabled"];
					if (o.contains("ObjName")) Config::ObjectESP->ObjName = o["ObjName"];
					if (o.contains("DistanceFromMe")) Config::ObjectESP->DistanceFromMe = o["DistanceFromMe"];
					if (o.contains("MaxDistance")) Config::ObjectESP->MaxDistance = o["MaxDistance"];
					if (o.contains("NameCol")) Config::ObjectESP->NameCol = JsonToImCol(o["NameCol"]);
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("Vehicle")) {
					auto& v = CfgJson["Features"]["Vehicle"];
					if (v.contains("PrimaryColorIdx")) Config::Vehicle->PrimaryColorIdx = v["PrimaryColorIdx"];
					if (v.contains("SecondaryColorIdx")) Config::Vehicle->SecondaryColorIdx = v["SecondaryColorIdx"];
				}

				if (CfgJson.contains("Features") && CfgJson["Features"].contains("Player")) {
					auto& p = CfgJson["Features"]["Player"];
					if (p.contains("FastRun")) Config::Player->FastRun = p["FastRun"];
					if (p.contains("RunSpeed")) Config::Player->RunSpeed = p["RunSpeed"];
					if (p.contains("InfiniteStamina")) Config::Player->InfiniteStamina = p["InfiniteStamina"];
					if (p.contains("WeaponOptions")) Config::Player->WeaponOptions = p["WeaponOptions"];
					if (p.contains("NoRecoilEnabled")) Config::Player->NoRecoilEnabled = p["NoRecoilEnabled"];
					if (p.contains("RecoilValue")) Config::Player->RecoilValue = p["RecoilValue"];
					if (p.contains("NoSpreadEnabled")) Config::Player->NoSpreadEnabled = p["NoSpreadEnabled"];
					if (p.contains("SpreadValue")) Config::Player->SpreadValue = p["SpreadValue"];
					if (p.contains("InfiniteAmmoEnabled")) Config::Player->InfiniteAmmoEnabled = p["InfiniteAmmoEnabled"];
					if (p.contains("NoReloadEnabled")) Config::Player->NoReloadEnabled = p["NoReloadEnabled"];
					if (p.contains("NoClipEnabled")) Config::Player->NoClipEnabled = p["NoClipEnabled"];
					if (p.contains("NoClipKey")) Config::Player->NoClipKey = p["NoClipKey"];
					if (p.contains("NoClipSpeed")) Config::Player->NoClipSpeed = p["NoClipSpeed"];
					if (p.contains("HandlingEditor")) Config::Player->HandlingEditor = p["HandlingEditor"];
					if (p.contains("InfiniteCombatRoll")) Config::Player->InfiniteCombatRoll = p["InfiniteCombatRoll"];
					if (p.contains("EnableGodMode")) Config::Player->EnableGodMode = p["EnableGodMode"];
					if (p.contains("GodModeKey")) Config::Player->GodModeKey = p["GodModeKey"];
					if (p.contains("VehicleGodMode")) Config::Player->VehicleGodMode = p["VehicleGodMode"];
					if (p.contains("SeatBelt")) Config::Player->SeatBelt = p["SeatBelt"];
					if (p.contains("ForceWeaponWheel")) Config::Player->ForceWeaponWheel = p["ForceWeaponWheel"];
					if (p.contains("ShrinkEnabled")) Config::Player->ShrinkEnabled = p["ShrinkEnabled"];
					if (p.contains("ShrinkScale")) Config::Player->ShrinkScale = p["ShrinkScale"];
					if (p.contains("BigPedEnabled")) Config::Player->BigPedEnabled = p["BigPedEnabled"];
					if (p.contains("BigPedScale")) Config::Player->BigPedScale = p["BigPedScale"];
					if (p.contains("NoRagDollEnabled")) Config::Player->NoRagDollEnabled = p["NoRagDollEnabled"];
					if (p.contains("AntiHSEnabled")) Config::Player->AntiHSEnabled = p["AntiHSEnabled"];
					if (p.contains("AntiBubbleAll")) Config::Player->AntiBubbleAll = p["AntiBubbleAll"];
					if (p.contains("KillAllPlayers")) Config::Player->KillAllPlayers = p["KillAllPlayers"];
					if (p.contains("StealCarEnabled")) Config::Player->StealCarEnabled = p["StealCarEnabled"];
					if (p.contains("FreeCam")) Config::Player->FreeCam = p["FreeCam"];
					if (p.contains("FreeCamSpeed")) Config::Player->FreeCamSpeed = p["FreeCamSpeed"];
					if (p.contains("FreeCamKey")) Config::Player->FreeCamKey = p["FreeCamKey"];
					if (p.contains("FreeCamTeleport")) Config::Player->FreeCamTeleport = p["FreeCamTeleport"];
					if (p.contains("FreeCamTeleportKey")) Config::Player->FreeCamTeleportKey = p["FreeCamTeleportKey"];
					if (p.contains("FreeCamLockControls")) Config::Player->FreeCamLockControls = p["FreeCamLockControls"];
					if (p.contains("FreeCamLockControlsKey")) Config::Player->FreeCamLockControlsKey = p["FreeCamLockControlsKey"];
					if (p.contains("SpectateEnabled")) Config::Player->SpectateEnabled = p["SpectateEnabled"];
					if (p.contains("SpectateTargetIndex")) Config::Player->SpectateTargetIndex = p["SpectateTargetIndex"];
					if (p.contains("ExplosiveAmmoEnabled")) Config::Player->ExplosiveAmmoEnabled = p["ExplosiveAmmoEnabled"];
					if (p.contains("FireAmmoEnabled")) Config::Player->FireAmmoEnabled = p["FireAmmoEnabled"];
					if (p.contains("DamageBoost")) Config::Player->DamageBoost = p["DamageBoost"];
					if (p.contains("Boost")) Config::Player->Boost = p["Boost"];
					if (p.contains("BeastJumpEnabled")) Config::Player->BeastJumpEnabled = p["BeastJumpEnabled"];
					if (p.contains("SuperJumpEnabled")) Config::Player->SuperJumpEnabled = p["SuperJumpEnabled"];
					if (p.contains("SuperFistEnabled")) Config::Player->SuperFistEnabled = p["SuperFistEnabled"];
					if (p.contains("ExplosiveFistEnabled")) Config::Player->ExplosiveFistEnabled = p["ExplosiveFistEnabled"];
					if (p.contains("InvisibleEnabled")) Config::Player->InvisibleEnabled = p["InvisibleEnabled"];
					if (p.contains("AutoHealEnabled")) Config::Player->AutoHealEnabled = p["AutoHealEnabled"];
					if (p.contains("AutoHealThreshold")) Config::Player->AutoHealThreshold = p["AutoHealThreshold"];
					if (p.contains("AutoArmorEnabled")) Config::Player->AutoArmorEnabled = p["AutoArmorEnabled"];
					if (p.contains("AutoArmorThreshold")) Config::Player->AutoArmorThreshold = p["AutoArmorThreshold"];
					if (p.contains("CustomFovEnabled")) Config::Player->CustomFovEnabled = p["CustomFovEnabled"];
					if (p.contains("FovValue")) Config::Player->FovValue = p["FovValue"];
					if (p.contains("SwimSpeed")) Config::Player->SwimSpeed = p["SwimSpeed"];
				}

				Config::ESP->UpdateCfgESP = true;
				if ( out_parsed )
					*out_parsed = CfgJson;
				std::cout << xorstr("Config loaded successfully!") << std::endl;
			}
			catch (const std::exception& e) {
				std::cerr << xorstr("Error loading config file: ") << e.what() << std::endl;
			}
			catch (...) {
				std::cerr << xorstr("Unknown error loading config!") << std::endl;
			}
		}

		std::string SaveCurrentConfig( std::string CfgName )
		{
			try {
				nlohmann::json CfgJson;
				auto& GeneralCfg = CfgJson[ xorstr( "General" ) ];
				auto& FeaturesCfg = CfgJson;

				GeneralCfg[ xorstr( "StreamProof" ) ] = General->StreamProof;
				GeneralCfg[ xorstr( "WaterMark" ) ] = General->WaterMark;
				GeneralCfg[ xorstr( "WaterMarkCol" ) ] = General->WaterMarkCol;
				GeneralCfg[ xorstr( "ArrayList" ) ] = General->ArrayList;
				GeneralCfg[ xorstr( "ArrayListCol" ) ] = General->ArrayListCol;
				GeneralCfg[ xorstr( "VSync" ) ] = General->VSync;
				GeneralCfg[ xorstr( "SecondMonitorDisplay" ) ] = General->SecondMonitorDisplay;
				GeneralCfg[ xorstr( "enableRGBParticles" ) ] = General->enableRGBParticles;
				GeneralCfg[ xorstr( "ProcessPriority" ) ] = General->ProcessPriority;
				GeneralCfg[ xorstr( "MenuKey" ) ] = General->MenuKey;

				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "Enabled" ) ] = Aimbot->Enabled;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "ShowFov" ) ] = Aimbot->ShowFov;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "OnlyVisible" ) ] = Aimbot->OnlyVisible;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "IgnoreNPCs" ) ] = Aimbot->IgnoreNPCs;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "HitBox" ) ] = Aimbot->HitBox;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "Hitbox" ) ] = Aimbot->Hitbox;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "FOV" ) ] = Aimbot->FOV;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "MaxDistance" ) ] = Aimbot->MaxDistance;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "SmoothHorizontal" ) ] = Aimbot->SmoothHorizontal;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "SmoothVertical" ) ] = Aimbot->SmoothVertical;
				FeaturesCfg[xorstr("Aimbot")][xorstr("AimbotSpeed")] = Aimbot->AimbotSpeed;
				FeaturesCfg[xorstr("Aimbot")][xorstr("AimCurving")] = Aimbot->AimCurving;
				FeaturesCfg[xorstr("Aimbot")][xorstr("CurveStrength")] = Aimbot->CurveStrength;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "KeyBind" ) ] = Aimbot->KeyBind;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "FovColor" ) ] = ImColToJson( Aimbot->FovColor );
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "InVehicleAimbot" ) ] = Aimbot->InVehicleAimbot;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "UsePrediction" ) ] = Aimbot->UsePrediction;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "PredictionTime" ) ] = Aimbot->PredictionTime;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "RandomizeAngle" ) ] = Aimbot->RandomizeAngle;

				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "Enabled" ) ] = TriggerBot->Enabled;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "ShowFov" ) ] = TriggerBot->ShowFov;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "OnlyVisible" ) ] = TriggerBot->OnlyVisible;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "IgnoreNPCs" ) ] = TriggerBot->IgnoreNPCs;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "SmartTrigger" ) ] = TriggerBot->SmartTrigger;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "FOV" ) ] = TriggerBot->FOV;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "MaxDistance" ) ] = TriggerBot->MaxDistance;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "Delay" ) ] = TriggerBot->Delay;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "KeyBind" ) ] = TriggerBot->KeyBind;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "FovColor" ) ] = ImColToJson( TriggerBot->FovColor );

				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "Enabled" ) ] = SilentAim->Enabled;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "ShowFov" ) ] = SilentAim->ShowFov;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "OnlyVisible" ) ] = SilentAim->OnlyVisible;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "IgnoreNPCs" ) ] = SilentAim->IgnoreNPCs;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "FOV" ) ] = SilentAim->FOV;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "MaxDistance" ) ] = SilentAim->MaxDistance;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "KeyBind" ) ] = SilentAim->KeyBind;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "HitBox" ) ] = SilentAim->Hitbox;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "MissChance" ) ] = SilentAim->MissChance;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "MagicBullets" ) ] = SilentAim->MagicBullets;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "FovColor" ) ] = ImColToJson( SilentAim->FovColor );

				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Enabled" ) ] = ESP->Enabled;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Box" ) ] = ESP->Box;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FilledBox" ) ] = ESP->FilledBox;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "BoxState" ) ] = ESP->BoxState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Skeleton" ) ] = ESP->Skeleton;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HealthBar" ) ] = ESP->HealthBar;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HealthBarPos" ) ] = nlohmann::json::array( { ESP->HealthBarPos.x, ESP->HealthBarPos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HealthBarState" ) ] = ESP->HealthBarState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ArmorBar" ) ] = ESP->ArmorBar;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ArmorBarPos" ) ] = nlohmann::json::array( { ESP->ArmorBarPos.x, ESP->ArmorBarPos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ArmorBarState" ) ] = ESP->ArmorBarState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponName" ) ] = ESP->WeaponName;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponNamePos" ) ] = nlohmann::json::array( { ESP->WeaponNamePos.x, ESP->WeaponNamePos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponNameState" ) ] = ESP->WeaponNameState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "SnapLines" ) ] = ESP->SnapLines;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNames" ) ] = ESP->UserNames;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNamesPos" ) ] = nlohmann::json::array( { ESP->UserNamesPos.x, ESP->UserNamesPos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNamesState" ) ] = ESP->UserNamesState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HeadCircle" ) ] = ESP->HeadCircle;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "IgnoreNPCs" ) ] = ESP->IgnoreNPCs;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HighlightVisible" ) ] = ESP->HighlightVisible;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "IgnoreDead" ) ] = ESP->IgnoreDead;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceFromMe" ) ] = ESP->DistanceFromMe;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceFromMePos" ) ] = nlohmann::json::array( { ESP->DistanceFromMePos.x, ESP->DistanceFromMePos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceFromMeState" ) ] = ESP->DistanceFromMeState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "MaxDistance" ) ] = ESP->MaxDistance;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ShowLocalPlayer" ) ] = ESP->ShowLocalPlayer;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FriendsMarker" ) ] = ESP->FriendsMarker;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FriendsMarkerBind" ) ] = ESP->FriendsMarkerBind;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceCol" ) ] = ImColToJson( ESP->DistanceCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNamesCol" ) ] = ImColToJson( ESP->UserNamesCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponNameCol" ) ] = ImColToJson( ESP->WeaponNameCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "SkeletonCol" ) ] = ImColToJson( ESP->SkeletonCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "BoxCol" ) ] = ImColToJson( ESP->BoxCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FilledBoxCol" ) ] = ImColToJson( ESP->FilledBoxCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "SnapLinesCol" ) ] = ImColToJson( ESP->SnapLinesCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FriendCol" ) ] = ImColToJson( ESP->FriendCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Tracer" ) ] = ESP->Tracer;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "TracerThickness" ) ] = ESP->TracerThickness;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "TracerDuration" ) ] = ESP->TracerDuration;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "TracerColor" ) ] = ImColToJson( ESP->TracerColor );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "KeyBind" ) ] = ESP->KeyBind;

                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "Enabled" ) ] = VehicleESP->Enabled;
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "SnapLines" ) ] = VehicleESP->SnapLines;
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "ShowLockUnlock" ) ] = VehicleESP->ShowLockUnlock;
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "VehName" ) ] = VehicleESP->VehName;
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "DistanceFromMe" ) ] = VehicleESP->DistanceFromMe;
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "MaxDistance" ) ] = VehicleESP->MaxDistance;
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "SnapLinesCol" ) ] = ImColToJson( VehicleESP->SnapLinesCol );
                FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "KeyBind" ) ] = VehicleESP->KeyBind;

                FeaturesCfg[ xorstr( "Vehicle" ) ][ xorstr( "PrimaryColorIdx" ) ] = Vehicle->PrimaryColorIdx;
                FeaturesCfg[ xorstr( "Vehicle" ) ][ xorstr( "SecondaryColorIdx" ) ] = Vehicle->SecondaryColorIdx;

				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FastRun" ) ] = Player->FastRun;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "RunSpeed" ) ] = Player->RunSpeed;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InfiniteStamina" ) ] = Player->InfiniteStamina;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "WeaponOptions" ) ] = Player->WeaponOptions;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoRecoilEnabled" ) ] = Player->NoRecoilEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "RecoilValue" ) ] = Player->RecoilValue;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoSpreadEnabled" ) ] = Player->NoSpreadEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SpreadValue" ) ] = Player->SpreadValue;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InfiniteAmmoEnabled" ) ] = Player->InfiniteAmmoEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoReloadEnabled" ) ] = Player->NoReloadEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoClipEnabled" ) ] = Player->NoClipEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoClipKey" ) ] = Player->NoClipKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoClipSpeed" ) ] = Player->NoClipSpeed;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "HandlingEditor" ) ] = Player->HandlingEditor;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InfiniteCombatRoll" ) ] = Player->InfiniteCombatRoll;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "EnableGodMode" ) ] = Player->EnableGodMode;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "GodModeKey" ) ] = Player->GodModeKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "VehicleGodMode" ) ] = Player->VehicleGodMode;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SeatBelt" ) ] = Player->SeatBelt;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ForceWeaponWheel" ) ] = Player->ForceWeaponWheel;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ShrinkEnabled" ) ] = Player->ShrinkEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ShrinkScale" ) ] = Player->ShrinkScale;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "BigPedEnabled" ) ] = Player->BigPedEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "BigPedScale" ) ] = Player->BigPedScale;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoRagDollEnabled" ) ] = Player->NoRagDollEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AntiHSEnabled" ) ] = Player->AntiHSEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "KillAllPlayers" ) ] = Player->KillAllPlayers;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "StealCarEnabled" ) ] = Player->StealCarEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCam" ) ] = Player->FreeCam;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamSpeed" ) ] = Player->FreeCamSpeed;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamKey" ) ] = Player->FreeCamKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamTeleport" ) ] = Player->FreeCamTeleport;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamTeleportKey" ) ] = Player->FreeCamTeleportKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamLockControls" ) ] = Player->FreeCamLockControls;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamLockControlsKey" ) ] = Player->FreeCamLockControlsKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SpectateEnabled" ) ] = Player->SpectateEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SpectateTargetIndex" ) ] = Player->SpectateTargetIndex;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ExplosiveAmmoEnabled" ) ] = Player->ExplosiveAmmoEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FireAmmoEnabled" ) ] = Player->FireAmmoEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "DamageBoost" ) ] = Player->DamageBoost;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "Boost" ) ] = Player->Boost;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "BeastJumpEnabled" ) ] = Player->BeastJumpEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SuperJumpEnabled" ) ] = Player->SuperJumpEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SuperFistEnabled" ) ] = Player->SuperFistEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ExplosiveFistEnabled" ) ] = Player->ExplosiveFistEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InvisibleEnabled" ) ] = Player->InvisibleEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoHealEnabled" ) ] = Player->AutoHealEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoHealThreshold" ) ] = Player->AutoHealThreshold;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoArmorEnabled" ) ] = Player->AutoArmorEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoArmorThreshold" ) ] = Player->AutoArmorThreshold;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "CustomFovEnabled" ) ] = Player->CustomFovEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FovValue" ) ] = Player->FovValue;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SwimSpeed" ) ] = Player->SwimSpeed;

				std::string CfgJsonStr = CfgJson.dump( );
				std::string code = Utils::EncodeB64( CfgJsonStr );
				Utils::PasteClipboard( code.c_str( ) );
				return xorstr( "Config Exported to Clipboard." );
			}
			catch ( const std::exception & e ) {
				return xorstr( "Failed to save config." );
			}
		}

		nlohmann::json GetCurrentConfigJson( )
		{
			nlohmann::json CfgJson;
				auto& GeneralCfg = CfgJson[ xorstr( "General" ) ];
				auto& FeaturesCfg = CfgJson;
				GeneralCfg[ xorstr( "StreamProof" ) ] = General->StreamProof;
				GeneralCfg[ xorstr( "WaterMark" ) ] = General->WaterMark;
				GeneralCfg[ xorstr( "WaterMarkCol" ) ] = General->WaterMarkCol;
				GeneralCfg[ xorstr( "ArrayList" ) ] = General->ArrayList;
				GeneralCfg[ xorstr( "ArrayListCol" ) ] = General->ArrayListCol;
				GeneralCfg[ xorstr( "VSync" ) ] = General->VSync;
				GeneralCfg[ xorstr( "SecondMonitorDisplay" ) ] = General->SecondMonitorDisplay;
				GeneralCfg[ xorstr( "enableRGBParticles" ) ] = General->enableRGBParticles;
				GeneralCfg[ xorstr( "ProcessPriority" ) ] = General->ProcessPriority;
				GeneralCfg[ xorstr( "MenuKey" ) ] = General->MenuKey;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "Enabled" ) ] = Aimbot->Enabled;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "ShowFov" ) ] = Aimbot->ShowFov;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "OnlyVisible" ) ] = Aimbot->OnlyVisible;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "IgnoreNPCs" ) ] = Aimbot->IgnoreNPCs;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "HitBox" ) ] = Aimbot->HitBox;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "Hitbox" ) ] = Aimbot->Hitbox;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "FOV" ) ] = Aimbot->FOV;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "MaxDistance" ) ] = Aimbot->MaxDistance;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "SmoothHorizontal" ) ] = Aimbot->SmoothHorizontal;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "SmoothVertical" ) ] = Aimbot->SmoothVertical;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "AimbotSpeed" ) ] = Aimbot->AimbotSpeed;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "AimCurving" ) ] = Aimbot->AimCurving;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "CurveStrength" ) ] = Aimbot->CurveStrength;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "KeyBind" ) ] = Aimbot->KeyBind;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "FovColor" ) ] = ImColToJson( Aimbot->FovColor );
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "InVehicleAimbot" ) ] = Aimbot->InVehicleAimbot;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "UsePrediction" ) ] = Aimbot->UsePrediction;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "PredictionTime" ) ] = Aimbot->PredictionTime;
				FeaturesCfg[ xorstr( "Aimbot" ) ][ xorstr( "RandomizeAngle" ) ] = Aimbot->RandomizeAngle;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "Enabled" ) ] = TriggerBot->Enabled;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "ShowFov" ) ] = TriggerBot->ShowFov;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "OnlyVisible" ) ] = TriggerBot->OnlyVisible;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "IgnoreNPCs" ) ] = TriggerBot->IgnoreNPCs;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "SmartTrigger" ) ] = TriggerBot->SmartTrigger;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "FOV" ) ] = TriggerBot->FOV;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "MaxDistance" ) ] = TriggerBot->MaxDistance;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "Delay" ) ] = TriggerBot->Delay;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "KeyBind" ) ] = TriggerBot->KeyBind;
				FeaturesCfg[ xorstr( "TriggerBot" ) ][ xorstr( "FovColor" ) ] = ImColToJson( TriggerBot->FovColor );
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "Enabled" ) ] = SilentAim->Enabled;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "ShowFov" ) ] = SilentAim->ShowFov;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "OnlyVisible" ) ] = SilentAim->OnlyVisible;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "IgnoreNPCs" ) ] = SilentAim->IgnoreNPCs;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "FOV" ) ] = SilentAim->FOV;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "MaxDistance" ) ] = SilentAim->MaxDistance;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "KeyBind" ) ] = SilentAim->KeyBind;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "HitBox" ) ] = SilentAim->Hitbox;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "MissChance" ) ] = SilentAim->MissChance;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "MagicBullets" ) ] = SilentAim->MagicBullets;
				FeaturesCfg[ xorstr( "SilentAim" ) ][ xorstr( "FovColor" ) ] = ImColToJson( SilentAim->FovColor );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Enabled" ) ] = ESP->Enabled;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Box" ) ] = ESP->Box;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FilledBox" ) ] = ESP->FilledBox;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "BoxState" ) ] = ESP->BoxState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Skeleton" ) ] = ESP->Skeleton;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HealthBar" ) ] = ESP->HealthBar;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HealthBarPos" ) ] = nlohmann::json::array( { ESP->HealthBarPos.x, ESP->HealthBarPos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HealthBarState" ) ] = ESP->HealthBarState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ArmorBar" ) ] = ESP->ArmorBar;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ArmorBarPos" ) ] = nlohmann::json::array( { ESP->ArmorBarPos.x, ESP->ArmorBarPos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ArmorBarState" ) ] = ESP->ArmorBarState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponName" ) ] = ESP->WeaponName;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponNamePos" ) ] = nlohmann::json::array( { ESP->WeaponNamePos.x, ESP->WeaponNamePos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponNameState" ) ] = ESP->WeaponNameState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "SnapLines" ) ] = ESP->SnapLines;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNames" ) ] = ESP->UserNames;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNamesPos" ) ] = nlohmann::json::array( { ESP->UserNamesPos.x, ESP->UserNamesPos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNamesState" ) ] = ESP->UserNamesState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HeadCircle" ) ] = ESP->HeadCircle;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "IgnoreNPCs" ) ] = ESP->IgnoreNPCs;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "HighlightVisible" ) ] = ESP->HighlightVisible;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "IgnoreDead" ) ] = ESP->IgnoreDead;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceFromMe" ) ] = ESP->DistanceFromMe;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceFromMePos" ) ] = nlohmann::json::array( { ESP->DistanceFromMePos.x, ESP->DistanceFromMePos.y } );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceFromMeState" ) ] = ESP->DistanceFromMeState;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "MaxDistance" ) ] = ESP->MaxDistance;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "ShowLocalPlayer" ) ] = ESP->ShowLocalPlayer;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FriendsMarker" ) ] = ESP->FriendsMarker;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FriendsMarkerBind" ) ] = ESP->FriendsMarkerBind;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "DistanceCol" ) ] = ImColToJson( ESP->DistanceCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "UserNamesCol" ) ] = ImColToJson( ESP->UserNamesCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "WeaponNameCol" ) ] = ImColToJson( ESP->WeaponNameCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "SkeletonCol" ) ] = ImColToJson( ESP->SkeletonCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "BoxCol" ) ] = ImColToJson( ESP->BoxCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FilledBoxCol" ) ] = ImColToJson( ESP->FilledBoxCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "SnapLinesCol" ) ] = ImColToJson( ESP->SnapLinesCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "FriendCol" ) ] = ImColToJson( ESP->FriendCol );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "Tracer" ) ] = ESP->Tracer;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "TracerThickness" ) ] = ESP->TracerThickness;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "TracerDuration" ) ] = ESP->TracerDuration;
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "TracerColor" ) ] = ImColToJson( ESP->TracerColor );
				FeaturesCfg[ xorstr( "ESP" ) ][ xorstr( "KeyBind" ) ] = ESP->KeyBind;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "Enabled" ) ] = VehicleESP->Enabled;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "SnapLines" ) ] = VehicleESP->SnapLines;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "ShowLockUnlock" ) ] = VehicleESP->ShowLockUnlock;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "VehName" ) ] = VehicleESP->VehName;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "DistanceFromMe" ) ] = VehicleESP->DistanceFromMe;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "MaxDistance" ) ] = VehicleESP->MaxDistance;
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "SnapLinesCol" ) ] = ImColToJson( VehicleESP->SnapLinesCol );
				FeaturesCfg[ xorstr( "VehicleESP" ) ][ xorstr( "KeyBind" ) ] = VehicleESP->KeyBind;

				FeaturesCfg[ xorstr( "ObjectESP" ) ][ xorstr( "Enabled" ) ] = ObjectESP->Enabled;
				FeaturesCfg[ xorstr( "ObjectESP" ) ][ xorstr( "ObjName" ) ] = ObjectESP->ObjName;
				FeaturesCfg[ xorstr( "ObjectESP" ) ][ xorstr( "DistanceFromMe" ) ] = ObjectESP->DistanceFromMe;
				FeaturesCfg[ xorstr( "ObjectESP" ) ][ xorstr( "MaxDistance" ) ] = ObjectESP->MaxDistance;
				FeaturesCfg[ xorstr( "ObjectESP" ) ][ xorstr( "NameCol" ) ] = ImColToJson( ObjectESP->NameCol );

				FeaturesCfg[ xorstr( "Vehicle" ) ][ xorstr( "PrimaryColorIdx" ) ] = Vehicle->PrimaryColorIdx;
				FeaturesCfg[ xorstr( "Vehicle" ) ][ xorstr( "SecondaryColorIdx" ) ] = Vehicle->SecondaryColorIdx;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FastRun" ) ] = Player->FastRun;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "RunSpeed" ) ] = Player->RunSpeed;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InfiniteStamina" ) ] = Player->InfiniteStamina;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "WeaponOptions" ) ] = Player->WeaponOptions;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoRecoilEnabled" ) ] = Player->NoRecoilEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "RecoilValue" ) ] = Player->RecoilValue;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoSpreadEnabled" ) ] = Player->NoSpreadEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SpreadValue" ) ] = Player->SpreadValue;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InfiniteAmmoEnabled" ) ] = Player->InfiniteAmmoEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoReloadEnabled" ) ] = Player->NoReloadEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoClipEnabled" ) ] = Player->NoClipEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoClipKey" ) ] = Player->NoClipKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoClipSpeed" ) ] = Player->NoClipSpeed;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "HandlingEditor" ) ] = Player->HandlingEditor;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InfiniteCombatRoll" ) ] = Player->InfiniteCombatRoll;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "EnableGodMode" ) ] = Player->EnableGodMode;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "GodModeKey" ) ] = Player->GodModeKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "VehicleGodMode" ) ] = Player->VehicleGodMode;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SeatBelt" ) ] = Player->SeatBelt;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ForceWeaponWheel" ) ] = Player->ForceWeaponWheel;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ShrinkEnabled" ) ] = Player->ShrinkEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ShrinkScale" ) ] = Player->ShrinkScale;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "BigPedEnabled" ) ] = Player->BigPedEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "BigPedScale" ) ] = Player->BigPedScale;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "NoRagDollEnabled" ) ] = Player->NoRagDollEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AntiHSEnabled" ) ] = Player->AntiHSEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "KillAllPlayers" ) ] = Player->KillAllPlayers;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "StealCarEnabled" ) ] = Player->StealCarEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCam" ) ] = Player->FreeCam;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamSpeed" ) ] = Player->FreeCamSpeed;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamKey" ) ] = Player->FreeCamKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamTeleport" ) ] = Player->FreeCamTeleport;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamTeleportKey" ) ] = Player->FreeCamTeleportKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamLockControls" ) ] = Player->FreeCamLockControls;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FreeCamLockControlsKey" ) ] = Player->FreeCamLockControlsKey;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SpectateEnabled" ) ] = Player->SpectateEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SpectateTargetIndex" ) ] = Player->SpectateTargetIndex;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ExplosiveAmmoEnabled" ) ] = Player->ExplosiveAmmoEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FireAmmoEnabled" ) ] = Player->FireAmmoEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "DamageBoost" ) ] = Player->DamageBoost;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "Boost" ) ] = Player->Boost;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "BeastJumpEnabled" ) ] = Player->BeastJumpEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SuperJumpEnabled" ) ] = Player->SuperJumpEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SuperFistEnabled" ) ] = Player->SuperFistEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "ExplosiveFistEnabled" ) ] = Player->ExplosiveFistEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "InvisibleEnabled" ) ] = Player->InvisibleEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoHealEnabled" ) ] = Player->AutoHealEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoHealThreshold" ) ] = Player->AutoHealThreshold;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoArmorEnabled" ) ] = Player->AutoArmorEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "AutoArmorThreshold" ) ] = Player->AutoArmorThreshold;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "CustomFovEnabled" ) ] = Player->CustomFovEnabled;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "FovValue" ) ] = Player->FovValue;
				FeaturesCfg[ xorstr( "Player" ) ][ xorstr( "SwimSpeed" ) ] = Player->SwimSpeed;
				return CfgJson;
		}

		std::string GetCurrentConfigCode( const nlohmann::json* merge_extra = nullptr )
		{
			try {
				nlohmann::json j = GetCurrentConfigJson( );
				j.update( ConfigTheme::GetThemeConfigJson( ) );
				if ( merge_extra && !merge_extra->empty( ) )
					j.update( *merge_extra );
				std::string CfgJsonStr = j.dump( );
				return Utils::EncodeB64( CfgJsonStr );
			}
			catch ( const std::exception & ) { return ""; }
		}

		std::string LoadCfg( std::string CfgName, std::string CfgCode, nlohmann::json* out_parsed = nullptr )
		{
			try {
				std::string jsonStr;
				CfgCode.erase( 0, CfgCode.find_first_not_of( " \t\r\n" ) );
				CfgCode.erase( CfgCode.find_last_not_of( " \t\r\n" ) + 1 );
				if ( CfgCode.size( ) >= 2 && CfgCode[ 0 ] == '{' ) {
					jsonStr = CfgCode;
				} else {
					jsonStr = Utils::DecodeB64( CfgCode );
					if ( jsonStr.empty( ) || ( jsonStr.size( ) >= 1 && jsonStr[ 0 ] != '{' ) ) {
						jsonStr = Utils::DecodeB64( Utils::Hex2Str( Utils::DecodeB64( CfgCode ) ) );
					}
				}
				nlohmann::json CfgJson = nlohmann::json::parse( jsonStr );

				nlohmann::json dummyGeneral;
				auto& GeneralCfg = CfgJson.contains( xorstr( "General" ) ) ? CfgJson[ xorstr( "General" ) ] : dummyGeneral;
				auto& FeaturesCfg = CfgJson.contains( xorstr( "Features" ) ) ? CfgJson[ xorstr( "Features" ) ] : CfgJson;

				if ( CfgJson.contains( xorstr( "General" ) ) ) {
					if ( GeneralCfg.contains( xorstr( "StreamProof" ) ) ) General->StreamProof = GeneralCfg[ xorstr( "StreamProof" ) ];
					if ( GeneralCfg.contains( xorstr( "WaterMark" ) ) ) General->WaterMark = GeneralCfg[ xorstr( "WaterMark" ) ];
					if ( GeneralCfg.contains( xorstr( "WaterMarkCol" ) ) ) General->WaterMarkCol = GeneralCfg[ xorstr( "WaterMarkCol" ) ];
					if ( GeneralCfg.contains( xorstr( "ArrayList" ) ) ) General->ArrayList = GeneralCfg[ xorstr( "ArrayList" ) ];
					if ( GeneralCfg.contains( xorstr( "ArrayListCol" ) ) ) General->ArrayListCol = GeneralCfg[ xorstr( "ArrayListCol" ) ];
					if ( GeneralCfg.contains( xorstr( "VSync" ) ) ) General->VSync = GeneralCfg[ xorstr( "VSync" ) ];
					if ( GeneralCfg.contains( xorstr( "SecondMonitorDisplay" ) ) ) General->SecondMonitorDisplay = GeneralCfg[ xorstr( "SecondMonitorDisplay" ) ];
					if ( GeneralCfg.contains( xorstr( "enableRGBParticles" ) ) ) General->enableRGBParticles = GeneralCfg[ xorstr( "enableRGBParticles" ) ];
					if ( GeneralCfg.contains( xorstr( "ProcessPriority" ) ) ) General->ProcessPriority = GeneralCfg[ xorstr( "ProcessPriority" ) ];
					if ( GeneralCfg.contains( xorstr( "MenuKey" ) ) ) General->MenuKey = GeneralCfg[ xorstr( "MenuKey" ) ];
				}

				if ( FeaturesCfg.contains( xorstr( "Aimbot" ) ) ) {
					auto& a = FeaturesCfg[ xorstr( "Aimbot" ) ];
					if ( a.contains( xorstr( "Enabled" ) ) ) Aimbot->Enabled = a[ xorstr( "Enabled" ) ];
					if ( a.contains( xorstr( "ShowFov" ) ) ) Aimbot->ShowFov = a[ xorstr( "ShowFov" ) ];
					if ( a.contains( xorstr( "OnlyVisible" ) ) ) Aimbot->OnlyVisible = a[ xorstr( "OnlyVisible" ) ];
					if ( a.contains( xorstr( "IgnoreNPCs" ) ) ) Aimbot->IgnoreNPCs = a[ xorstr( "IgnoreNPCs" ) ];
					if ( a.contains( xorstr( "HitBox" ) ) ) Aimbot->HitBox = a[ xorstr( "HitBox" ) ];
					if ( a.contains( xorstr( "Hitbox" ) ) ) Aimbot->Hitbox = a[ xorstr( "Hitbox" ) ];
					if ( a.contains( xorstr( "FOV" ) ) ) Aimbot->FOV = a[ xorstr( "FOV" ) ];
					if ( a.contains( xorstr( "MaxDistance" ) ) ) Aimbot->MaxDistance = a[ xorstr( "MaxDistance" ) ];
					if ( a.contains( xorstr( "SmoothHorizontal" ) ) ) Aimbot->SmoothHorizontal = a[ xorstr( "SmoothHorizontal" ) ];
					if ( a.contains( xorstr( "SmoothVertical" ) ) ) Aimbot->SmoothVertical = a[ xorstr( "SmoothVertical" ) ];
					if ( a.contains( xorstr( "AimbotSpeed" ) ) ) Aimbot->AimbotSpeed = a[ xorstr( "AimbotSpeed" ) ];
					if ( a.contains( xorstr( "AimSpeed" ) ) ) Aimbot->AimbotSpeed = a[ xorstr( "AimSpeed" ) ];
					if ( a.contains( xorstr( "AimCurving" ) ) ) Aimbot->AimCurving = a[ xorstr( "AimCurving" ) ];
					if ( a.contains( xorstr( "CurveStrength" ) ) ) Aimbot->CurveStrength = a[ xorstr( "CurveStrength" ) ];
					if ( a.contains( xorstr( "KeyBind" ) ) ) Aimbot->KeyBind = a[ xorstr( "KeyBind" ) ];
					if ( a.contains( xorstr( "FovColor" ) ) ) Aimbot->FovColor = JsonToImCol( a[ xorstr( "FovColor" ) ] );
					if ( a.contains( xorstr( "InVehicleAimbot" ) ) ) Aimbot->InVehicleAimbot = a[ xorstr( "InVehicleAimbot" ) ];
					if ( a.contains( xorstr( "UsePrediction" ) ) ) Aimbot->UsePrediction = a[ xorstr( "UsePrediction" ) ];
					if ( a.contains( xorstr( "PredictionTime" ) ) ) Aimbot->PredictionTime = a[ xorstr( "PredictionTime" ) ];
					if ( a.contains( xorstr( "RandomizeAngle" ) ) ) Aimbot->RandomizeAngle = a[ xorstr( "RandomizeAngle" ) ];
				}
				if ( FeaturesCfg.contains( xorstr( "TriggerBot" ) ) ) {
					auto& t = FeaturesCfg[ xorstr( "TriggerBot" ) ];
					if ( t.contains( xorstr( "Enabled" ) ) ) TriggerBot->Enabled = t[ xorstr( "Enabled" ) ];
					if ( t.contains( xorstr( "ShowFov" ) ) ) TriggerBot->ShowFov = t[ xorstr( "ShowFov" ) ];
					if ( t.contains( xorstr( "OnlyVisible" ) ) ) TriggerBot->OnlyVisible = t[ xorstr( "OnlyVisible" ) ];
					if ( t.contains( xorstr( "IgnoreNPCs" ) ) ) TriggerBot->IgnoreNPCs = t[ xorstr( "IgnoreNPCs" ) ];
					if ( t.contains( xorstr( "SmartTrigger" ) ) ) TriggerBot->SmartTrigger = t[ xorstr( "SmartTrigger" ) ];
					if ( t.contains( xorstr( "FOV" ) ) ) TriggerBot->FOV = t[ xorstr( "FOV" ) ];
					if ( t.contains( xorstr( "MaxDistance" ) ) ) TriggerBot->MaxDistance = t[ xorstr( "MaxDistance" ) ];
					if ( t.contains( xorstr( "Delay" ) ) ) TriggerBot->Delay = t[ xorstr( "Delay" ) ];
					if ( t.contains( xorstr( "KeyBind" ) ) ) TriggerBot->KeyBind = t[ xorstr( "KeyBind" ) ];
					if ( t.contains( xorstr( "FovColor" ) ) ) TriggerBot->FovColor = JsonToImCol( t[ xorstr( "FovColor" ) ] );
				}
				if ( FeaturesCfg.contains( xorstr( "SilentAim" ) ) ) {
					auto& s = FeaturesCfg[ xorstr( "SilentAim" ) ];
					if ( s.contains( xorstr( "Enabled" ) ) ) SilentAim->Enabled = s[ xorstr( "Enabled" ) ];
					if ( s.contains( xorstr( "ShowFov" ) ) ) SilentAim->ShowFov = s[ xorstr( "ShowFov" ) ];
					if ( s.contains( xorstr( "OnlyVisible" ) ) ) SilentAim->OnlyVisible = s[ xorstr( "OnlyVisible" ) ];
					if ( s.contains( xorstr( "IgnoreNPCs" ) ) ) SilentAim->IgnoreNPCs = s[ xorstr( "IgnoreNPCs" ) ];
					if ( s.contains( xorstr( "FOV" ) ) ) SilentAim->FOV = s[ xorstr( "FOV" ) ];
					if ( s.contains( xorstr( "MaxDistance" ) ) ) SilentAim->MaxDistance = s[ xorstr( "MaxDistance" ) ];
					if ( s.contains( xorstr( "KeyBind" ) ) ) SilentAim->KeyBind = s[ xorstr( "KeyBind" ) ];
					if ( s.contains( xorstr( "HitBox" ) ) ) SilentAim->Hitbox = s[ xorstr( "HitBox" ) ];
					if ( s.contains( xorstr( "MissChance" ) ) ) SilentAim->MissChance = s[ xorstr( "MissChance" ) ];
					if ( s.contains( xorstr( "MagicBullets" ) ) ) SilentAim->MagicBullets = s[ xorstr( "MagicBullets" ) ];
					if ( s.contains( xorstr( "FovColor" ) ) ) SilentAim->FovColor = JsonToImCol( s[ xorstr( "FovColor" ) ] );
				}

				if ( FeaturesCfg.contains( xorstr( "ESP" ) ) ) {
					auto& e = FeaturesCfg[ xorstr( "ESP" ) ];
					if ( e.contains( xorstr( "Enabled" ) ) ) ESP->Enabled = e[ xorstr( "Enabled" ) ];
					if ( e.contains( xorstr( "Box" ) ) ) ESP->Box = e[ xorstr( "Box" ) ];
					if ( e.contains( xorstr( "FilledBox" ) ) ) ESP->FilledBox = e[ xorstr( "FilledBox" ) ];
					if ( e.contains( xorstr( "BoxState" ) ) ) ESP->BoxState = e[ xorstr( "BoxState" ) ];
					if ( e.contains( xorstr( "Skeleton" ) ) ) ESP->Skeleton = e[ xorstr( "Skeleton" ) ];
					if ( e.contains( xorstr( "HealthBar" ) ) ) ESP->HealthBar = e[ xorstr( "HealthBar" ) ];
					if ( e.contains( xorstr( "HealthBarPos" ) ) ) ESP->HealthBarPos = JsonToImVec2( e[ xorstr( "HealthBarPos" ) ] );
					if ( e.contains( xorstr( "HealthBarState" ) ) ) ESP->HealthBarState = e[ xorstr( "HealthBarState" ) ];
					if ( e.contains( xorstr( "ArmorBar" ) ) ) ESP->ArmorBar = e[ xorstr( "ArmorBar" ) ];
					if ( e.contains( xorstr( "ArmorBarPos" ) ) ) ESP->ArmorBarPos = JsonToImVec2( e[ xorstr( "ArmorBarPos" ) ] );
					if ( e.contains( xorstr( "ArmorBarState" ) ) ) ESP->ArmorBarState = e[ xorstr( "ArmorBarState" ) ];
					if ( e.contains( xorstr( "WeaponName" ) ) ) ESP->WeaponName = e[ xorstr( "WeaponName" ) ];
					if ( e.contains( xorstr( "WeaponNamePos" ) ) ) ESP->WeaponNamePos = JsonToImVec2( e[ xorstr( "WeaponNamePos" ) ] );
					if ( e.contains( xorstr( "WeaponNameState" ) ) ) ESP->WeaponNameState = e[ xorstr( "WeaponNameState" ) ];
					if ( e.contains( xorstr( "SnapLines" ) ) ) ESP->SnapLines = e[ xorstr( "SnapLines" ) ];
					if ( e.contains( xorstr( "UserNames" ) ) ) ESP->UserNames = e[ xorstr( "UserNames" ) ];
					if ( e.contains( xorstr( "UserNamesPos" ) ) ) ESP->UserNamesPos = JsonToImVec2( e[ xorstr( "UserNamesPos" ) ] );
					if ( e.contains( xorstr( "UserNamesState" ) ) ) ESP->UserNamesState = e[ xorstr( "UserNamesState" ) ];
					if ( e.contains( xorstr( "HeadCircle" ) ) ) ESP->HeadCircle = e[ xorstr( "HeadCircle" ) ];
					if ( e.contains( xorstr( "IgnoreNPCs" ) ) ) ESP->IgnoreNPCs = e[ xorstr( "IgnoreNPCs" ) ];
					if ( e.contains( xorstr( "ShowLocalPlayer" ) ) ) ESP->ShowLocalPlayer = e[ xorstr( "ShowLocalPlayer" ) ];
					if ( e.contains( xorstr( "HighlightVisible" ) ) ) ESP->HighlightVisible = e[ xorstr( "HighlightVisible" ) ];
					if ( e.contains( xorstr( "IgnoreDead" ) ) ) ESP->IgnoreDead = e[ xorstr( "IgnoreDead" ) ];
					if ( e.contains( xorstr( "DistanceFromMe" ) ) ) ESP->DistanceFromMe = e[ xorstr( "DistanceFromMe" ) ];
					if ( e.contains( xorstr( "DistanceFromMePos" ) ) ) ESP->DistanceFromMePos = JsonToImVec2( e[ xorstr( "DistanceFromMePos" ) ] );
					if ( e.contains( xorstr( "DistanceFromMeState" ) ) ) ESP->DistanceFromMeState = e[ xorstr( "DistanceFromMeState" ) ];
					if ( e.contains( xorstr( "MaxDistance" ) ) ) ESP->MaxDistance = e[ xorstr( "MaxDistance" ) ];
					if ( e.contains( xorstr( "FriendsMarker" ) ) ) ESP->FriendsMarker = e[ xorstr( "FriendsMarker" ) ];
					if ( e.contains( xorstr( "FriendsMarkerBind" ) ) ) ESP->FriendsMarkerBind = e[ xorstr( "FriendsMarkerBind" ) ];
					if ( e.contains( xorstr( "DistanceCol" ) ) ) ESP->DistanceCol = JsonToImCol( e[ xorstr( "DistanceCol" ) ] );
					if ( e.contains( xorstr( "UserNamesCol" ) ) ) ESP->UserNamesCol = JsonToImCol( e[ xorstr( "UserNamesCol" ) ] );
					if ( e.contains( xorstr( "WeaponNameCol" ) ) ) ESP->WeaponNameCol = JsonToImCol( e[ xorstr( "WeaponNameCol" ) ] );
					if ( e.contains( xorstr( "SkeletonCol" ) ) ) ESP->SkeletonCol = JsonToImCol( e[ xorstr( "SkeletonCol" ) ] );
					if ( e.contains( xorstr( "BoxCol" ) ) ) ESP->BoxCol = JsonToImCol( e[ xorstr( "BoxCol" ) ] );
					if ( e.contains( xorstr( "FilledBoxCol" ) ) ) ESP->FilledBoxCol = JsonToImCol( e[ xorstr( "FilledBoxCol" ) ] );
					if ( e.contains( xorstr( "SnapLinesCol" ) ) ) ESP->SnapLinesCol = JsonToImCol( e[ xorstr( "SnapLinesCol" ) ] );
					if ( e.contains( xorstr( "FriendCol" ) ) ) ESP->FriendCol = JsonToImCol( e[ xorstr( "FriendCol" ) ] );
					if ( e.contains( xorstr( "Tracer" ) ) ) ESP->Tracer = e[ xorstr( "Tracer" ) ];
					if ( e.contains( xorstr( "TracerThickness" ) ) ) ESP->TracerThickness = e[ xorstr( "TracerThickness" ) ];
					if ( e.contains( xorstr( "TracerDuration" ) ) ) ESP->TracerDuration = e[ xorstr( "TracerDuration" ) ];
					if ( e.contains( xorstr( "TracerColor" ) ) ) ESP->TracerColor = JsonToImCol( e[ xorstr( "TracerColor" ) ] );
					if ( e.contains( xorstr( "KeyBind" ) ) ) ESP->KeyBind = e[ xorstr( "KeyBind" ) ];
				}

				if ( FeaturesCfg.contains( xorstr( "VehicleESP" ) ) ) {
					auto& v = FeaturesCfg[ xorstr( "VehicleESP" ) ];
					if ( v.contains( xorstr( "Enabled" ) ) ) VehicleESP->Enabled = v[ xorstr( "Enabled" ) ];
					if ( v.contains( xorstr( "SnapLines" ) ) ) VehicleESP->SnapLines = v[ xorstr( "SnapLines" ) ];
					if ( v.contains( xorstr( "ShowLockUnlock" ) ) ) VehicleESP->ShowLockUnlock = v[ xorstr( "ShowLockUnlock" ) ];
					if ( v.contains( xorstr( "VehName" ) ) ) VehicleESP->VehName = v[ xorstr( "VehName" ) ];
					if ( v.contains( xorstr( "DistanceFromMe" ) ) ) VehicleESP->DistanceFromMe = v[ xorstr( "DistanceFromMe" ) ];
					if ( v.contains( xorstr( "MaxDistance" ) ) ) VehicleESP->MaxDistance = v[ xorstr( "MaxDistance" ) ];
					if ( v.contains( xorstr( "SnapLinesCol" ) ) ) VehicleESP->SnapLinesCol = JsonToImCol( v[ xorstr( "SnapLinesCol" ) ] );
					if ( v.contains( xorstr( "KeyBind" ) ) ) VehicleESP->KeyBind = v[ xorstr( "KeyBind" ) ];
				}

				if ( FeaturesCfg.contains( xorstr( "ObjectESP" ) ) ) {
					auto& o = FeaturesCfg[ xorstr( "ObjectESP" ) ];
					if ( o.contains( xorstr( "Enabled" ) ) ) ObjectESP->Enabled = o[ xorstr( "Enabled" ) ];
					if ( o.contains( xorstr( "ObjName" ) ) ) ObjectESP->ObjName = o[ xorstr( "ObjName" ) ];
					if ( o.contains( xorstr( "DistanceFromMe" ) ) ) ObjectESP->DistanceFromMe = o[ xorstr( "DistanceFromMe" ) ];
					if ( o.contains( xorstr( "MaxDistance" ) ) ) ObjectESP->MaxDistance = o[ xorstr( "MaxDistance" ) ];
					if ( o.contains( xorstr( "NameCol" ) ) ) ObjectESP->NameCol = JsonToImCol( o[ xorstr( "NameCol" ) ] );
				}

				if ( FeaturesCfg.contains( xorstr( "Vehicle" ) ) ) {
					auto& v = FeaturesCfg[ xorstr( "Vehicle" ) ];
					if ( v.contains( xorstr( "PrimaryColorIdx" ) ) ) Vehicle->PrimaryColorIdx = v[ xorstr( "PrimaryColorIdx" ) ];
					if ( v.contains( xorstr( "SecondaryColorIdx" ) ) ) Vehicle->SecondaryColorIdx = v[ xorstr( "SecondaryColorIdx" ) ];
				}
				if ( FeaturesCfg.contains( xorstr( "Player" ) ) ) {
					auto& p = FeaturesCfg[ xorstr( "Player" ) ];
					if ( p.contains( xorstr( "FastRun" ) ) ) Player->FastRun = p[ xorstr( "FastRun" ) ];
					if ( p.contains( xorstr( "RunSpeed" ) ) ) Player->RunSpeed = p[ xorstr( "RunSpeed" ) ];
					if ( p.contains( xorstr( "InfiniteStamina" ) ) ) Player->InfiniteStamina = p[ xorstr( "InfiniteStamina" ) ];
					if ( p.contains( xorstr( "WeaponOptions" ) ) ) Player->WeaponOptions = p[ xorstr( "WeaponOptions" ) ];
					if ( p.contains( xorstr( "NoRecoilEnabled" ) ) ) Player->NoRecoilEnabled = p[ xorstr( "NoRecoilEnabled" ) ];
					if ( p.contains( xorstr( "RecoilValue" ) ) ) Player->RecoilValue = p[ xorstr( "RecoilValue" ) ];
					if ( p.contains( xorstr( "NoSpreadEnabled" ) ) ) Player->NoSpreadEnabled = p[ xorstr( "NoSpreadEnabled" ) ];
					if ( p.contains( xorstr( "SpreadValue" ) ) ) Player->SpreadValue = p[ xorstr( "SpreadValue" ) ];
					if ( p.contains( xorstr( "InfiniteAmmoEnabled" ) ) ) Player->InfiniteAmmoEnabled = p[ xorstr( "InfiniteAmmoEnabled" ) ];
					if ( p.contains( xorstr( "NoReloadEnabled" ) ) ) Player->NoReloadEnabled = p[ xorstr( "NoReloadEnabled" ) ];
					if ( p.contains( xorstr( "NoClipEnabled" ) ) ) Player->NoClipEnabled = p[ xorstr( "NoClipEnabled" ) ];
					if ( p.contains( xorstr( "NoClipKey" ) ) ) Player->NoClipKey = p[ xorstr( "NoClipKey" ) ];
					if ( p.contains( xorstr( "NoClipSpeed" ) ) ) Player->NoClipSpeed = p[ xorstr( "NoClipSpeed" ) ];
					if ( p.contains( xorstr( "HandlingEditor" ) ) ) Player->HandlingEditor = p[ xorstr( "HandlingEditor" ) ];
					if ( p.contains( xorstr( "InfiniteCombatRoll" ) ) ) Player->InfiniteCombatRoll = p[ xorstr( "InfiniteCombatRoll" ) ];
					if ( p.contains( xorstr( "EnableGodMode" ) ) ) Player->EnableGodMode = p[ xorstr( "EnableGodMode" ) ];
					if ( p.contains( xorstr( "GodModeKey" ) ) ) Player->GodModeKey = p[ xorstr( "GodModeKey" ) ];
					if ( p.contains( xorstr( "VehicleGodMode" ) ) ) Player->VehicleGodMode = p[ xorstr( "VehicleGodMode" ) ];
					if ( p.contains( xorstr( "SeatBelt" ) ) ) Player->SeatBelt = p[ xorstr( "SeatBelt" ) ];
					if ( p.contains( xorstr( "ForceWeaponWheel" ) ) ) Player->ForceWeaponWheel = p[ xorstr( "ForceWeaponWheel" ) ];
					if ( p.contains( xorstr( "ShrinkEnabled" ) ) ) Player->ShrinkEnabled = p[ xorstr( "ShrinkEnabled" ) ];
					if ( p.contains( xorstr( "ShrinkScale" ) ) ) Player->ShrinkScale = p[ xorstr( "ShrinkScale" ) ];
					if ( p.contains( xorstr( "BigPedEnabled" ) ) ) Player->BigPedEnabled = p[ xorstr( "BigPedEnabled" ) ];
					if ( p.contains( xorstr( "BigPedScale" ) ) ) Player->BigPedScale = p[ xorstr( "BigPedScale" ) ];
					if ( p.contains( xorstr( "NoRagDollEnabled" ) ) ) Player->NoRagDollEnabled = p[ xorstr( "NoRagDollEnabled" ) ];
					if ( p.contains( xorstr( "AntiHSEnabled" ) ) ) Player->AntiHSEnabled = p[ xorstr( "AntiHSEnabled" ) ];
					if ( p.contains( xorstr( "KillAllPlayers" ) ) ) Player->KillAllPlayers = p[ xorstr( "KillAllPlayers" ) ];
					if ( p.contains( xorstr( "StealCarEnabled" ) ) ) Player->StealCarEnabled = p[ xorstr( "StealCarEnabled" ) ];
					if ( p.contains( xorstr( "FreeCam" ) ) ) Player->FreeCam = p[ xorstr( "FreeCam" ) ];
					if ( p.contains( xorstr( "FreeCamSpeed" ) ) ) Player->FreeCamSpeed = p[ xorstr( "FreeCamSpeed" ) ];
					if ( p.contains( xorstr( "FreeCamKey" ) ) ) Player->FreeCamKey = p[ xorstr( "FreeCamKey" ) ];
					if ( p.contains( xorstr( "FreeCamTeleport" ) ) ) Player->FreeCamTeleport = p[ xorstr( "FreeCamTeleport" ) ];
					if ( p.contains( xorstr( "FreeCamTeleportKey" ) ) ) Player->FreeCamTeleportKey = p[ xorstr( "FreeCamTeleportKey" ) ];
					if ( p.contains( xorstr( "FreeCamLockControls" ) ) ) Player->FreeCamLockControls = p[ xorstr( "FreeCamLockControls" ) ];
					if ( p.contains( xorstr( "FreeCamLockControlsKey" ) ) ) Player->FreeCamLockControlsKey = p[ xorstr( "FreeCamLockControlsKey" ) ];
					if ( p.contains( xorstr( "SpectateEnabled" ) ) ) Player->SpectateEnabled = p[ xorstr( "SpectateEnabled" ) ];
					if ( p.contains( xorstr( "SpectateTargetIndex" ) ) ) Player->SpectateTargetIndex = p[ xorstr( "SpectateTargetIndex" ) ];
					if ( p.contains( xorstr( "ExplosiveAmmoEnabled" ) ) ) Player->ExplosiveAmmoEnabled = p[ xorstr( "ExplosiveAmmoEnabled" ) ];
					if ( p.contains( xorstr( "FireAmmoEnabled" ) ) ) Player->FireAmmoEnabled = p[ xorstr( "FireAmmoEnabled" ) ];
					if ( p.contains( xorstr( "DamageBoost" ) ) ) Player->DamageBoost = p[ xorstr( "DamageBoost" ) ];
					if ( p.contains( xorstr( "Boost" ) ) ) Player->Boost = p[ xorstr( "Boost" ) ];
					if ( p.contains( xorstr( "BeastJumpEnabled" ) ) ) Player->BeastJumpEnabled = p[ xorstr( "BeastJumpEnabled" ) ];
					if ( p.contains( xorstr( "SuperJumpEnabled" ) ) ) Player->SuperJumpEnabled = p[ xorstr( "SuperJumpEnabled" ) ];
					if ( p.contains( xorstr( "SuperFistEnabled" ) ) ) Player->SuperFistEnabled = p[ xorstr( "SuperFistEnabled" ) ];
					if ( p.contains( xorstr( "ExplosiveFistEnabled" ) ) ) Player->ExplosiveFistEnabled = p[ xorstr( "ExplosiveFistEnabled" ) ];
					if ( p.contains( xorstr( "InvisibleEnabled" ) ) ) Player->InvisibleEnabled = p[ xorstr( "InvisibleEnabled" ) ];
					if ( p.contains( xorstr( "AutoHealEnabled" ) ) ) Player->AutoHealEnabled = p[ xorstr( "AutoHealEnabled" ) ];
					if ( p.contains( xorstr( "AutoHealThreshold" ) ) ) Player->AutoHealThreshold = p[ xorstr( "AutoHealThreshold" ) ];
					if ( p.contains( xorstr( "AutoArmorEnabled" ) ) ) Player->AutoArmorEnabled = p[ xorstr( "AutoArmorEnabled" ) ];
					if ( p.contains( xorstr( "AutoArmorThreshold" ) ) ) Player->AutoArmorThreshold = p[ xorstr( "AutoArmorThreshold" ) ];
					if ( p.contains( xorstr( "CustomFovEnabled" ) ) ) Player->CustomFovEnabled = p[ xorstr( "CustomFovEnabled" ) ];
					if ( p.contains( xorstr( "FovValue" ) ) ) Player->FovValue = p[ xorstr( "FovValue" ) ];
					if ( p.contains( xorstr( "SwimSpeed" ) ) ) Player->SwimSpeed = p[ xorstr( "SwimSpeed" ) ];
				}

				ESP->UpdateCfgESP = true;

				ConfigTheme::ApplyThemeFromConfigJson( CfgJson );

				if ( out_parsed )
					*out_parsed = CfgJson;
				return xorstr( "Config loaded successfully." );
			}
			catch ( const std::exception & ) {
				return xorstr( "Error Loading Config!" );
			}
		}

	};

	inline Config g_Config;

}
