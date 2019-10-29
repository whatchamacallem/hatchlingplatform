#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// hxConsole internals.  See hxConsole.h instead

struct hxCommand {
	virtual bool execute(const char* str) = 0; // Return false for parse errors.
	virtual void usage(const char* id=hxnull) = 0;
};

// Explicit registration, takes ownership of fn, id expected to be a string literal.
void hxConsoleRegister(hxCommand* fn, const char* id);

struct hxConsoleConstructor {
	HX_INLINE hxConsoleConstructor(hxCommand* fn, const char* id) { hxConsoleRegister(fn, id); }
};

// Console tokens are delimited by any whitespace and non-printing low-ASCII characters including null.
HX_INLINE static bool hxIsDelimiter(char c) { return c <= 32; }

// Checks for printing characters.
HX_INLINE static bool hxIsEndOfLine(const char* str) {
	while (*str != 0 && *str <= 32) {
		++str;
	}
	return *str == 0 || *str == '#'; // Skip comments
}

// Yes, re-implementing std:min/max is a bit much.  But it makes this freestanding.
template<bool Signed, typename T> struct hxLimitsSelect {
	// Unsigned min/max.
	static HX_CONSTEXPR T minVal() { return T(0); }
	static HX_CONSTEXPR T maxVal() { return ~T(0); }
};

template<typename T> struct hxLimitsSelect<true, T> {
	// Signed min/max.  Does silly things to avoid overflowing a signed integer type.
	static HX_CONSTEXPR T minVal() { return T(-1) - maxVal(); }
	static HX_CONSTEXPR T maxVal() { return msb1() | (msb1() - T(1)); }
	static HX_CONSTEXPR T msb1() { return (T)(T(1) << (sizeof(T) * 8 - 2)); }
};

// select an implementation depending on whether T is signed.
template<typename T> struct hxLimits : public hxLimitsSelect<(T)~T(0) < T(0), T> { };

template <typename T, typename R>
HX_INLINE void hxArgParse(T& val, const char* str, char** next, R(*parser)(char const*, char**, int)) {
	R r = parser(str, next, 10);
	// These compares should be optimized away when not needed.
	if (r < hxLimits<T>::minVal() || r > hxLimits<T>::maxVal()) {
		hxWarn("overflow");
		*next = const_cast<char*>(str); // reject input.
		val = (T)0; // Otherwise gcc will incorrectly complain.
		return;
	}
	val = (T)r;
	return;
}

// hxArg. Binds string parsing operations to function args.  Invalid arguments are
// set to 0, arguments out of range result in the maximum representable values.

template<typename T> struct hxArg; // Undefined. Specialization required.

template<> struct hxArg<int8_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtol); }
	HX_INLINE static const char* getLabel() { return "s8"; }
	int8_t val;
};
template<> struct hxArg<int16_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtol); }
	HX_INLINE static const char* getLabel() { return "s16"; }
	int16_t val;
};
template<> struct hxArg<int32_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtol); }
	HX_INLINE static const char* getLabel() { return "s32"; }
	int32_t val;
};
#if HX_USE_64_BIT_TYPES
template<> struct hxArg<int64_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtoll); }
	HX_INLINE static const char* getLabel() { return "s64"; }
	int64_t val;
};
#endif
template<> struct hxArg<uint8_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtoul); }
	HX_INLINE static const char* getLabel() { return "u8"; }
	uint8_t val;
};
template<> struct hxArg<uint16_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtoul); }
	HX_INLINE static const char* getLabel() { return "u16"; }
	uint16_t val;
};
template<> struct hxArg<uint32_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtoul); }
	HX_INLINE static const char* getLabel() { return "u32"; }
	uint32_t val;
};
#if HX_USE_64_BIT_TYPES
template<> struct hxArg<uint64_t> {
	HX_INLINE hxArg(const char* str, char** next) { hxArgParse(val, str, next, ::strtoull); }
	HX_INLINE static const char* getLabel() { return "u64"; }
	uint64_t val;
};
#endif
template<> struct hxArg<float> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtof(str, next)) { }
	HX_INLINE static const char* getLabel() { return "f32"; }
	float val;
};
template<> struct hxArg<double> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtod(str, next)) { }
	HX_INLINE static const char* getLabel() { return "f64"; }
	double val;
};
// bool params must be 0 or 1.
template<> struct hxArg<bool> {
	HX_INLINE hxArg(const char* str, char** next) {
		unsigned long t = ::strtoul(str, next, 2);
		val = (t != 0ul);
	}
	HX_INLINE static const char* getLabel() { return "0/1"; }
	bool val;
};

