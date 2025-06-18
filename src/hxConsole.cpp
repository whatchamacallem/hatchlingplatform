// Copyright 2017-2025 Adrian Johnston

#include <hx/hxConsole.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTable.hpp>
#include <hx/hxSort.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxConsoleCommandTable_
//
// Compares command lines to static strings. Hashing stops at first non-printing
// character on command line.

namespace {

struct hxConsoleLess_ {
	inline bool operator()(const hxConsoleHashTableNode_* lhs,
			const hxConsoleHashTableNode_* rhs) const {
		return hxKeyLess(lhs->key().str_, rhs->key().str_);
	}
};

struct hxConsoleCommandTable_ : public hxHashTable<hxConsoleHashTableNode_, 2> {
	// do not delete the nodes. they are statically allocated.
	~hxConsoleCommandTable_(void) {
		this->releaseAll();
	}
};

// Wrapped to ensure correct construction order.
hxConsoleCommandTable_& hxConsoleCommands_() { static hxConsoleCommandTable_ tbl; return tbl; }

} // namespace

// ----------------------------------------------------------------------------
// Console API

void hxConsoleRegister_(hxConsoleHashTableNode_* node) {
	hxAssertMsg(node->key().str_ && node->command_(), "hxConsoleRegister_ args");
	hxAssertMsg(!hxConsoleCommands_().find(node->key()), "command already registered: %s", node->key().str_);

	hxConsoleCommands_().insertNode(node);
}

// Nodes are statically allocated. Do not delete.
void hxConsoleDeregister(const char* id) {
	hxConsoleCommands_().releaseKey(hxConsoleHashTableKey_(id));
}

bool hxConsoleExecLine(const char* command) {
	// Skip leading whitespace
	const char* pos = command;
	while (*pos != '\0' && hxConsoleIsDelimiter_(*pos)) {
		++pos;
	}

	// Skip comments and blank lines
	if (hxConsoleIsEndOfline_(pos)) {
		return true;
	}

	const hxConsoleHashTableNode_* node = hxConsoleCommands_().find(hxConsoleHashTableKey_(pos));
	if (!node) {
		hxLogWarning("unknown command: %s", command);
		return false;
	}

	// Skip command name
	while (!hxConsoleIsDelimiter_(*pos)) {
		++pos;
	}

#ifdef __cpp_exceptions
	try
#endif
	{
		bool result = node->command_()->execute_(pos);
		hxWarnMsg(result, "command failed: %s", command);
		return result;
	}
#ifdef __cpp_exceptions
	catch (...) {
		hxLogWarning("unexpected exception: %s", command);
		return false;
	}
#endif
}

bool hxConsoleExecFile(hxFile& file) {
	char buf[HX_MAX_LINE];
	bool result = true;
	while (result && file.getLine(buf)) {
		result = hxConsoleExecLine(buf);
	}
	return result;
}

bool hxConsoleExecFilename(const char* filename) {
	hxFile file(hxFile::in, "%s", filename);
	hxWarnMsg(file.isOpen(), "cannot open: %s", filename);
	if (file.isOpen()) {
		bool isOk = hxConsoleExecFile(file);
		hxWarnMsg(isOk, "encountering errors: %s", filename);
		return isOk;
	}
	return false;
}

// ----------------------------------------------------------------------------
// Built-in console commands

// Lists variables and commands in order.
bool hxConsoleHelp() {
	if ((HX_RELEASE) < 2) {
		hxInit();
		hxMemoryAllocatorScope temporaryStack(hxMemoryAllocator_TemporaryStack);
		hxArray<const hxConsoleHashTableNode_*> cmds;
		cmds.reserve(hxConsoleCommands_().size());
		for (hxConsoleCommandTable_::constIterator it = hxConsoleCommands_().cBegin();
				it != hxConsoleCommands_().cEnd(); ++it) {
			if (::strncmp(it->key().str_, "hxConsoleTest", 13) == 0 ||
					::strncmp(it->key().str_, "s_hxConsoleTest", 15) == 0) {
				continue;
			}
			cmds.pushBack(&*it);
		}

		hxInsertionSort<const hxConsoleHashTableNode_*, hxConsoleLess_>(cmds.begin(), cmds.end(), hxConsoleLess_());

		for (hxArray<const hxConsoleHashTableNode_*>::iterator it = cmds.begin();
				it != cmds.end(); ++it) {
			(*it)->command_()->usage_((*it)->key().str_);
		}
	}
	return true;
}

#if (HX_RELEASE) < 2 && !defined __EMSCRIPTEN__

static bool hxConsolePeek(hxconsolehex_t address, hxconsolenumber_t bytes) {
	hxHexDump((const void*)address, bytes, 0);
	return true;
}

// Writes bytes from hex value in little endian format (LSB first). hex value is
// repeated every 8 bytes/64-bits in memory. hex is also 64-bit.
static bool hxConsolePoke(hxconsolehex_t address_, hxconsolenumber_t bytes_, hxconsolehex_t hex_) {
	volatile uint8_t* address = address_;
	uint32_t bytes = bytes_;
	uint64_t hex = hex_;
	while (bytes--) {
		*address++ = (uint8_t)hex;
		hex = (hex >> 8) | (hex << 56);
	}
	return true;
}

static bool hxConsoleHexDump(hxconsolehex_t address, hxconsolenumber_t bytes) {
	hxHexDump((const void*)address, bytes, 1);
	return true;
}

static bool hxConsoleFloatDump(hxconsolehex_t address, hxconsolenumber_t bytes) {
	hxFloatDump((const float*)address, bytes);
	return true;
}

// List console commands and argument types.
hxConsoleCommandNamed(hxConsoleHelp, help);

// Write bytes to console.
hxConsoleCommandNamed(hxConsolePeek, peek);

// Write bytes to memory.
hxConsoleCommandNamed(hxConsolePoke, poke);

// Write bytes to console with pretty formatting.
hxConsoleCommandNamed(hxConsoleHexDump, hexdump);

// Write floats to console.
hxConsoleCommandNamed(hxConsoleFloatDump, floatdump);
#endif

// Executes commands and settings in file. usage: "exec <filename>"
hxConsoleCommandNamed(hxConsoleExecFilename, exec);
