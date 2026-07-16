#pragma once
#include "../../Offsets.hpp"
#include "../../Variables.hpp"
#include "../Memory.hpp"
#include <D3dx9math.h>

#pragma region Enums
enum BoneMasks : int {
	SKEL_ROOT = 0x0,
	SKEL_Pelvis = 0x2e28,
	SKEL_L_Thigh = 0xe39f,
	SKEL_L_Calf = 0xf9bb,
	SKEL_L_Foot = 0x3779,
	SKEL_L_Toe0 = 0x83c,
	IK_L_Foot = 0xfedd,
	PH_L_Foot = 0xe175,
	MH_L_Knee = 0xb3fe,
	SKEL_R_Thigh = 0xca72,
	SKEL_R_Calf = 0x9000,
	SKEL_R_Foot = 0xcc4d,
	SKEL_R_Toe0 = 0x512d,
	IK_R_Foot = 0x8aae,
	PH_R_Foot = 0x60e6,
	MH_R_Knee = 0x3fcf,
	RB_L_ThighRoll = 0x5c57,
	RB_R_ThighRoll = 0x192a,
	SKEL_Spine_Root = 0xe0fd,
	SKEL_Spine0 = 0x5c01,
	SKEL_Spine1 = 0x60f0,
	SKEL_Spine2 = 0x60f1,
	SKEL_Spine3 = 0x60f2,
	SKEL_L_Clavicle = 0xfcd9,
	SKEL_L_UpperArm = 0xb1c5,
	SKEL_L_Forearm = 0xeeeb,
	SKEL_L_Hand = 0x49d9,
	SKEL_L_Finger00 = 0x67f2,
	SKEL_L_Finger01 = 0xff9,
	SKEL_L_Finger02 = 0xffa,
	SKEL_L_Finger10 = 0x67f3,
	SKEL_L_Finger11 = 0x1049,
	SKEL_L_Finger12 = 0x104a,
	SKEL_L_Finger20 = 0x67f4,
	SKEL_L_Finger21 = 0x1059,
	SKEL_L_Finger22 = 0x105a,
	SKEL_L_Finger30 = 0x67f5,
	SKEL_L_Finger31 = 0x1029,
	SKEL_L_Finger32 = 0x102a,
	SKEL_L_Finger40 = 0x67f6,
	SKEL_L_Finger41 = 0x1039,
	SKEL_L_Finger42 = 0x103a,
	PH_L_Hand = 0xeb95,
	IK_L_Hand = 0x8cbd,
	RB_L_ForeArmRoll = 0xee4f,
	RB_L_ArmRoll = 0x1470,
	MH_L_Elbow = 0x58b7,
	SKEL_R_Clavicle = 0x29d2,
	SKEL_R_UpperArm = 0x9d4d,
	SKEL_R_Forearm = 0x6e5c,
	SKEL_R_Hand = 0xdead,
	SKEL_R_Finger00 = 0xe5f2,
	SKEL_R_Finger01 = 0xfa10,
	SKEL_R_Finger02 = 0xfa11,
	SKEL_R_Finger10 = 0xe5f3,
	SKEL_R_Finger11 = 0xfa60,
	SKEL_R_Finger12 = 0xfa61,
	SKEL_R_Finger20 = 0xe5f4,
	SKEL_R_Finger21 = 0xfa70,
	SKEL_R_Finger22 = 0xfa71,
	SKEL_R_Finger30 = 0xe5f5,
	SKEL_R_Finger31 = 0xfa40,
	SKEL_R_Finger32 = 0xfa41,
	SKEL_R_Finger40 = 0xe5f6,
	SKEL_R_Finger41 = 0xfa50,
	SKEL_R_Finger42 = 0xfa51,
	PH_R_Hand = 0x6f06,
	IK_R_Hand = 0x188e,
	RB_R_ForeArmRoll = 0xab22,
	RB_R_ArmRoll = 0x90ff,
	MH_R_Elbow = 0xbb0,
	SKEL_Neck_1 = 0x9995,
	SKEL_Head = 0x796e,
	IK_Head = 0x322c,
	FACIAL_facialRoot = 0xfe2c,
	FB_L_Brow_Out_000 = 0xe3db,
	FB_L_Lid_Upper_000 = 0xb2b6,
	FB_L_Eye_000 = 0x62ac,
	FB_L_CheekBone_000 = 0x542e,
	FB_L_Lip_Corner_000 = 0x74ac,
	FB_R_Lid_Upper_000 = 0xaa10,
	FB_R_Eye_000 = 0x6b52,
	FB_R_CheekBone_000 = 0x4b88,
	FB_R_Brow_Out_000 = 0x54c,
	FB_R_Lip_Corner_000 = 0x2ba6,
	FB_Brow_Centre_000 = 0x9149,
	FB_UpperLipRoot_000 = 0x4ed2,
	FB_UpperLip_000 = 0xf18f,
	FB_L_Lip_Top_000 = 0x4f37,
	FB_R_Lip_Top_000 = 0x4537,
	FB_Jaw_000 = 0xb4a0,
	FB_LowerLipRoot_000 = 0x4324,
	FB_LowerLip_000 = 0x508f,
	FB_L_Lip_Bot_000 = 0xb93b,
	FB_R_Lip_Bot_000 = 0xc33b,
	FB_Tongue_000 = 0xb987,
	RB_Neck_1 = 0x8b93,
	IK_Root = 0xdd1c
};
enum ePedConfigFlag {
	NoCriticalHits = 2,
	DrownsInWater = 3,
	DisableReticuleFixedLockon = 4,
	UpperBodyDamageAnimsOnly = 7,
	NeverLeavesGroup = 13,
	BlockNonTemporaryEvents = 17,
	CanPunch = 18,
	IgnoreSeenMelee = 24,
	GetOutUndriveableVehicle = 29,
	CanFlyThruWindscreen = 32,
	DiesWhenRagdoll = 33,
	HasHelmet = 34,
	PutOnMotorcycleHelmet = 35,
	DontTakeOffHelmet = 36,
	DisableEvasiveDives = 39,
	DontInfluenceWantedLevel = 42,
	DisablePlayerLockon = 43,
	DisableLockonToRandomPeds = 44,
	AllowLockonToFriendlyPlayers = 45,
	BeingDeleted = 47,
	BlockWeaponSwitching = 48,
	NoCollision = 52,
	IsShooting = 58,
	WasShooting = 59,
	IsOnGround = 60,
	WasOnGround = 61,
	InVehicle = 62,
	OnMount = 63,
	AttachedToVehicle = 64,
	IsSwimming = 65,
	WasSwimming = 66,
	IsSkiing = 67,
	IsSitting = 68,
	KilledByStealth = 69,
	KilledByTakedown = 70,
	Knockedout = 71,
	IsSniperScopeActive = 72,
	SuperDead = 73,
	UsingCoverPoint = 75,
	IsInTheAir = 76,
	IsAimingGun = 78,
	ForcePedLoadCover = 93,
	VaultFromCover = 97,
	IsDrunk = 100,
	ForcedAim = 101,
	IsNotRagdollAndNotPlayingAnim = 104,
	ForceReload = 105,
	DontActivateRagdollFromVehicleImpact = 106,
	DontActivateRagdollFromBulletImpact = 107,
	DontActivateRagdollFromExplosions = 108,
	DontActivateRagdollFromFire = 109,
	DontActivateRagdollFromElectrocution = 110,
	KeepWeaponHolsteredUnlessFired = 113,
	GetOutBurningVehicle = 116,
	BumpedByPlayer = 117,
	RunFromFiresAndExplosions = 118,
	TreatAsPlayerDuringTargeting = 119,
	IsHandCuffed = 120,
	IsAnkleCuffed = 121,
	DisableMelee = 122,
	DisableUnarmedDrivebys = 123,
	JustGetsPulledOutWhenElectrocuted = 124,
	NmMessage466 = 125,
	WillNotHotwireLawEnforcementVehicle = 126,
	WillCommandeerRatherThanJack = 127,
	CanBeAgitated = 128,
	ForcePedToFaceLeftInCover = 129,
	ForcePedToFaceRightInCover = 130,
	BlockPedFromTurningInCover = 131,
	KeepRelationshipGroupAfterCleanUp = 132,
	ForcePedToBeDragged = 133,
	PreventPedFromReactingToBeingJacked = 134,
	IsScuba = 135,
	WillArrestRatherThanJack = 136,
	RemoveDeadExtraFarAway = 137,
	RidingTrain = 138,
	ArrestResult = 139,
	CanAttackFriendly = 140,
	WillJackAnyPlayer = 141,
	WillJackWantedPlayersRatherThanStealCar = 144,
	ShootingAnimFlag = 145,
	DisableLadderClimbing = 146,
	StairsDetected = 147,
	SlopeDetected = 148,
	CowerInsteadOfFlee = 150,
	CanActivateRagdollWhenVehicleUpsideDown = 151,
	AlwaysRespondToCriesForHelp = 152,
	DisableBloodPoolCreation = 153,
	ShouldFixIfNoCollision = 154,
	CanPerformArrest = 155,
	CanPerformUncuff = 156,
	CanBeArrested = 157,
	PlayerPreferFrontSeatMP = 159,
	IsInjured = 166,
	DontEnterVehiclesInPlayersGroup = 167,
	PreventAllMeleeTaunts = 169,
	IsInjured2 = 170,
	AlwaysSeeApproachingVehicles = 171,
	CanDiveAwayFromApproachingVehicles = 172,
	AllowPlayerToInterruptVehicleEntryExit = 173,
	OnlyAttackLawIfPlayerIsWanted = 174,
	PedsJackingMeDontGetIn = 177,
	PedIgnoresAnimInterruptEvents = 179,
	IsInCustody = 180,
	ForceStandardBumpReactionThresholds = 181,
	LawWillOnlyAttackIfPlayerIsWanted = 182,
	IsAgitated = 183,
	PreventAutoShuffleToDriversSeat = 184,
	UseKinematicModeWhenStationary = 185,
	EnableWeaponBlocking = 186,
	HasHurtStarted = 187,
	DisableHurt = 188,
	PlayerIsWeird = 189,
	DoNothingWhenOnFootByDefault = 193,
	UsingScenario = 194,
	VisibleOnScreen = 195,
	DontActivateRagdollOnVehicleCollisionWhenDead = 199,
	HasBeenInArmedCombat = 200,
	AvoidanceIgnoreAll = 202,
	AvoidanceIgnoredByAll = 203,
	AvoidanceIgnoreGroup1 = 204,
	AvoidanceMemberOfGroup1 = 205,
	ForcedToUseSpecificGroupSeatIndex = 206,
	DisableExplosionReactions = 208,
	DodgedPlayer = 209,
	WaitingForPlayerControlInterrupt = 210,
	ForcedToStayInCover = 211,
	GeneratesSoundEvents = 212,
	ListensToSoundEvents = 213,
	AllowToBeTargetedInAVehicle = 214,
	WaitForDirectEntryPointToBeFreeWhenExiting = 215,
	OnlyRequireOnePressToExitVehicle = 216,
	ForceExitToSkyDive = 217,
	DontEnterLeadersVehicle = 220,
	DisableExitToSkyDive = 221,
	Shrink = 223,
	MeleeCombat = 224,
	DisablePotentialToBeWalkedIntoResponse = 225,
	DisablePedAvoidance = 226,
	ForceRagdollUponDeath = 227,
	DisablePanicInVehicle = 229,
	AllowedToDetachTrailer = 230,
	IsHoldingProp = 236,
	BlocksPathingWhenDead = 237,
	ForceSkinCharacterCloth = 240,
	DisableStoppingVehicleEngine = 241,
	PhoneDisableTextingAnimations = 242,
	PhoneDisableTalkingAnimations = 243,
	PhoneDisableCameraAnimations = 244,
	DisableBlindFiringInShotReactions = 245,
	AllowNearbyCoverUsage = 246,
	CanPlayInCarIdles = 248,
	CanAttackNonWantedPlayerAsLaw = 249,
	WillTakeDamageWhenVehicleCrashes = 250,
	AICanDrivePlayerAsRearPassenger = 251,
	PlayerCanJackFriendlyPlayers = 252,
	IsOnStairs = 253,
	AIDriverAllowFriendlyPassengerSeatEntry = 255,
	AllowMissionPedToUseInjuredMovement = 257,
	PreventUsingLowerPrioritySeats = 261,
	DisableClosingVehicleDoor = 264,
	TeleportToLeaderVehicle = 268,
	AvoidanceIgnoreWeirdPedBuffer = 269,
	OnStairSlope = 270,
	DontBlipCop = 272,
	ClimbedShiftedFence = 273,
	KillWhenTrapped = 275,
	EdgeDetected = 276,
	AvoidTearGas = 279,
	NoWrithe = 281,
	OnlyUseForcedSeatWhenEnteringHeliInGroup = 282,
	DisableWeirdPedEvents = 285,
	ShouldChargeNow = 286,
	RagdollingOnBoat = 287,
	HasBrandishedWeapon = 288,
	FreezePosition = 292,
	DisableShockingEvents = 294,
	NeverReactToPedOnRoof = 296,
	DisableShockingDrivingOnPavementEvents = 299,
	DisablePedConstraints = 301,
	ForceInitialPeekInCover = 302,
	DisableJumpingFromVehiclesAfterLeader = 305,
	IsInCluster = 310,
	ShoutToGroupOnPlayerMelee = 311,
	IgnoredByAutoOpenDoors = 312,
	NoPedMelee = 314,
	CheckLoSForSoundEvents = 315,
	CanSayFollowedByPlayerAudio = 317,
	ActivateRagdollFromMinorPlayerContact = 318,
	ForcePoseCharacterCloth = 320,
	HasClothCollisionBounds = 321,
	HasHighHeels = 322,
	DontBehaveLikeLaw = 324,
	DisablePoliceInvestigatingBody = 326,
	DisableWritheShootFromGround = 327,
	LowerPriorityOfWarpSeats = 328,
	DisableTalkTo = 329,
	DontBlip = 330,
	IsSwitchingWeapon = 331,
	IgnoreLegIkRestrictions = 332,
	AllowTaskDoNothingTimeslicing = 339,
	NotAllowedToJackAnyPlayers = 342,
	AlwaysLeaveTrainUponArrival = 345,
	OnlyWritheFromWeaponDamage = 347,
	UseSloMoBloodVfx = 348,
	EquipJetpack = 349,
	PreventDraggedOutOfCarThreatResponse = 350,
	ForceDeepSurfaceCheck = 356,
	DisableDeepSurfaceAnims = 357,
	DontBlipNotSynced = 358,
	IsDuckingInVehicle = 359,
	PreventAutoShuffleToTurretSeat = 360,
	DisableEventInteriorStatusCheck = 361,
	HasReserveParachute = 362,
	UseReserveParachute = 363,
	TreatDislikeAsHateWhenInCombat = 364,
	OnlyUpdateTargetWantedIfSeen = 365,
	AllowAutoShuffleToDriversSeat = 366,
	PreventReactingToSilencedCloneBullets = 372,
	DisableInjuredCryForHelpEvents = 373,
	NeverLeaveTrain = 374,
	DontDropJetpackOnDeath = 375,
	DisableAutoEquipHelmetsInBikes = 380,
	IsClimbingLadder = 388,
	HasBareFeet = 389,
	GoOnWithoutVehicleIfItIsUnableToGetBackToRoad = 391,
	BlockDroppingHealthSnacksOnDeath = 392,
	ForceThreatResponseToNonFriendToFriendMeleeActions = 394,
	DontRespondToRandomPedsDamage = 395,
	AllowContinuousThreatResponseWantedLevelUpdates = 396,
	KeepTargetLossResponseOnCleanup = 397,
	PlayersDontDragMeOutOfCar = 398,
	BroadcastRepondedToThreatWhenGoingToPointShooting = 399,
	IgnorePedTypeForIsFriendlyWith = 400,
	TreatNonFriendlyAsHateWhenInCombat = 401,
	DontLeaveVehicleIfLeaderNotInVehicle = 402,
	AllowMeleeReactionIfMeleeProofIsOn = 404,
	UseNormalExplosionDamageWhenBlownUpInVehicle = 407,
	DisableHomingMissileLockForVehiclePedInside = 408,
	DisableTakeOffScubaGear = 409,
	Alpha = 410,
	LawPedsCanFleeFromNonWantedPlayer = 411,
	ForceBlipSecurityPedsIfPlayerIsWanted = 412,
	IsHolsteringWeapon = 413,
	UseGoToPointForScenarioNavigation = 414,
	DontClearLocalPassengersWantedLevel = 415,
	BlockAutoSwapOnWeaponPickups = 416,
	ThisPedIsATargetPriorityForAI = 417,
	IsSwitchingHelmetVisor = 418,
	ForceHelmetVisorSwitch = 419,
	FlamingFootprints = 421,
	DisableVehicleCombat = 422,
	DisablePropKnockOff = 423,
	FallsLikeAircraft = 424,
	UseLockpickVehicleEntryAnimations = 426,
	IgnoreInteriorCheckForSprinting = 427,
	SwatHeliSpawnWithinLastSpottedLocation = 428,
	DisableStartingVehicleEngine = 429,
	IgnoreBeingOnFire = 430,
	DisableTurretOrRearSeatPreference = 431,
	DisableWantedHelicopterSpawning = 432,
	UseTargetPerceptionForCreatingAimedAtEvents = 433,
	DisableHomingMissileLockon = 434,
	ForceIgnoreMaxMeleeActiveSupportCombatants = 435,
	StayInDefensiveAreaWhenInVehicle = 436,
	DontShoutTargetPosition = 437,
	DisableHelmetArmor = 438,
	PreventVehExitDueToInvalidWeapon = 441,
	IgnoreNetSessionFriendlyFireCheckForAllowDamage = 442,
	DontLeaveCombatIfTargetPlayerIsAttackedByPolice = 443,
	CheckLockedBeforeWarp = 444,
	DontShuffleInVehicleToMakeRoom = 445,
	GiveWeaponOnGetup = 446,
	DontHitVehicleWithProjectiles = 447,
	DisableForcedEntryForOpenVehiclesFromTryLockedDoor = 448,
	FiresDummyRockets = 449,
	IsArresting = 450,
	IsDecoyPed = 451,
	HasEstablishedDecoy = 452,
	BlockDispatchedHelicoptersFromLanding = 453,
	DontCryForHelpOnStun = 454,
	CanBeIncapacitated = 456,
	MutableForcedAim = 457,
	DontChangeTargetFromMelee = 458,
	AllowPlayerLockOnIfFriendly = 45,
	TreatAsFriendlyForTargetingAndDamage = 457,
};
#pragma endregion
#pragma region CPed
class CPed;
class CVehicle;
class CPlayerInfo;
class CWeaponManager;