// const char* args capture remainder of line including #'s.
template<> struct hxArg<const char*> {
	HX_INLINE hxArg(const char* str, char** next) {
		while (*str != '\0' && hxIsDelimiter(*str)) { // Skip leading whitespace
			++str;
		}
		val = str;
		*next = const_cast<char*>("");
	}
	HX_INLINE static const char* getLabel() { return "string"; }
	const char* val;
};

template<typename R>
struct hxCommand0 : public hxCommand {
	hxCommand0(R(*fn)()) : m_fn(fn) { }
	virtual bool execute(const char* str) HX_OVERRIDE {
		if(hxIsEndOfLine(str)) {
			m_fn();
			return true;
		}

		usage();
		return false;
	}
	virtual void usage(const char* id=hxnull) HX_OVERRIDE {
		hxLogConsole("%s\n", id ? id : "use no args"); (void)id;
	}
	R(*m_fn)();
};

template<typename R, typename A>
struct hxCommand1 : public hxCommand {
	hxCommand1(R(*fn)(A)) : m_fn(fn) { }
	virtual bool execute(const char* str) HX_OVERRIDE {
		char* ptr = hxnull;
		hxArg<A> arg1(str, &ptr);
		if (str != ptr && hxIsEndOfLine(ptr)) {
			m_fn(arg1.val);
			return true;
		}
		usage();
		return false;
	}
	virtual void usage(const char* id=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id ? id : "usage:", hxArg<A>::getLabel()); (void)id;
	}
	R(*m_fn)(A);
};

template<typename R, typename A1, typename A2>
struct hxCommand2 : public hxCommand {
	hxCommand2(R(*fn)(A1, A2)) : m_fn(fn) { }
	virtual bool execute(const char* p) HX_OVERRIDE {
		char* pA = hxnull;
		char* pB = hxnull;
		hxArg<A1> arg1(p, &pA);
		if (p != pA) {
			hxArg<A2> arg2(pA, &pB);
			if (pA != pB && hxIsEndOfLine(pB)) {
				m_fn(arg1.val, arg2.val);
				return true;
			}
		}
		usage();
		return false;
	}
	virtual void usage(const char* id=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s, %s\n", id ? id : "usage:", hxArg<A1>::getLabel(), hxArg<A2>::getLabel()); (void)id;
	}
	R(*m_fn)(A1, A2);
};

template<typename R, typename A1, typename A2, typename A3>
struct hxCommand3 : public hxCommand {
	hxCommand3(R(*fn)(A1, A2, A3)) : m_fn(fn) { }
	virtual bool execute(const char* p) HX_OVERRIDE {
		char* pA = hxnull;
		char* pB = hxnull;
		hxArg<A1> arg1(p, &pA);
		if (p != pA) {
			hxArg<A2> arg2(pA, &pB);
			if (pA != pB) {
				hxArg<A3> arg3(pB, &pA);
				if (pA != pB && hxIsEndOfLine(pA)) {
					m_fn(arg1.val, arg2.val, arg3.val);
					return true;
				}
			}
		}

		usage();
		return false;
	}
	virtual void usage(const char* id=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s, %s, %s\n", id ? id : "usage:", hxArg<A1>::getLabel(), hxArg<A2>::getLabel(), hxArg<A3>::getLabel()); (void)id;
	}
	R(*m_fn)(A1, A2, A3);
};

