// Copyright 2017-2025 Adrian Johnston

#include <hx/hxConsole.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTable.hpp>
#include <hx/hxSort.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxConsoleHashTableNode_
//
// Compares command lines to static strings.  Hashing stops at first non-printing
// character on command line.

namespace {

// Wrap the string type because it does not behave normally.
struct hxConsoleHashTableKey_ {
	explicit hxConsoleHashTableKey_(const char* s_) : str_(s_) { }
	const char* str_;
};

// Uses FNV-1a string hashing.
uint32_t hxKeyHash(hxConsoleHashTableKey_ k_) {
	uint32_t x_ = (uint32_t)0x811c9dc5;
	while (!hxIsDelimiter_(*k_.str_)) {
		x_ ^= (uint32_t)*k_.str_++;
		x_ *= (uint32_t)0x01000193;
	}
	return x_;
}

uint32_t hxKeyEqual(hxConsoleHashTableKey_ lhs_, hxConsoleHashTableKey_ rhs_)
{
	size_t len = hxmin(::strlen(lhs_.str_), ::strlen(rhs_.str_));
	return ::strncmp(lhs_.str_, rhs_.str_, len) == 0;
};

class hxConsoleHashTableNode_ : public hxHashTableMapNode<hxConsoleHashTableKey_, hxCommand_*> {
public:
	typedef hxHashTableMapNode<hxConsoleHashTableKey_, hxCommand_*> Base;
	HX_INLINE hxConsoleHashTableNode_(hxConsoleHashTableKey_ key_)
			: Base(key_, hxnull) {
		if ((HX_RELEASE) < 1) {
			const char* k = key_.str_;
			while (!hxIsDelimiter_(*k)) {
				++k;
			}
			hxAssertMsg(*k == '\0', "console symbol contains delimiter: \"%s\"", key_.str_);
		}
	}

	HX_INLINE ~hxConsoleHashTableNode_() { hxFree(value()); }
};

struct hxConsoleLess_ {
	HX_INLINE bool operator()(const hxConsoleHashTableNode_* lhs,
			const hxConsoleHashTableNode_* rhs) const {
		return ::strcasecmp(lhs->key().str_, rhs->key().str_) < 0;
	}
};

typedef hxHashTable<hxConsoleHashTableNode_, 4> hxCommandTable_;

// Wrapped to ensure correct construction order.
hxCommandTable_& hxConsoleCommands_() { static hxCommandTable_ tbl; return tbl; }

} // namespace

// ----------------------------------------------------------------------------
// Console API

void hxConsoleRegister_(hxCommand_* fn, const char* id) {
	hxAssertMsg(fn && id, "hxConsoleRegister_ args");
	hxConsoleHashTableNode_& node
		= hxConsoleCommands_().insertUnique(hxConsoleHashTableKey_(id), hxMemoryManagerId_Heap);
	hxAssertMsg(!node.value(), "command already registered: %s", id);
	node.setValue(fn);
}

void hxConsoleDeregister(const char* id) {
	hxConsoleCommands_().erase(hxConsoleHashTableKey_(id));
}

void hxConsoleDeregisterAll() {
	hxConsoleCommands_().clear();
}

bool hxConsoleExecLine(const char* command) {
	// Skip leading whitespace
	const char* pos = command;
	while (*pos != '\0' && hxIsDelimiter_(*pos)) {
		++pos;
	}

	// Skip comments and blank lines
	if (*pos == '\0' || *pos == '#') {
		return true;
	}

	hxConsoleHashTableNode_* node = hxConsoleCommands_().find(hxConsoleHashTableKey_(pos));
	if (!node) {
		hxWarn("command not found: %s", command);
		return false;
	}

	// Skip command name
	while (!hxIsDelimiter_(*pos)) {
		++pos;
	}

	bool result = node->value()->execute_(pos); // skips leading whitespace.
	hxWarnCheck(result, "cannot execute_: %s", command);
	return result;
}

bool hxConsoleExecFile(hxFile& file) {
	char buf[HX_MAX_LINE];
	bool result = true;
	while (result && file.getLine(buf)) {
		hxLog("console: %s", buf);
		result = hxConsoleExecLine(buf);
	}
	return result;
}

void hxConsoleExecFilename(const char* filename) {
	hxFile file(hxFile::in, "%s", filename);
	hxWarnCheck(file.isOpen(), "cannot open: %s", filename);
	if (file.isOpen()) {
		bool isOk = hxConsoleExecFile(file);
		hxWarnCheck(isOk, "encountering errors: %s", filename); (void)isOk;
	}
}

// ----------------------------------------------------------------------------
// Built-in console commands

// Lists variables and commands in order.
void hxConsoleHelp() {
	if ((HX_RELEASE) < 2) {
		hxMemoryManagerScope heap(hxMemoryManagerId_Heap);

		hxArray<const hxConsoleHashTableNode_*> cmds;
		cmds.reserve(hxConsoleCommands_().size());
		for (hxCommandTable_::constIterator it = hxConsoleCommands_().cBegin();
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
			(*it)->value()->usage_((*it)->key().str_);
		}
	}
}

#if (HX_RELEASE) < 2 && !HX_USE_WASM

static void hxConsolePeek(size_t address, uint32_t bytes) {
	hxHexDump((const void*)address, bytes, 0);
}

static void hxConsolePoke(size_t address, uint8_t bytes, uint32_t littleEndianWord) {
	volatile uint8_t* t = (uint8_t*)address;
	while (bytes--) {
		*t++ = (uint8_t)littleEndianWord;
		littleEndianWord >>= 8;
	}
}

static void hxConsoleHexDump(size_t address, uint32_t bytes) {
	hxHexDump((const void*)address, bytes, 1);
}

// List console commands and argument types.
hxConsoleCommandNamed(hxConsoleHelp, help);

// Write bytes to console.
hxConsoleCommandNamed(hxConsolePeek, peek);

// Write bytes to memory.
hxConsoleCommandNamed(hxConsolePoke, poke);

// Write bytes to console with pretty formatting.
hxConsoleCommandNamed(hxConsoleHexDump, hex);
#endif

// Executes commands and settings in file.  usage_: "exec <filename>"
hxConsoleCommandNamed(hxConsoleExecFilename, exec);
