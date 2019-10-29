#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// hxConsole internals.  See hxConsole.h instead.

struct hxCommand {
	virtual bool execute(const char* str_) = 0; // Return false for parse errors.
	virtual void usage(const char* id_=hxnull) = 0; // Expects command name.
};

// Explicit registration, takes ownership of fn, id expected to be a string literal.
void hxConsoleRegister(hxCommand* fn_, const char* id_);

struct hxConsoleConstructor {
	HX_INLINE hxConsoleConstructor(hxCommand* fn_, const char* id_) {
		hxConsoleRegister(fn_, id_);
	}
};

// Console tokens are delimited by any whitespace and non-printing low-ASCII
// characters.  NUL must be checked for separately and UTF-8 is not supported.
HX_INLINE static bool hxIsDelimiter(char ch_) { return ch_ <= 32; }

// Checks for printing characters.
HX_INLINE static bool hxIsEndOfLine(const char* str_) {
	while (*str_ != 0 && *str_ <= 32) {
		++str_;
	}
	return *str_ == 0 || *str_ == '#'; // Skip comments
}

// Unsigned min<T>()/max<T>().  Yes, re-implementing std::min<T>()/max<T>() is
// a bit much.  But it makes this freestanding.
template<bool IsSigned_, typename T_> struct hxLimitsSelect {
	static HX_CONSTEXPR_FN T_ minVal() { return T_(0); }
	static HX_CONSTEXPR_FN T_ maxVal() { return ~T_(0); }
};

// Signed min<T>()/max<T>().  Does silly things to avoid overflowing a signed
// integer type.
template<typename T_> struct hxLimitsSelect<true, T_> {
	static HX_CONSTEXPR_FN T_ minVal() { return T_(-1) - maxVal(); }
	static HX_CONSTEXPR_FN T_ maxVal() { return msb1_() | (msb1_() - T_(1)); }
private:
	// Using CHAR_BIT would require <limits.h> and this code assumes <stdint.h>
	// types are being used anyway.
	static HX_CONSTEXPR_FN T_ msb1_() { return T_(1) << (sizeof(T_) * 8 - 2); }
};

// select an implementation of min<T>()/max<T>() depending on whether T is signed.
template<typename T_> struct hxLimits : public hxLimitsSelect<(T_)~T_(0) < T_(0), T_> { };

// Wrapper for strtol() style parser.  You may not want to force inlining of this.
template <typename T_, typename R_>
HX_INLINE void hxArgParse(T_& val_, const char* str_, char** next_, R_(*parser_)(char const*, char**, int)) {
	R_ r_ = parser_(str_, next_, 10);
	// These compares are optimized away when not needed.
	if (r_ < hxLimits<T_>::minVal() || r_ > hxLimits<T_>::maxVal()) {
		hxWarn("overflow");
		*next_ = const_cast<char*>(str_); // reject input.
		val_ = (T_)0; // Otherwise gcc will incorrectly complain.
		return;
	}
	val_ = (T_)r_;
	return;
}

// hxArg<T>. Binds string parsing operations to function args.  Invalid arguments are
// set to 0, arguments out of range result in the maximum representable values.

template<typename T_> struct hxArg; // Undefined. Specialization required.

