#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.
// hxconsole inline header and a lot of internals. See hxconsole.h.

// This is a little old fashioned but a simple rewrite wouldn't make the
// template bloat any better. Although this code is still very efficient. In
// theory a byte code interpreter that understood the target calling
// convention could shave off 10k from a system with 100s of commands.

// Automatic cast from double with clamping.
template<typename T_>
hxconsolenumber_t::operator T_(void) const {
	// Reimplement std::numeric_limits for 2's compliment. The << operator
	// promotes its operands to int and so that requires more casting.
	const bool is_signed_ = static_cast<T_>(-1) < T_(0u);
	const T_ min_value_ = is_signed_ ? T_(T_(1u) << (sizeof(T_) * 8 - 1)) : T_(0u);
	const T_ max_value_ = ~min_value_;

	double clamped_ = hxclamp(m_x_, (double)min_value_, (double)max_value_);
	hxassertmsg(m_x_ == clamped_, "parameter_overflow %lf -> %lf", m_x_, clamped_);

	// Avoid the undefined behavior sanitizer by clamping value.
	T_ t = (T_)clamped_;
	return t;
}

// Automatic cast from uint without clamping. The sanitizer doesn't complain.
template<typename T_>
hxconsolehex_t::operator T_(void) const {
	T_ t = (T_)m_x_;
	return t;
}

namespace hxdetail_ {

// Console tokens are delimited by any whitespace and non-printing low-ASCII
// characters. `NUL` is considered a delimiter and must be checked for separately.
// This happens to be UTF-8 compatable because it ignores characters `>= U+0100`.
inline bool hxconsole_is_delimiter_(char ch_) { return ch_ <= 32; }

// Checks for printing characters.
inline bool hxconsole_is_end_of_line_(const char* str_) {
	while(*str_ != '\0' && hxconsole_is_delimiter_(*str_)) {
		++str_;
	}
	return *str_ == '\0' || *str_ == '#'; // Skip comments
}

// `hxconsole_arg_<T_>` - Binds string parsing operations to function args. Invalid
// arguments are set to 0, arguments out of range result in the maximum
// representable values.
template<typename T_> class hxconsole_arg_ {
private:
	// Unsupported parameter type. No class, class or reference args allowed.
	// Use the following overloads.
	hxconsole_arg_(const char* str_, char** next_) = delete;
};
template<> class hxconsole_arg_<hxconsolenumber_t> {
public:
	hxconsole_arg_(const char* str_, char** next_) : value_(::strtod(str_, next_)) { }
	static const char* get_label_(void) { return "f64"; }
	hxconsolenumber_t value_;
};
template<> class hxconsole_arg_<hxconsolehex_t> {
public:
	hxconsole_arg_(const char* str_, char** next_) : value_(::strtoull(str_, next_, 16)) { }
	static const char* get_label_(void) { return "hex"; }
	hxconsolehex_t value_;
};
// const char* args capture remainder of line including comments starting with #'s.
// Leading whitespace is discarded and string may be empty.
template<> class hxconsole_arg_<const char*> {
public:
	hxconsole_arg_(const char* str_, char** next_) {
		while(*str_ != '\0' && hxconsole_is_delimiter_(*str_)) {
			++str_;
		}
		value_ = str_;

		// the end of line pointer must be valid to compare < with str_.
		while(*str_ != '\0') { ++str_; }
		*next_ = const_cast<char*>(str_);
	}
	static const char* get_label_(void) { return "char*"; }
	const char* value_;
};

class hxconsole_command_ {
public:
	virtual bool execute_(const char* str_) = 0; // Return false for parse errors.
	virtual void usage_(const char* id_=hxnull) = 0; // Expects command name.

