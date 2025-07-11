// Copyright 2017-2025 Adrian Johnston

#include "entanglement_cpp_test.hpp"

int g_a(0), g_b(0), g_c(0);
float g_f(0.0f), g_g(0.0f), g_h(0.0f);

/// fn1 - Returns the sum of global variables g_a, g_b, g_c, g_f, g_g, and g_h.
float fn1(void) { return g_a + g_b + g_c + g_f + g_g + g_h; }

float fn2(void) { return fn1(); }

#if 0

float class3::fn5() { return fn1(); }

#endif
