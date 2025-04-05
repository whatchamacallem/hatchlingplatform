#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/internal/hxConsoleInternal.hpp>

class hxFile;

// ----------------------------------------------------------------------------
// hxConsole API
//
// Implements a simple console for remote use or to implement configuration
// files.  Output is directed to the system log with hxLogLevel_Console.  A
// remote console will require forwarding commands to the target and reporting
// the system log back.  Configuration files only require file I/O.  All calls
// with up to 4 args which are fundamental types are supported.  Setting
// variables of a fundamental type are also supported.  const char* args will
// capture the remainder of the line including #'s.

// Registers a function.  Use in a global scope.
//   E.g. hxConsoleCommand(srand);
#define hxConsoleCommand(x_) hxConsoleConstructor_ \
	HX_CONCATENATE(g_hxConsoleSymbol_,x_)(hxCommandFactory_(&(x_)), HX_QUOTE(x_))

// Registers a named function.  Use in a global scope.  Provided name must be a
// valid C identifier.
#define hxConsoleCommandNamed(x_, name_) hxConsoleConstructor_ \
	HX_CONCATENATE(g_hxConsoleSymbol_,name_)(hxCommandFactory_(&(x_)), HX_QUOTE(name_))

// Registers a variable.  Use in a global scope.
//   E.g. bool g_isEnabled=false; hxConsoleVariable(g_isEnabled);
#define hxConsoleVariable(x_) hxConsoleConstructor_ \
	HX_CONCATENATE(g_hxConsoleSymbol_,x_)(hxVariableFactory_(&(x_)), HX_QUOTE(x_))

// Registers a named variable.  Use in a global scope.  Provided name must be a
// valid C identifier.
#define hxConsoleVariableNamed(x_, name_) hxConsoleConstructor_ \
	HX_CONCATENATE(g_hxConsoleSymbol_,name_)(hxVariableFactory_(&(x_)), HX_QUOTE(name_))

// Explicit de-registration of a console symbol.
void hxConsoleDeregister(const char* id_);

// Explicit de-registration of all console symbols.
void hxConsoleDeregisterAll();

// Evaluates a console command to either call a function or set a variable.
//   E.g.: "srand 77" or "aVariable 5"
bool hxConsoleExecLine(const char* command_);

// Executes a configuration file which is opened for reading.  Ignores blank
// lines and comments starting with #.
bool hxConsoleExecFile(hxFile& file_);

// Opens a configuration file by name and executes it.
void hxConsoleExecFilename(const char* filename_);

// Logs all console symbols to the console log.
void hxConsoleHelp();
