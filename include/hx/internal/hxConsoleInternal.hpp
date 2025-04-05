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
	HX_INLINE hxConsoleConstructor_(hxCommand_* fn_, const char* id_) {
		hxConsoleRegister_(fn_, id_);
	}
};

// Console tokens are delimited by any whitespace and non-printing low-ASCII
// characters.  NUL is considered a delimiter and must be checked for separately.
HX_INLINE static bool hxIsDelimiter_(char ch_) { return ch_ <= 32; }

// Checks for printing characters.
HX_INLINE static bool hxIsEndOfline_(const char* str_) {
	while (*str_ != 0 && *str_ <= 32) {
		++str_;
	}
	return *str_ == 0 || *str_ == '#'; // Skip comments
}

// Wrapper for strtol() style parser.  You may not want to force inlining of this.
template <typename T_, typename R_>
HX_INLINE void hxArgParse_(T_& val_, const char* str_, char** next_, R_(*parser_)(char const*, char**, int)) {
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
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel_() { return "s8"; }
	signed char value_;
};
template<> struct hxArg_<signed short> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel_() { return "s16"; }
	signed short value_;
};
template<> struct hxArg_<signed int> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel_() { return "s32"; }
	signed int value_;
};
template<> struct hxArg_<signed long int> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel_() { return "s32"; }
	signed long int value_;
};
template<> struct hxArg_<signed long long int> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoll); }
	HX_INLINE static const char* getLabel_() { return "s64"; }
	signed long long int value_;
};
template<> struct hxArg_<unsigned char> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel_() { return "u8"; }
	unsigned char value_;
};
template<> struct hxArg_<unsigned short> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel_() { return "u16"; }
	unsigned short value_;
};
template<> struct hxArg_<unsigned int> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel_() { return "u32"; }
	unsigned int value_;
};
template<> struct hxArg_<unsigned long int> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoul); }
	HX_INLINE static const char* getLabel_() { return "u32"; }
	unsigned long int value_;
};
template<> struct hxArg_<unsigned long long int> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtoull); }
	HX_INLINE static const char* getLabel_() { return "u64"; }
	unsigned long long int value_;
};
template<> struct hxArg_<char> {
	HX_INLINE hxArg_(const char* str_, char** next_) { hxArgParse_(value_, str_, next_, ::strtol); }
	HX_INLINE static const char* getLabel_() { return "s8"; }
	char value_;
};
template<> struct hxArg_<float> {
	HX_INLINE hxArg_(const char* str_, char** next_) : value_(::strtof(str_, next_)) { }
	HX_INLINE static const char* getLabel_() { return "f32"; }
	float value_;
};
template<> struct hxArg_<double> {
	HX_INLINE hxArg_(const char* str_, char** next_) : value_(::strtod(str_, next_)) { }
	HX_INLINE static const char* getLabel_() { return "f64"; }
	double value_;
};
// bool params must be 0 or 1.
template<> struct hxArg_<bool> {
	HX_INLINE hxArg_(const char* str_, char** next_) {
		unsigned long t_ = ::strtoul(str_, next_, 2);
		value_ = (t_ != 0ul);
	}
	HX_INLINE static const char* getLabel_() { return "0/1"; }
	bool value_;
};

// const char* args capture remainder of line including #'s.
template<> struct hxArg_<const char*> {
	HX_INLINE hxArg_(const char* str_, char** next_) {
		while (*str_ != '\0' && hxIsDelimiter_(*str_)) { // Skip leading whitespace
			++str_;
		}
		value_ = str_;
		*next_ = const_cast<char*>("");
	}
	HX_INLINE static const char* getLabel_() { return "string"; }
	const char* value_;
};

