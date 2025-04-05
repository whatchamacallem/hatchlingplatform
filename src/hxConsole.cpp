// Copyright 2017-2025 Adrian Johnston

#include <hx/hxConsole.hpp>
#include <hx/hxFile.hpp>
#include <hx/hxHashTable.hpp>
#include <hx/hxSort.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxConsoleHashTableNode
//
// Compares command lines to static strings.  Hashing stops at first non-printing
// character.

namespace {

class hxConsoleHashTableNode : public hxHashTableNodeBase<const char*> {
public:
	typedef hxHashTableNodeBase<Key> Base;
	HX_INLINE hxConsoleHashTableNode(const char* key_, uint32_t hash_)
			: Base(key_), m_cmd(0), m_hash(hash_) {
		if ((HX_RELEASE) < 1) {
			const char* k = key;
			while (!hxIsDelimiter_(*k)) {
				++k;
			}
			hxAssertMsg(*k == '\0', "console symbol contains delimiter: \"%s\"", key);
		}
	}
	HX_INLINE ~hxConsoleHashTableNode() { hxFree(m_cmd); }

	HX_INLINE uint32_t hash() const {
		return m_hash;
	}

	// Hashing stops at first non-printing character.
	HX_INLINE static uint32_t hash(const char*const& key) {
		const char* k_ = key;
		uint32_t x_ = (uint32_t)0x811c9dc5; // FNV-1a string hashing.
		while (!hxIsDelimiter_(*k_)) {
			x_ ^= (uint32_t)*k_++;
			x_ *= (uint32_t)0x01000193;
		}
		return x_;
	}

	// Compare first whitespace delimited tokens.
	HX_INLINE static bool keyEqual(const hxConsoleHashTableNode& lhs, const char*const& rhs, uint32_t rhsHash) {
		(void)rhs;
		hxAssertMsg(lhs.hash() != rhsHash || ::strncmp(lhs.key, rhs, ::strlen(lhs.key)) == 0,
			"console symbol hash collision: %s %s", lhs.key, rhs);
		return lhs.hash() == rhsHash;
	}

	hxCommand_* m_cmd;
	uint32_t m_hash;
};

struct hxConsoleLess_ {
	HX_INLINE bool operator()(const hxConsoleHashTableNode* lhs,
			const hxConsoleHashTableNode* rhs) const {
		return ::strcasecmp(lhs->key, rhs->key) < 0;
	}
};

typedef hxHashTable<hxConsoleHashTableNode, 4> hxCommandTable_;

// Wrapped to ensure correct construction order.
hxCommandTable_& hxConsoleCommands_() { static hxCommandTable_ tbl; return tbl; }

} // namespace

// ----------------------------------------------------------------------------
// Console API

void hxConsoleRegister_(hxCommand_* fn, const char* id) {
	hxAssertMsg(fn && id, "hxConsoleRegister_ args");
	hxConsoleHashTableNode& node = hxConsoleCommands_().insertUnique(id, hxMemoryManagerId_Heap);
	hxAssertMsg(!node.m_cmd, "command already registered: %s", id);
	node.m_cmd = fn;
}

void hxConsoleDeregister(const char* id) {
	hxConsoleCommands_().erase(id);
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

	hxConsoleHashTableNode* node = hxConsoleCommands_().find(pos);
	if (!node) {
		hxWarn("command not found: %s", command);
		return false;
	}

	// Skip command name
	while (!hxIsDelimiter_(*pos)) {
		++pos;
	}

	bool result = node->m_cmd->execute_(pos); // skips leading whitespace.
	hxWarnCheck(result, "cannot execute_: %s", command);
	return result;
}

bool hxConsoleExecFile(hxFile& file) {
	char buf[HX_MAX_LINE];
	bool result = true;
	while (result && file.getline(buf)) {
		hxLog("console: %s", buf);
		result = hxConsoleExecLine(buf);
	}
	return result;
}

struct hxConsoleLess {
	HX_INLINE bool operator()(const hxConsoleHashTableNode* lhs,
			const hxConsoleHashTableNode* rhs) const {
		return lhs->m_hash < rhs->m_hash;
	}
};

void hxConsoleExecFilename(const char* filename) {
	hxFile file(hxFile::in, "%s", filename);
	hxWarnCheck(file.is_open(), "cannot open: %s", filename);
	if (file.is_open()) {
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

		hxArray<const hxConsoleHashTableNode*> cmds;
		cmds.reserve(hxConsoleCommands_().size());
		for (hxCommandTable_::constIterator it = hxConsoleCommands_().cBegin();
				it != hxConsoleCommands_().cEnd(); ++it) {
			if (::strncmp(it->key, "hxConsoleTest", 13) == 0 ||
					::strncmp(it->key, "s_hxConsoleTest", 15) == 0) {
				continue;
			}
			cmds.pushBack(&*it);
		}

		hxInsertionSort<const hxConsoleHashTableNode*, hxConsoleLess_>(cmds.begin(), cmds.end(), hxConsoleLess_());

		for (hxArray<const hxConsoleHashTableNode*>::iterator it = cmds.begin();
				it != cmds.end(); ++it) {
			(*it)->m_cmd->usage_((*it)->key);
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
