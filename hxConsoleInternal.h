#pragma once
// Copyright 2017 Adrian Johnston

#include "hatchling.h"

// ----------------------------------------------------------------------------------
// hxConsole internals.  See hxConsole.h instead

struct hxCommand {
	virtual bool execute(const char* str) = 0; // Return false for parse errors.
	virtual void log(const char* id) = 0;
};

// Explicit registration, takes ownership of fn, id expected to be static.
void hxConsoleRegister(hxCommand* fn, const char* id);

struct hxConsoleConstructor {
	HX_INLINE hxConsoleConstructor(hxCommand* fn, const char* id) {
		hxConsoleRegister(fn, id);
	}
};

// Console IDs are delimited by any whitespace and non-printing low-ASCII characters including null.
HX_INLINE static bool hxIsDelimiter(char c) { return c <= 32; }

// Checks for printing characters.
HX_INLINE static bool hxIsEndOfLine(const char* str) {
	while (*str != 0 && *str <= 32) {
		++str;
	}
	return *str == 0 || *str == '#'; // Skip comments
}

// hxArg. Binds string parsing operations to function args.  Invalid arguments are
// set to 0, arguments out of range result in the maximum representable values.

template<typename T> struct hxArg; // Undefined. Specialization required.

template<> struct hxArg<int8_t> {
	HX_INLINE hxArg(const char* str, char** next) {
		int32_t l = ::strtol(str, next, 10);
		val = (int8_t)l;
		if ((int32_t)val != l) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "s8"; }
	int8_t val;
};
template<> struct hxArg<int16_t> {
	HX_INLINE hxArg(const char* str, char** next) {
		int32_t l = ::strtol(str, next, 10);
		val = (int16_t)l;
		if ((int32_t)val != l) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "s16"; }
	int16_t val;
};
template<> struct hxArg<int32_t> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtol(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "s32"; }
	int32_t val;
};
#if HX_HAS_64_BIT_TYPES
template<> struct hxArg<int64_t> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtoll(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "s64"; }
	int64_t val;
};
#endif
template<> struct hxArg<uint8_t> {
	HX_INLINE hxArg(const char* str, char** next) {
		uint32_t ul = ::strtoul(str, next, 10);
		val = (uint8_t)ul;
		if ((uint32_t)val != ul) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "u8"; }
	uint8_t val;
};
template<> struct hxArg<uint16_t> {
	HX_INLINE hxArg(const char* str, char** next) {
		uint32_t ul = ::strtoul(str, next, 10);
		val = (uint16_t)ul;
		if ((uint32_t)val != ul) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "u16"; }
	uint16_t val;
};
template<> struct hxArg<uint32_t> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtoul(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "u32"; }
	uint32_t val;
};
#if HX_HAS_64_BIT_TYPES
template<> struct hxArg<uint64_t> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtoull(str, next, 10)) { }
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
		uint32_t t = ::strtoul(str, next, 2);
		val = (t == 1ul);
		if (t > 1ul) {
			*next = (char*)str; // reject input.
		}
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
		*next = (char*)"";
	}
	HX_INLINE static const char* getLabel() { return "string"; }
	const char* val;
};

template<typename R>
struct hxFunction0 : public hxCommand {
	hxFunction0(R(*fn)()) : m_fn(fn) { }
	virtual bool execute(const char* str) HX_OVERRIDE {
		if(hxIsEndOfLine(str)) {
			m_fn();
			return true;
		}

		hxLogConsole("Error: Expecting no parameters\n");
		return false;
	}
	virtual void log(const char* id) HX_OVERRIDE {
		hxLogConsole("%s\n", id);
	}
	R(*m_fn)();
};

template<typename R, typename A>
struct hxFunction1 : public hxCommand {
	hxFunction1(R(*fn)(A)) : m_fn(fn) { }
	virtual bool execute(const char* str) HX_OVERRIDE {
		char* ptr = hxnull;
		hxArg<A> arg1(str, &ptr);
		if (str != ptr && hxIsEndOfLine(ptr)) {
			m_fn(arg1.val);
			return true;
		}
		log("Usage:");
		return false;
	}
	virtual void log(const char* id) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id, hxArg<A>::getLabel());
	}
	R(*m_fn)(A);
};

