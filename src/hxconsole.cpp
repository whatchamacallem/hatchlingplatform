// Copyright 2017-2025 Adrian Johnston

#include <hx/hxconsole.hpp>
#include <hx/hxfile.hpp>
#include <hx/hxhash_table.hpp>
#include <hx/hxsort.hpp>
#include <hx/hxarray.hpp>

HX_REGISTER_FILENAME_HASH

// ----------------------------------------------------------------------------
// hxconsole_command_table_
//
// Compares command lines to static strings. Hashing stops at first non-printing
// character on command line.

namespace hxdetail_ {

class hxconsole_less_ {
public:
	inline bool operator()(const hxconsole_hash_table_node_* lhs,
			const hxconsole_hash_table_node_* rhs) const {
		return hxkey_less(lhs->key().str_, rhs->key().str_);
	}
};

class hxconsole_command_table_
	: public hxhash_table<hxconsole_hash_table_node_, 2, hxdo_not_delete> {
};

// Wrapped to enforce a construction order dependency. Modification of the table
// is not thread safe and it is normally constructed before main.
hxconsole_command_table_& hxconsole_commands_(void) {
	static hxconsole_command_table_ table_;
	return table_;
}

// ----------------------------------------------------------------------------
// Console API

void hxconsole_register_(hxconsole_hash_table_node_* node) {
	hxassertmsg(node->key().str_ && node->command_(), "invalid_parameter");
	hxassertmsg(!hxconsole_commands_().find(node->key()), "command_reregistered %s", node->key().str_);

	hxconsole_commands_().insert_node(node);
}

} // hxdetail_
using namespace hxdetail_;

// Nodes are statically allocated. Do not delete.
void hxconsole_deregister(const char* id) {
	hxconsole_commands_().release_key(hxconsole_hash_table_key_(id));
}

bool hxconsole_exec_line(const char* command) {
	// Skip leading whitespace
	const char* pos = command;
	while (*pos != '\0' && hxconsole_is_delimiter_(*pos)) {
		++pos;
	}

	// Skip comments and blank lines
	if (hxconsole_is_end_of_line_(pos)) {
		return true;
	}

	const hxconsole_hash_table_node_* node = hxconsole_commands_().find(hxconsole_hash_table_key_(pos));
	if (!node) {
		hxwarnmsg(0, "unknown_command %s", command);
		return false;
	}

	// Skip command name
	while (!hxconsole_is_delimiter_(*pos)) {
		++pos;
	}

#ifdef __cpp_exceptions
	try
#endif
	{
		bool result = node->command_()->execute_(pos);
		hxwarnmsg(result, "command_failed %s", command);
		return result;
	}
#ifdef __cpp_exceptions
	catch (...) {
		hxwarnmsg(0, "unexpected_exception %s", command);
		return false;
	}
#endif
}

bool hxconsole_exec_file(hxfile& file) {
	char buf[HX_MAX_LINE];
	bool result = true;
	while (result && file.get_line(buf)) {
		result = hxconsole_exec_line(buf);
	}
	return result;
}

bool hxconsole_exec_filename(const char* filename) {
	// Please don't assert.
	hxfile file(hxfile::in|hxfile::failable, "%s", filename);
	hxwarnmsg(file.is_open(), "cannot open: %s", filename);
	if (file.is_open()) {
		bool is_ok = hxconsole_exec_file(file);
		hxwarnmsg(is_ok, "encountering errors: %s", filename);
		return is_ok;
	}
	return false;
}

// ----------------------------------------------------------------------------
// Built-in console commands

// Lists variables and commands in order.
bool hxconsole_help(void) {
	if ((HX_RELEASE) < 2) {
		hxinit();
		hxsystem_allocator_scope temporary_stack(hxsystem_allocator_temporary_stack);
		hxarray<const hxconsole_hash_table_node_*> cmds;
		cmds.reserve(hxconsole_commands_().size());
		for (hxconsole_command_table_::const_iterator it = hxconsole_commands_().cbegin();
				it != hxconsole_commands_().cend(); ++it) {
			if (::strncmp(it->key().str_, "hxconsole_test", 13) == 0 ||
					::strncmp(it->key().str_, "s_hxconsole_test", 15) == 0) {
				continue;
			}
			cmds.push_back(&*it);
		}

		hxinsertion_sort<const hxconsole_hash_table_node_*, hxconsole_less_>(cmds.begin(), cmds.end(), hxconsole_less_());

		for (hxarray<const hxconsole_hash_table_node_*>::iterator it = cmds.begin();
				it != cmds.end(); ++it) {
			(*it)->command_()->usage_((*it)->key().str_);
		}
	}
	return true;
}

#if (HX_RELEASE) < 2 && !defined __EMSCRIPTEN__

static bool hxconsole_peek(hxconsolehex_t address, hxconsolenumber_t bytes) {
	hxhex_dump((const void*)address, bytes, 0);
	return true;
}

// Writes bytes from hex value in little endian format (LSB first). hex value is
// repeated every 8 bytes/64-bits in memory. hex is also 64-bit.
static bool hxconsole_poke(hxconsolehex_t address_, hxconsolenumber_t bytes_, hxconsolehex_t hex_) {
	volatile uint8_t* address = address_;
	uint32_t bytes = bytes_;
	uint64_t hex = hex_;
	while (bytes--) {
		*address++ = (uint8_t)hex;
		hex = (hex >> 8) | (hex << 56);
	}
	return true;
}

static bool hxconsole_hex_dump(hxconsolehex_t address, hxconsolenumber_t bytes) {
	hxhex_dump((const void*)address, bytes, 1);
	return true;
}

static bool hxconsole_float_dump(hxconsolehex_t address, hxconsolenumber_t bytes) {
	hxfloat_dump((const float*)address, bytes);
	return true;
}

// List console commands and argument types.
hxconsole_command_named(hxconsole_help, help);

// Write bytes to console.
hxconsole_command_named(hxconsole_peek, peek);

// Write bytes to memory.
hxconsole_command_named(hxconsole_poke, poke);

// Write bytes to console with pretty formatting.
hxconsole_command_named(hxconsole_hex_dump, hexdump);

// Write floats to console.
hxconsole_command_named(hxconsole_float_dump, floatdump);
#endif

// Executes commands and settings in file. usage: "exec <filename>"
hxconsole_command_named(hxconsole_exec_filename, exec);
