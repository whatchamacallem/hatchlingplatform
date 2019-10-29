#pragma once
// Copyright 2019 Adrian Johnston
//
// Tiny vsnprintf/snprintf implementation from https://github.com/mpaland/printf
// Can be configured to be even smaller.  To disable use: -Dhxvsnprintf=vsnprintf 

#if !HATCHLING_VER
#error #include <hx/hatchling.h>
#endif

#if defined(hxvsnprintf) && HX_USE_STDIO_H
// provide declaration of vsnprintf when hxvsnprintf is not in use.
#include <stdio.h>
#endif

#if __cplusplus
extern "C" {
#endif

#if !defined(hxvsnprintf)
int hxvsnprintf(char* buffer, size_t count, const char* format, va_list va);
#endif

int hxsnprintf(char* buffer, size_t count, const char* format, ...);

#if __cplusplus
}
#endif