template<typename R, typename A1, typename A2>
struct hxFunction2 : public hxCommand {
	hxFunction2(R(*fn)(A1, A2)) : m_fn(fn) { }
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
		log("Usage:");
		return false;
	}
	virtual void log(const char* id) HX_OVERRIDE {
		hxLogConsole("%s %s, %s\n", id, hxArg<A1>::getLabel(), hxArg<A2>::getLabel());
	}
	R(*m_fn)(A1, A2);
};

template<typename R, typename A1, typename A2, typename A3>
struct hxFunction3 : public hxCommand {
	hxFunction3(R(*fn)(A1, A2, A3)) : m_fn(fn) { }
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

		log("Usage:");
		return false;
	}
	virtual void log(const char* id) HX_OVERRIDE {
		hxLogConsole("%s %s, %s, %s\n", id, hxArg<A1>::getLabel(), hxArg<A2>::getLabel(), hxArg<A3>::getLabel());
	}
	R(*m_fn)(A1, A2, A3);
};

template<typename R, typename A1, typename A2, typename A3, typename A4>
struct hxFunction4 : public hxCommand {
	hxFunction4(R(*fn)(A1, A2, A3, A4)) : m_fn(fn) { }
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
		log("Usage:");
		return false;
	}
	virtual void log(const char* id) HX_OVERRIDE {
		hxLogConsole("%s %s, %s, %s, %s\n", id, hxArg<A1>::getLabel(), hxArg<A2>::getLabel(), hxArg<A3>::getLabel(),
			hxArg<A4>::getLabel());
	}
	R(*m_fn)(A1, A2, A3, A4);
};

template<typename T>
struct hxVariable : public hxCommand {
	hxVariable(volatile T* var) : m_var(var) { }
	virtual bool execute(const char* str) HX_OVERRIDE {
		char* ptr = hxnull;
		hxArg<T> x(str, &ptr);
		if (ptr != str && hxIsEndOfLine(ptr)) {
			*m_var = x.val;
			return true;
		}
		log("Error: Expected type (and current value):");
		return false;
	}
	virtual void log(const char* id) HX_OVERRIDE {
#if HX_HAS_64_BIT_TYPES
		if (*m_var == (T)(int64_t)*m_var) {
			// If the current value fits in a long long, use that.
			hxLogConsole("%s %s (%lld)\n", id, hxArg<T>::getLabel(), (long long)*m_var);
		}
		else {
			hxLogConsole("%s %s (%lf)\n", id, hxArg<T>::getLabel(), (double)*m_var);
		}
	}
#else
		if (*m_var == (T)(int32_t)*m_var) {
			// If the current value fits in a int, use that.
			hxLogConsole("%s %s (%d)\n", id, hxArg<T>::getLabel(), (int)*m_var);
		}
		else {
			// This cast is just a gesture, as variadic float args are promoted to double.
			hxLogConsole("%s %s (%f)\n", id, hxArg<T>::getLabel(), (float)*m_var);
		}
	}
#endif
	volatile T* m_var;
};

template<typename R>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)()) {
	return hxNewExt<hxFunction0<R>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1)) {
	return hxNewExt<hxFunction1<R, A1>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1, typename A2>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1, A2)) {
	return hxNewExt<hxFunction2<R, A1, A2>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1, typename A2, typename A3>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1, A2, A3)) {
	return hxNewExt<hxFunction3<R, A1, A2, A3>, hxMemoryManagerId_Console>(fn);
}

template<typename R, typename A1, typename A2, typename A3, typename A4>
HX_INLINE hxCommand* hxCommandFactory(R(*fn)(A1, A2, A3, A4)) {
	return hxNewExt<hxFunction4<R, A1, A2, A3, A4>, hxMemoryManagerId_Console>(fn);
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
