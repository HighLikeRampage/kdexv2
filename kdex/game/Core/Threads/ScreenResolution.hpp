#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <Core/Variables.hpp>

#include <string>

namespace Core
{
	namespace Threads
	{
		class cScreenResolution
		{
		public:
			void MoveAndSetResolution( )
			{
				if ( !IsWindow( g_Variables.g_hGameWindow ) ) {
					g_Variables.g_hGameWindow = Utils::FindFiveMWindow();
				}
			}

			void Update( )
			{
				while ( !g_Variables.g_Unload )
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(400));

					try
					{
						MoveAndSetResolution( );
					}
					catch ( std::exception e )
					{
						continue;
					}
				}
			}
		};

		inline cScreenResolution g_ScreenResolution;
	}
}
