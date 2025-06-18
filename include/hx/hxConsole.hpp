#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

class hxFile;

// hxConsole API - Implements a simple console for remote use or to implement
// configuration files. Output is directed to the system log with
// hxLogLevel_Console. A remote console will require forwarding commands to the
// target and reporting the system log back. Configuration files only require
// file I/O. C-style calls returning bool with up to 4 args using "const char*",
// hxconsolenumber_t or hxconsolehex_t as parameter types are required for the
// bindings to work.

// hxconsolenumber_t - A number. Uses double as an intermediate type. This
// reduces template bloat by limiting parameter types. This is the same type of
// generic number approach JavaScript uses. Always 64-bit.
class hxconsolenumber_t {
public:
    hxconsolenumber_t(void) : m_x_(0.0) { }
    template<typename T_> hxconsolenumber_t(T_ x_) : m_x_((double)x_) { }
	template<typename T_> operator T_() const {
        T_ t = (T_)m_x_;
        hxWarnMsg((double)t == m_x_, "precision error: %lf -> %lf", m_x_, (double)t);
        return t;
    }

private:
    // ERROR - Numbers are not pointers or references.
    template<typename T_> operator T_*() const HX_DELETE_FN;

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
        hxWarnMsg((uint64_t)t == m_x_, "precision error: %llx -> %llx",
            (unsigned long long)m_x_, (unsigned long long)t);
        return t;
    }

private:
    HX_STATIC_ASSERT(sizeof(uint64_t) >= sizeof(uintptr_t), "128-bit pointers?");
	uint64_t m_x_;
};

// hxConsoleCommand - Registers a function using a global constructor. Use in a
// global scope. Command will have the same name and args as the function.
// - x: Valid C identifier that evaluates to a function pointer.
//   E.g. hxConsoleCommand(srand);
#define hxConsoleCommand(x_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,x_)(hxConsoleCommandFactory_(&(x_)), HX_QUOTE(x_))

// hxConsoleCommandNamed - Registers a named function using a global constructor.
// Use in a global scope. Provided name_ must be a valid C identifier.
// - x: Any expression that evaluates to a function pointer.
// - name: Valid C identifier that identifies the command.
//   E.g. hxConsoleCommandNamed(srand, seed_rand);
#define hxConsoleCommandNamed(x_, name_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,name_)(hxConsoleCommandFactory_(&(x_)), HX_QUOTE(name_))

// hxConsoleVariable - Registers a variable. Use in a global scope. Will have the
// same name as the variable.
// - x: Valid C identifier that evaluates to a variable.
//   E.g.
//   static bool isMyHackEnabled=false;
//   hxConsoleVariable(isMyHackEnabled);
#define hxConsoleVariable(x_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,x_)(hxConsoleVariableFactory_(&(x_)), HX_QUOTE(x_))

// hxConsoleVariableNamed - Registers a named variable. Use in a global scope.
// Provided name must be a valid C identifier.
// - x: Any expression that evaluates to a variable.
// - name: Valid C identifier that identifies the variable.
//   E.g.
//   static bool isMyHackEnabled=false;
//   hxConsoleVariableNamed(isMyHackEnabled, f_hack); // add "f_hack" to the console.
#define hxConsoleVariableNamed(x_, name_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,name_)(hxConsoleVariableFactory_(&(x_)), HX_QUOTE(name_))

// hxConsoleDeregister - Explicit de-registration of a console symbol.
// - id: Valid C identifier that identifies the variable.
void hxConsoleDeregister(const char* id_);

// hxConsoleExecLine - Evaluates a console command to either call a function or
// set a variable. E.g.: "srand 77" or "aVariable 5"
// - command: A string executed by the console.
bool hxConsoleExecLine(const char* command_);

// hxConsoleExecFile - Executes a configuration file which is opened for reading.
// Ignores blank lines and comments starting with #.
// - file: A file containing commands.
bool hxConsoleExecFile(hxFile& file_);

// hxConsoleExecFilename - Opens a configuration file by name and executes it.
// - filename: A file containing commands.
bool hxConsoleExecFilename(const char* filename_);

// hxConsoleHelp - Logs all console symbols to the console log.
bool hxConsoleHelp();

// Include internals after hxconsolehex_t
#include <hx/internal/hxConsoleInternal.hpp>
