#pragma once
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/Variables.hpp>
#include <Core/SDK/SDK.hpp>

namespace Core
{
	namespace Features
	{
		class cTriggerbot
		{
		private:
			void Shoot( int delay );
		public:
			void Start( );
		};

		inline cTriggerbot g_Triggerbot;

	}
}
