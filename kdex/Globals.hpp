#pragma once
#include <math.h>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"

#include <d3d11.h>
#pragma comment( lib, "d3d11.lib" )

class cColors {
public:
	ImVec4 Base = ImColor( 180, 180, 180 );
	ImVec4 PrimaryText = ImColor( 200, 200, 200 );
	ImVec4 SecundaryText = ImColor( 80, 80, 80 );

	ImVec4 FeaturesText = ImColor( 140, 140, 140 );
	ImVec4 SecundaryFeaturesText = ImColor( 100, 100, 100 );

	ImVec4 BorderCol = ImColor( 32, 32, 34 );
	ImVec4 LinesCol = ImColor( 30, 30, 33 );
	ImVec4 BackgroundCol = ImColor( 12, 12, 14 );

	ImVec4 ChildCol = ImColor( 16, 16, 18 );
	ImVec4 ChildBorderCol = ImColor( 18, 18, 20 );

	ImVec4 TitleBar = ImColor( 15, 15, 18 );
	ImVec4 TitleBarBorder = ImColor( 21, 21, 24 );

	ImVec4 SideBar = ImColor( 14, 14, 16 );
	ImVec4 SideBarBorder = ImColor( 21, 21, 23 );

	ImVec4 ButtonHovered = ImColor( 180, 180, 180, 100 );
	ImVec4 TestingTest = ImColor(180, 180, 180);

	ImVec4 InputBackground = ImColor( 16, 16, 16, 255 );
	ImVec4 InputBorder = ImColor( 22, 22, 22, 255 );

	ImVec4 GetTestingTestSafe() {
		if (!isfinite(TestingTest.x) || !isfinite(TestingTest.y) ||
			!isfinite(TestingTest.z) || !isfinite(TestingTest.w) ||
			TestingTest.w < 0.0f || TestingTest.w > 1.0f) {
			return ImVec4(135/255.0f, 206/255.0f, 235/255.0f, 1.0f);
		}
		return TestingTest;
	}
};

inline cColors g_Col;

class c_globals {
public:
	std::string id;
	std::string version;
	DWORD ProcIdFiveM = 0;
	int Pid;

	bool g_bPassedByThisVerify;
	uintptr_t g_VerifyLogin;
	bool done;
	bool IsOpen;

	std::string ServerIp;
	std::string UserName;
	std::string Role;

	char userLogin[64];
	char passLogin[64];

	char userRegister[64];
	char passRegister[64];
	char licenseRegister[64];

	bool login;
	bool register_;

    bool Logged = false;

	ImVec2 TestePos;

	char m_Config[ 6000 ];
	HWND g_hCheatWindow;
	HWND g_hGameWindow;
	ImVec2 g_vGameWindowSize;
	ImVec2 g_vGameWindowPos;
	ImVec2 g_vGameWindowCenter;

	ImFont * m_FontBig;
	ImFont * m_FontBigSmall;
	ImFont * m_FontNormal;
	ImFont * m_FontSecundary;
	ImFont * m_FontSmaller;
	ImFont * m_DrawFont;
	ImFont * m_Expand;

	ImFont * FontAwesomeSolid;
	ImFont * FontAwesomeSolidSmall;
	ImFont * FontAwesomeRegular;
	ImFont * FontAwesomeBrands;

	ID3D11ShaderResourceView * Logo = nullptr;