template<> struct hxArg<int8_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel() { return "s8"; }
	int8_t value;
};
template<> struct hxArg<int16_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel() { return "s16"; }
	int16_t value;
};
template<> struct hxArg<int32_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel() { return "s32"; }
	int32_t value;
};
#if HX_USE_64_BIT_TYPES
template<> struct hxArg<int64_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtoll); }
	HX_INLINE static const char* getLabel() { return "s64"; }
	int64_t value;
};
#endif
template<> struct hxArg<uint8_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel() { return "u8"; }
	uint8_t value;
};
template<> struct hxArg<uint16_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel() { return "u16"; }
	uint16_t value;
};
template<> struct hxArg<uint32_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel() { return "u32"; }
	uint32_t value;
};
#if HX_USE_64_BIT_TYPES
template<> struct hxArg<uint64_t> {
	HX_INLINE hxArg(const char* str_, char** next_) { hxArgParse(value, str_, next_, ::strtoull); }
	HX_INLINE static const char* getLabel() { return "u64"; }
	uint64_t value;
};
#endif
template<> struct hxArg<float> {
	HX_INLINE hxArg(const char* str_, char** next_) : value(::strtof(str_, next_)) { }
	HX_INLINE static const char* getLabel() { return "f32"; }
	float value;
};
template<> struct hxArg<double> {
	HX_INLINE hxArg(const char* str_, char** next_) : value(::strtod(str_, next_)) { }
	HX_INLINE static const char* getLabel() { return "f64"; }
	double value;
};
// bool params must be 0 or 1.
template<> struct hxArg<bool> {
	HX_INLINE hxArg(const char* str_, char** next_) {
		unsigned long t_ = ::strtoul(str_, next_, 2);
		value = (t_ != 0ul);
	}
	HX_INLINE static const char* getLabel() { return "0/1"; }
	bool value;
};

// const char* args capture remainder of line including #'s.
template<> struct hxArg<const char*> {
	HX_INLINE hxArg(const char* str_, char** next_) {
		while (*str_ != '\0' && hxIsDelimiter(*str_)) { // Skip leading whitespace
			++str_;
		}
		value = str_;
		*next_ = const_cast<char*>("");
	}
	HX_INLINE static const char* getLabel() { return "string"; }
	const char* value;
};

template<typename R_>
struct hxCommand0 : public hxCommand {
	hxCommand0(R_(*fn_)()) : m_fn(fn_) { }
	virtual bool execute(const char* str_) HX_OVERRIDE {
		if(hxIsEndOfLine(str_)) {
			m_fn();
			return true;
		}

		usage();
		return false;
	}
	virtual void usage(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s\n", id_ ? id_ : "use no args"); (void)id_;
	}
	R_(*m_fn)();
};

template<typename R_, typename A_>
struct hxCommand1 : public hxCommand {
	hxCommand1(R_(*fn_)(A_)) : m_fn(fn_) { }
	virtual bool execute(const char* str_) HX_OVERRIDE {
		char* ptr_ = hxnull;
		hxArg<A_> arg1_(str_, &ptr_);
		if (str_ != ptr_ && hxIsEndOfLine(ptr_)) {
			m_fn(arg1_.value);
			return true;
		}
		usage();
		return false;
	}
	virtual void usage(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id_ ? id_ : "usage:", hxArg<A_>::getLabel()); (void)id_;
	}
	R_(*m_fn)(A_);
};

template<typename R_, typename A1_, typename A2_>
struct hxCommand2 : public hxCommand {
	hxCommand2(R_(*fn_)(A1_, A2_)) : m_fn(fn_) { }
	virtual bool execute(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_ && hxIsEndOfLine(pB_)) {
				m_fn(arg1_.value, arg2_.value);
				return true;
			}
		}
		usage();
		return false;
	}
	virtual void usage(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s, %s\n", id_ ? id_ : "usage:", hxArg<A1_>::getLabel(), hxArg<A2_>::getLabel()); (void)id_;
	}
	R_(*m_fn)(A1_, A2_);
};

template<typename R_, typename A1_, typename A2_, typename A3_>
struct hxCommand3 : public hxCommand {
	hxCommand3(R_(*fn_)(A1_, A2_, A3_)) : m_fn(fn_) { }
	virtual bool execute(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_) {
				hxArg<A3_> arg3_(pB_, &pA_);
				if (pA_ != pB_ && hxIsEndOfLine(pA_)) {
					m_fn(arg1_.value, arg2_.value, arg3_.value);
					return true;
				}
			}
		}

		usage();
		return false;
	}
	virtual void usage(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s, %s, %s\n", id_ ? id_ : "usage:", hxArg<A1_>::getLabel(), hxArg<A2_>::getLabel(), hxArg<A3_>::getLabel()); (void)id_;
	}
	R_(*m_fn)(A1_, A2_, A3_);
};

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
struct hxCommand4 : public hxCommand {
	hxCommand4(R_(*fn_)(A1_, A2_, A3_, A4_)) : m_fn(fn_) { }
	virtual bool execute(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_) {
				hxArg<A3_> arg3_(pB_, &pA_);
				if (pA_ != pB_) {
					hxArg<A4_> arg4_(pA_, &pB_);
					if (pA_ != pB_ && hxIsEndOfLine(pB_)) {
						m_fn(arg1_.value, arg2_.value, arg3_.value, arg4_.value);
						return true;
					}
				}
			}
		}
		usage();
		return false;
	}
	virtual void usage(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s, %s, %s, %s\n", id_ ? id_ : "usage:", hxArg<A1_>::getLabel(), hxArg<A2_>::getLabel(), hxArg<A3_>::getLabel(),
			hxArg<A4_>::getLabel()); (void)id_;
	}
	R_(*m_fn)(A1_, A2_, A3_, A4_);
};