static uintptr_t citizenModBase = 0;

namespace CPlayerResetFlags {
	enum PlayerResetFlags : __int32
	{
		PRF_NOT_ALLOWED_TO_ENTER_ANY_CAR = 0x1,
		PRF_ASSISTED_AIMING_ON = 0x2,
		PRF_FORCED_ZOOM = 0x4,
		PRF_FORCE_PLAYER_INTO_COVER = 0x8,
		PRF_FORCED_AIMING = 0x10,
		PRF_DISABLE_HEALTH_RECHARGE = 0x20,
		PRF_FORCE_SKIP_AIM_INTRO = 0x40,
		PRF_DISABLE_AIM_CAMERA = 0x80,
		PRF_RUN_AND_GUN = 0x100,
		PRF_SKIP_COVER_ENTRY_ANIM = 0x200,
		PRF_NO_RETICULE_AIM_ASSIST_ON = 0x400,
		PRF_EXPLOSIVE_AMMO_ON = 0x800,
		PRF_FIRE_AMMO_ON = 0x1000,
		PRF_EXPLOSIVE_MELEE_ON = 0x2000,
		PRF_SUPER_JUMP_ON = 0x4000,
		PRF_INCREASE_JUMP_SUPPRESSION_RANGE = 0x8000,
		PRF_SPECIFY_INITIAL_COVER_HEADING = 0x10000,
		PRF_FACING_LEFT_IN_COVER = 0x20000,
		PRF_USE_COVER_THREAT_WEIGHTING = 0x40000,
		PRF_DISABLE_DISPATCHED_HELI_REFUEL = 0x80000,
		PRF_DISABLE_DISPATCHED_HELI_DESTROYED_SPAWN_DELAY = 0x100000,
		PRF_DISABLE_VEHICLE_REWARDS = 0x200000,
		PRF_PREFER_REAR_SEATS = 0x400000,
		PRF_PREFER_FRONT_PASSENGER_SEAT = 0x800000,
		PRF_DISABLE_CAMERA_VIEW_MODE_CYCLE = 0x1000000,
		PRF_CAMERA_VIEW_MODE_SWITCHED_TO_OR_FROM_FIRST_PERSON = 0x2000000,
		PRF_CAMERA_VIEW_MODE_SWITCHED_TO_FIRST_PERSON = 0x4000000,
		PRF_DISABLE_CAN_USE_COVER = 0x8000000,
		PRF_BEAST_JUMP_ON = 0x10000000,
		PRF_FORCED_JUMP = 0x20000000,
		PRF_DOT_SHOCKED = 0x40000000,
		PRF_DOT_CHOKING = 0x80000000,
	};
}

