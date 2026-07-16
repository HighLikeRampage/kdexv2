#pragma once
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

extern const char * keys[ ];

namespace Core
{
	namespace Features
	{
		class cEsp
		{
		public:
			void Draw( );
			void DrawVehicle();
			void DrawObjects();
			void DrawRadar();
		};

		void StartTracer();

		inline cEsp g_Esp;

	}
}
