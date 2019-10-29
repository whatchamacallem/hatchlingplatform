#pragma once
// Copyright 2017-2019 Adrian Johnston

#include <hx/internal/hxConsoleInternal.h>

class hxFile;

// ----------------------------------------------------------------------------
// hxConsole API
//
// Implements a simple console for remote use or to implement configuration files.
// Output is directed to the system log with hxLogLevel_Console.  A remote console
// will require forwarding commands to the target and reporting the system log back.
// Configuration files only require file I/O.  Case calls with up to 4 args which are
// fundamental types are supported.  Setting variables of a fundamental type are also
// supported.

// Registers a function.  Use in a global scope.
//   E.g. hxConsoleCommand(srand);
#define hxConsoleCommand(x) hxConsoleConstructor \
	HX_CONCATENATE(g_hxConsoleSymbol_,x)(hxCommandFactory(&(x)), HX_QUOTE(x))

// Registers a named function.  Use in a global scope.  Provided name must be a
// valid C identifier.
#define hxConsoleCommandNamed(x, name) hxConsoleConstructor \
	HX_CONCATENATE(g_hxConsoleSymbol_,name)(hxCommandFactory(&(x)), HX_QUOTE(name))

// Registers a variable.  Use in a global scope.
//   E.g. bool g_isEnabled=false; hxConsoleVariable(g_isEnabled);
#define hxConsoleVariable(x) hxConsoleConstructor \
	HX_CONCATENATE(g_hxConsoleSymbol_,x)(hxVariableFactory(&(x)), HX_QUOTE(x))

// Registers a named variable.  Use in a global scope.  Provided name must be a
// valid C identifier.
#define hxConsoleVariableNamed(x, name) hxConsoleConstructor \
	HX_CONCATENATE(g_hxConsoleSymbol_,name)(hxVariableFactory(&(x)), HX_QUOTE(name))

// Explicit de-registration of a console symbol.
void hxConsoleDeregister(const char* id);

// Explicit de-registration of all console symbols.
void hxConsoleDeregisterAll();

// Evaluates a console command which either calls a function or sets a variable.
//   E.g.: "srand 77" or "aVariable 5"
bool hxConsoleExecLine(const char* command);

// Executes a configuration file which is opened for reading.  Ignores blank
// lines and comments starting with #.
bool hxConsoleExecFile(hxFile& file);

// Opens a configuration file by name and executes it.
void hxConsoleExecFilename(const char* filename);

// Logs all console symbols to the console log.
void hxConsoleHelp();
