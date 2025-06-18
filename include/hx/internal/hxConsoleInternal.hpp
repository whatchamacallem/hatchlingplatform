#pragma once
// Copyright 2017-2025 Adrian Johnston

// hxConsole internals. See hxConsole.h instead.

// Console tokens are delimited by any whitespace and non-printing low-ASCII
// characters. NUL is considered a delimiter and must be checked for separately.
// This happens to be UTF-8 compatable because it ignores characters >= U+0100.
HX_CONSTEXPR_FN static bool hxConsoleIsDelimiter_(char ch_) { return ch_ <= 32; }

// Checks for printing characters.
HX_CONSTEXPR_FN static bool hxConsoleIsEndOfline_(const char* str_) {
	while (*str_ != '\0' && hxConsoleIsDelimiter_(*str_)) {
		++str_;
	}
	return *str_ == '\0' || *str_ == '#'; // Skip comments
}

// hxConsoleArg_<T_>. Binds string parsing operations to function args. Invalid
// arguments are set to 0, arguments out of range result in the maximum
// representable values.
template<typename T_> struct hxConsoleArg_ {
private:
	// Unsupported parameter type. No struct, class or reference args allowed.
	// Use the following overloads.
    hxConsoleArg_(const char* str_, char** next_) HX_DELETE_FN;
};
template<> struct hxConsoleArg_<hxconsolenumber_t> {
	inline hxConsoleArg_(const char* str_, char** next_) : value_(::strtod(str_, next_)) { }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "f64"; }
	hxconsolenumber_t value_;
};
template<> struct hxConsoleArg_<hxconsolehex_t> {
	inline hxConsoleArg_(const char* str_, char** next_) : value_(::strtoull(str_, next_, 16)) { }
	HX_CONSTEXPR_FN static const char* getLabel_() { return "hex"; }
	hxconsolehex_t value_;
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

struct hxConsoleCommand_ {
	virtual bool execute_(const char* str_) = 0; // Return false for parse errors.
	virtual void usage_(const char* id_=hxnull) = 0; // Expects command name.

	// Returns 0 if no parameter. Returns 1 if a single number was found. Returns
	// 2 to indicate a parse error. This avoids template bloat by being in a
	// base class.
	// str - Parameters.
	// number - Overwritten with the parsed value or undefined.
	static int executeNumber_(const char* str_, double* number_) {
		if(hxConsoleIsEndOfline_(str_)) {
			return 0; // success, do not modify
		}

		char* ptr_ = const_cast<char*>(str_);
		*number_ = ::strtod(str_, &ptr_);
		if(str_ < ptr_ && hxConsoleIsEndOfline_(ptr_)) {
			return 1; // success, do modify
		}

		hxLogConsole("parse error: %s", str_);
		return 2; // failure, do not modify
	}
};

struct hxConsoleCommand0_ : public hxConsoleCommand_ {
	inline hxConsoleCommand0_(bool(*fn_)()) : m_fn_(fn_) { }

	virtual bool execute_(const char* str_) HX_OVERRIDE {
		if(hxConsoleIsEndOfline_(str_)) {
			return m_fn_();
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s\n", id_ ? id_ : "usage: no args"); (void)id_;
	}
	bool(*m_fn_)();
};

template<typename A_>
struct hxConsoleCommand1_ : public hxConsoleCommand_ {
	inline hxConsoleCommand1_(bool(*fn_)(A_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* ptr_ = const_cast<char*>(str_);
		hxConsoleArg_<A_> arg1_(str_, &ptr_);
		if (str_ < ptr_ && hxConsoleIsEndOfline_(ptr_)) {
			return m_fn_(arg1_.value_);
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A_>::getLabel_()); (void)id_;
	}
	bool(*m_fn_)(A_);
};

template<typename A1_, typename A2_>
struct hxConsoleCommand2_ : public hxConsoleCommand_ {
	inline hxConsoleCommand2_(bool(*fn_)(A1_, A2_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* pA_ = const_cast<char*>(str_);
		char* pB_ = const_cast<char*>(str_);
		hxConsoleArg_<A1_> arg1_(str_, &pA_);
		if (str_ < pA_) {
			hxConsoleArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ < pB_ && hxConsoleIsEndOfline_(pB_)) {
				return m_fn_(arg1_.value_, arg2_.value_);
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A1_>::getLabel_(), hxConsoleArg_<A2_>::getLabel_()); (void)id_;
	}
	bool(*m_fn_)(A1_, A2_);
};

template<typename A1_, typename A2_, typename A3_>
struct hxConsoleCommand3_ : public hxConsoleCommand_ {
	inline hxConsoleCommand3_(bool(*fn_)(A1_, A2_, A3_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* pA_ = const_cast<char*>(str_);
		char* pB_ = const_cast<char*>(str_);
		hxConsoleArg_<A1_> arg1_(str_, &pA_);
		if (str_ < pA_) {
			hxConsoleArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ < pB_) {
				hxConsoleArg_<A3_> arg3_(pB_, &pA_);
				if (pB_ < pA_ && hxConsoleIsEndOfline_(pA_)) {
					m_fn_(arg1_.value_, arg2_.value_, arg3_.value_);
				}
			}
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		hxLogConsole("%s %s %s %s\n", id_ ? id_ : "usage:", hxConsoleArg_<A1_>::getLabel_(), hxConsoleArg_<A2_>::getLabel_(), hxConsoleArg_<A3_>::getLabel_()); (void)id_;
	}
	bool(*m_fn_)(A1_, A2_, A3_);
};

template<typename A1_, typename A2_, typename A3_, typename A4_>
struct hxConsoleCommand4_ : public hxConsoleCommand_ {
	inline hxConsoleCommand4_(bool(*fn_)(A1_, A2_, A3_, A4_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		char* pA_ = const_cast<char*>(str_);
		char* pB_ = const_cast<char*>(str_);
		hxConsoleArg_<A1_> arg1_(str_, &pA_);
		if (str_ < pA_) {
			hxConsoleArg_<A2_> arg2_(pA_, &pB_);
			if (pA_ < pB_) {
				hxConsoleArg_<A3_> arg3_(pB_, &pA_);
				if (pB_ < pA_) {
					hxConsoleArg_<A4_> arg4_(pA_, &pB_);
					if (pA_ < pB_ && hxConsoleIsEndOfline_(pB_)) {
						return m_fn_(arg1_.value_, arg2_.value_, arg3_.value_, arg4_.value_);
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
	bool(*m_fn_)(A1_, A2_, A3_, A4_);
};

template<typename T_>
struct hxConsoleVariable_ : public hxConsoleCommand_ {
	inline hxConsoleVariable_(volatile T_* var_) : m_var_(var_) { }

	// Use executeNumber_ to avoid template bloat.
	virtual bool execute_(const char* str_) HX_OVERRIDE {
		double number_ = 0.0;
		int code_ = executeNumber_(str_, &number_);
		if(code_ == 1) {
			// 1 indicates a value was read.
			*m_var_ = (T_)number_;
		}
		hxLogConsole("value: %.15g\n", (double)*m_var_);
		return code_ != 2; // 2 is unexpected args.
	}

	virtual void usage_(const char* id_=hxnull) HX_OVERRIDE {
		(void)id_;
		hxLogConsole("%s <optional-value>\n", id_ ? id_ : "usage:");
	}
	volatile T_* m_var_;
};

inline hxConsoleCommand0_ hxConsoleCommandFactory_(bool(*fn_)(void)) {
	return hxConsoleCommand0_(fn_);
}

template<typename A1_>
inline hxConsoleCommand1_<A1_> hxConsoleCommandFactory_(bool(*fn_)(A1_)) {
	return hxConsoleCommand1_<A1_>(fn_);
}

template<typename A1_, typename A2_>
inline hxConsoleCommand2_<A1_, A2_> hxConsoleCommandFactory_(bool(*fn_)(A1_, A2_)) {
	return hxConsoleCommand2_<A1_, A2_>(fn_);
}

template<typename A1_, typename A2_, typename A3_>
inline hxConsoleCommand3_<A1_, A2_, A3_> hxConsoleCommandFactory_(bool(*fn_)(A1_, A2_, A3_)) {
	return hxConsoleCommand3_<A1_, A2_, A3_>(fn_);
}

template<typename A1_, typename A2_, typename A3_, typename A4_>
inline hxConsoleCommand4_<A1_, A2_, A3_, A4_> hxConsoleCommandFactory_(bool(*fn_)(A1_, A2_, A3_, A4_)) {
	return hxConsoleCommand4_<A1_, A2_, A3_, A4_>(fn_);
}

template<typename T_>
inline hxConsoleVariable_<T_> hxConsoleVariableFactory_(volatile T_* var_) {
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

// Uses FNV-1a string hashing. Stops at whitespace.
inline uint32_t hxKeyHash(hxConsoleHashTableKey_ k_) {
	uint32_t x_ = (uint32_t)0x811c9dc5;
	while (!hxConsoleIsDelimiter_(*k_.str_)) {
		x_ ^= (uint32_t)*k_.str_++;
		x_ *= (uint32_t)0x01000193;
	}
	return x_;
}

// A version of ::strcmp that stops at whitespace or NUL.
inline uint32_t hxKeyEqual(hxConsoleHashTableKey_ a_, hxConsoleHashTableKey_ b_) {
	while(!hxConsoleIsDelimiter_(*a_.str_) && *a_.str_ == *b_.str_) { ++a_.str_; ++b_.str_; }
	return hxConsoleIsDelimiter_(*a_.str_) && hxConsoleIsDelimiter_(*b_.str_);
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
	hxConsoleCommand_* command_(void) const { return m_command_; }
	void setCommand_(hxConsoleCommand_* x_) { m_command_ = x_; }

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
		m_node_.setCommand_((Command_*)(m_storage_ + 0));
		hxConsoleRegister_(&m_node_);
	}

	// Provide static storage instead of using allocator before main.
	hxConsoleHashTableNode_ m_node_;
	char m_storage_[sizeof(hxConsoleCommand0_)]; // .vtable and user function pointer
};
