#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "hatchling.h"

#if HX_CPLUSPLUS
extern "C" {
#endif

/// `hxhex_dump` - Prints an array of bytes formatted in hexadecimal. Additional
/// information provided when pretty is non-zero.
/// - `address` : Pointer to the start of the byte array.
/// - `bytes` : The number of bytes to print.
/// - `pretty` : Set non-zero to include extended visualization.
void hxhex_dump(const void* address_, size_t bytes_, int pretty_) hxattr_nonnull(1) hxattr_cold;

/// `hxfloat_dump` - Prints an array of floating point values.
/// - `address` : Pointer to the start of the float array.
/// - `floats` : The number of floats to print.
void hxfloat_dump(const float* address_, size_t floats_) hxattr_nonnull(1) hxattr_cold;

/// `hxbasename` - Returns a pointer to those characters following the last `\` or
/// `/` character or path if those are not present.
/// - `path` : The file path as a null-terminated string.
const char* hxbasename(const char* path_) hxattr_nonnull(1);

// ----------------------------------------------------------------------------
// These are <ctypes.h>.

/// Implements standard `isgraph` for a locale where all non-ASCII characters
/// are considered graphical or mark making. This is compatable with scanf-style
/// parsing of UTF-8 string parameters. However, this is not `en_US.UTF-8` or
/// the default C locale.
inline bool hxisgraph(char ch_) {
	return ((unsigned char)ch_ - 0x21u) < 0x5Eu
		|| ((unsigned char)ch_ & 0x80u);
}

/// Implements standard `isspace` for a locale where all non-ASCII characters
/// are considered graphical or mark making. Returns nonzero for space and
/// `\t \n \v \f \r`. This is compatable with scanf-style parsing of
/// UTF-8 string parameters. However, this is not `en_US.UTF-8` or the default
/// C locale.
inline bool hxisspace(char ch_) {
	return ch_ == ' ' || ((unsigned char)ch_ - 0x09u) < 0x05u;
}

/// Returns `log2(n)` as an integer which is the power of 2 of the largest bit
/// in `n`. NOTA BENE: `hxlog2i(0)` is currently -127 and is undefined.
/// - `i` : A `size_t`.
inline int hxlog2i(size_t i_) {
	// Use the floating point hardware because this isn't important enough for
	// The memcpy is an intrinsic.
	float f_ = (float)i_; uint32_t bits_; memcpy(&bits_, &f_, sizeof f_);
	return (int)((bits_ >> 23) & 0xffu) - 127;
}

/// Returns true if the float `x` is finite (not NaN or ±inf). Implements `isfinitef`.
inline int hxisfinitef(float x_) {
	uint32_t u_; memcpy(&u_, &x_, sizeof u_); // An intrinsic.
	return (u_ & 0x7f800000u) != 0x7f800000u;
}

/// Returns true if the double `x` is finite (not NaN or ±inf). Implements `isfinitel`.
inline int hxisfinitel(double x_) {
	uint64_t u_; memcpy(&u_, &x_, sizeof u_); // An intrinsic.
	return (u_ & 0x7ff0000000000000ull) != 0x7ff0000000000000ull;
}

// ----------------------------------------------------------------------------
// C++ Template Utility API

#if HX_CPLUSPLUS
} // extern "C"

/// `hxnullptr_t` - A class that will only convert to a null `T` pointer. Useful
/// when an integer constant arg would be ambiguous or otherwise break template
/// code. `hxnullptr` is a `hxnullptr_t`. Use plain `hxnull` for comparisons.
class hxnullptr_t {
public:
	/// Null `T` pointer.
	template<typename T_> constexpr operator T_*() const { return 0; }
	/// Null `T` member function pointer.
	template<typename T_, typename M_> constexpr operator M_ T_::*() const { return 0; }
	/// No address-of operator.
	void operator&() const = delete;
};

/// `hxnullptr` - An instance of a class that will only convert to a null
/// pointer. Useful when an integer constant arg would be ambiguous or otherwise
/// break template code. `hxnullptr` is a `hxnullptr_t`. Use plain `hxnull` for
/// `==` and `!=` comparisons.
#define hxnullptr hxnullptr_t()

/// Implements `std::enable_if`. This is available instead of the `requires`
/// keyword when backwards compatibility is required. Used by `hxenable_if_t.`
template<bool condition_, typename type_=void> struct hxenable_if { };
template<typename type_> struct hxenable_if<true, type_> { using type = type_; };

