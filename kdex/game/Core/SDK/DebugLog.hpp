#pragma once
#include <Windows.h>
#include <cstdio>
#include <cstdarg>

inline void DebugLog(const char* fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
    va_end(args);

    fprintf(stdout, "%s", buf);
    fflush(stdout);

    OutputDebugStringA(buf);
}
