#pragma once
// Copyright 2017 Adrian Johnston

#include "hatchling.h"

class hxFile;

// ----------------------------------------------------------------------------------
// hxConsole API

// Using a bunch of macros and global constuctors to run code based on a text file is
// not a great design pattern.  But that is the idea here.

// Registers a function. name must be a valid C identifier. E.g. hxConsoleCommand(srand).
#define hxConsoleCommand(x) hxConsoleConstructor HX_CONCATENATE(g_hxConsoleSymbol_,x)(hxCommandFactory(&(x)), HX_QUOTE(x))
#define hxConsoleCommandNamed(x, name) hxConsoleConstructor HX_CONCATENATE(g_hxConsoleSymbol_,name)(hxCommandFactory(&(x)), HX_QUOTE(name))

// Registers a variable. name must be a valid C identifier. E.g. bool g_isEnabled=false; hxConsoleVariable(g_isEnabled).
#define hxConsoleVariable(x) hxConsoleConstructor HX_CONCATENATE(g_hxConsoleSymbol_,x)(hxVariableFactory(&(x)), HX_QUOTE(x))
#define hxConsoleVariableNamed(x, name) hxConsoleConstructor HX_CONCATENATE(g_hxConsoleSymbol_,name)(hxVariableFactory(&(x)), HX_QUOTE(name))

// Explicit deregistration
void hxConsoleDeregister(const char* id);

void hxConsoleDeregisterAll();

// Calls functions and sets variables. E.g.: "srand 77" and "aVariable 5"
bool hxConsoleExecLine(const char* command);

// Executes config file which has been opened for reading.  Ignores blank lines
// and comments starting with #.
bool hxConsoleExecFile(hxFile& file);

// Opens config file and executes it.
void hxConsoleExecFilename(const char* filename);

// Logs all symbols
void hxConsoleHelp();

// ----------------------------------------------------------------------------------
// hxConsole internals

struct hxCommand {
	virtual bool execute(const char* str) = 0; // Return false for parse errors.
	virtual void log(const char* id) = 0;
};

// Explicit registration, takes ownersip of fn, id expected to be static.
void hxConsoleRegister(hxCommand* fn, const char* id);

struct hxConsoleConstructor {
	HX_INLINE hxConsoleConstructor(hxCommand* fn, const char* id) {
		hxConsoleRegister(fn, id);
	}
};

// Non-printing low-ascii characters including null.
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

template<> struct hxArg<char> {
	HX_INLINE hxArg(const char* str, char** next) {
		long l = ::strtol(str, next, 10);
		val = (char)l;
		if ((long)val != l) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "char"; }
	char val;
};
template<> struct hxArg<short> {
	HX_INLINE hxArg(const char* str, char** next) {
		long l = ::strtol(str, next, 10);
		val = (short)l;
		if ((long)val != l) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "short"; }
	short val;
};
template<> struct hxArg<int> {
	HX_INLINE hxArg(const char* str, char** next) {
		long l = ::strtol(str, next, 10);
		val = (int)l;
		if ((long)val != l) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "int"; }
	int val;
};
template<> struct hxArg<long> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtol(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "long"; }
	long val;
};
template<> struct hxArg<long long> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtoll(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "long long"; }
	long long val;
};
template<> struct hxArg<unsigned char> {
	HX_INLINE hxArg(const char* str, char** next) {
		unsigned long ul = ::strtoul(str, next, 10);
		val = (unsigned char)ul;
		if ((unsigned long)val != ul) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "unsigned char"; }
	unsigned char val;
};
template<> struct hxArg<unsigned short> {
	HX_INLINE hxArg(const char* str, char** next) {
		unsigned long ul = ::strtoul(str, next, 10);
		val = (unsigned short)ul;
		if ((unsigned long)val != ul) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "unsigned short"; }
	unsigned short val;
};
template<> struct hxArg<unsigned int> {
	HX_INLINE hxArg(const char* str, char** next) { 
		unsigned long ul = ::strtoul(str, next, 10);
		val = (unsigned int)ul;
		if ((unsigned long)val != ul) {
			hxWarn("%s overflow", getLabel());
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "unsigned int"; }
	unsigned int val;
};
template<> struct hxArg<unsigned long> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtoul(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "unsigned long"; }
	unsigned long val;
};
template<> struct hxArg<unsigned long long> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtoull(str, next, 10)) { }
	HX_INLINE static const char* getLabel() { return "unsigned long long"; }
	unsigned long long val;
};
template<> struct hxArg<float> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtof(str, next)) { }
	HX_INLINE static const char* getLabel() { return "float"; }
	float val;
};
template<> struct hxArg<double> {
	HX_INLINE hxArg(const char* str, char** next) : val(::strtod(str, next)) { }
	HX_INLINE static const char* getLabel() { return "double"; }
	double val;
};
// bool params must be 0 or 1.
template<> struct hxArg<bool> {
	HX_INLINE hxArg(const char* str, char** next) {
		unsigned long t = ::strtoul(str, next, 2);
		val = (t == 1ul);
		if (t > 1ul) {
			*next = (char*)str; // reject input.
		}
	}
	HX_INLINE static const char* getLabel() { return "bool (0 or 1)"; }
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
	virtual bool execute(const char* str) override {
		if(hxIsEndOfLine(str)) {
			m_fn();
			return true;
		}

		hxLogRelease("Error: Expecting no parameters\n");
		return false;
	}
	virtual void log(const char* id) override {
		hxLogRelease("%s\n", id);
	}
	R(*m_fn)();
};