	// Returns 0 if no parameter. Returns 1 if a single number was found. Returns
	// 2 to indicate a parse error. This avoids template bloat by being in a
	// base class.
	// str - Parameters.
	// number - Overwritten with the parsed value or undefined.
	static int execute_number_(const char* str_, double* number_) {
		if(hxconsole_is_end_of_line_(str_)) {
			return 0; // success, do not modify
		}

		char* ptr_ = const_cast<char*>(str_);
		*number_ = ::strtod(str_, &ptr_);
		if(str_ < ptr_ && hxconsole_is_end_of_line_(ptr_)) {
			return 1; // success, do modify
		}

		hxlogconsole("parse error: %s", str_);
		return 2; // failure, do not modify
	}
};

class hxconsole_command0_ : public hxconsole_command_ {
public:
	hxconsole_command0_(bool(*fn_)()) : m_fn_(fn_) { }

	virtual bool execute_(const char* str_) override {
		if(hxconsole_is_end_of_line_(str_)) {
			return m_fn_();
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) override {
		hxlogconsole("%s\n", id_ ? id_ : "usage: no args"); (void)id_;
	}
private:
	bool(*m_fn_)();
};

template<typename A_>
class hxconsole_command1_ : public hxconsole_command_ {
public:
	hxconsole_command1_(bool(*fn_)(A_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) override {
		char* ptr_ = const_cast<char*>(str_);
		hxconsole_arg_<A_> arg1_(str_, &ptr_);
		if(str_ < ptr_ && hxconsole_is_end_of_line_(ptr_)) {
			return m_fn_(arg1_.value_);
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) override {
		hxlogconsole("%s %s\n", id_ ? id_ : "usage:", hxconsole_arg_<A_>::get_label_()); (void)id_;
	}
private:
	bool(*m_fn_)(A_);
};

template<typename arg1_t_, typename arg2_t_>
class hxconsole_command2_ : public hxconsole_command_ {
public:
	hxconsole_command2_(bool(*fn_)(arg1_t_, arg2_t_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) override {
		char* p_a_ = const_cast<char*>(str_);
		char* p_b_ = const_cast<char*>(str_);
		hxconsole_arg_<arg1_t_> arg1_(str_, &p_a_);
		if(str_ < p_a_) {
			hxconsole_arg_<arg2_t_> arg2_(p_a_, &p_b_);
			if(p_a_ < p_b_ && hxconsole_is_end_of_line_(p_b_)) {
				return m_fn_(arg1_.value_, arg2_.value_);
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) override {
		hxlogconsole("%s %s %s\n", id_ ? id_ : "usage:", hxconsole_arg_<arg1_t_>::get_label_(), hxconsole_arg_<arg2_t_>::get_label_()); (void)id_;
	}
private:
	bool(*m_fn_)(arg1_t_, arg2_t_);
};

template<typename arg1_t_, typename arg2_t_, typename arg3_t_>
class hxconsole_command3_ : public hxconsole_command_ {
public:
	hxconsole_command3_(bool(*fn_)(arg1_t_, arg2_t_, arg3_t_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) override {
		char* p_a_ = const_cast<char*>(str_);
		char* p_b_ = const_cast<char*>(str_);
		hxconsole_arg_<arg1_t_> arg1_(str_, &p_a_);
		if(str_ < p_a_) {
			hxconsole_arg_<arg2_t_> arg2_(p_a_, &p_b_);
			if(p_a_ < p_b_) {
				hxconsole_arg_<arg3_t_> arg3_(p_b_, &p_a_);
				if(p_b_ < p_a_ && hxconsole_is_end_of_line_(p_a_)) {
					return m_fn_(arg1_.value_, arg2_.value_, arg3_.value_);
				}
			}
		}

		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) override {
		hxlogconsole("%s %s %s %s\n", id_ ? id_ : "usage:", hxconsole_arg_<arg1_t_>::get_label_(), hxconsole_arg_<arg2_t_>::get_label_(), hxconsole_arg_<arg3_t_>::get_label_()); (void)id_;
	}
private:
	bool(*m_fn_)(arg1_t_, arg2_t_, arg3_t_);
};

template<typename arg1_t_, typename arg2_t_, typename arg3_t_, typename arg4_t_>
class hxconsole_command4_ : public hxconsole_command_ {
public:
	hxconsole_command4_(bool(*fn_)(arg1_t_, arg2_t_, arg3_t_, arg4_t_)) : m_fn_(fn_) { }
	virtual bool execute_(const char* str_) override {
		char* p_a_ = const_cast<char*>(str_);
		char* p_b_ = const_cast<char*>(str_);
		hxconsole_arg_<arg1_t_> arg1_(str_, &p_a_);
		if(str_ < p_a_) {
			hxconsole_arg_<arg2_t_> arg2_(p_a_, &p_b_);
			if(p_a_ < p_b_) {
				hxconsole_arg_<arg3_t_> arg3_(p_b_, &p_a_);
				if(p_b_ < p_a_) {
					hxconsole_arg_<arg4_t_> arg4_(p_a_, &p_b_);
					if(p_a_ < p_b_ && hxconsole_is_end_of_line_(p_b_)) {
						return m_fn_(arg1_.value_, arg2_.value_, arg3_.value_, arg4_.value_);
					}
				}
			}
		}
		usage_();
		return false;
	}
	virtual void usage_(const char* id_=hxnull) override {
		hxlogconsole("%s %s %s %s %s\n", id_ ? id_ : "usage:", hxconsole_arg_<arg1_t_>::get_label_(), hxconsole_arg_<arg2_t_>::get_label_(), hxconsole_arg_<arg3_t_>::get_label_(),
			hxconsole_arg_<arg4_t_>::get_label_()); (void)id_;
	}
private:
	bool(*m_fn_)(arg1_t_, arg2_t_, arg3_t_, arg4_t_);
};

template<typename T_>
class hxconsole_variable_ : public hxconsole_command_ {
public:
	hxconsole_variable_(volatile T_* var_) : m_var_(var_) { }

	// Use execute_number_ to avoid template bloat.
	virtual bool execute_(const char* str_) override {
		double number_ = 0.0;
		int code_ = execute_number_(str_, &number_);
		if(code_ == 0) {
			// 0 parameters is a query
			hxloghandler(hxloglevel_console, "%.15g\n", (double)*m_var_);
			return true;
		}
		else if(code_ == 1) {
			// 1 parameter is assignment
			// Use hxconsolenumber_t to oversee casting to an arbitrary type.
			hxconsolenumber_t wrapper_(number_);
			*m_var_ = (T_)wrapper_;
			return true;
		}
		return false; // 2 is unexpected args.

	}

	virtual void usage_(const char* id_=hxnull) override {
		(void)id_;
		hxlogconsole("%s <optional-value>\n", id_ ? id_ : "usage:");
	}
private:
	volatile T_* m_var_;
};

inline hxconsole_command0_ hxconsole_command_factory_(bool(*fn_)(void)) {
	return hxconsole_command0_(fn_);
}

template<typename arg1_t_>
inline hxconsole_command1_<arg1_t_> hxconsole_command_factory_(bool(*fn_)(arg1_t_)) {
	return hxconsole_command1_<arg1_t_>(fn_);
}

template<typename arg1_t_, typename arg2_t_>
inline hxconsole_command2_<arg1_t_, arg2_t_> hxconsole_command_factory_(bool(*fn_)(arg1_t_, arg2_t_)) {
	return hxconsole_command2_<arg1_t_, arg2_t_>(fn_);
}

template<typename arg1_t_, typename arg2_t_, typename arg3_t_>
inline hxconsole_command3_<arg1_t_, arg2_t_, arg3_t_> hxconsole_command_factory_(bool(*fn_)(arg1_t_, arg2_t_, arg3_t_)) {
	return hxconsole_command3_<arg1_t_, arg2_t_, arg3_t_>(fn_);
}

template<typename arg1_t_, typename arg2_t_, typename arg3_t_, typename arg4_t_>
inline hxconsole_command4_<arg1_t_, arg2_t_, arg3_t_, arg4_t_> hxconsole_command_factory_(bool(*fn_)(arg1_t_, arg2_t_, arg3_t_, arg4_t_)) {
	return hxconsole_command4_<arg1_t_, arg2_t_, arg3_t_, arg4_t_>(fn_);
}

template<typename T_>
inline hxconsole_variable_<T_> hxconsole_variable_factory_(volatile T_* var_) {
	return hxconsole_variable_<T_>(var_);
}

// ERROR: Pointers cannot be console variables.
template<typename T_>
inline void hxconsole_variable_factory_(volatile T_** var_) = delete;
template<typename T_>
inline void hxconsole_variable_factory_(const volatile T_** var_) = delete;

// Wrap the string literal type because it is not used normally.
class hxconsole_hash_table_key_ {
public:
	explicit hxconsole_hash_table_key_(const char* s_) : str_(s_) { }
	const char* str_;
};

// Uses FNV-1a string hashing. Stops at whitespace.
inline hxhash_t hxkey_hash(hxconsole_hash_table_key_ k_) {
	hxhash_t x_ = (hxhash_t)0x811c9dc5;
	while(!hxconsole_is_delimiter_(*k_.str_)) {
		x_ ^= (hxhash_t)*k_.str_++;
		x_ *= (hxhash_t)0x01000193;
	}
	return x_;
}

// A version of ::strcmp that stops at whitespace or NUL.
inline hxhash_t hxkey_equal(hxconsole_hash_table_key_ a_, hxconsole_hash_table_key_ b_) {
	while(!hxconsole_is_delimiter_(*a_.str_) && *a_.str_ == *b_.str_) { ++a_.str_; ++b_.str_; }
	return hxconsole_is_delimiter_(*a_.str_) && hxconsole_is_delimiter_(*b_.str_);
};

// this is how to write a hash node without including hash table code.
class hxconsole_hash_table_node_ {
public:
	typedef hxconsole_hash_table_key_ key_t;

	hxconsole_hash_table_node_(hxconsole_hash_table_key_ key_)
			: m_hash_next_(hxnull), m_key_(key_), m_hash_(hxkey_hash(key_)), m_command_(hxnull) {
		if((HX_RELEASE) < 1) {
			const char* k_ = key_.str_;
			while(!hxconsole_is_delimiter_(*k_)) {
				++k_;
			}
			hxassertmsg(*k_ == '\0', "bad_console_symbol \"%s\"", key_.str_);
		}
	}

	// Boilerplate required by hxhash_table.
	void* hash_next(void) const { return m_hash_next_; }
	void*& hash_next(void) { return m_hash_next_; }

	hxconsole_hash_table_key_ key(void) const { return m_key_; }
	hxhash_t hash(void) const { return m_hash_; }
	hxconsole_command_* command_(void) const { return m_command_; }
	void set_command_(hxconsole_command_* x_) { m_command_ = x_; }

private:
	void* m_hash_next_;
	hxconsole_hash_table_key_ m_key_;
	hxhash_t m_hash_;
	hxconsole_command_* m_command_;
};

void hxconsole_register_(hxconsole_hash_table_node_* node);

// registers a console command using a global variable without memory allocations.
// There is no reason to deregister or destruct anything.
class hxconsole_constructor_ {
public:
	template<typename Command_>
	hxconsole_constructor_(Command_ fn_, const char* id_)
			: m_node_(hxconsole_hash_table_key_(id_)) {
		::new(m_storage_ + 0) Command_(fn_);
		m_node_.set_command_((Command_*)(m_storage_ + 0));
		hxconsole_register_(&m_node_);
	}

private:
	// Provide static storage instead of using allocator before main.
	hxconsole_hash_table_node_ m_node_;
	char m_storage_[sizeof(hxconsole_command0_)]; // .vtable and user function pointer
};

} // hxdetail_
using namespace hxdetail_;
