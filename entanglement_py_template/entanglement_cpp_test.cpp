// Copyright 2017-2025 Adrian Johnston

#include "entanglement_cpp_test.hpp"

int g_a(0), g_b(0), g_c(0);
float g_f(0.0f), g_g(0.0f), g_h(0.0f);

float fn1(void) { return g_a + g_b + g_c + g_f + g_g + g_h; }
float fn1(float f) { g_f = f; return fn1(); }
float fn1(char a, short b) { g_a = a; g_b = b; return fn1(); }
float fn1(float* f, float g, float h) { g_f = *f; g_g = g; g_h = h; return fn1(); }


float fn2(void) { return fn1(); }

enum1 fn2(enum1 x) { return x; }

bool fn2(enum2 a, enum2 b) { return a == b; }

enum3 fn2(enum3 a, enum3 b, enum3 c) { return a == b ? a : c; }

class2::class2(int x) : m_x(x) { }

class2::class2(float x, float) : m_x((int)x) { }

int class2::fn3(void) { return m_x; }

#if 0

float class3::fn5() { return fn1(); }

#endif