	struct FiveM_Weapons_t {
		ID3D11ShaderResourceView * gadget_parachute = nullptr;
		ID3D11ShaderResourceView * weapon_advancedrifle = nullptr;
		ID3D11ShaderResourceView * weapon_appistol = nullptr;
		ID3D11ShaderResourceView * weapon_assaultrifle = nullptr;
		ID3D11ShaderResourceView * weapon_assaultrifle_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_assaultshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_assaultsmg = nullptr;
		ID3D11ShaderResourceView * weapon_autoshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_ball = nullptr;
		ID3D11ShaderResourceView * weapon_bat = nullptr;
		ID3D11ShaderResourceView * weapon_battleaxe = nullptr;
		ID3D11ShaderResourceView * weapon_bottle = nullptr;
		ID3D11ShaderResourceView * weapon_bullpuprifle = nullptr;
		ID3D11ShaderResourceView * weapon_bullpuprifle_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_bullpupshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_bzgas = nullptr;
		ID3D11ShaderResourceView * weapon_carbinerifle = nullptr;
		ID3D11ShaderResourceView * weapon_carbinerifle_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_ceramicpistol = nullptr;
		ID3D11ShaderResourceView * weapon_combatmg = nullptr;
		ID3D11ShaderResourceView * weapon_combatmg_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_combatpdw = nullptr;
		ID3D11ShaderResourceView * weapon_combatpistol = nullptr;
		ID3D11ShaderResourceView * weapon_combatshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_compactlauncher = nullptr;
		ID3D11ShaderResourceView * weapon_compactrifle = nullptr;
		ID3D11ShaderResourceView * weapon_crowbar = nullptr;
		ID3D11ShaderResourceView * weapon_dagger = nullptr;
		ID3D11ShaderResourceView * weapon_dbshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_doubleaction = nullptr;
		ID3D11ShaderResourceView * weapon_fireextinguisher = nullptr;
		ID3D11ShaderResourceView * weapon_firework = nullptr;
		ID3D11ShaderResourceView * weapon_flare = nullptr;
		ID3D11ShaderResourceView * weapon_flaregun = nullptr;
		ID3D11ShaderResourceView * weapon_flashlight = nullptr;
		ID3D11ShaderResourceView * weapon_gadgetpistol = nullptr;
		ID3D11ShaderResourceView * weapon_golfclub = nullptr;
		ID3D11ShaderResourceView * weapon_grenade = nullptr;
		ID3D11ShaderResourceView * weapon_grenadelauncher = nullptr;
		ID3D11ShaderResourceView * weapon_grenadelauncher_smoke = nullptr;
		ID3D11ShaderResourceView * weapon_gusenberg = nullptr;
		ID3D11ShaderResourceView * weapon_hammer = nullptr;
		ID3D11ShaderResourceView * weapon_hatchet = nullptr;
		ID3D11ShaderResourceView * weapon_hazardcan = nullptr;
		ID3D11ShaderResourceView * weapon_heavypistol = nullptr;
		ID3D11ShaderResourceView * weapon_heavyshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_heavysniper = nullptr;
		ID3D11ShaderResourceView * weapon_heavysniper_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_hominglauncher = nullptr;
		ID3D11ShaderResourceView * weapon_knife = nullptr;
		ID3D11ShaderResourceView * weapon_knuckle = nullptr;
		ID3D11ShaderResourceView * weapon_machete = nullptr;
		ID3D11ShaderResourceView * weapon_machinepistol = nullptr;
		ID3D11ShaderResourceView * weapon_marksmanpistol = nullptr;
		ID3D11ShaderResourceView * weapon_marksmanrifle = nullptr;
		ID3D11ShaderResourceView * weapon_marksmanrifle_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_mg = nullptr;
		ID3D11ShaderResourceView * weapon_microsmg = nullptr;
		ID3D11ShaderResourceView * weapon_militaryrifle = nullptr;
		ID3D11ShaderResourceView * weapon_minigun = nullptr;
		ID3D11ShaderResourceView * weapon_minismg = nullptr;
		ID3D11ShaderResourceView * weapon_molotov = nullptr;
		ID3D11ShaderResourceView * weapon_musket = nullptr;
		ID3D11ShaderResourceView * weapon_navyrevolver = nullptr;
		ID3D11ShaderResourceView * weapon_nightstick = nullptr;
		ID3D11ShaderResourceView * weapon_petrolcan = nullptr;
		ID3D11ShaderResourceView * weapon_pipebomb = nullptr;
		ID3D11ShaderResourceView * weapon_pistol = nullptr;
		ID3D11ShaderResourceView * weapon_pistol50 = nullptr;
		ID3D11ShaderResourceView * weapon_pistol_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_poolcue = nullptr;
		ID3D11ShaderResourceView * weapon_proxmine = nullptr;
		ID3D11ShaderResourceView * weapon_pumpshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_pumpshotgun_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_railgun = nullptr;
		ID3D11ShaderResourceView * weapon_raycarbine = nullptr;
		ID3D11ShaderResourceView * weapon_rayminigun = nullptr;
		ID3D11ShaderResourceView * weapon_raypistol = nullptr;
		ID3D11ShaderResourceView * weapon_revolver = nullptr;
		ID3D11ShaderResourceView * weapon_revolver_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_rpg = nullptr;
		ID3D11ShaderResourceView * weapon_sawnoffshotgun = nullptr;
		ID3D11ShaderResourceView * weapon_smg = nullptr;
		ID3D11ShaderResourceView * weapon_smg_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_smokegrenade = nullptr;
		ID3D11ShaderResourceView * weapon_sniperrifle = nullptr;
		ID3D11ShaderResourceView * weapon_snowball = nullptr;
		ID3D11ShaderResourceView * weapon_snspistol = nullptr;
		ID3D11ShaderResourceView * weapon_snspistol_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_specialcarbine = nullptr;
		ID3D11ShaderResourceView * weapon_specialcarbine_mk2 = nullptr;
		ID3D11ShaderResourceView * weapon_stickybomb = nullptr;
		ID3D11ShaderResourceView * weapon_stone_hatchet = nullptr;
		ID3D11ShaderResourceView * weapon_stungun = nullptr;
		ID3D11ShaderResourceView * weapon_switchblade = nullptr;
		ID3D11ShaderResourceView * weapon_unarmed = nullptr;
		ID3D11ShaderResourceView * weapon_vintagepistol = nullptr;
		ID3D11ShaderResourceView * weapon_wrench = nullptr;

	}FiveM_WeaponsPic;

};

inline ImFont* font_icon;
inline ImFont* weapons_icon;
inline ImFont* weapons_icon2;
inline ImFont* rocket_font_icon;
inline ImFont* poppins;
inline ImFont* baloo;
inline ImFont* tab_title;

inline c_globals g_Variables;

class c_menu {
public:

	enum PAGES {
		Combat,
		Visuals,
		Local,
		World,
		Exploits,
		Settings,
		Login
	};

	bool IsOpen;
    bool IsLoginOpen = true;

	int iTabCount = 0;
	float TabAlpha = 0.f;
	int iCurrentPage = 6;
	float TabAdd = 0.f;
    bool IsLogged = false;

	char cDiscordId[ 200 ] ;
	char user[30] = "";
	char pass[30] = "";
	bool login_success = false;
	std::string server_response;

	ImVec2 MenuSize{ 680, 500 };
	bool particles = true;

};

inline c_menu g_MenuInfo;

extern HINSTANCE g_hInstance;
extern bool g_IsInjectedDll;
