// Copyright 2017 Adrian Johnston

#include "hxConsole.h"
#include "hxHashTable.h"
#include "hxFile.h"
#include "hxArray.h"

#include <algorithm>

HX_REGISTER_FILENAME_HASH;

// ----------------------------------------------------------------------------------
// Console commands

// Lists all variables and commands
hxConsoleCommandNamed(hxConsoleHelp, help);

// Executes commands and settings in file.  usage: "exec <filename>"
hxConsoleCommandNamed(hxConsoleExecFilename, exec);

// ----------------------------------------------------------------------------------
// hxConsoleHashTableNode

class hxConsoleHashTableNode : public hxHashTableNodeBase<const char*> {
public:
	typedef hxHashTableNodeBase<Key> Base;
	HX_INLINE hxConsoleHashTableNode(const char* key_, uint32_t hash) : Base(key_), m_cmd(0), m_hash(hash) {
		if ((HX_RELEASE) < 1) {
			const char* k = key;
			while (!hxIsDelimiter(*k)) {
				++k;
			}
			hxAssertMsg(*k == '\0', "console symbol contains delimiter: \"%s\"", key);
		}
	}
	HX_INLINE ~hxConsoleHashTableNode() { hxFree(m_cmd); }

	HX_INLINE uint32_t hash() const { return m_hash; }

	// Hashing stops at first non-printing character.
	HX_INLINE static uint32_t hash(const char*const& key) {
		const char* k = key;
		uint32_t x = (uint32_t)Base::c_hashMultiplier;
		while (!hxIsDelimiter(*k)) {
			x ^= (uint32_t)*k++;
			x *= (uint32_t)Base::c_hashMultiplier;
		}
		return x;
	}

	// Compare first whitespace delimited tokens.
	HX_INLINE static bool keyEqual(const hxConsoleHashTableNode& lhs, const char*const& rhs, uint32_t rhsHash) {
		if ((HX_RELEASE) < 1 && lhs.m_hash == rhsHash) {
			const char* a = lhs.key;
			const char* b = rhs;
			while (*a == *b && !hxIsDelimiter(*a)) { // hxIsDelimiter('\0') is true
				++a;
				++b;
			}
			hxAssertMsg(hxIsDelimiter(*a) && hxIsDelimiter(*b), "console symbol hash collision");
		}
		return lhs.m_hash == rhsHash;
	}

	hxCommand* m_cmd;
private:
	uint32_t m_hash;
};

typedef hxHashTable<hxConsoleHashTableNode, 6> hxCommandTable;

// Wrapped to ensure correct construction order.
static hxCommandTable& hxConsoleCommands() {
	static hxCommandTable tbl;
	return tbl;
}

// ----------------------------------------------------------------------------------
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
		hxWarn("Command not found: %s", command);
		return false;
	}

	// Skip command name
	while (!hxIsDelimiter(*pos)) {
		++pos;
	}

	bool result = node->m_cmd->execute(pos); // The hxArgs skip leading whitespace.
	hxWarnCheck(result, "Cannot execute: %s", command);
	return result;
}

bool hxConsoleExecFile(hxFile& file) {
	char buf[HX_MAX_LINE];
	bool result = true;
	while (file.getline(buf)) {
		hxLog("CONSOLE: %s", buf);
		bool rv = hxConsoleExecLine(buf);
		result = result && rv; // Don't stop on errors.
	}
	return result;
}

void hxConsoleExecFilename(const char* filename) {
	hxFile file(hxFile::in, "%s", filename);
	hxWarnCheck(file.is_open(), "Cannot open: %s", filename);
	if (file.is_open()) {
		bool isOk = hxConsoleExecFile(file);
		hxWarnCheck(isOk, "Cannot execute: %s", filename); (void)isOk;
	}
}

struct hxConsoleLT {
	HX_INLINE bool operator()(const hxConsoleHashTableNode* lhs, const hxConsoleHashTableNode* rhs) const {
		return ::strcmp(lhs->key, rhs->key) < 0;
	}
};

// List commands in sorted order.
void hxConsoleHelp() {
	if ((HX_RELEASE) < 2) {
		hxMemoryManagerScope allocator(hxMemoryManagerId_Heap);

		hxArray<const hxConsoleHashTableNode*> cmds;
		cmds.reserve(hxConsoleCommands().size());
		for (hxCommandTable::const_iterator it = hxConsoleCommands().cbegin(); it != hxConsoleCommands().cend(); ++it) {
			if (::strncmp(it->key, "hxConsoleTest", 13) == 0 || ::strncmp(it->key, "s_hxConsoleTest", 15) == 0) {
				continue;
			}
			cmds.push_back(&*it);
		}

		std::sort(cmds.begin(), cmds.end(), hxConsoleLT());

		hxLogRelease("CONSOLE_SYMBOLS:\n");
		for (hxArray<const hxConsoleHashTableNode*>::iterator it = cmds.begin(); it != cmds.end(); ++it) {
			(*it)->m_cmd->log((*it)->key);
		}
		hxLogRelease("--------\n");
	}
}
