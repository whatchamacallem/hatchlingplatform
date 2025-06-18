#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxRandom - 64-bit MMIX LCG. Knuth, D. 2002 (Modified to perturb return.)
// Performs an automatic cast to any integer or floating point value. Generates
// a new value every time. Usable as a functor or by simply calling the
// provided cast operator to your type. Has a period of 2^64 and passes routine
// numerical tests with only 8 bytes of state and using simple arithmetic.
struct hxRandom {
public:
	// Constructor to initialize the random number generator with a seed.
    // - seed: Initial seed value for the random number generator.
    HX_CONSTEXPR_FN hxRandom(uint32_t seed_ = 1u) : m_state_(seed_) { }

    // Functor returns hxRandom& which converts itself to the type it is
    // assigned to. Enables traditional syntax.
    // E.g. unsigned int i = m_prng(); // Returns type.
    // E.g. double i = m_prng(); // Returns [0..1).
    HX_CONSTEXPR_FN hxRandom& operator()(void) { return *this; }

    // Automatic cast to any integer or floating point value. Floating point
    // results are between [0..1).  They can safely be used to generate array
    // indicies without overflowing.
    // E.g. unsigned int = m_prng; // Deduces the required type.
    template<typename T_> HX_CONSTEXPR_FN operator T_() {
        return static_cast<T_>(this->advance32_());
    }
    HX_CONSTEXPR_FN operator float() {
        return (float)this->advance32_() * 0x1p-32f;
    }
    HX_CONSTEXPR_FN operator double() {
        return (double)this->advance64_() * 0x1p-64;
    }
    HX_CONSTEXPR_FN operator int64_t() {
        return (int64_t)this->advance64_();
    }
    HX_CONSTEXPR_FN operator uint64_t() {
        return this->advance64_();
    }

    // Returns a random number in the range [base..base+range). range(0.0f,10.0f)
    // will return 0.0f to 9.999f and not 10.0f. Uses a floating point multiply
    // instead of a divide. base + size must not overflow type and size must be
    // positive.
    // - base: The beginning of the range. E.g. 0.
    // - size: Positive size of the range. E.g. 10 elements.
    template<typename T_> HX_CONSTEXPR_FN T_ range(T_ base_, T_ size_) {
        // Use double parameters if you need a bigger size. An emulated floating
        // point multiply is faster and more stable than integer modulo.
        hxAssertMsg((float)size_ < 0x1p24f, "insufficient precision");
        return base_ + (T_)((float)size_ * 0x1p-32f * (float)this->advance32_());
    }
    HX_CONSTEXPR_FN double range(double base_, double size_) {
        // Use uint64_t parameters if you need a bigger size. An emulated floating
        // point multiply is faster and more stable than integer modulo.
        hxAssertMsg(size_ < (double)0x1p54f, "insufficient precision");
        return base_ + size_ * (double)0x1p-64f * (double)this->advance64_();
    }
    HX_CONSTEXPR_FN int64_t range(int64_t base_, int64_t size_) {
        return base_ + (int64_t)(this->advance64_() % size_);
    }
    HX_CONSTEXPR_FN uint64_t range(uint64_t base_, uint64_t size_) {
        return base_ + this->advance64_() % size_;
    }

private:
    HX_CONSTEXPR_FN uint32_t advance32_(void) {
        m_state_ = 0x5851f42d4c957f2dull * m_state_ + 0x14057b7ef767814full;

        // MODIFICATION: Use the 4 msb bits as a 0..15 bit variable shift control.
        // Ignores the low 13 bits because they are low quality. Returns 32 bits
        // chosen at a random offset starting between the 13th and 28th bits.
        // 4 bits shift control + 32 returned + up to 15 shifted + 13 discarded.
        uint32_t result_ = (uint32_t)(m_state_ >> ((unsigned int)(m_state_ >> 60) + 13u));
        return result_;
    }

    HX_CONSTEXPR_FN uint64_t advance64_(void) {
        uint64_t result_ = (uint64_t)this->advance32_() | ((uint64_t)this->advance32_() << 32);
        return result_;
    }

    uint64_t m_state_;
};

// operator&(hxRandom& a, T b) - Bitwise & with random T generated from a.
template <typename T_> inline T_ operator&(hxRandom& a_, T_ b_) { return T_(a_) & b_; }

// operator&(T a, hxRandom& b) - Bitwise & with random T generated from b.
template <typename T_> inline T_ operator&(T_ a_, hxRandom& b_) { return a_ & T_(b_); }

// operator&=(T& a, hxRandom& b) - Bitwise &= with random T generated from b.
template <typename T_> inline T_ operator&=(T_& a_, hxRandom& b_) { a_ &= T_(b_); return a_; }

// operator%(hxRandom&, T_) - Generate an number of type T in the range 0..b.
// Works with floating point divisors and uses no actual modulo or division.
template <typename T_> inline T_ operator%(hxRandom& dividend_, T_ divisor_) {
    return dividend_.range((T_)0, divisor_);
}