/// hxenable_if_t<condition> - Implements `std::enable_if_t`. This is available
/// instead of the `requires` keyword when backwards compatibility is required.
template<bool condition_, typename type_=void>
using hxenable_if_t = typename hxenable_if<condition_, type_>::type;

/// Internal. Returns `T` with references removed as
/// `hxremove_reference_<T>::type`. Used by `hxremove_reference_t`.
template<class T_> struct hxremove_reference_       { using type = T_; };
template<class T_> struct hxremove_reference_<T_&>  { using type = T_; };
template<class T_> struct hxremove_reference_<T_&&> { using type = T_; };

/// hxremove_reference_t<T> - Returns `T` with references removed.
template<class T_> using hxremove_reference_t = typename hxremove_reference_<T_>::type;

/// Implements `std::true_type`/`std::false_type` without going into the weeds.
struct hxfalse_t { enum { value = 0 }; };
struct hxtrue_t { enum { value = 1 }; };

/// Implements `std::is_lvalue_reference`.
template<typename T_> struct hxis_lvalue_reference : public hxfalse_t { };
template<typename T_> struct hxis_lvalue_reference<T_&> : public hxtrue_t { };

/// Internal. Implements `std::remove_cv`.
template<class T_> struct hxremove_cv_ { using type = T_; };
template<class T_> struct hxremove_cv_<const T_> { using type = T_; };
template<class T_> struct hxremove_cv_<volatile T_> { using type = T_; };
template<class T_> struct hxremove_cv_<const volatile T_> { using type = T_; };

/// Removes const and volatile from a type. Implements `std::remove_cv_t`.
/// This is used to maintain semantic compatibility with the standard.
template<class T_> using hxremove_cv_t = typename hxremove_cv_<T_>::type;

/// Internal. Returns `std::is_pointer` as `hxis_pointer_<T>::type` but without
/// handling cv.
template<typename T_> struct hxis_pointer_ : public hxfalse_t { };
template<typename T_> struct hxis_pointer_<T_*> : public hxtrue_t { };

/// Returns whether T is a pointer type as `hxis_pointer_<T>::type`. Implements
/// `std::is_pointer`.
template<class T> struct hxis_pointer : hxis_pointer_<hxremove_cv_t<T>> { };

/// Internal. Adds the `__restrict` keyword to C++ pointers. Used by
/// `hxadd_attr_if_ptr_t`.
template<class T_, int = hxis_pointer<T_>::value> struct hxrestrict_t_;
template<class T_> struct hxrestrict_t_<T_, 0> { using type = T_; };
template<class T_> struct hxrestrict_t_<T_, 1> { using type = T_ hxrestrict; };

/// Adds the `__restrict` keyword to C++ pointers.
template<class T_> using hxrestrict_t = typename hxrestrict_t_<T_>::type;

/// Implements `std::move`. Converts either a `T&` or a `T&&` to a `T&&`. Do not
/// specify `T` explicitly as it will not work as expected. This uses the rules
/// about reference collapsing to handle both `T&` and `T&&`.
template<class T_>
constexpr hxremove_reference_t<T_>&& hxmove(T_&& t_) {
	return static_cast<hxremove_reference_t<T_>&&>(t_);
}

/// Implements `std::forward`. Call as `hxforward<T>(x)` **from inside a
/// templated forwarding function** where the parameter was declared `T&&` and
/// `T` was **deduced**. `T` must be explicitly specified. E.g.,
/// ```cpp
///   template<class T>
///   void forwards_temp(T&&x) { requires_temp(hxforward<T>(x)); }
/// ```
/// This is the `T&&` version of hxforward<T>.
template<class T_>
constexpr T_&& hxforward(hxremove_reference_t<T_>&& t) noexcept {
	static_assert(!hxis_lvalue_reference<T_>::value, "T must be a `T&&` reference.");
	return static_cast<T_&&>(t);
}

/// This is the `T&` version of hxforward<T>. It gets invoked when `T` turns out
/// to be an l-value. This happens then a `T&` is passed as a `T&&`.
template<class T_>
constexpr T_&& hxforward(hxremove_reference_t<T_>& t) noexcept {
	return static_cast<T_&&>(t);
}