#pragma region Infos
class CPlayerInfo {
public:
	int PlayerID() {
		if (!this) { return 0; }
		return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_PlayerId);
	}

	void SetSuperJump(bool Toggle) {
		if (!this) { return; }
		int frameFlags = Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag);
		if (Toggle) {
			frameFlags |= CPlayerResetFlags::PRF_SUPER_JUMP_ON;
		} else {
			frameFlags &= ~CPlayerResetFlags::PRF_SUPER_JUMP_ON;
		}
		Core::Mem.Write<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag, frameFlags);
	}

	void SetExplosiveAmmo(bool Toggle) {
		if (!this) { return; }
		int frameFlags = Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag);
		if (Toggle) {
			frameFlags |= CPlayerResetFlags::PRF_EXPLOSIVE_AMMO_ON;
		} else {
			frameFlags &= ~CPlayerResetFlags::PRF_EXPLOSIVE_AMMO_ON;
		}
		Core::Mem.Write<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag, frameFlags);
	}

	void SetFireAmmo(bool Toggle) {
		if (!this) { return; }
		int frameFlags = Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag);
		if (Toggle) {
			frameFlags |= CPlayerResetFlags::PRF_FIRE_AMMO_ON;
		} else {
			frameFlags &= ~CPlayerResetFlags::PRF_FIRE_AMMO_ON;
		}
		Core::Mem.Write<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag, frameFlags);
	}

	void SetBeastJump(bool Toggle) {
		if (!this) { return; }
		int frameFlags = Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag);
		if (Toggle) {
			frameFlags |= CPlayerResetFlags::PRF_BEAST_JUMP_ON;
		} else {
			frameFlags &= ~CPlayerResetFlags::PRF_BEAST_JUMP_ON;
		}
		Core::Mem.Write<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag, frameFlags);
	}

	void SetExplosiveFist(bool Toggle) {
		if (!this) { return; }
		int frameFlags = Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag);
		if (Toggle) {
			frameFlags |= CPlayerResetFlags::PRF_EXPLOSIVE_MELEE_ON;
		} else {
			frameFlags &= ~CPlayerResetFlags::PRF_EXPLOSIVE_MELEE_ON;
		}
		Core::Mem.Write<int>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_FrameFlag, frameFlags);
	}
};
#pragma endregion
#pragma region CWeaponInfo
class CWeaponInfo {
public:
	std::string GetName() {
		if (!this) { return xorstr(""); }
		return Core::Mem.ReadString(Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x5F0));
	}
};
#pragma endregion
#pragma region CVehicleList
class CVehicleList {
public:
	CVehicle* Vehicle(int Idx)
	{
		if (!this) { return 0; }
		return (CVehicle*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + (Idx * 0x10U));
	}

};
#pragma endregion

