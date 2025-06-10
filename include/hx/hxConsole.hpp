#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/internal/hxConsoleInternal.hpp>

class hxFile;

// hxConsole API - Implements a simple console for remote use or to implement
// configuration files. Output is directed to the system log with
// hxLogLevel_Console. A remote console will require forwarding commands to the
// target and reporting the system log back. Configuration files only require
// file I/O. All calls with up to 4 args which are fundamental types are
// supported. Setting variables of a fundamental type are also supported.
// const char* args will capture the remainder of the line including #'s.

// hxConsoleCommand - Registers a function using a global constructor. Use in a
// global scope. Command will have the same name and args as the function. x_
// must be a valid C identifier that evaluates to a function pointer.
//   E.g. hxConsoleCommand(srand);
#define hxConsoleCommand(x_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,x_)(hxConsoleCommandFactory_(&(x_)), HX_QUOTE(x_))

// hxConsoleCommandNamed - Registers a named function using a global constructor.
// Use in a global scope. Provided name_ must be a valid C identifier. x_ may be
// any expression that evaluates to a function pointer.
//   E.g. hxConsoleCommandNamed(srand, seed_rand);
#define hxConsoleCommandNamed(x_, name_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,name_)(hxConsoleCommandFactory_(&(x_)), HX_QUOTE(name_))

// hxConsoleVariable - Registers a variable. Use in a global scope. Will have the
// same name as the variable.
//   E.g.
//   static bool isMyHackEnabled=false;
//   hxConsoleVariable(isMyHackEnabled);
#define hxConsoleVariable(x_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,x_)(hxConsoleVariableFactory_(&(x_)), HX_QUOTE(x_))

// hxConsoleVariableNamed - Registers a named variable. Use in a global scope.
// Provided name must be a valid C identifier.
//   E.g.
//   static bool isMyHackEnabled=false;
//   hxConsoleVariableNamed(isMyHackEnabled, f_hack); // add "f_hack" to the console.
#define hxConsoleVariableNamed(x_, name_) static hxConsoleConstructor_ \
    HX_CONCATENATE(g_hxConsoleSymbol_,name_)(hxConsoleVariableFactory_(&(x_)), HX_QUOTE(name_))

// hxConsoleIsOkResult - Determines whether a console function's return value is
// OK. Overload as needed. Overload must be consistent wherever your type is
// registered. A void return is separately handled as an OK result.
template<typename T>
bool hxConsoleIsOkResult(T t) { return (bool)t; }

// hxConsoleDeregister - Explicit de-registration of a console symbol.
void hxConsoleDeregister(const char* id_);

// hxConsoleExecLine - Evaluates a console command to either call a function or
// set a variable. E.g.: "srand 77" or "aVariable 5"
bool hxConsoleExecLine(const char* command_);

// hxConsoleExecFile - Executes a configuration file which is opened for reading.
// Ignores blank lines and comments starting with #.
bool hxConsoleExecFile(hxFile& file_);

// hxConsoleExecFilename - Opens a configuration file by name and executes it.
bool hxConsoleExecFilename(const char* filename_);

// hxConsoleHelp - Logs all console symbols to the console log.
void hxConsoleHelp();
