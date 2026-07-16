#pragma once
#include <cstdio>
#include <cstring>
#include <io.h>
#include <process.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef _DLL
extern "C" {
    void* __imp_write = (void*)&_write;
    void* __imp_access = (void*)&_access;
    void* __imp_close = (void*)&_close;
    void* __imp_feof = (void*)&feof;
    void* __imp_fgets = (void*)&fgets;
    void* __imp_open = (void*)&_open;
    void* __imp_read = (void*)&_read;
    void* __imp_strerror = (void*)&strerror;
    void* __imp_strspn = (void*)&strspn;
    void* __imp_unlink = (void*)&_unlink;
    void* __imp__access = (void*)&_access;
    void* __imp__fstat64 = (void*)&_fstat64;
    void* __imp__getpid = (void*)&_getpid;
    void* __imp__stat64 = (void*)&_stat64;
    void* __imp__unlink = (void*)&_unlink;
}

extern "C" int _sys_nerr;
extern "C" void* __imp___sys_nerr = (void*)&_sys_nerr;
#endif