template<typename T>
struct hxVariable : public hxCommand {
	hxVariable(volatile T* var_) : m_var(var_) { }
	virtual bool execute(const char* str_) HX_OVERRIDE {
		if (hxIsEndOfLine(str_)) {
			usage(""); // print type and value.
			return true;
		}
		char* ptr_ = hxnull;
		hxArg<T> x_(str_, &ptr_);
		if (ptr_ != str_ && hxIsEndOfLine(ptr_)) {
			*m_var = x_.value;
			return true;
		}
		usage("error with variable: ");
		return false;
	}
	virtual void usage(const char* id_=hxnull) HX_OVERRIDE {
		(void)id_;
#if HX_USE_64_BIT_TYPES
		if (*m_var == (T)(long long)*m_var) {
			// If the current value fits in a long long, use that.
			hxLogConsole("%s %s %lld\n", id_ ? id_ : "usage:", hxArg<T>::getLabel(), (long long)*m_var);
		}
		else {
			hxLogConsole("%s %s %lf\n", id_ ? id_ : "usage:", hxArg<T>::getLabel(), (double)*m_var);
		}
	}
#else
		if (*m_var == (T)(int)*m_var) {
			// If the current value fits in a int, use that.
			hxLogConsole("%s %s %d\n", id_ ? id_ : "usage:", hxArg<T>::getLabel(), (int)*m_var);
		}
		else {
			// This cast is just a gesture, as variadic float args are promoted to double.
			hxLogConsole("%s %s %f\n", id_ ? id_ : "usage:", hxArg<T>::getLabel(), (float)*m_var);
		}
	}
#endif
	volatile T* m_var;
};

template<typename R_>
HX_INLINE hxCommand* hxCommandFactory(R_(*fn_)()) {
	return hxNewExt<hxCommand0<R_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_>
HX_INLINE hxCommand* hxCommandFactory(R_(*fn_)(A1_)) {
	return hxNewExt<hxCommand1<R_, A1_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_>
HX_INLINE hxCommand* hxCommandFactory(R_(*fn_)(A1_, A2_)) {
	return hxNewExt<hxCommand2<R_, A1_, A2_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_>
HX_INLINE hxCommand* hxCommandFactory(R_(*fn_)(A1_, A2_, A3_)) {
	return hxNewExt<hxCommand3<R_, A1_, A2_, A3_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_INLINE hxCommand* hxCommandFactory(R_(*fn_)(A1_, A2_, A3_, A4_)) {
	return hxNewExt<hxCommand4<R_, A1_, A2_, A3_, A4_>, hxMemoryManagerId_Console>(fn_);
}

template<typename T>
HX_INLINE hxCommand* hxVariableFactory(volatile T* var_) {
    // Warning: Whole program optimization was breaking with this: return hxNew<hxVariable<T> >(var_);
	return ::new(hxMallocExt(sizeof(hxVariable<T>), hxMemoryManagerId_Console, HX_ALIGNMENT_MASK)) hxVariable<T>(var_);
}

// ERROR: Pointers cannot be console variables.
template<typename T>
inline hxCommand* hxVariableFactory(volatile T** var_); // = delete
template<typename T>
inline hxCommand* hxVariableFactory(const volatile T** var_); // = delete