template<typename R, typename A>
struct hxFunction1 : public hxCommand {
	hxFunction1(R(*fn)(A)) : m_fn(fn) { }
	virtual bool execute(const char* str) override {
		char* ptr = null;
		hxArg<A> arg1(str, &ptr);
		if (str != ptr && hxIsEndOfLine(ptr)) {
			m_fn(arg1.val);
			return true;
		}
		log("Usage:");
		return false;
	}
	virtual void log(const char* id) override {
		hxLogRelease("%s %s\n", id, hxArg<A>::getLabel());
	}
	R(*m_fn)(A);
};

template<typename R, typename A1, typename A2>
struct hxFunction2 : public hxCommand {
	hxFunction2(R(*fn)(A1, A2)) : m_fn(fn) { }
	virtual bool execute(const char* p) override {
		char* pA = null;
		char* pB = null;
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
	virtual void log(const char* id) override {
		hxLogRelease("%s %s, %s\n", id, hxArg<A1>::getLabel(), hxArg<A2>::getLabel());
	}
	R(*m_fn)(A1, A2);
};

template<typename R, typename A1, typename A2, typename A3>
struct hxFunction3 : public hxCommand {
	hxFunction3(R(*fn)(A1, A2, A3)) : m_fn(fn) { }
	virtual bool execute(const char* p) override {
		char* pA = null;
		char* pB = null;
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
	virtual void log(const char* id) override {
		hxLogRelease("%s %s, %s, %s\n", id, hxArg<A1>::getLabel(), hxArg<A2>::getLabel(), hxArg<A3>::getLabel());
	}
	R(*m_fn)(A1, A2, A3);
};

template<typename R, typename A1, typename A2, typename A3, typename A4>
struct hxFunction4 : public hxCommand {
	hxFunction4(R(*fn)(A1, A2, A3, A4)) : m_fn(fn) { }
	virtual bool execute(const char* p) override {
		char* pA = null;
		char* pB = null;
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
	virtual void log(const char* id) override {
		hxLogRelease("%s %s, %s, %s, %s\n", id, hxArg<A1>::getLabel(), hxArg<A2>::getLabel(), hxArg<A3>::getLabel(),
			hxArg<A4>::getLabel());
	}
	R(*m_fn)(A1, A2, A3, A4);
};

template<typename T>
struct hxVariable : public hxCommand {
	hxVariable(volatile T* var) : m_var(var) { }
	virtual bool execute(const char* str) override {
		char* ptr = null;
		hxArg<T> x(str, &ptr);
		if (ptr != str && hxIsEndOfLine(ptr)) {
			*m_var = x.val;
			return true;
		}
		log("Error: Expected type (and current value):");
		return false;
	}
	virtual void log(const char* id) override {
		if (*m_var == (T)(long long)*m_var) {
			// If the current value fits in a long long, use that.
			hxLogRelease("%s %s (%lld)\n", id, hxArg<T>::getLabel(), (long long)*m_var);
		}
		else {
			hxLogRelease("%s %s (%lf)\n", id, hxArg<T>::getLabel(), (double)*m_var);
		}
	}
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
