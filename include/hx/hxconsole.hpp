#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

class hxfile;

// hxconsole API - Implements a simple console for remote use or to implement
// configuration files. Output is directed to the system log with
// hxloglevel_console. A remote console will require forwarding commands to the
// target and reporting the system log back. Configuration files only require
// file I/O. C-style calls returning bool with up to 4 args using "const char*",
// hxconsolenumber_t or hxconsolehex_t as parameter types are required for the
// bindings to work.

// hxconsolenumber_t - A number. Uses double as an intermediate type. This
// reduces template bloat by limiting parameter types. This is the same type of
// generic number approach Java_script uses. Always 64-bit.
class hxconsolenumber_t {
public:
    hxconsolenumber_t(void) : m_x_(0.0) { }
    template<typename T_> hxconsolenumber_t(T_ x_) : m_x_((double)x_) { }
	template<typename T_> operator T_() const {
        // Reimplement std::numeric_limits for 2's compliment. The << operator
        // promotes its operands to int and so that requires more casting.
        // Manipulating the sign bit is not supported by the standard. Sorry.
        const bool is_signed_ = static_cast<T_>(-1) < T_(0u);
        const T_ min_value_ = is_signed_ ? T_(T_(1u) << (sizeof(T_) * 8 - 1)) : T_(0u);
        const T_ max_value_ = ~min_value_;

        double clamped_ = hxclamp(m_x_, (double)min_value_, (double)max_value_);
        hxassertmsg(m_x_ == clamped_, "parameter overflow: %lf -> %lf", m_x_, clamped_);

        // Asserts may be skipped. Avoid the undefined behavior sanitizer by
        // clamping value.
        T_ t = (T_)clamped_;
        return t;
    }

	operator bool() const { return m_x_ != 0.0; }
	operator float() const { return (float)m_x_; }
	operator double() const { return m_x_; }

private:
    // ERROR - Numbers are not pointers or references.
    template<typename T_> operator T_*() const hxdelete_fn;

    double m_x_;
};

// hxconsolehex_t - A hex value. Uses uint64_t as an intermediate type. This
// type of command parameter parses hex and then uses a C-style cast to
// convert to any type. Useful for passing pointers and hash values via the
// console. Always 64-bit.
class hxconsolehex_t {
public:
    hxconsolehex_t(void) : m_x_(0u) { }
    hxconsolehex_t(uint64_t x_) : m_x_(x_) { }
	template<typename T_> operator T_() const {
        T_ t = (T_)m_x_;
        hxwarnmsg((uint64_t)t == m_x_, "precision error: %llx -> %llx",
            (unsigned long long)m_x_, (unsigned long long)t);
        return t;
    }

private:
    hxstatic_assert(sizeof(uint64_t) >= sizeof(uintptr_t), "128-bit pointers?");
	uint64_t m_x_;
};

// hxconsole_command - Registers a function using a global constructor. Use in a
// global scope. Command will have the same name and args as the function.
// - x: Valid C identifier that evaluates to a function pointer.
//   E.g. hxconsole_command(srand);
#define hxconsole_command(x_) static hxconsole_constructor_ \
    g_hxconsole_symbol_##x_(hxconsole_command_factory_(&(x_)), #x_)

// hxconsole_command_named - Registers a named function using a global constructor.
// Use in a global scope. Provided name_ must be a valid C identifier.
// - x: Any expression that evaluates to a function pointer.
// - name: Valid C identifier that identifies the command.
//   E.g. hxconsole_command_named(srand, seed_rand);
#define hxconsole_command_named(x_, name_) static hxconsole_constructor_ \
    g_hxconsole_symbol_##name_(hxconsole_command_factory_(&(x_)), #name_)

// hxconsole_variable - Registers a variable. Use in a global scope. Will have the
// same name as the variable.
// - x: Valid C identifier that evaluates to a variable.
//   E.g.
//   static bool is_my_hack_enabled=false;
//   hxconsole_variable(is_my_hack_enabled);
#define hxconsole_variable(x_) static hxconsole_constructor_ \
    g_hxconsole_symbol_##x_(hxconsole_variable_factory_(&(x_)), #x_)

// hxconsole_variable_named - Registers a named variable. Use in a global scope.
// Provided name must be a valid C identifier.
// - x: Any expression that evaluates to a variable.
// - name: Valid C identifier that identifies the variable.
//   E.g.
//   static bool is_my_hack_enabled=false;
//   hxconsole_variable_named(is_my_hack_enabled, f_hack); // add "f_hack" to the console.
#define hxconsole_variable_named(x_, name_) static hxconsole_constructor_ \
    g_hxconsole_symbol_##name_(hxconsole_variable_factory_(&(x_)), #name_)

// hxconsole_deregister - Explicit de-registration of a console symbol.
// - id: Valid C identifier that identifies the variable.
void hxconsole_deregister(const char* id_);

// hxconsole_exec_line - Evaluates a console command to either call a function or
// set a variable. E.g.: "srand 77" or "a_variable 5"
// - command: A string executed by the console.
bool hxconsole_exec_line(const char* command_);

// hxconsole_exec_file - Executes a configuration file which is opened for reading.
// Ignores blank lines and comments starting with #.
// - file: A file containing commands.
bool hxconsole_exec_file(hxfile& file_);

// hxconsole_exec_filename - Opens a configuration file by name and executes it.
// - filename: A file containing commands.
bool hxconsole_exec_filename(const char* filename_);

// hxconsole_help - Logs all console symbols to the console log.
bool hxconsole_help();

// Include internals after hxconsolehex_t
#include <hx/internal/hxconsole_internal.hpp>