#include <chrono>
#include <unordered_map>
#include <string>
#include <algorithm>

namespace PlayerList {
	inline uint64_t now_ms() {
		static const auto epoch = std::chrono::steady_clock::now();
		return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - epoch).count());
	}

	inline std::unordered_map<int, std::string> g_nameCache;
	inline uint64_t g_lastNameCacheUpdate = 0;

	inline std::string read_remote_std_string(uint64_t address) {
		uint64_t chunk0 = Core::Mem.Read<uint64_t>(address);
		uint64_t chunk1 = Core::Mem.Read<uint64_t>(address + 0x8);
		uint64_t length = Core::Mem.Read<uint64_t>(address + 0x10);

		if (length == 0 || length > 128) return {};

		std::string result;
		result.reserve((size_t)length);

		if (length <= 15) {
			for (size_t i = 0; i < length; ++i) {
				char c = (i < 8) ? reinterpret_cast<char*>(&chunk0)[i] : reinterpret_cast<char*>(&chunk1)[i - 8];
				if (!c) break;
				result.push_back(c);
			}
		}
		else {
			uint64_t heap_ptr = chunk0;
			for (size_t i = 0; i < length; ++i) {
				char c = Core::Mem.Read<char>(heap_ptr + i);
				if (!c) break;
				result.push_back(c);
			}
		}
		return result;
	}

	inline std::string read_remote_wchar_string(uint64_t address, size_t maxLen = 32) {
		uint16_t buf[32] = {};
		size_t readLen = maxLen < 32 ? maxLen : 32;
		Core::Mem.ReadRaw(static_cast<uintptr_t>(address), buf, readLen * sizeof(uint16_t));
		std::string result;
		result.reserve(readLen);
		for (size_t i = 0; i < readLen; ++i) {
			if (buf[i] == 0) break;
			result += (buf[i] < 255) ? static_cast<char>(buf[i]) : '?';
		}
		return result;
	}

	inline void UpdateNameCache() {
		static uintptr_t listAddr = 0;
		static uintptr_t countAddr = 0;
		static DWORD last_pid = 0;

		if (last_pid != Core::g_Variables.ProcIdFiveM) {
			listAddr = 0;
			countAddr = 0;
			last_pid = Core::g_Variables.ProcIdFiveM;
		}

		if (!listAddr || !countAddr) {
			uintptr_t modSize = 0;
			uintptr_t base = Core::Mem.GetModuleBaseAddr(Core::Mem.ProcId, xorstr("citizen-playernames-five.dll"), &modSize);
			if (!base) return;

			uintptr_t sigAddr = Core::Mem.FindSignatureBypass(Core::Mem.Pattern2Vector(xorstr("48 8B 15 ? ? ? ? 48 C1 E1")), base, modSize);
			if (!sigAddr) return;

			listAddr = Core::Mem.ResolveRelativeAddress(sigAddr, 7);
			countAddr = listAddr + 8;
		}

		uintptr_t listHead = Core::Mem.Read<uintptr_t>(listAddr);
		int count = Core::Mem.Read<int>(countAddr);

		if (!listHead || listHead > 0x7FFFFFFFFFFF) return;

		std::unordered_map<int, std::string> newCache;
		uintptr_t current = Core::Mem.Read<uintptr_t>(listHead);

		for (int i = 0; i < count && current && current < 0x7FFFFFFFFFFF; ++i) {
			uint16_t netId = Core::Mem.Read<uint16_t>(current + 0x10);

			std::string name = read_remote_std_string(current + 0x18);

			if (netId != 0 && !name.empty()) {
				newCache[static_cast<int>(netId)] = name;
			}

			current = Core::Mem.Read<uintptr_t>(current);
		}

		if (!newCache.empty()) {
			g_nameCache.swap(newCache);
			g_lastNameCacheUpdate = now_ms();
		}
	}

	inline std::string GetPedNameExtern(uintptr_t ped) {
		if (!ped) return xorstr("NPC");

		uintptr_t net_obj = Core::Mem.Read<uintptr_t>(ped + 0xD0);
		uint16_t net_id = net_obj ? Core::Mem.Read<uint16_t>(net_obj + 0xA) : 0;
		if (net_id == 0) return xorstr("NPC");

		uintptr_t playerInfo = Core::Mem.Read<uintptr_t>(ped + Core::g_Offsets.m_PlayerInfo);
		if (!playerInfo) return xorstr("NPC");

		int targetId = Core::Mem.Read<int>(playerInfo + Core::g_Offsets.m_PlayerId);
		if (targetId <= 0) return xorstr("NPC");

		if (now_ms() - g_lastNameCacheUpdate > 2000) {
			UpdateNameCache();
		}

		auto it = g_nameCache.find(targetId);
		if (it != g_nameCache.end()) {
			return it->second;
		}

		return std::string(xorstr("ID:")) + std::to_string(targetId);
	}
}

