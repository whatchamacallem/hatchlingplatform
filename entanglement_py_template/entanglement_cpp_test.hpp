// Copyright 2017-2025 Adrian Johnston
#pragma once

#include <entanglement.h>

namespace ns0 {

// - Multiple declarations followed by a definition and 6 overloads.

/// `fn1` - Returns the sum of global variables g_a, g_b, g_c, g_f, g_g, and g_h.
ENTANGLEMENT_LINK float fn1(void);

/// `fn1` - Second declaration.
ENTANGLEMENT_LINK float fn1(void);

// - Overloads by arity.

// Should all be ignored.
namespace ns1 {
extern int g_a, g_b, g_c; // Ignored.
extern float g_f, g_g, g_h; // Ignored.
} // namespace ns1
using namespace ns1; // Ignored.

/// `fn1` - Sets global variables g_a and g_b and returns the sum of global variables.
/// - `a` : An integer value to assign to g_a.
/// - `b` : An integer value to assign to g_b.
ENTANGLEMENT_LINK float fn1(char a, short b);

// - Overloads by first paramater type (int vs. float) and then arity.

/// `fn1` - Sets the global variable g_f and returns the sum of global variables.
/// - `f` : A float value to assign to g_f.
ENTANGLEMENT_LINK float fn1(float f);

/// `fn1` - Sets global variables g_f, g_g, and g_h and returns the sum of global variables.
/// - `f` : A float value to assign to g_f.
/// - `g` : A float value to assign to g_g.
/// - `h` : A float value to assign to g_h.
ENTANGLEMENT_LINK float fn1(float* f, float g, float h);

ENTANGLEMENT_LINK float fn2(void);
ENTANGLEMENT_LINK float fn2(void);

} // namespace ns0

namespace ns0 {

// - Multiple declarations, external linkage.

/// `fn2` - Returns an integer.

// - Enums

/// `enum1` - An empty enumeration.
enum ENTANGLEMENT_TYPE enum1 { };
/// `enum2` - An enumeration with a single member.
enum class ENTANGLEMENT_TYPE enum2 { enum2_1=0u };
/// `enum3` - An enumeration with multiple members.
enum ENTANGLEMENT_TYPE enum3 : short { enum3_1=-10, enum3_2=0, enum3_3=10 };

/// `fn1` - Returns the input enum1 value.
/// - `x` : The enum1 value to return.
ENTANGLEMENT_LINK enum1 fn2(enum1 x);
/// `fn1` - Compares two enum2 values for equality.
/// - `a` : The first enum2 value.
/// - `b` : The second enum2 value.
ENTANGLEMENT_LINK bool fn2(enum2 a, enum2 b);
/// `fn1` - Compares two enum3 values and returns one of the inputs.
/// - `a` : The first enum3 value.
/// - `b` : The second enum3 value.
/// - `c` : The third enum3 value.
ENTANGLEMENT_LINK enum3 fn2(enum3 a, enum3 b, enum3 c);


/// `class1` - Null test for a struct/class. It has a public constructor.
class ENTANGLEMENT_TYPE class1 {
public:
};

// - Classes

/// `class2` - Two constructors and zero destructors.
class ENTANGLEMENT_TYPE class2 {
public:
	enum ENTANGLEMENT_TYPE : short { anonymous_1, anonymous_2, anonymous_3 };

	/// `class2` - Constructs a class2 object with an integer value.
	/// - `x` : An integer value to initialize m_x.
	ENTANGLEMENT_LINK explicit class2(int x);
	/// `class2` - Constructs a class2 object with a float value, cast to an integer.
	/// - `x` : A float value to initialize m_x.
	ENTANGLEMENT_LINK explicit class2(float x, float y);
	/// `fn3` - Returns the private member m_x.
	ENTANGLEMENT_LINK int fn3(void);
private:
	int m_x;
};

/// `class3` - A complex class with a constructor, virtual destructor, and overloaded methods.
class ENTANGLEMENT_TYPE class3 {
public:
	/// `class3` - Constructs a class3 object, initializing all members to zero.
	ENTANGLEMENT_LINK explicit class3(void);
	/// `~class3` - Virtual destructor for class3.
	ENTANGLEMENT_LINK virtual ~class3(void);

	/// `fn4` - Returns the sum of private member variables m_a, m_b, m_c, m_f, m_g, and m_h.
	/// Checks method overload resolution as well.
	ENTANGLEMENT_LINK float fn4(void);
	/// `fn4` - Sets private member variables m_a and m_b and returns the sum of private members.
	/// - `a` : An integer value to assign to m_a.
	/// - `b` : An integer value to assign to m_b.
	ENTANGLEMENT_LINK float fn4(int a, int b);
	/// `fn4` - Sets the private member variable m_f and returns the sum of private members.
	/// - `f` : A float value to assign to m_f.
	ENTANGLEMENT_LINK float fn4(float f);
	/// `fn4` - Sets private member variables m_f, m_g, and m_h and returns the sum of private members.
	/// - `f` : A float value to assign to m_f.
	/// - `g` : A float value to assign to m_g.
	/// - `h` : A float value to assign to m_h.
	ENTANGLEMENT_LINK float fn4(float f, float g, float h);

	/// `fn5` - External linkage. Returns fn4();
	ENTANGLEMENT_LINK float fn5(void);

private:
	int m_a, m_b, m_c;
	float m_f, m_g, m_h;
};

} // namespace ns0