template<typename R, typename A1, typename A2, typename A3, typename A4>
struct hxCommand4 : public hxCommand {
	hxCommand4(R(*fn)(A1, A2, A3, A4)) : m_fn(fn) { }
	virtual bool execute(const char* p) HX_OVERRIDE {
		char* pA = hxnull;
		char* pB = hxnull;
		hxArg<A1> arg1(p, &pA);
		if (p != pA) {
			hxArg<A2> arg2(pA, &pB);
			if (pA != pB) {
				hxArg<A3> arg3(pB, &pA);
				if (pA != pB) {
					hxArg<A4> arg4(pA, &pB);
					if (pA != pB && hxIsEndOfLine(pB)) {
						m_fn(arg1.val, arg2.val, arg3.val, arg4.val);
						return true;
					}
				}
			}
		}
		usage();
		return false;
	}
	virtual void usage(const char* id=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s, %s, %s, %s\n", id ? id : "usage:", hxArg<A1>::getLabel(), hxArg<A2>::getLabel(), hxArg<A3>::getLabel(),
			hxArg<A4>::getLabel()); (void)id;
	}
	R(*m_fn)(A1, A2, A3, A4);
};

template<typename T>
struct hxVariable : public hxCommand {
	hxVariable(volatile T* var) : m_var(var) { }
	virtual bool execute(const char* str) HX_OVERRIDE {
		if (hxIsEndOfLine(str)) {
			usage(""); // print type and value.
			return true;
		}
		char* ptr = hxnull;
		hxArg<T> x(str, &ptr);
		if (ptr != str && hxIsEndOfLine(ptr)) {
			*m_var = x.val;
			return true;
		}
		usage("error with variable: ");
		return false;
	}
	virtual void usage(const char* id=hxnull) HX_OVERRIDE {
		(void)id;
#if HX_USE_64_BIT_TYPES
		if (*m_var == (T)(long long)*m_var) {
			// If the current value fits in a long long, use that.
			hxLogConsole("%s %s %lld\n", id ? id : "usage:", hxArg<T>::getLabel(), (long long)*m_var);
		}
		else {
			hxLogConsole("%s %s %lf\n", id ? id : "usage:", hxArg<T>::getLabel(), (double)*m_var);
		}
	}
#else
		if (*m_var == (T)(int)*m_var) {
			// If the current value fits in a int, use that.
			hxLogConsole("%s %s %d\n", id ? id : "usage:", hxArg<T>::getLabel(), (int)*m_var);
		}
		else {
			// This cast is just a gesture, as variadic float args are promoted to double.
			hxLogConsole("%s %s %f\n", id ? id : "usage:", hxArg<T>::getLabel(), (float)*m_var);
		}
	}
#endif
	volatile T* m_var;
};

template<typename R>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)()) {
	return hxNewExt<hxCommand0<R>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1)) {
	return hxNewExt<hxCommand1<R, A1>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1, typename A2>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1, A2)) {
	return hxNewExt<hxCommand2<R, A1, A2>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1, typename A2, typename A3>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1, A2, A3)) {
	return hxNewExt<hxCommand3<R, A1, A2, A3>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1, typename A2, typename A3, typename A4>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1, A2, A3, A4)) {
	return hxNewExt<hxCommand4<R, A1, A2, A3, A4>, hxMemoryManagerId_Console>(fn);
}

template<typename T>
HX_INLINE hxCommand* hxVariableFactory(volatile T* var) {
    // Warning: Whole program optimization was breaking with this: return hxNew<hxVariable<T> >(var);
	return ::new(hxMallocExt(sizeof(hxVariable<T>), hxMemoryManagerId_Console, HX_ALIGNMENT_MASK)) hxVariable<T>(var);
}

// ERROR: Pointers cannot be console variables.
template<typename T>
inline hxCommand* hxVariableFactory(volatile T** var); // = delete
template<typename T>
inline hxCommand* hxVariableFactory(const volatile T** var); // = delete