#pragma region CPedList
class CPedList {
public:
	CPed* Ped(int Idx) {
		if (!this) { return 0; }
		return (CPed*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + (Idx * 0x10U));
	}
};
#pragma endregion
class CPickupList {
public:
	uintptr_t Pickup(int Idx) {
		if (!this) { return 0; }
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + (Idx * 0x10U));
	}
};
#pragma endregion
#pragma region CObjectList
class CObjectList {
public:
	uintptr_t Object(int Idx) {
		if (!this) { return 0; }
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + (Idx * 0x10U));
	}
};
#pragma endregion
#pragma region InterFaces
class CPickupInterFace {
public:
	int MaxPickups() { return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + 0x110); }
	CPickupList* PickupList() { return (CPickupList*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x100); }
};
#pragma endregion
#pragma region CObjectInterFace
class CObjectInterFace {
public:
	int MaxObjects() { return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + 0x160); }
	CObjectList* ObjectList() { return (CObjectList*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x158); }
};
#pragma endregion
#pragma region CPedInterFace
class CPedInterFace {
public:
	int MaxPed() { return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + 0x108); }
	int PedCount() { return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + 0x110); }
	CPedList* PedList() { return (CPedList*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x100); }
};
#pragma endregion
#pragma region CVehInterFace
class CVehInterFace {
public:
	int MaxVehicles() { return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + 0x188); }
	int VehicleCount() { return Core::Mem.Read<int>(reinterpret_cast<uintptr_t>(this) + 0x190); }
	CVehicleList* VehicleList() { return (CVehicleList*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x180); }
};
#pragma endregion
#pragma region CReplayInterFace
class CReplayInterFace {
public:

	CPedInterFace* InterfacePed() {
		if (!this) { return 0; }
		return (CPedInterFace*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x18);
	}

	CVehInterFace* InterfaceVeh() {
		if (!this) { return 0; }
		return (CVehInterFace*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x10);
	}

	CPickupInterFace* InterfacePickup() {
		if (!this) { return 0; }
		return (CPickupInterFace*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x20);
	}

