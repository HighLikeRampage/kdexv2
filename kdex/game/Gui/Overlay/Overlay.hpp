#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace Gui {
    class Overlay {
    public:
        void Render();
    };
    inline Overlay cOverlay;
    void RequestEmergencyStop();
    void PerformRestoreAll();
}
