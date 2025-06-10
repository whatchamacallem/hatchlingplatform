#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

// hxConsole internals. See hxConsole.h instead.

struct hxConsoleCommand_ {
	virtual bool execute_(const char* str_) = 0; // Return false for parse errors.
	virtual void usage_(const char* id_=hxnull) = 0; // Expects command name.
};

// Console tokens are delimited by any whitespace and non-printing low-ASCII
// characters. NUL is considered a delimiter and must be checked for separately.
// This happens to be UTF-8 compatable because it ignores characters >= U+0100.
HX_CONSTEXPR_FN static bool hxConsoleIsDelimiter_(char ch_) { return ch_ <= 32 || ch_ == 127; }

// Checks for printing characters.
HX_CONSTEXPR_FN static bool hxConsoleIsEndOfline_(const char* str_) {
	while (*str_ != 0 && hxConsoleIsDelimiter_(*str_)) {
		++str_;
	}
	return *str_ == 0 || *str_ == '#'; // Skip comments
}

// Wrapper for C/strtol style parsers.
template <typename T_, typename R_>
HX_CONSTEXPR_FN void hxConsoleArgParse_(T_& val_, const char* str_, char** next_, R_(*parser_)(char const*, char**, int)) {
	R_ r_ = parser_(str_, next_, 10);
	if(r_ != (T_)r_) {
		hxLogWarning("console operand overflow: %s", str_);
		*next_ = const_cast<char*>(str_); // reject input.
	}
	val_ = (T_)r_;
}

// hxConsoleArg_<T_>. Binds string parsing operations to function args. Invalid arguments are
// set to 0, arguments out of range result in the maximum representable values.

template<typename T_> struct hxConsoleArg_; // Undefined. Specialization required.

template<> struct hxConsoleArg_<signed char> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s8"; }
	signed char value_;
};
template<> struct hxConsoleArg_<signed short> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s16"; }
	signed short value_;
};
template<> struct hxConsoleArg_<signed int> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s32"; }
	signed int value_;
};
template<> struct hxConsoleArg_<signed long int> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s32"; }
	signed long int value_;
};
template<> struct hxConsoleArg_<signed long long int> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtoll); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s64"; }
	signed long long int value_;
};
template<> struct hxConsoleArg_<unsigned char> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u8"; }
	unsigned char value_;
};
template<> struct hxConsoleArg_<unsigned short> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u16"; }
	unsigned short value_;
};
template<> struct hxConsoleArg_<unsigned int> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u32"; }
	unsigned int value_;
};
template<> struct hxConsoleArg_<unsigned long int> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtoul); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u32"; }
	unsigned long int value_;
};
template<> struct hxConsoleArg_<unsigned long long int> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtoull); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "u64"; }
	unsigned long long int value_;
};
template<> struct hxConsoleArg_<char> {
	inline hxConsoleArg_(const char* str_, char** next_) { hxConsoleArgParse_(value_, str_, next_, ::strtol); }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "s8"; }
	char value_;
};
template<> struct hxConsoleArg_<float> {
	inline hxConsoleArg_(const char* str_, char** next_) : value_(::strtof(str_, next_)) { }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "f32"; }
	float value_;
};
template<> struct hxConsoleArg_<double> {
	inline hxConsoleArg_(const char* str_, char** next_) : value_(::strtod(str_, next_)) { }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "f64"; }
	double value_;
};
// bool params must be 0 or 1.
template<> struct hxConsoleArg_<bool> {
	inline hxConsoleArg_(const char* str_, char** next_) {
		unsigned long t_ = ::strtoul(str_, next_, 2);
		value_ = (t_ != 0ul);
	}
	HX_CONSTEXPR_FN static const char* getLabel_() { return "0/1"; }
	bool value_;
};

// const char* args capture remainder of line including comments starting with #'s.
// Leading whitespace is discarded and string may be empty.
template<> struct hxConsoleArg_<const char*> {
	inline hxConsoleArg_(const char* str_, char** next_) {
		while (*str_ != '\0' && hxConsoleIsDelimiter_(*str_)) {
			++str_;
		}
		value_ = str_;

		// the end of line pointer must be valid to compare < with str_.
		while(*str_ != '\0') { ++str_; }
		*next_ = const_cast<char*>(str_);
	}
	HX_CONSTEXPR_FN static const char* getLabel_() { return "char*"; }
	const char* value_;
};

