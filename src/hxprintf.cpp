// Copyright 2019 Adrian Johnston

#include <hx/hatchling.h>

// Prevent inclusion from inside anonymous namespace.
#include <stdbool.h>

#if !defined(hxvsnprintf)

// Import the printf module into an anonymous namespace.  printf.h requires
// _putchar to be defined even when not used. 
namespace {
#if HX_USE_C_FILE
#define _putchar putchar
#else
#define _putchar(x) ((void)0)
#endif

#include "../printf/printf.h"
#include "../printf/printf.c"
}

// vsnprintf_ is the only symbol used from printf.h.
extern "C"
int hxvsnprintf(char* buffer, size_t count, const char* format, va_list va) {
	return ::vsnprintf_(buffer, count, format, va);
}

#endif // !defined(hxvsnprintf)

extern "C"
int hxsnprintf(char* buffer, size_t count, const char* format, ...) {
	va_list va;
	va_start(va, format);
	const int ret = hxvsnprintf(buffer, count, format, va);
	va_end(va);
	return ret;
}
