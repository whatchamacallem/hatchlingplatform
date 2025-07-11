// Copyright 2017-2025 Adrian Johnston
#pragma once

#include <hx/hatchling.h>

// - Multiple declarations followed by a definition and 6 overloads.

/// fn1 - Returns the sum of global variables g_a, g_b, g_c, g_f, g_g, and g_h.
float fn1(void);

/// fn1 - Second declaration.
float fn1(void);

// - Overloads by arity.

extern int g_a, g_b, g_c;
extern float g_f, g_g, g_h;

/// fn1 - Sets the global variable g_a and returns the sum of global variables.
/// - a: An integer value to assign to g_a.
//inline float fn1(int a) { g_a = a; return fn1(); }
/// fn1 - Sets global variables g_a and g_b and returns the sum of global variables.
/// - a: An integer value to assign to g_a.
/// - b: An integer value to assign to g_b.
inline float fn1(char a, short b) { g_a = a; g_b = b; return fn1(); }
/// fn1 - Sets global variables g_a, g_b, and g_c and returns the sum of global variables.
/// - a: An integer value to assign to g_a.
/// - b: An integer value to assign to g_b.
/// - c: An integer value to assign to g_c.
//inline float fn1(bool a, unsigned short b, float c) { g_a = a; g_b = b; g_c = c; return fn1(); }

// - Overloads by first paramater type (int vs. float) and then arity.

/// fn1 - Sets the global variable g_f and returns the sum of global variables.
/// - f: A float value to assign to g_f.
inline float fn1(float f) { g_f = f; return fn1(); }
/// fn1 - Sets global variables g_f and g_g and returns the sum of global variables.
/// - f: A float value to assign to g_f.
/// - g: A float value to assign to g_g.
//inline float fn1(double f, float g) { g_f = f; g_g = g; return fn1(); }
/// fn1 - Sets global variables g_f, g_g, and g_h and returns the sum of global variables.
/// - f: A float value to assign to g_f.
/// - g: A float value to assign to g_g.
/// - h: A float value to assign to g_h.
inline float fn1(float* f, float g, float h) { g_f = *f; g_g = g; g_h = h; return fn1(); }

float fn2(void);
float fn2(void);


#if 0

// - Multiple declarations, external linkage.

/// fn2 - Returns an integer.

// - Enums

/// enum1 - An empty enumeration.
enum enum1 { };
/// enum2 - An enumeration with a single member.
enum enum2 { enum2_1=0u };
/// enum3 - An enumeration with multiple members.
enum enum3 { enum3_1=-10, enum3_2=0, enum3_3=10 };

/// fn1 - Returns the input enum1 value.
/// - x: The enum1 value to return.
inline enum1 fn1(enum1 x) { return x; }
/// fn1 - Compares two enum2 values for equality.
/// - a: The first enum2 value.
/// - b: The second enum2 value.
inline bool fn1(enum2 a, enum2 b) { return a == b; }
/// fn1 - Compares two enum3 values and returns one of the inputs.
/// - a: The first enum3 value.
/// - b: The second enum3 value.
/// - c: The third enum3 value.
inline enum3 fn1(enum3 a, enum3 b, enum3 c) { return a == b ? a : c; }

/// class1 - Null test for a struct/class. It has a public constructor.
class class1 {
public:
};

// - Classes

/// class2 - Two constructors and zero destructors.
class class2 {
public:
	/// class2 - Constructs a class2 object with an integer value.
	/// - x: An integer value to initialize m_x.
	explicit class2(int x) : m_x(x) { }
	/// class2 - Constructs a class2 object with a float value, cast to an integer.
	/// - x: A float value to initialize m_x.
	explicit class2(float x) : m_x((int)x) { }
	/// fn3 - Returns the private member m_x.
	int fn3(void) { return m_x; }
private:
	int m_x;
};

/// class3 - A complex class with a constructor, virtual destructor, and overloaded methods.
class class3 {
public:
	/// class3 - Constructs a class3 object, initializing all members to zero.
	explicit class3() : m_a(0), m_b(0), m_c(0), m_f(0.0f), m_g(0.0f), m_h(0.0f) { }
	/// ~class3 - Virtual destructor for class3.
	virtual ~class3() { }

	/// fn4 - Returns the sum of private member variables m_a, m_b, m_c, m_f, m_g, and m_h.
	/// Checks method overload resolution as well.
	inline float fn4(void) { return m_a + m_b + m_c + m_f + m_g + m_h; }
	/// fn4 - Sets the private member variable m_a and returns the sum of private members.
	/// - a: An integer value to assign to m_a.
	inline float fn4(int a) { m_a = a; return fn4(); }
	/// fn4 - Sets private member variables m_a and m_b and returns the sum of private members.
	/// - a: An integer value to assign to m_a.
	/// - b: An integer value to assign to m_b.
	inline float fn4(int a, int b) { m_a = a; m_b = b; return fn4(); }
	/// fn4 - Sets private member variables m_a, m_b, and m_c and returns the sum of private members.
	/// - a: An integer value to assign to m_a.
	/// - b: An integer value to assign to m_b.
	/// - c: An integer value to assign to m_c.
	inline float fn4(int a, int b, int c) { m_a = a; m_b = b; m_c = c; return fn4(); }
	/// fn4 - Sets the private member variable m_f and returns the sum of private members.
	/// - f: A float value to assign to m_f.
	inline float fn4(float f) { m_f = f; return fn4(); }
	/// fn4 - Sets private member variables m_f and m_g and returns the sum of private members.
	/// - f: A float value to assign to m_f.
	/// - g: A float value to assign to m_g.
	inline float fn4(float f, float g) { m_f = f; m_g = g; return fn4(); }
	/// fn4 - Sets private member variables m_f, m_g, and m_h and returns the sum of private members.
	/// - f: A float value to assign to m_f.
	/// - g: A float value to assign to m_g.
	/// - h: A float value to assign to m_h.
	inline float fn4(float f, float g, float h) { m_f = f; m_g = g; m_h = h; return fn4(); }

	/// fn5 - External linkage. Returns fn4();
	float fn5(void);

private:
	int m_a, m_b, m_c;
	float m_f, m_g, m_h;
};

#endif
