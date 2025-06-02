#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// ----------------------------------------------------------------------------
// hxConsole internals.  See hxConsole.h instead.

struct hxCommand_ {
	virtual bool execute_(const char* str_) = 0; // Return false for parse errors.
	virtual void usage_(const char* id_=hxnull) = 0; // Expects command name.
};

// Explicit registration, takes ownership of fn, id expected to be a string literal.
void hxConsoleRegister_(hxCommand_* fn_, const char* id_);

struct hxConsoleConstructor_ {
	inline hxConsoleConstructor_(hxCommand_* fn_, const char* id_) {
		hxConsoleRegister_(fn_, id_);
	}
};

// Console tokens are delimited by any whitespace and non-printing low-ASCII
// characters.  NUL is considered a delimiter and must be checked for separately.
// This happens to be UTF-8 compatable because it ignores characters >= U+0100.
HX_CONSTEXPR_FN static bool hxIsDelimiter_(char ch_) { return ch_ <= 32 || ch_ == 127; }

// Checks for printing characters.
HX_CONSTEXPR_FN static bool hxIsEndOfline_(const char* str_) {
	while (*str_ != 0 && hxIsDelimiter_(*str_)) {
		++str_;
	}
	return *str_ == 0 || *str_ == '#'; // Skip comments
}

// Wrapper for strtol() style parser.  You may not want to force inlining of this.
template <typename T_, typename R_>
HX_CONSTEXPR_FN void hxArgParse_(T_& val_, const char* str_, char** next_, R_(*parser_)(char const*, char**, int)) {
	R_ r_ = parser_(str_, next_, 10);
	if (r_ != (T_)r_) {
		hxWarn("operand overflow");
		*next_ = const_cast<char*>(str_); // reject input.
		val_ = (T_)0; // Otherwise gcc will incorrectly complain.
		return;
	}
	val_ = (T_)r_;
}

// hxArg_<T_>. Binds string parsing operations to function args.  Invalid arguments are
// set to 0, arguments out of range result in the maximum representable values.

template<typename T_> struct hxArg_; // Undefined. Specialization required.

template<> struct hxArg_<signed char> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s8"; }
	signed char value_;
};
template<> struct hxArg_<signed short> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s16"; }
	signed short value_;
};
template<> struct hxArg_<signed int> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s32"; }
	signed int value_;
};
template<> struct hxArg_<signed long int> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s32"; }
	signed long int value_;
};
template<> struct hxArg_<signed long long int> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoll); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s64"; }
	signed long long int value_;
};
template<> struct hxArg_<unsigned char> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u8"; }
	unsigned char value_;
};
template<> struct hxArg_<unsigned short> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u16"; }
	unsigned short value_;
};
template<> struct hxArg_<unsigned int> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u32"; }
	unsigned int value_;
};
template<> struct hxArg_<unsigned long int> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u32"; }
	unsigned long int value_;
};
template<> struct hxArg_<unsigned long long int> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoull); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u64"; }
	unsigned long long int value_;
};
template<> struct hxArg_<char> {
	inline hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s8"; }
	char value_;
};
template<> struct hxArg_<float> {
	inline hxArg_(const char* str_, char** next_) : value_(::strtof(str_, next_)) { }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "f32"; }
	float value_;
};
template<> struct hxArg_<double> {
	inline hxArg_(const char* str_, char** next_) : value_(::strtod(str_, next_)) { }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "f64"; }
	double value_;
};
// bool params must be 0 or 1.
template<> struct hxArg_<bool> {
	inline hxArg_(const char* str_, char** next_) {
		unsigned long t_ = ::strtoul(str_, next_, 2);
		value_ = (t_ != 0ul);
	}
	HX_CONSTEXPR_FN static const char* getLabel_() { return "0/1"; }
	bool value_;
};

// const char* args capture remainder of line including comments starting with #'s.
// Leading whitespace is discarded.
template<> struct hxArg_<const char*> {
	inline hxArg_(const char* str_, char** next_) {
		value_ = str_;
		while (*value_ != 0 && hxIsDelimiter_(*value_)) {
			++value_;
		}
		*next_ = const_cast<char*>("");
	}
	HX_CONSTEXPR_FN static const char* getLabel_() { return "string"; }
	const char* value_;
};

