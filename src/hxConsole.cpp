// Copyright 2017-2019 Adrian Johnston

#include <hx/hxConsole.h>
#include <hx/hxFile.h>
#include <hx/hxHashTable.h>
#include <hx/hxSort.h>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxConsoleHashTableNode
//
// Compares command lines to static strings.  Hashing stops at first non-printing
// character.

class hxConsoleHashTableNode : public hxHashTableNodeBase<const char*> {
public:
	typedef hxHashTableNodeBase<Key> Base;
	HX_INLINE hxConsoleHashTableNode(const char* key_, uint32_t hash_)
			: Base(key_), m_cmd(0), m_hash(hash_) {
		if ((HX_RELEASE) < 1) {
			const char* k = key;
			while (!hxIsDelimiter(*k)) {
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
		while (!hxIsDelimiter(*k_)) {
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

	hxCommand* m_cmd;
	uint32_t m_hash;
};

typedef hxHashTable<hxConsoleHashTableNode, 4> hxCommandTable;

// Wrapped to ensure correct construction order.
static hxCommandTable& hxConsoleCommands() { static hxCommandTable tbl; return tbl; }

// ----------------------------------------------------------------------------
// Console API

void hxConsoleRegister(hxCommand* fn, const char* id) {
	hxAssertMsg(fn && id, "hxConsoleRegister args");
	hxConsoleHashTableNode& node = hxConsoleCommands().insert_unique(id, hxMemoryManagerId_Heap);
	hxAssertMsg(!node.m_cmd, "command already registered: %s", id);
	node.m_cmd = fn;
}

void hxConsoleDeregister(const char* id) {
	hxConsoleCommands().erase(id);
}

void hxConsoleDeregisterAll() {
	hxConsoleCommands().clear();
}

bool hxConsoleExecLine(const char* command) {
	// Skip leading whitespace
	const char* pos = command;
	while (*pos != '\0' && hxIsDelimiter(*pos)) {
		++pos;
	}

	// Skip comments and blank lines
	if (*pos == '\0' || *pos == '#') {
		return true;
	}

	hxConsoleHashTableNode* node = hxConsoleCommands().find(pos);
	if (!node) {
		hxWarn("command not found: %s", command);
		return false;
	}

	// Skip command name
	while (!hxIsDelimiter(*pos)) {
		++pos;
	}

	bool result = node->m_cmd->execute(pos); // The hxArgs skip leading whitespace.
	hxWarnCheck(result, "cannot execute: %s", command);
	return result;
}

bool hxConsoleExecFile(hxFile& file) {
	char buf[HX_MAX_LINE];
	bool result = true;
	while ((result || file.is_fallible()) && file.getline(buf)) {
		hxLog("console: %s", buf);
		result = hxConsoleExecLine(buf) && result;
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
	hxFile file(hxFile::in | hxFile::fallible, "%s", filename);
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
		cmds.reserve(hxConsoleCommands().size());
		for (hxCommandTable::const_iterator it = hxConsoleCommands().cbegin();
				it != hxConsoleCommands().cend(); ++it) {
			if (::strncmp(it->key, "hxConsoleTest", 13) == 0 ||
					::strncmp(it->key, "s_hxConsoleTest", 15) == 0) {
				continue;
			}
			cmds.push_back(&*it);
		}

		hxInsertionSort(cmds.begin(), cmds.end(), hxConsoleLess());

		for (hxArray<const hxConsoleHashTableNode*>::iterator it = cmds.begin();
				it != cmds.end(); ++it) {
			(*it)->m_cmd->usage((*it)->key);
		}
	}
}

#if (HX_RELEASE) < 2 && !HX_USE_WASM

HX_STATIC_ASSERT(sizeof(size_t) == 4 || HX_USE_64_BIT_TYPES, "console support for 64-bit size_t requires 64-bit types");

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

// Executes commands and settings in file.  usage: "exec <filename>"
hxConsoleCommandNamed(hxConsoleExecFilename, exec);