	CObjectInterFace* InterfaceObject() {
		if (!this) { return 0; }
		return (CObjectInterFace*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x28);
	}
};
#pragma endregion
#pragma region CPed
class CPed {
public:
	float GetMaxHealth() {
		if (!this) { return 0; }
		return Core::Mem.Read<float>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_MaxHealth);
	}

	float GetHealth() {
		if (!this) { return 0; }
		return Core::Mem.Read<float>(reinterpret_cast<uintptr_t>(this) + 0x280);
	}

	void SetHealth(float Health) {
		if (!this) { return; }
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(this) + 0x280, Health);
	}

	float GetArmor() {
		if (!this) { return 0; }
		return Core::Mem.Read<float>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_Armor);
	}

	void SetArmor(float Armor) {
		if (!this) { return; }
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_Armor, Armor);
	}

	uint64_t GetModelInfo()
	{
		if (!this)
			return 0;

		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x20);
	}

	float GetSpeed() {
		if (!this) { return 0; }
		CPlayerInfo* PlayerInfo = this->GetPlayerInfo();
		return Core::Mem.Read<float>(reinterpret_cast<uintptr_t>(PlayerInfo) + Core::g_Offsets.m_Speed);
	}

	void SetSpeed(bool Toggle) {
		if (!this) { return; }
		SetSpeed(Toggle ? 20.f : 1.f);
	}

	void SetSpeed(float value) {
		if (!this) { return; }
		CPlayerInfo* PlayerInfo = this->GetPlayerInfo();
		if (!PlayerInfo) return;
		float speed = (value > 0.f && value < 100.f) ? value : 1.f;
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(PlayerInfo) + Core::g_Offsets.m_Speed, speed);
	}

	D3DXVECTOR3 GetPos() {
		if (!this) { return D3DXVECTOR3(0, 0, 0); }
		return Core::Mem.Read<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + 0x90);
	}

	void SetPos(D3DXVECTOR3 Pos) {
		if (!this) { return; }
		bool InsideVehicle = InVehicle();
		uintptr_t LastVeh = reinterpret_cast<uintptr_t>(GetLastVehicle());

		if (LastVeh && InsideVehicle) {
			uintptr_t Navigation = Core::Mem.Read<uintptr_t>(LastVeh + 0x30);
			Core::Mem.Write<D3DXVECTOR3>(Navigation + 0x30, D3DXVECTOR3(0, 0, 0));
			Core::Mem.Write<D3DXVECTOR3>(LastVeh + 0x90, Pos);
		}
		else if (!InsideVehicle) {
			uintptr_t Navigation = this->GetNavigation();
			Core::Mem.Write<D3DXVECTOR3>(Navigation + 0x30, D3DXVECTOR3{ 0, 0, 0 });
			Core::Mem.Write<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + 0x90, Pos);
		}
	}

	void SeatBealt(bool toggle) {
		if (!this) { return; }
		bool InsideVehicle = InVehicle();

		if (InsideVehicle) {
			if (toggle) {
				uintptr_t Path = Core::Mem.FindSignatureStr(xorstr("83 a1 00 00 00 00 00 83 e2"));
				Core::Mem.WriteBytes(Path, { 0x90,0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, });
				Core::Mem.Write<BYTE>((uintptr_t)this + Core::g_Offsets.m_SeatBealt, 0xC9);
			}
			else {
				Core::Mem.Write<BYTE>((uintptr_t)this + Core::g_Offsets.m_SeatBealt, 0xC8);
			}
		}
	}

	CVehicle* GetLastVehicle() {
		if (!this) return 0;
		return (CVehicle*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_LastVehicle);
	}

	CPlayerInfo* GetPlayerInfo() {
		if (!this) return 0;
		return (CPlayerInfo*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_PlayerInfo);
	}

	CWeaponManager* GetWeaponManager() {
		if (!this) return 0;
		return (CWeaponManager*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_WeaponManager);
	}

	uint32_t GetPedType() {
		if (!this) { return 0; }
		return Core::Mem.Read<uint32_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_EntityType) << 11 >> 25;
	}

	int GetID() {
		if (!this) { return 0; }
		CPlayerInfo* PlayerInfo = (CPlayerInfo*)GetPlayerInfo();
		if (!PlayerInfo) return 0;
		int Id = PlayerInfo->PlayerID();
		return Id;
	}

	bool HasFlag(ePedConfigFlag Flag)
	{
		if (!this) { return false; }

		auto v1 = (int)Flag;
		if (!this || v1 > 0x1CA) return false;

		auto v2 = 1 << (v1 & 0x1F);
		auto v3 = v1 >> 5;
		auto v4 = reinterpret_cast<uintptr_t>(this) + 4 * v3 + Core::g_Offsets.m_PedFlag;
		auto v5 = Core::Mem.Read<long>(v4);

		return (v2 & v5) != 0;
	}

	void SetConfigFlag(ePedConfigFlag Flag, bool Value)
	{
		if (!this) { return; }

		auto v1 = (int)Flag;
		if (!this || v1 > 0x1CA) return;

		auto v2 = 1 << (v1 & 0x1F);
		auto v3 = v1 >> 5;
		auto v4 = (uintptr_t)(this) + 4 * v3 + Core::g_Offsets.m_PedFlag;
		auto v5 = Core::Mem.Read<long>(v4);

		if (Value != ((v2 & v5) != 0)) {
			auto v6 = v2 & (v5 ^ -(uint8_t)(Value ? 1 : 0));
			v5 ^= v6;
			Core::Mem.Write(v4, v5);
		}
	}

	void setConfigFlags(ePedConfigFlag Flag, bool Value) {
		SetConfigFlag(Flag, Value);
	}

	bool isValid() {
		return this != nullptr && GetHealth() > 0.1f;
	}

	void antiAim(bool toggle) {
		if (!isValid()) return;
		this->setConfigFlags(DisablePlayerLockon, toggle);
		this->setConfigFlags(AllowPlayerLockOnIfFriendly, toggle);
		this->setConfigFlags(TreatAsFriendlyForTargetingAndDamage, toggle);
	}

	void spectate(bool toggle) {
		if (this == nullptr) return;
		if (toggle && !isValid()) return;

		static std::vector<uint8_t> OriginalCamBytes;
		static std::vector<uint8_t> NopTable = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

		if (OriginalCamBytes.empty())
		{
			OriginalCamBytes = Core::Mem.ReadBytes(Core::g_Offsets.m_GameplayCamHolder, NopTable.size());
		}

		Core::Mem.WriteBytes(Core::g_Offsets.m_GameplayCamHolder, toggle ? NopTable : OriginalCamBytes);
		Core::Mem.Write<uintptr_t>(Core::g_Offsets.m_GameplayCamTarget, toggle ? reinterpret_cast<uintptr_t>(this) : 0x0);
	}

	bool IsVisible() {
		if (this == nullptr)
			return false;

		BYTE VisibilityFlag = HasFlag(ePedConfigFlag::VisibleOnScreen);;

		return !(VisibilityFlag == 36 || VisibilityFlag == 0 || VisibilityFlag == 4);
	}

	bool InVehicle() {
		if (!this) return false;
		return HasFlag(ePedConfigFlag::InVehicle);
	}

	void NoRagDoll(bool Toggle)
	{
		if (!this) return;
		Core::Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_NoRagDoll, Toggle ? 0x80 : 0x20);
	}

	void SetInvisible(bool state) {
		if (!this) { return; }
		Core::Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(this) + 0xD1, state ? 1 : 0);
	}

	D3DXVECTOR3 GetVelocity() {
		if (!this) { return D3DXVECTOR3(0, 0, 0); }
		uintptr_t off = Core::g_Offsets.m_Velocity ? Core::g_Offsets.m_Velocity : 0x320u;
		return Core::Mem.Read<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + off);
	}

	void FreezePed(bool Toggle) {
		if (!this) { return; }
		if (!InVehicle()) {
			uintptr_t CModelInfo = Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x20);
			Core::Mem.Write<float>(CModelInfo + 0x2C, Toggle ? 0.f : 1.f);
		}
		else {
			Core::Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(GetLastVehicle()) + 0x2E, Toggle ? 1 : 0);
		}
	}

	void SetGodMode(bool Toggle) {
		if (!this) { return; }

		uintptr_t Addr = reinterpret_cast<uintptr_t>(this) + 0x188;
		DWORD flag = Core::Mem.Read<DWORD>(Addr);
		Core::Mem.Write<DWORD>(Addr, Toggle == true ? flag |= (1 << 9) : flag &= ~(1 << 9));
	}

	void SetInfStamina(bool Toggle) {
		if (!this) { return; }
		CPlayerInfo* PlayerInfo = (CPlayerInfo*)GetPlayerInfo();
		if (!PlayerInfo) return;
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(PlayerInfo) + Core::g_Offsets.m_PlayerStamina, Toggle ? 100.f : 0.f);
	}

	void ApplyInfStaminaOld(bool Toggle) {
		CPlayerInfo* PlayerInfo = (CPlayerInfo*)GetPlayerInfo();
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(PlayerInfo) + 0xCF4, Toggle ? FLT_MAX : 100);
	}

	void SetInfCombatRoll(bool enable) {
		if (!this) { return; }

		uintptr_t Address = Core::g_Offsets.m_InfiniteCombatRoll;
		static std::vector<uint8_t> OriginalTable;

		if (OriginalTable.empty()) {
			OriginalTable = Core::Mem.ReadBytes(Address, 6);
		}

		std::vector <uint8_t> Patch = { 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };

		Core::Mem.WriteBytes(Address, enable ? Patch : OriginalTable);
	}

	D3DXVECTOR3 GetBonePosDefault(const int Bone)
	{
		if (!this) { return D3DXVECTOR3(0, 0, 0); }
		D3DXMATRIX Mtx = Core::Mem.Read<D3DXMATRIX>(reinterpret_cast<uintptr_t>(this) + 0x60);
		D3DXVECTOR3 BonePos = Core::Mem.Read<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + (Core::g_Offsets.CurrentBuild >= 2802 ? 0x410 : 0x430) + Bone * 0x10);

		D3DXVECTOR4 Transform;
		D3DXVec3Transform(&Transform, &BonePos, &Mtx);
		return D3DXVECTOR3(Transform.x, Transform.y, Transform.z);
	}

	float GetDistance(D3DXVECTOR3 pos1, D3DXVECTOR3 pos2) {
		float dx = pos2.x - pos1.x, dy = pos2.y - pos1.y, dz = pos2.z - pos1.z;
		return std::sqrtf(dx * dx + dy * dy + dz * dz);
	}

	uintptr_t GetCPedInventory() {
		if (!this) { return 0; }
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + (Core::g_Offsets.m_WeaponManager - 8));
	}

	uintptr_t GetNavigation() {
		if (!this) { return 0; }
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x30);
	}

	void RemoveKinematics()
	{
		if (!Core::g_Offsets.m_ArmsKinematics || !Core::g_Offsets.m_LegsKinematics)
			return;

		Core::Mem.PatchFunc(Core::g_Offsets.m_ArmsKinematics, 5);
		Core::Mem.PatchFunc(Core::g_Offsets.m_LegsKinematics, 5);
	}

	void ForceWeaponWheel(bool toggle)
	{
		if (!this) return;

		static uintptr_t DisableControlAction = 0;
		static uintptr_t HideHudComponentThisFrame = 0;
		static DWORD last_pid = 0;

		if (last_pid != Core::g_Variables.ProcIdFiveM) {
			DisableControlAction = 0;
			HideHudComponentThisFrame = 0;
			last_pid = Core::g_Variables.ProcIdFiveM;
		}

		if (DisableControlAction == 0) {
			DisableControlAction = Core::Mem.FindSignatureStr(
				xorstr("48 8b 41 00 83 78 00 00 8b 50 00 8b 08 e9 00 00 00 00 48 89 5c 24")
			);
		}

		if (HideHudComponentThisFrame == 0) {
			HideHudComponentThisFrame = Core::Mem.FindSignatureStr(
				xorstr("48 83 ec 00 48 8b 41 00 83 38 00 48 89 6c 24")
			);
		}

		if (toggle) {
			Core::Mem.WriteBytes(DisableControlAction, { 0xC3 });
			Core::Mem.WriteBytes(HideHudComponentThisFrame, { 0xC3 });
			SetConfigFlag(BlockWeaponSwitching, false);
		}
		else {
			Core::Mem.WriteBytes(DisableControlAction, { 0x48 });
			Core::Mem.WriteBytes(HideHudComponentThisFrame, { 0x48 });
			SetConfigFlag(BlockWeaponSwitching, true);
		}
	}

	inline std::string GetPedName(CPed* Ped)
	{
		if (!Ped) return "";
		return PlayerList::GetPedNameExtern(reinterpret_cast<uintptr_t>(Ped));
	}

};
#pragma endregion
#pragma region CHandlingData
class CHandlingData {
public:

