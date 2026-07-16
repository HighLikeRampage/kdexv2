#pragma once
#include <Windows.h>
#include <cstring>

namespace JunkCode {

    inline void JunkCode1() {
        volatile int dummy = 0;
        for (int i = 0; i < 100; i++) {
            dummy += (i * 0x41414141) ^ 0x12345678;
            dummy = dummy << 1 | dummy >> 31;
        }
        volatile char buffer[256];
        memset((void*)buffer, 0xCC, sizeof(buffer));
    }

    inline void JunkCode2() {
        volatile unsigned long long hash = 0x1337DEADBEEF;
        for (int i = 0; i < 50; i++) {
            hash = hash * 1103515245 + 12345;
            hash ^= (hash << 13) | (hash >> 19);
        }
        volatile DWORD tick = GetTickCount();
        hash ^= tick;
    }

    inline void JunkCode3() {
        volatile float f1 = 3.14159f, f2 = 2.71828f;
        for (int i = 0; i < 75; i++) {
            f1 = f1 * f2 + 0.12345f;
            f2 = f2 / f1 - 0.67890f;
        }
        SYSTEMTIME st;
        GetSystemTime(&st);
        f1 += st.wMilliseconds;
    }

    inline void RunAll() {
        JunkCode1();
        JunkCode2();
        JunkCode3();
    }

}
