#pragma once
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/Variables.hpp>
#include <Core/SDK/SDK.hpp>

namespace Core
{
	namespace Features
	{
		class cMagicBullets {
		public:
			bool Initialized;
		private:
			uintptr_t GetCWeaponObj( );
			uintptr_t CWeapon;
		public:
			void Initialize( );
			void Restore( );
			void Start( );
		};

		inline cMagicBullets g_MagicBullets;

	}
}