	uint64_t qword0;
	uint32_t m_model_hash;
	float m_mass;
	float m_initial_drag_coeff;
	float m_downforce_multiplier;
	float m_popup_light_rotation;
	char pad_001C[4];
	D3DXVECTOR3 m_centre_of_mass;
	char pad_002C[4];
	D3DXVECTOR3 m_inertia_mult;
	char pad_003C[4];
	float m_buoyancy;
	float m_drive_bias_rear;
	float m_drive_bias_front;
	float m_acceleration;
	uint8_t m_initial_drive_gears;
	char pad_0051[3];
	float m_drive_inertia;
	float m_upshift;
	float m_downshift;
	float m_initial_drive_force;
	float m_drive_max_flat_velocity;
	float m_initial_drive_max_flat_vel;
	float m_brake_force;
	char pad_0070[4];
	float m_brake_bias_front;
	float m_brake_bias_rear;
	float m_handbrake_force;
	float m_steering_lock;
	float m_steering_lock_ratio;
	float m_traction_curve_max;
	float m_traction_curve_lateral;
	float m_traction_curve_min;
	float m_traction_curve_ratio;
	float m_curve_lateral;
	float m_curve_lateral_ratio;
	float m_traction_spring_delta_max;
	float m_traction_spring_delta_max_ratio;
	float m_low_speed_traction_loss_mult;
	float m_camber_stiffness;
	float m_traction_bias_front;
	float m_traction_bias_rear;
	float m_traction_loss_mult;
	float m_suspension_force;
	float m_suspension_comp_damp;
	float m_suspension_rebound_damp;
	float m_suspension_upper_limit;
	float m_suspension_lower_limit;
	float m_suspension_raise;
	float m_suspension_bias_front;
	float m_suspension_bias_rear;
	float m_anti_rollbar_force;
	float m_anti_rollbar_bias_front;
	float m_anti_rollbar_bias_rear;
	float m_roll_centre_height_front;
	float m_roll_centre_height_rear;
	float m_collision_damage_mult;
	float m_weapon_damamge_mult;
	float m_deformation_mult;
	float m_engine_damage_mult;
	float m_petrol_tank_volume;
	float m_oil_volume;
	char pad_0108[4];
	D3DXVECTOR3 m_seat_offset_dist;
	uint32_t m_monetary_value;
	char pad_011C[8];
	uint32_t m_model_flags;
	uint32_t m_handling_flags;
	uint32_t m_damage_flags;
	char pad_0130[12];
	uint32_t m_ai_handling_hash;
	char pad_140[24];
};
#pragma endregion
#pragma region CVehicle
class CVehicle {
public:

