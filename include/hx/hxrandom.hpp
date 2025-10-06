#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxrandom.hpp Provides a random number generator that automatically
/// casts itself in well-defined ways. Useful for test data.

#include "hatchling.h"

/// `hxrandom` - 64-bit MMIX LCG. Knuth, D. 2002. (Modified to perturb the return
/// value so that all bits are of equal quality.) Uses a floating point multiply
/// instead of integer modulo when generating numbers in a range. Requires at
/// least 64-bit integer emulation as well. Automatically casts to any unsigned
/// integer or floating point value. Operator overloads allow bitwise operations
/// with random numbers that are signed as well. Usable as a functor or via the
/// provided cast operator for your type. Has a period of `2^64` and passes
/// routine numerical tests with only eight bytes of state while using simple
/// arithmetic. Intended for test data or games, not mathematical applications.
class hxrandom {
public:
	/// Constructor to initialize the random number generator.
	/// - `stream` : Index or seed value for a given stream of random numbers.
	hxrandom(uint64_t stream_ = 1u) : m_state_(stream_) { }

	/// Functor returns `hxrandom&` which converts itself to the type it is
	/// assigned to. Enables traditional syntax.
	/// e.g., `uint32_t = m_prng(); // Returns [0..2^32).`
	/// e.g., `double i = m_prng(); // Returns [0..1).`
	hxrandom& operator()(void) { return *this; }

	/// Automatically casts to an unsigned integer or floating point value.
	/// Floating-point results are between `[0..1)`. They can safely be used to
	/// generate array indices without overflowing. e.g.,
	/// `unsigned int = m_prng; // Returns [0..UINT_MAX].`
	operator float(void) {
		return (float)this->generate32() * (1.0f / 4294967296.0f); // 0x1p-32f
	}
	operator double(void) {
		return (double)this->generate64() * (1.0 / 18446744073709551616.0); // 0x1p-64;
	}

	operator uint8_t(void) { return (uint8_t)this->generate32(); }
	operator uint16_t(void) { return (uint16_t)this->generate32(); }
	operator uint32_t(void) { return this->generate32(); }
	operator uint64_t(void) { return this->generate64(); }

	/// Returns a random number in the range [base..base+range).
	/// `range(0.0f,10.0f)` returns `0.0f` to `9.999f` and not `10.0f`. Uses a
	/// floating point multiply instead of a divide. `base + size` must not
	/// overflow the type and `size` must be positive.
	/// - `base` : The beginning of the range. e.g., 0.
	/// - `size` : Positive size of the range. e.g., 10 elements.
	template<typename T_> T_ range(T_ base_, T_ size_) {
		// Use double parameters if you need a bigger size. An emulated
		// floating point multiply is faster and more stable than integer modulo.
		hxassertmsg((float)size_ < (float)0x01000000, "insufficient_precision %f", (float)size_); // 0x1p24f
		return base_ + (T_)((float)size_ * (float)*this);
	}
	double range(double base_, double size_) {
		// Use `uint64_t` parameters if you need a bigger size. An emulated
		// floating point multiply is faster and more stable than integer modulo.
		hxassertmsg(size_ < (double)0x40000000000000ll, "insufficient_precision %f", (double)size_); // 0x1p54f
		return base_ + size_ * (double)*this;
	}

	// Negative size is undefined.
	int64_t range(int64_t base_, int64_t size_) {
		return base_ + (int64_t)(this->generate64() % (uint64_t)size_);
	}
	uint64_t range(uint64_t base_, uint64_t size_) {
		return base_ + this->generate64() % size_;
	}

	/// Reads a specified number of random bytes into the provided buffer. The
	/// sequence generated matches a little-endian stream of
	/// `uint32_t` generated using `generate32`.
	/// - `bytes` : Pointer to the buffer where the random bytes will be stored.
	/// - `count` : Number of bytes to read.
	void read(void* bytes_, size_t count_) hxattr_nonnull(2) {
		uint8_t* chars_ = (uint8_t*)bytes_;
		while(count_>=4) {
			uint32_t x_ = this->generate32();
			*chars_++ = (uint8_t)x_;
			*chars_++ = (uint8_t)(x_ >> 8);
			*chars_++ = (uint8_t)(x_ >> 16);
			*chars_++ = (uint8_t)(x_ >> 24);
			count_ -= 4;
		}
		if(count_) {
			uint32_t x_ = this->generate32();
			do {
				*chars_++ = (uint8_t)x_;
				x_ >>= 8;
			} while(--count_)
				/* */;
		}
	}

	/// Returns a pseudorandom number in the interval `[0..2^32)`.
	uint32_t generate32(void) {
		m_state_ = (uint64_t)0x5851f42d4c957f2dull * m_state_ + (uint64_t)0x14057b7ef767814full;

		// MODIFICATION: Use the 4 msb bits as a random 0..15 bit variable shift
		// control. Ignores the low 13 bits because they are low quality.
		// Returns 32 bits chosen at a random offset starting between the 13th
		// and 28th bits. 4 bits shift control + 32 returned + up to 15 shifted
		// off + 13 always discarded = 64 bits.
		uint32_t result_ = (uint32_t)(m_state_ >> ((m_state_ >> 60) + 13u));
		return result_;
	}

	/// Returns a pseudorandom number in the interval `[0..2^64)`.
	uint64_t generate64(void) {
		uint64_t result_ = (uint64_t)this->generate32() | ((uint64_t)this->generate32() << 32);
		return result_;
	}

private:
	uint64_t m_state_;
};

/// `operator&(hxrandom& a, T b)` - Bitwise `&` with random `T` generated from
/// `a`.
/// - `a` : Bits to mask off. Undefined behavior when a negative integer.
/// - `b` : A `hxrandom`.
template <typename T_> T_ operator&(T_ a_, hxrandom& b_) {
	return T_((uint32_t)a_ & b_.generate32());
}

/// `operator&(int64_t a_, hxrandom& b_)` - 64-bit version. Allows a signed
/// 64-bit type here because it can be generated without automatically hitting
/// undefined behavior.
/// - `a` : Bits to mask off. Undefined behavior when negative.
/// - `b` : A `hxrandom`.
inline int64_t operator&(int64_t a_, hxrandom& b_) {
   return (int64_t)((uint64_t)a_ & b_.generate64());
}

/// `operator&(uint64_t a_, hxrandom& b_)` - 64-bit version.
inline uint64_t operator&(uint64_t a_, hxrandom& b_) {
	return a_ & b_.generate64();
}

/// `operator&(T a, hxrandom& b)` - Bitwise `&` with random `T` generated from `a`.
/// - `a` : A `hxrandom`.
/// - `b` : Bits to mask off. Must be a positive integer.
template<typename T_> T_ operator&(hxrandom& a_, T_ b_) {
	return b_ & a_;
}

/// `operator&=(T& a, hxrandom& b)` - Bitwise `&=` with random `T` generated
/// from `b`.
/// - `a` : Bits to mask off. Must be a positive integer.
/// - `b` : A `hxrandom`.
template<typename T_> T_ operator&=(T_& a_, hxrandom& b_) {
	return (a_ = a_ & b_);
}

/// `operator%(hxrandom& a, T_ b)` - Generates a number of type `T` in the range
/// `[0..b)`. Works with floating point divisors and uses no actual modulo or
/// division.
template<typename T_> T_ operator%(hxrandom& dividend_, T_ divisor_) {
	return dividend_.range(T_(0), divisor_);
}
