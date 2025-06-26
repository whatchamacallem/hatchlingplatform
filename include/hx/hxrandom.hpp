#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

/// hxrandom - 64-bit MMIX LCG. Knuth, D. 2002 (Modified to perturb return.)
/// Performs an automatic cast to any integer or floating point value. Generates
/// a new value every time. Usable as a functor or by simply calling the
/// provided cast operator to your type. Has a period of 2^64 and passes routine
/// numerical tests with only 8 bytes of state and using simple arithmetic.
/// Entirely constexpr if you need test data for template parameters.
class hxrandom {
public:
	/// Constructor to initialize the random number generator with a seed.
    /// - seed: Initial seed value for the random number generator.
    hxconstexpr_fn hxrandom(uint32_t seed_ = 1u) : m_state_(seed_) { }

    /// Functor returns hxrandom& which converts itself to the type it is
    /// assigned to. Enables traditional syntax.
    /// E.g. uint32_t = m_prng(); // Returns [0..2^32).
    /// E.g. double i = m_prng(); // Returns [0..1).
    hxconstexpr_fn hxrandom& operator()(void) { return *this; }

    /// Automatic cast to unsigned integer or floating point value. Floating point
    /// results are between [0..1).  They can safely be used to generate array
    /// indicies without overflowing.
    /// E.g. unsigned int = m_prng; // Returns [0..UINT_MAX].
    hxconstexpr_fn operator float(void) {
        return (float)this->advance32() * (1.0f / 4294967296.0f); // 0x1p-32f
    }
    hxconstexpr_fn operator double(void) {
        return (double)this->advance64() * (1.0 / 18446744073709551616.0); // 0x1p-64;
    }
    hxconstexpr_fn operator uint8_t(void) {
        return this->advance32();
    }
    hxconstexpr_fn operator uint16_t(void) {
        return this->advance32();
    }
    hxconstexpr_fn operator uint32_t(void) {
        return this->advance32();
    }
    hxconstexpr_fn operator uint64_t(void) {
        return this->advance64();
    }

    /// Returns a random number in the range [base..base+range). range(0.0f,10.0f)
    /// will return 0.0f to 9.999f and not 10.0f. Uses a floating point multiply
    /// instead of a divide. base + size must not overflow type and size must be
    /// positive.
    /// - base: The beginning of the range. E.g. 0.
    /// - size: Positive size of the range. E.g. 10 elements.
    template<typename T_> hxconstexpr_fn T_ range(T_ base_, T_ size_) {
        // Use double parameters if you need a bigger size. An emulated floating
        // point multiply is faster and more stable than integer modulo.
        hxassertmsg((float)size_ < (float)0x01000000, "insufficient_precision %f", (float)size_); // 0x1p24f
        return base_ + (T_)((float)size_ * (float)*this);
    }
    hxconstexpr_fn double range(double base_, double size_) {
        // Use uint64_t parameters if you need a bigger size. An emulated floating
        // point multiply is faster and more stable than integer modulo.
        hxassertmsg(size_ < (double)0x40000000000000ll, "insufficient_precision %f", (double)size_); // 0x1p54f
        return base_ + size_ * (double)*this;
    }
    hxconstexpr_fn int64_t range(int64_t base_, int64_t size_) {
        return base_ + (int64_t)(this->advance64() % size_);
    }
    hxconstexpr_fn uint64_t range(uint64_t base_, uint64_t size_) {
        return base_ + this->advance64() % size_;
    }

    /// Returns [0..2^32).
    hxconstexpr_fn uint32_t advance32(void) {
        m_state_ = 0x5851f42d4c957f2dull * m_state_ + 0x14057b7ef767814full;

        // MODIFICATION: Use the 4 msb bits as a 0..15 bit variable shift control.
        // Ignores the low 13 bits because they are low quality. Returns 32 bits
        // chosen at a random offset starting between the 13th and 28th bits.
        // 4 bits shift control + 32 returned + up to 15 shifted + 13 discarded.
        uint32_t result_ = (uint32_t)(m_state_ >> ((unsigned int)(m_state_ >> 60) + 13u));
        return result_;
    }

    /// Returns [0..2^64).
    hxconstexpr_fn uint64_t advance64(void) {
        uint64_t result_ = (uint64_t)this->advance32() | ((uint64_t)this->advance32() << 32);
        return result_;
    }

private:
    uint64_t m_state_;
};

/// operator&(hxrandom& a, T b) - Bitwise & with random T generated from a.
/// a - Bits to mask off. Undefined behavior when a negative integer.
/// b - A hxrandom.
template <typename T_> hxconstexpr_fn  T_ operator&(T_ a_, hxrandom& b_) {
    return T_((uint32_t)a_ & b_.advance32());
}
/// operator&(int64_t a_, hxrandom& b_) - Allow a signed 64-bit type here because
/// it can be generated without automatically hitting undefined behaviour.
/// a - Bits to mask off. Undefined behavior when negative.
/// b - A hxrandom.
hxconstexpr_fn  int64_t operator&(int64_t a_, hxrandom& b_) {
   return (int64_t)((uint64_t)a_ & b_.advance64());
}
hxconstexpr_fn  uint64_t operator&(uint64_t a_, hxrandom& b_) {
    return a_ & b_.advance64();
}

/// operator&(T a, hxrandom& b) - Bitwise & with random T generated from a.
/// a - A hxrandom.
/// b - Bits to mask off. Must be a positive integer.
template <typename T_> hxconstexpr_fn  T_ operator&(hxrandom& a_, T_ b_) {
    return b_ & a_;
}

/// operator&=(T& a, hxrandom& b) - Bitwise &= with random T generated from b.
/// a - Bits to mask off. Must be a positive integer.
/// b - A hxrandom.
template <typename T_> hxconstexpr_fn  T_ operator&=(T_& a_, hxrandom& b_) {
    return (a_ = a_ & b_);
}

/// operator%(hxrandom& a, T_ b) - Generate an number of type T in the range 0..b.
/// Works with floating point divisors and uses no actual modulo or division.
template <typename T_> hxconstexpr_fn  T_ operator%(hxrandom& dividend_, T_ divisor_) {
    return dividend_.range((T_)0, divisor_);
}