/// `hxmin` - Returns the minimum value of `x` and `y` using a `<` comparison.
/// `operator<`. Returns the minimum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
template<typename T_>
constexpr const T_& hxmin(const T_& x_, const T_& y_) { return ((x_) < (y_)) ? (x_) : (y_); }

/// `hxmax` - Returns the maximum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
template<typename T_>
constexpr const T_& hxmax(const T_& x_, const T_& y_) { return ((y_) < (x_)) ? (x_) : (y_); }

/// `hxabs` - Returns the absolute value of `x` using a `<` comparison.
/// - `x` : The value to compute the absolute value for.
template<typename T_>
constexpr const T_ hxabs(const T_& x_) { return ((x_) < (T_)0) ? ((T_)0 - (x_)) : (x_); }

/// `hxclamp` - Returns `x` clamped between the `minimum` and `maximum` using `<`
/// comparisons.
/// - `x` : The value to clamp.
/// - `minimum` : The minimum allowable value.
/// - `maximum` : The maximum allowable value.
template<typename T_>
constexpr const T_& hxclamp(const T_& x_, const T_& minimum_, const T_& maximum_) {
	hxassertmsg(!(maximum_ < minimum_), "minimum <= maximum");
	return (x_ < minimum_) ? minimum_ : ((maximum_ < x_) ? maximum_ : x_);
}

/// `hxswap` - Exchanges the contents of `x` and `y` using a temporary. If `T`
/// has `T::T(T&&)` or `T::operator=(T&&)` then those will be used.
template<typename T_>
constexpr void hxswap(T_& x_, T_& y_) {
	hxassertmsg(&x_ != &y_, "hxswap No swapping with self.");
	T_ t_(hxmove(x_));
	x_ = hxmove(y_);
	y_ = hxmove(t_);
}

/// `hxswap_memcpy` - Exchanges the contents of `x` and `y` using `memcpy` and a
/// stack temporary. This is intended for internal use where it is known to be
/// safe to do so. It is a cheap way to implement `T::operator=(T&&)`.
/// - `x` : First `T&`.
/// - `y` : Second `T&`.
template<typename T_>
constexpr void hxswap_memcpy(T_& x_, T_& y_) {
	hxassertmsg(&x_ != &y_, "hxswap_memcpy No swapping with self.");
	char t_[sizeof x_];
	::memcpy(t_, &y_, sizeof x_);
	::memcpy((void*)&y_, &x_, sizeof x_);
	::memcpy((void*)&x_, t_, sizeof x_);
}

#else // !HX_CPLUSPLUS
// ----------------------------------------------------------------------------
// C Macro Utility API - Does it all backwards in heels.

/// `hxmin` - Returns the minimum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
#define hxmin(x_, y_) ((x_) < (y_) ? (x_) : (y_))

/// `hxmax` - Returns the maximum value of `x` and `y` using a `<` comparison.
/// - `x` : The first value.
/// - `y` : The second value.
#define hxmax(x_, y_) ((y_) < (x_) ? (x_) : (y_))

/// `hxabs` - Returns the absolute value of `x` using a `<` comparison.
/// - `x` : The value to compute the absolute value for.
#define hxabs(x_) ((x_) < 0 ? (0 - (x_)) : (x_))

/// `hxclamp` - Returns `x` clamped between the `minimum` and `maximum` using `<`
/// comparisons.
/// - `x` : The value to clamp.
/// - `minimum` : The minimum allowable value.
/// - `maximum` : The maximum allowable value.
#define hxclamp(x_, minimum_, maximum_) \
	((x_) < (minimum_) ? (minimum_) : ((maximum_) < (x_) ? (maximum_) : (x_)))

/// `hxswap_memcpy` - Exchanges the contents of `x` and `y` using `memcpy` and a
/// stack temporary. This is intended for internal use where it is known to be
/// safe to do so.
/// - `x` : First object.
/// - `y` : Second object.
#define hxswap_memcpy(x_,y_) do { \
	char t_[sizeof(x_) == sizeof(y_) ? (int)sizeof(x_) : -1]; \
	memcpy(t_, &(y_), sizeof(x_)); \
	memcpy(&(y_), &(x_), sizeof(x_)); \
	memcpy(&(x_), t_, sizeof(x_)); } while(0)

#endif // !HX_CPLUSPLUS