// Defined in hxConsole.hpp. User overloadable in following templates.
template<typename R_> bool hxConsoleIsOkResult(R_ r_);

// Interpret functions returning void as having a successful result.
// Mixing variadic templates with specialization is not working.
template<typename R_> inline bool hxConsoleCommandIsOk0_(R_(*fn_)()) { return hxConsoleIsOkResult(fn_()); }
template<> inline bool hxConsoleCommandIsOk0_<void>(void(*fn_)()) { fn_(); return true; };

template<typename R_>
struct hxConsoleCommand0_ : public hxConsoleCommand_ {
	inline hxConsoleCommand0_(R_(*fn_)()) : m_fn_(fn_) { }

	virtual bool execute_(const char* str_) HX_OVERRIDE {
		if(hxConsoleIsEndOfline_(str_)) {
			return hxConsoleCommandIsOk0_<R_>(m_fn_);
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s\n", id_ ? id_ : "usage: no args"); (void)id_;
	}
	R_(*m_fn_)();
};

template<typename R_, typename A_>
struct hxConsoleCommandIsOk1_{
	static bool execute_(R_(*fn_)(A_), A_ a_) { return hxConsoleIsOkResult(fn_(a_)); }
};
template<typename A_>
struct hxConsoleCommandIsOk1_<void, A_> {
	static bool execute_(void(*fn_)(A_), A_ a_) { fn_(a_); return true; };
};
template<typename R_, typename A_>
struct hxConsoleCommand1_ : public hxConsoleCommand_ {
	inline hxConsoleCommand1_(R_(*fn_)(A_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* ptr_ = const_cast<char*>(str_);
		hxConsoleArg_<A_> arg1_(str_, &ptr_);
		if (str_ < ptr_ && hxConsoleIsEndOfline_(ptr_)) {
			return hxConsoleCommandIsOk1_<R_, A_>::execute_(m_fn_, arg1_.value_);
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A_);
};

template<typename R_, typename A1_, typename A2_>
struct hxConsoleCommandIsOk2_{
	static bool execute_(R_(*fn_)(A1_, A2_), A1_ a1_, A2_ a2_) { return hxConsoleIsOkResult(fn_(a1_, a2_)); }
};
template<typename A1_, typename A2_>
struct hxConsoleCommandIsOk2_<void, A1_, A2_> {
	static bool execute_(void(*fn_)(A1_, A2_), A1_ a1_, A2_ a2_) { fn_(a1_, a2_); return true; };
};
template<typename R_, typename A1_, typename A2_>
struct hxConsoleCommand2_ : public hxConsoleCommand_ {
	inline hxConsoleCommand2_(R_(*fn_)(A1_, A2_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = const_cast<char*>(p_);
		char* pB_ = const_cast<char*>(p_);
		hxConsoleArg_<A1_> arg1_(p_, &pA_);
		if (p_ < pA_) {
			hxConsoleArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ < pB_ && hxConsoleIsEndOfline_(pB_)) {
				return hxConsoleCommandIsOk2_<R_, A1_, A2_>::execute_(m_fn_, arg1_.value_, arg2_.value_);
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A1_>::getLabel_(), hxConsoleArg_<A2_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_);
};

template<typename R_, typename A1_, typename A2_, typename A3_>
struct hxConsoleCommandIsOk3_{
	static bool execute_(R_(*fn_)(A1_, A2_, A3_), A1_ a1_, A2_ a2_, A3_ a3_) { return hxConsoleIsOkResult(fn_(a1_, a2_, a3_)); }
};
template<typename A1_, typename A2_, typename A3_>
struct hxConsoleCommandIsOk3_<void, A1_, A2_, A3_> {
	static bool execute_(void(*fn_)(A1_, A2_, A3_), A1_ a1_, A2_ a2_, A3_ a3_) { fn_(a1_, a2_, a3_); return true; };
};
template<typename R_, typename A1_, typename A2_, typename A3_>
struct hxConsoleCommand3_ : public hxConsoleCommand_ {
	inline hxConsoleCommand3_(R_(*fn_)(A1_, A2_, A3_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = const_cast<char*>(p_);
		char* pB_ = const_cast<char*>(p_);
		hxConsoleArg_<A1_> arg1_(p_, &pA_);
		if (p_ < pA_) {
			hxConsoleArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ < pB_) {
				hxConsoleArg_<A3_> arg3_(pB_, &pA_);
				if (pB_ < pA_ && hxConsoleIsEndOfline_(pA_)) {
					return hxConsoleCommandIsOk3_<R_, A1_, A2_, A3_>::execute_(m_fn_, arg1_.value_, arg2_.value_, arg3_.value_);
				}
			}
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A1_>::getLabel_(), hxConsoleArg_<A2_>::getLabel_(), hxConsoleArg_<A3_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_, A3_);
};

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
struct hxConsoleCommandIsOk4_{
	static bool execute_(R_(*fn_)(A1_, A2_, A3_, A4_), A1_ a1_, A2_ a2_, A3_ a3_, A4_ a4_) { return hxConsoleIsOkResult(fn_(a1_, a2_, a3_, a4_)); }
};
template<typename A1_, typename A2_, typename A3_, typename A4_>
struct hxConsoleCommandIsOk4_<void, A1_, A2_, A3_, A4_> {
	static bool execute_(void(*fn_)(A1_, A2_, A3_, A4_), A1_ a1_, A2_ a2_, A3_ a3_, A4_ a4_) { fn_(a1_, a2_, a3_, a4_); return true; };
};
template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
struct hxConsoleCommand4_ : public hxConsoleCommand_ {
	inline hxConsoleCommand4_(R_(*fn_)(A1_, A2_, A3_, A4_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* p_) HX_OVERRIDE {
		char* pA_ = const_cast<char*>(p_);
		char* pB_ = const_cast<char*>(p_);
		hxConsoleArg_<A1_> arg1_(p_, &pA_);
		if (p_ < pA_) {
			hxConsoleArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ < pB_) {
				hxConsoleArg_<A3_> arg3_(pB_, &pA_);
				if (pB_ < pA_) {
					hxConsoleArg_<A4_> arg4_(pA_, &pB_);
					if (pA_ < pB_ && hxConsoleIsEndOfline_(pB_)) {
						return hxConsoleCommandIsOk4_<R_, A1_, A2_, A3_, A4_>::execute_(m_fn_, arg1_.value_, arg2_.value_, arg3_.value_, arg4_.value_);
					}
				}
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A1_>::getLabel_(), hxConsoleArg_<A2_>::getLabel_(), hxConsoleArg_<A3_>::getLabel_(),
			hxConsoleArg_<A4_>::getLabel_()); (void)id_;
	}
	R_(*m_fn_)(A1_, A2_, A3_, A4_);
};

template<typename T_>
struct hxConsoleVariable_ : public hxConsoleCommand_ {
	inline hxConsoleVariable_(volatile T_* var_) : m_var_(var_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		if (hxConsoleIsEndOfline_(str_)) {
			usage_("value_ is:"); // print type and value_.
			return true;
		}
		char* ptr_ = const_cast<char*>(str_);
		hxConsoleArg_<T_> x_(str_, &ptr_);
		if (str_ < ptr_ && hxConsoleIsEndOfline_(ptr_)) {
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
			hxLogConsole("%s %s(=%lld)\n", id_ ? id_ : "usage:", hxConsoleArg_<T_>::getLabel_(), (long long)*m_var_);
		}
		else {
			hxLogConsole("%s %s(=%lf)\n", id_ ? id_ : "usage:", hxConsoleArg_<T_>::getLabel_(), (double)*m_var_);
		}
	}
	volatile T_* m_var_;
};

template<typename R_>
HX_CONSTEXPR_FN hxConsoleCommand0_<R_> hxConsoleCommandFactory_(R_(*fn_)(void)) {
	return hxConsoleCommand0_<R_>(fn_);
}

template<typename R_, typename A1_>
HX_CONSTEXPR_FN hxConsoleCommand1_<R_, A1_> hxConsoleCommandFactory_(R_(*fn_)(A1_)) {
	return hxConsoleCommand1_<R_, A1_>(fn_);
}

template<typename R_, typename A1_, typename A2_>
HX_CONSTEXPR_FN hxConsoleCommand2_<R_, A1_, A2_> hxConsoleCommandFactory_(R_(*fn_)(A1_, A2_)) {
	return hxConsoleCommand2_<R_, A1_, A2_>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_>
HX_CONSTEXPR_FN hxConsoleCommand3_<R_, A1_, A2_, A3_> hxConsoleCommandFactory_(R_(*fn_)(A1_, A2_, A3_)) {
	return hxConsoleCommand3_<R_, A1_, A2_, A3_>(fn_);
}

template<typename R_, typename A1_, typename A2_, typename A3_, typename A4_>
HX_CONSTEXPR_FN hxConsoleCommand4_<R_, A1_, A2_, A3_, A4_> hxConsoleCommandFactory_(R_(*fn_)(A1_, A2_, A3_, A4_)) {
	return hxConsoleCommand4_<R_, A1_, A2_, A3_, A4_>(fn_);
}

template<typename T_>
HX_CONSTEXPR_FN hxConsoleVariable_<T_> hxConsoleVariableFactory_(volatile T_* var_) {
	return hxConsoleVariable_<T_>(var_);
}

// ERROR: Pointers cannot be console variables.
template<typename T_>
inline void hxConsoleVariableFactory_(volatile T_** var_) HX_DELETE_FN;
template<typename T_>
inline void hxConsoleVariableFactory_(const volatile T_** var_) HX_DELETE_FN;

// Wrap the string literal type because it is not used normally.
struct hxConsoleHashTableKey_ {
	explicit hxConsoleHashTableKey_(const char* s_) : str_(s_) { }
	const char* str_;
};

// Uses FNV-1a string hashing.
inline uint32_t hxKeyHash(hxConsoleHashTableKey_ k_) {
	uint32_t x_ = (uint32_t)0x811c9dc5;
	while (!hxConsoleIsDelimiter_(*k_.str_)) {
		x_ ^= (uint32_t)*k_.str_++;
		x_ *= (uint32_t)0x01000193;
	}
	return x_;
}

inline uint32_t hxKeyEqual(hxConsoleHashTableKey_ lhs_, hxConsoleHashTableKey_ rhs_)
{
	size_t len_ = hxmin(::strlen(lhs_.str_), ::strlen(rhs_.str_));
	return ::strncmp(lhs_.str_, rhs_.str_, len_) == 0;
};

// this is how to write a hash node without including hash table code.
class hxConsoleHashTableNode_ {
public:
	typedef hxConsoleHashTableKey_ Key;

	inline hxConsoleHashTableNode_(hxConsoleHashTableKey_ key_)
			: m_hashNext_(hxnull), m_key_(key_), m_hash_(hxKeyHash(key_)), m_command_(hxnull) {
		if ((HX_RELEASE) < 1) {
			const char* k_ = key_.str_;
			while (!hxConsoleIsDelimiter_(*k_)) {
				++k_;
			}
			hxAssertMsg(*k_ == '\0', "console symbol contains delimiter: \"%s\"", key_.str_);
		}
	}

	// Boilerplate required by hxHashTable.
	void* hashNext(void) const { return m_hashNext_; }
	void*& hashNext(void) { return m_hashNext_; }

	hxConsoleHashTableKey_ key(void) const { return m_key_; }
	uint32_t hash(void) const { return m_hash_; }
	hxConsoleCommand_* value(void) const { return m_command_; }
	void setValue(hxConsoleCommand_* x_) { m_command_ = x_; }

private:
	void* m_hashNext_;
	hxConsoleHashTableKey_ m_key_;
	uint32_t m_hash_;
	hxConsoleCommand_* m_command_;
};

void hxConsoleRegister_(hxConsoleHashTableNode_* node);

// registers a console command using a global variable without memory allocations.
// There is no reason to deregister or destruct anything.
struct hxConsoleConstructor_ {

	template<typename Command_>
	inline hxConsoleConstructor_(Command_ fn_, const char* id_)
			: m_node_(hxConsoleHashTableKey_(id_)) {
		::new(m_storage_ + 0) Command_(fn_);
		m_node_.setValue((Command_*)(m_storage_ + 0));
		hxConsoleRegister_(&m_node_);
	}

	// Provide static storage instead of using allocator before main.
	hxConsoleHashTableNode_ m_node_;
	char m_storage_[sizeof(hxConsoleCommand0_<void>)]; // .vtable and user function pointer
};