template<typename R_>
struct hxCommand0_ : public hxCommand_ {
	hxCommand0_(R_(*fn_)()) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		if(hxIsEndOfline_(str_)) {
			m_fn_();
			return true;
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
struct hxCommand1_ : public hxCommand_ {
	hxCommand1_(R_(*fn_)(A_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* ptr_ = hxnull;
		hxArg_<A_> arg1_(str_, &ptr_);
		if (str_ != ptr_ && hxIsEndOfline_(ptr_)) {
			m_fn_(arg1_.value_);
			return true;
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id_ ? id_ : "usage_:", hxArg_<A_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A_);
};

template<typename R_, typename A1_, typename A2_>
struct hxCommand2_ : public hxCommand_ {
	hxCommand2_(R_(*fn_)(A1_, A2_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg_<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_ && hxIsEndOfline_(pB_)) {
				m_fn_(arg1_.value_, arg2_.value_);
				return true;
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s\n", id_ ? id_ : "usage_:", hxArg_<A1_>::getLabel_(), hxArg_<A2_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_);
};

template<typename R_, typename A1_, typename A2_, typename A3_>
struct hxCommand3_ : public hxCommand_ {
	hxCommand3_(R_(*fn_)(A1_, A2_, A3_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = hxnull;
		char* pB_ = hxnull;
		hxArg_<A1_> arg1_(p_, &pA_);
		if (p_ != pA_) {
			hxArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ != pB_) {
				hxArg_<A3_> arg3_(pB_, &pA_);
				if (pA_ != pB_ && hxIsEndOfline_(pA_)) {
					m_fn_(arg1_.value_, arg2_.value_, arg3_.value_);
					return true;
				}
			}
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s\n", id_ ? id_ : "usage_:", hxArg_<A1_>::getLabel_(), hxArg_<A2_>::getLabel_(), hxArg_<A3_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_, A3_);
};

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
struct hxCommand4_ : public hxCommand_ {
	hxCommand4_(R_(*fn_)(A1_, A2_, A3_, A4_)) : m_fn_(fn_) { }
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
						m_fn_(arg1_.value_, arg2_.value_, arg3_.value_, arg4_.value_);
						return true;
					}
				}
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s %s\n", id_ ? id_ : "usage_:", hxArg_<A1_>::getLabel_(), hxArg_<A2_>::getLabel_(), hxArg_<A3_>::getLabel_(),
			hxArg_<A4_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_, A3_, A4_);
};

template<typename T_>
struct hxVariable_ : public hxCommand_ {
	hxVariable_(volatile T_* var_) : m_var_(var_) { }
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
		usage_("usage_:");
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		(void)id_;
		if (*m_var_ == (T_)(long long)*m_var_) {
			// If the current value_ fits in a long long, use that.
			hxLogConsole("%s %s(=%lld)\n", id_ ? id_ : "usage_:", hxArg_<T_>::getLabel_(), (long long)*m_var_);
		}
		else {
			hxLogConsole("%s %s(=%lf)\n", id_ ? id_ : "usage_:", hxArg_<T_>::getLabel_(), (double)*m_var_);
		}
	}
	volatile T_* m_var_;
};

template<typename R_>
HX_INLINE hxCommand_* hxCommandFactory_(R_(*fn_)()) {
	return hxNewExt<hxCommand0_<R_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_>
HX_INLINE hxCommand_* hxCommandFactory_(R_(*fn_)(A1_)) {
	return hxNewExt<hxCommand1_<R_, A1_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_>
HX_INLINE hxCommand_* hxCommandFactory_(R_(*fn_)(A1_, A2_)) {
	return hxNewExt<hxCommand2_<R_, A1_, A2_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_>
HX_INLINE hxCommand_* hxCommandFactory_(R_(*fn_)(A1_, A2_, A3_)) {
	return hxNewExt<hxCommand3_<R_, A1_, A2_, A3_>, hxMemoryManagerId_Console>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_INLINE hxCommand_* hxCommandFactory_(R_(*fn_)(A1_, A2_, A3_, A4_)) {
	return hxNewExt<hxCommand4_<R_, A1_, A2_, A3_, A4_>, hxMemoryManagerId_Console>(fn_);
}

template<typename T_>
HX_INLINE hxCommand_* hxVariableFactory_(volatile T_* var_) {
    // Warning: Whole program optimization was breaking with this: return hxNew<hxVariable_<T_> >(var_);
	return ::new(hxMallocExt(sizeof(hxVariable_<T_>), hxMemoryManagerId_Console, HX_ALIGNMENT_MASK)) hxVariable_<T_>(var_);
}

// ERROR: Pointers cannot be console variables.
template<typename T_>
inline hxCommand_* hxVariableFactory_(volatile T_** var_); // = delete
template<typename T_>
inline hxCommand_* hxVariableFactory_(const volatile T_** var_); // = delete
