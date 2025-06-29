// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// - Multiple definitions.
int fn1(void);
inline int fn1(void) { return 0; }
int fn1(void);

// - Overloads by arity.
inline int fn1(int) { return 1; }
inline int fn1(int, int) { return 2; }
inline int fn1(int, int, int) { return 3; }

// - Overloads by first paramater type and then arity.
inline int fn1(float) { return 4; }
inline int fn1(float, float) { return 5; }
inline int fn1(float, float, float) { return 6; }

// - Multiple definitions, external linkage.
int fn2(void);
int fn2(void);

// - Enums.
enum enum1 { };
enum enum2 { enum2_1 };
enum enum3 { enum3_1=-10, enum3_2=0, enum3_3=10 };

// - Enums as parameters.
inline int fn1(enum1) { return 7; }
inline int fn1(enum2, enum2) { return 8; }
inline int fn1(enum3, enum3, enum3) { return 9; }

// - Null test for a struct/class. It has a public constructor.
class class1 {
public:
};

// - Simple class. Two constructors.
class class2 {
public:
	explicit class2(int x) : m_x(x) { }
	explicit class2(float x) : m_x((int)x) { }
	~class2() { }
	int fn3(void) { return m_x; }
private:
	int m_x;
};

// - Complex class.
class class3 {
public:
	explicit class3() : m_a(0), m_b(0), m_c(0), m_f(0.0f), m_g(0.0f), m_h(0.0f) { }
	~class3() { }

	// - Check method overload resolution as well.
	float fn4(void) { return m_a + m_b + m_c + m_f + m_g + m_h; }
	inline float fn4(int a) { m_a = a; return fn4(); }
	inline float fn4(int a, int b) { m_a = a; m_b = b; return fn4(); }
	inline float fn4(int a, int b, int c) { m_a = a; m_b = b; m_c = c; return fn4(); }
	inline float fn4(float f) { m_f = f; return fn4(); }
	inline float fn4(float f, float g) { m_f = f; m_g = g; return fn4(); }
	inline float fn4(float f, float g, float h) { m_f = f; m_g = g; m_h = h; return fn4(); }

	// - External function.
	int fn5(void);
private:
	int m_a, m_b, m_c;
	float m_f, m_g, m_h;
};
