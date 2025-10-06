#pragma once
// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

/// \file hx/hxconsole.hpp Implements a simple console for debugging, remote use
/// or for parsing configuration files. Output is directed to the system log
/// with `hxloglevel_console`. A remote console requires forwarding commands to
/// the target and reporting the system log back. Configuration files only need
/// file I/O. C-style calls that return `bool` with up to four arguments using
/// `const char*`, `hxconsolenumber_t`, or `hxconsolehex_t` parameter types are
/// required for the bindings to work. See the following commands for examples.
///
/// | Parameter Type | Purpose |
/// | --- | --- |
/// | `const char*` | Passes ASCII/UTF-8 tokens directly to the command handler. |
/// | `hxconsolenumber_t` | Transports numeric arguments via a double-backed wrapper. |
/// | `hxconsolehex_t` | Transports integer or pointer arguments encoded as hexadecimal. |

#include "hatchling.h"

class hxfile;

/// `hxconsolenumber_t` - Numeric wrapper that uses `double` as an intermediate
/// type. This reduces template bloat by limiting parameter types. This mirrors
/// the generic number approach JavaScript uses. Always 64-bit.
class hxconsolenumber_t {
public:
	/// Initializes to zero.
	hxconsolenumber_t(void) : m_x_(0.0) { }

	/// Constructs from any number.
	template<typename T_> hxconsolenumber_t(T_ x_) : m_x_((double)x_) { }

	/// Automatically casts to all number types.
	template<typename T_> operator T_() const;
	operator bool(void) const { return m_x_ != 0.0; }
	operator float(void) const { return (float)m_x_; }
	operator double(void) const { return m_x_; }

private:
	// ERROR: Numbers are not pointers or references.
	template<typename T_> operator T_*() const = delete;

	double m_x_;
};

/// `hxconsolehex_t` - Hexadecimal wrapper that uses `uint64_t` as an
/// intermediate type. The command parameter parses hex and then uses a C-style
/// cast to convert to any type. Useful for passing pointers and hash values via
/// the console. Always 64-bit.
class hxconsolehex_t {
public:
	/// Initializes to zero.
	hxconsolehex_t(void) : m_x_(0u) { }

	/// Constructs from any integer.
	hxconsolehex_t(uint64_t x_) : m_x_(x_) { }

	/// Automatically casts to all integer types.
	template<typename T_> operator T_() const;

private:
	static_assert(sizeof(uint64_t) >= sizeof(uintptr_t), "128-bit pointers?");
	uint64_t m_x_;
};

/// `hxconsole_command` - Registers a function using a global constructor. Use in
/// a global scope. The command uses the same name and arguments as the function.
/// - `x` : Valid C identifier that evaluates to a function pointer.
///   e.g., `hxconsole_command(srand);`
#define hxconsole_command(x_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##x_(hxconsole_command_factory_(&(x_)), #x_)

/// `hxconsole_command_named` - Registers a named function using a global
/// constructor. Use in a global scope. The provided name must be a valid C
/// identifier.
/// - `x` : Any expression that evaluates to a function pointer.
/// - `name` : Valid C identifier that identifies the command.
///   e.g., `hxconsole_command_named(srand, seed_rand);`
#define hxconsole_command_named(x_, name_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##name_(hxconsole_command_factory_(&(x_)), #name_)

/// `hxconsole_variable` - Registers a variable. Use in a global scope. The
/// command has the same name as the variable.
/// - `x` : Valid C identifier that evaluates to a variable.
///   e.g.,
/// ```cpp
///   static bool is_my_hack_enabled=false;
///   hxconsole_variable(is_my_hack_enabled);
/// ```
#define hxconsole_variable(x_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##x_(hxconsole_variable_factory_(&(x_)), #x_)

/// `hxconsole_variable_named` - Registers a named variable. Use in a global
/// scope. The provided name must be a valid C identifier.
/// - `x` : Any expression that evaluates to a variable.
/// - `name` : Valid C identifier that identifies the variable.
///   e.g.,
/// ```cpp
///   static bool is_my_hack_enabled=false;
///   hxconsole_variable_named(is_my_hack_enabled, f_hack); // add "f_hack" to the console.
/// ```
#define hxconsole_variable_named(x_, name_) static hxconsole_constructor_ \
	g_hxconsole_symbol_##name_(hxconsole_variable_factory_(&(x_)), #name_)

/// `hxconsole_deregister` - Explicitly deregisters a console symbol.
/// - `id` : Valid C identifier that identifies the variable.
void hxconsole_deregister(const char* id_) hxattr_nonnull(1);

/// `hxconsole_exec_line` - Evaluates a console command to either call a function
/// or set a variable. e.g., `srand 77` or `a_variable 5`.
/// - `command` : A string executed by the console.
bool hxconsole_exec_line(const char* command_) hxattr_nonnull(1);

/// `hxconsole_exec_file` - Executes a configuration file that is opened for
/// reading. Ignores blank lines and comments that start with `#`.
/// - `file` : A file containing commands.
bool hxconsole_exec_file(hxfile& file_);

/// `hxconsole_exec_filename` - Opens a configuration file by name and executes it.
/// - `filename` : A file containing commands.
bool hxconsole_exec_filename(const char* filename_) hxattr_nonnull(1);

/// `hxconsole_help` - Logs every console symbol to the console log.
bool hxconsole_help(void);

// Include internals after hxconsolehex_t
#include "detail/hxconsole_detail.hpp"