	D3DXVECTOR3 GetPos() {
		if (!this) { return D3DXVECTOR3(0, 0, 0); }
		return Core::Mem.Read<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + 0x90);
	}

	void SetPos(D3DXVECTOR3 Pos) {
		if (!this) { return; }
		Core::Mem.Write<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + 0x90, Pos);
	}

	uintptr_t GetHandling() {
		if (!this) { return 0; }
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_Handling);
	}

	bool GetGodMode() {
		if (!this) { return false; }
		return Core::Mem.Read<BYTE>(reinterpret_cast<uintptr_t>(this) + 0x189);
	}

	void SetGodMode(bool Toggle) {
		if (!this) { return; }
		Core::Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(this) + 0x189, Toggle);
	}

	void Fix() {
		if (!this) { return; }

		float Value = 1000.0f;

		static const int Fix2Addr = Core::Mem.Read<int>(Core::Mem.FindSignatureStr(
			xorstr("f3 0f 10 80 00 00 00 00 f3 0f 10 89 00 00 00 00 0f 28 da")
		) + 4);

		static const int Fix3Addr = Core::Mem.Read<int>(Core::Mem.FindSignatureStr(
			xorstr("f3 0f 10 b8 00 00 00 00 8b 81")
		) + 4);

		static const int Fix4Addr = Core::Mem.Read<int>(Core::Mem.FindSignatureStr(
			xorstr("8a 87 00 00 00 00 c0 e8 00 88 82 00 00 00 00 8a 87 00 00 00 00 c0 e8 00 41 22 c6 88 82 00 00 00 00 48 8b 87")) + 2);

		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(this) + 0x280, Value);
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(this) + Fix2Addr, Value);
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_VehicleEngineHealth, Value);
		Core::Mem.Write<float>(reinterpret_cast<uintptr_t>(this) + Fix3Addr, Value);
		Core::Mem.Write<BYTE>(reinterpret_cast<uintptr_t>(this) + Fix4Addr, 1);
	}

	bool IsLocked() {
		if (!this) { return false; }
		return Core::Mem.Read<uint32_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_VehicleDoorsLockState) == 2;
	}

	void DoorState(bool Unlock) {
		if (!this) { return; }

		Core::Mem.Write<uint32_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_VehicleDoorsLockState, Unlock ? 1 : 2);
	}

	D3DXVECTOR3 GetVelocity() {
		if (!this) { return D3DXVECTOR3(0, 0, 0); }
		uintptr_t off = Core::g_Offsets.m_Velocity ? Core::g_Offsets.m_Velocity : 0x320u;
		return Core::Mem.Read<D3DXVECTOR3>(reinterpret_cast<uintptr_t>(this) + off);
	}

	CPed* GetDriver()
	{
		if (!this) return 0;
		return (CPed*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + Core::g_Offsets.m_VehicleDriver);
	}

	uintptr_t GetNavigation() {
		if (!this) return 0;
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x30);
	}

	uintptr_t GetModelInfo() {
		if (!this) return 0;
		return Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x20);
	}

};
#pragma endregion
#pragma region CWeaponManager
class CWeaponManager {
public:
	CWeaponInfo* GetWeaponInfo() {
		if (!this) { return 0; }
		return (CWeaponInfo*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x20);
	}

	float GetRecoil() {
		if (!this) { return 0.0f; }
		CWeaponInfo* WeaponInfo = (CWeaponInfo*)GetWeaponInfo();
		return Core::Mem.Read<float>((uintptr_t)WeaponInfo + Core::g_Offsets.m_Recoil);
	}

	float SetRecoil(float Recoil) {
		if (!this) { return 0.0f; }
		CWeaponInfo* WeaponInfo = (CWeaponInfo*)GetWeaponInfo();
		return Core::Mem.Write<float>((uintptr_t)WeaponInfo + Core::g_Offsets.m_Recoil, Recoil);
	}

	float GetSpread() {
		if (!this) { return 0.0f; }
		CWeaponInfo* WeaponInfo = (CWeaponInfo*)GetWeaponInfo();
		return Core::Mem.Read<float>((uintptr_t)WeaponInfo + Core::g_Offsets.m_Spread);
	}

	float SetSpread(float Spread) {
		if (!this) { return 0.0f; }
		CWeaponInfo* WeaponInfo = (CWeaponInfo*)GetWeaponInfo();
		return Core::Mem.Write<float>((uintptr_t)WeaponInfo + Core::g_Offsets.m_Spread, Spread);
	}
};
#pragma endregion
#pragma region CPedFactory
class CPedFactory {
public:
	CPed* GetLocalPlayer() {
		if (!this) { return 0; }
		return (CPed*)Core::Mem.Read<uintptr_t>(reinterpret_cast<uintptr_t>(this) + 0x8);
	}
};
#pragma endregion