// Defined in hxConsole.hpp. User overloadable in following templates.
template<typename R_> bool hxConsoleIsOkResult(R_ r_);

// Interpret functions returning void as having a successful result.
// Mixing variadic templates with specialization is not working.
template<typename R_> inline bool hxCommandWrap0_(R_(*fn_)()) { return hxConsoleIsOkResult(fn_()); }
template<> inline bool hxCommandWrap0_<void>(void(*fn_)()) { fn_(); return true; };

template<typename R_>
struct hxCommand0_ : public hxCommand_ {
	inline hxCommand0_(R_(*fn_)()) : m_fn_(fn_) { }

	virtual bool execute_(const char* str_) HX_OVERRIDE {
		if(hxIsEndOfline_(str_)) {
			return hxCommandWrap0_<R_>(m_fn_);
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s\n", id_ ? id_ : "usage_: no args"); (void)id_;
	}
	R_(*m_fn_)();
};

template<typename R_, typename A_>
struct hxCommandWrap1_{
	static bool execute_(R_(*fn_)(A_), A_ a_) { return hxConsoleIsOkResult(fn_(a_)); }
};
template<typename A_>
struct hxCommandWrap1_<void, A_> {
	static bool execute_(void(*fn_)(A_), A_ a_) { fn_(a_); return true; };
};
template<typename R_, typename A_>
struct hxCommand1_ : public hxCommand_ {
	inline hxCommand1_(R_(*fn_)(A_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* ptr_ = hxnull;
		hxArg_<A_> arg1_(str_, &ptr_);
		if (str_ != ptr_ && hxIsEndOfline_(ptr_)) {
			return hxCommandWrap1_<R_, A_>::execute_(m_fn_, arg1_.value_);
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id_ ? id_ : "usage:", hxArg_<A_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A_);
};

template<typename R_, typename A1_, typename A2_>
struct hxCommandWrap2_{
	static bool execute_(R_(*fn_)(A1_, A2_), A1_ a1_, A2_ a2_) { return hxConsoleIsOkResult(fn_(a1_, a2_)); }
};
template<typename A1_, typename A2_>
struct hxCommandWrap2_<void, A1_, A2_> {
	static bool execute_(void(*fn_)(A1_, A2_), A1_ a1_, A2_ a2_) { fn_(a1_, a2_); return true; };
};
template<typename R_, typename A1_, typename A2_>
struct hxCommand2_ : public hxCommand_ {
	inline hxCommand2_(R_(*fn_)(A1_, A2_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg_<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_ && hxIsEndOfline_(pB_)) {
				return hxCommandWrap2_<R_, A1_, A2_>::execute_(m_fn_, arg1_.value_, arg2_.value_);
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s\n", id_ ? id_ : "usage:", hxArg_<A1_>::getLabel_(), hxArg_<A2_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_);
};

template<typename R_, typename A1_, typename A2_, typename A3_>
struct hxCommandWrap3_{
	static bool execute_(R_(*fn_)(A1_, A2_, A3_), A1_ a1_, A2_ a2_, A3_ a3_) { return hxConsoleIsOkResult(fn_(a1_, a2_, a3_)); }
};
template<typename A1_, typename A2_, typename A3_>
struct hxCommandWrap3_<void, A1_, A2_, A3_> {
	static bool execute_(void(*fn_)(A1_, A2_, A3_), A1_ a1_, A2_ a2_, A3_ a3_) { fn_(a1_, a2_, a3_); return true; };
};
template<typename R_, typename A1_, typename A2_, typename A3_>
struct hxCommand3_ : public hxCommand_ {
	inline hxCommand3_(R_(*fn_)(A1_, A2_, A3_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg_<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_) {
				hxArg_<A3_> arg3_(pB_, &pA_);
				if (pA_ != pB_ && hxIsEndOfline_(pA_)) {
					return hxCommandWrap3_<R_, A1_, A2_, A3_>::execute_(m_fn_, arg1_.value_, arg2_.value_, arg3_.value_);
				}
			}
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s\n", id_ ? id_ : "usage:", hxArg_<A1_>::getLabel_(), hxArg_<A2_>::getLabel_(), hxArg_<A3_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_, A3_);
};

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
struct hxCommandWrap4_{
	static bool execute_(R_(*fn_)(A1_, A2_, A3_, A4_), A1_ a1_, A2_ a2_, A3_ a3_, A4_ a4_) { return hxConsoleIsOkResult(fn_(a1_, a2_, a3_, a4_)); }
};
template<typename A1_, typename A2_, typename A3_, typename A4_>
struct hxCommandWrap4_<void, A1_, A2_, A3_, A4_> {
	static bool execute_(void(*fn_)(A1_, A2_, A3_, A4_), A1_ a1_, A2_ a2_, A3_ a3_, A4_ a4_) { fn_(a1_, a2_, a3_, a4_); return true; };
};
template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
struct hxCommand4_ : public hxCommand_ {
	inline hxCommand4_(R_(*fn_)(A1_, A2_, A3_, A4_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg_<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_) {
				hxArg_<A3_> arg3_(pB_, &pA_);
				if (pA_ != pB_) {
					hxArg_<A4_> arg4_(pA_, &pB_);
					if (pA_ != pB_ && hxIsEndOfline_(pB_)) {
						return hxCommandWrap4_<R_, A1_, A2_, A3_, A4_>::execute_(m_fn_, arg1_.value_, arg2_.value_, arg3_.value_, arg4_.value_);
					}
				}
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s %s\n", id_ ? id_ : "usage:", hxArg_<A1_>::getLabel_(), hxArg_<A2_>::getLabel_(), hxArg_<A3_>::getLabel_(),
			hxArg_<A4_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_, A3_, A4_);
};

template<typename T_>
struct hxVariable_ : public hxCommand_ {
	inline hxVariable_(volatile T_* var_) : m_var_(var_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		if (hxIsEndOfline_(str_)) {
			usage_("value_ is:"); // print type and value_.
			return true;
		}
		char* ptr_ = hxnull;
		hxArg_<T_> x_(str_, &ptr_);
		if (ptr_ != str_ && hxIsEndOfline_(ptr_)) {
			*m_var_ = x_.value_;
			return true;
		}
		usage_("usage:");
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		(void)id_;
		if (*m_var_ == (T_)(long long)*m_var_) {
			// If the current value_ fits in a long long, use that.
			hxLogConsole("%s %s(=%lld)\n", id_ ? id_ : "usage:", hxArg_<T_>::getLabel_(), (long long)*m_var_);
		}
		else {
			hxLogConsole("%s %s(=%lf)\n", id_ ? id_ : "usage:", hxArg_<T_>::getLabel_(), (double)*m_var_);
		}
	}
	volatile T_* m_var_;
};

template<typename R_>
HX_CONSTEXPR_FN hxCommand_* hxCommandFactory_(R_(*fn_)()) {
	return hxNew<hxCommand0_<R_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_>
HX_CONSTEXPR_FN hxCommand_* hxCommandFactory_(R_(*fn_)(A1_)) {
	return hxNew<hxCommand1_<R_, A1_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_>
HX_CONSTEXPR_FN hxCommand_* hxCommandFactory_(R_(*fn_)(A1_, A2_)) {
	return hxNew<hxCommand2_<R_, A1_, A2_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_>
HX_CONSTEXPR_FN hxCommand_* hxCommandFactory_(R_(*fn_)(A1_, A2_, A3_)) {
	return hxNew<hxCommand3_<R_, A1_, A2_, A3_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_CONSTEXPR_FN hxCommand_* hxCommandFactory_(R_(*fn_)(A1_, A2_, A3_, A4_)) {
	return hxNew<hxCommand4_<R_, A1_, A2_, A3_, A4_>, hxMemoryManagerId_Console>(fn_);
}

template<typename T_>
HX_CONSTEXPR_FN hxCommand_* hxVariableFactory_(volatile T_* var_) {
	// Warning: Whole program optimization was breaking with this: return hxNew<hxVariable_<T_> >(var_);
	return ::new(hxMallocExt(sizeof(hxVariable_<T_>), hxMemoryManagerId_Console, HX_ALIGNMENT)) hxVariable_<T_>(var_);
}

// ERROR: Pointers cannot be console variables.
template<typename T_>
inline hxCommand_* hxVariableFactory_(volatile T_** var_) HX_DELETE_FN;
template<typename T_>
inline hxCommand_* hxVariableFactory_(const volatile T_** var_) HX_DELETE_FN;
