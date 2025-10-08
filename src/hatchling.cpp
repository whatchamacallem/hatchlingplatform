// SPDX-FileCopyrightText: © 2017-2025 Adrian Johnston.
// SPDX-License-Identifier: MIT
// This file is licensed under the MIT license found in the LICENSE.md file.

#include "../include/hx/hatchling.h"
#include "../include/hx/hxarray.hpp"
#include "../include/hx/hxconsole.hpp"
#include "../include/hx/hxfile.hpp"
#include "../include/hx/hxhash_table_nodes.hpp"
#include "../include/hx/hxprofiler.hpp"
#include "../include/hx/hxsort.hpp"

#include <stdio.h> // vsnprintf only.

HX_REGISTER_FILENAME_HASH

// g_hxinit_ver_ should not be explicitly zero-initialized; MSVC handles that
// differently.
extern "C" {
	// If non-zero the platform has been initialized without being shut down.
	// See `#define g_hxinit_ver_` in hxsettings.h for rationale.
	int g_hxinit_ver_; // Static initialize to 0.
}

// HX_FLOATING_POINT_TRAPS - Traps (FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW) in
// debug builds so you can safely run without checks in release builds. Use
// -DHX_FLOATING_POINT_TRAPS=0 to disable this debug facility. There is no C++
// standard-conforming way to disable floating point error checking; that
// requires a gcc/clang extension. Using -fno-math-errno and -fno-trapping-math
// will work if you require C++ conforming accuracy without the overhead of
// error checking. -ffast-math includes both of those switches. You need the math
// library -lm. Triggering or explicitly checking for floating point exceptions
// is not recommended.
#if (HX_RELEASE) == 0 && defined __GLIBC__ && !defined __FAST_MATH__
#include <fenv.h>
#if !defined HX_FLOATING_POINT_TRAPS
#define HX_FLOATING_POINT_TRAPS 1
#endif
#else
#undef HX_FLOATING_POINT_TRAPS
#define HX_FLOATING_POINT_TRAPS 0
#endif

// Exception-handling semantics exist in case they are enabled, but you are
// advised to use -fno-exceptions. This library does not provide the exception
// handling functions expected by the C++ ABI. In this codebase memory
// allocation cannot fail, and it encourages allocating enough memory in
// advance. The creation of hxthread.h classes cannot fail. By design there are
// no exceptions to handle, although there are many asserts.
#if (HX_RELEASE) >= 1 && defined __cpp_exceptions && !defined __INTELLISENSE__
static_assert(0, "Warning: C++ exceptions are not recommended for embedded use.");
#endif

// ----------------------------------------------------------------------------
// When not hosted, do not lock around the initialization of function-scope
// static variables. Provide release-mode asserts to enforce that locking is
// unnecessary and to ensure function-static constructors do not throw.
// Also provide error handling for virtual tables.

extern "C" {

void __sanitizer_report_error_summary(const char *error_summary);

#if HX_NO_LIBCXX

// Provide declarations even though they are part of the ABI.
int __cxa_guard_acquire(size_t *guard);
void __cxa_guard_release(size_t *guard);
void __cxa_guard_abort(uint64_t *guard);
void __cxa_deleted_virtual(void);
void __cxa_pure_virtual(void);

int __cxa_guard_acquire(size_t *guard) {
	// Return 0 if already constructed.
	if(*guard == 1u) { return 0; }

	// Function scope statics must be initialized before calling worker threads.
	// Checks whether the constructor is already in progress and flags any
	// potential race condition or reentrancy.
	hxassertrelease(*guard != 2u, "__cxa_guard_acquire no function scope static lock");
	*guard = 2u;
	return 1; // Signal construction required.
}

void __cxa_guard_release(size_t *guard) {
	// Marks the constructor as done and clears the in-progress flag.
	*guard = 1u;
}

void __cxa_guard_abort(uint64_t *guard) {
	hxassertrelease(0, "__cxa_guard_abort");
	*guard = 0u;
}

void __cxa_deleted_virtual(void) {
	hxassertrelease(0, "__cxa_deleted_virtual");
}

void __cxa_pure_virtual(void) {
	hxassertrelease(0, "__cxa_pure_virtual");
}

#endif

// ----------------------------------------------------------------------------
// Hooks clang's sanitizers into the debugger by overriding a weak library
// symbol in the sanitizer support library. This provides clickable error
// messages in VS Code and is otherwise unused.

void __sanitizer_report_error_summary(const char *error_summary) {
	// A clickable message has already been printed to standard output.
	hxbreakpoint(); (void)error_summary;
}

} // extern "C"

// ----------------------------------------------------------------------------
#if (HX_RELEASE) < 1

// Implements HX_REGISTER_FILENAME_HASH in debug builds. See hxstring_literal_hash.h.
namespace {

class hxhash_string_literal_
		: public hxhash_table<hxregister_string_literal_hash, 5, hxdo_not_delete> { };

class hxfilename_less {
public:
	bool operator()(const char* a, const char* b) const {
		return hxstring_literal_hash_debug(a) < hxstring_literal_hash_debug(b);
	}
};

hxhash_string_literal_& hxstring_literal_hashes_(void) {
	static hxhash_string_literal_ s_hxstring_literal_hashes_;
	return s_hxstring_literal_hashes_;
}

} // namespace {

// The key for the table is a string hash.
hxregister_string_literal_hash::hxregister_string_literal_hash(const char* str_)
		: m_hash_next_(0), m_hash_(hxstring_literal_hash_debug(str_)), m_str_(str_) {
	hxstring_literal_hashes_().insert_node(this);
}

// The hash table code expects to be able to hash a `key_t` and compare it equal
// to the `node_t`'s hash value. That results in double hashing here. It is just
// another multiply.
hxhash_t hxregister_string_literal_hash::hash(void) const {
	return hxkey_hash(m_hash_);
};

static bool hxprint_hashes(void) {
	hxinit();

	// Sort by hash.
	hxlogconsole("string literals in hash order:\n");
	hxsystem_allocator_scope temporary_stack(hxsystem_allocator_temporary_stack);

	hxarray<const char*> filenames; filenames.reserve(hxstring_literal_hashes_().size());

	for(const auto& it : hxstring_literal_hashes_()) {
		filenames.push_back(it.str());
	}

	hxinsertion_sort(filenames.begin(), filenames.end(), hxfilename_less());

	for(const char* str : filenames) {
		hxlog("  %08zx %s\n", (size_t)hxstring_literal_hash_debug(str), str);
	}
	return true;
}

static bool hxcheck_hash(hxconsolehex_t hash_) {
	hxregister_string_literal_hash* node = hxstring_literal_hashes_().find(hash_);
	if(node) {
		while(node) {
			hxlogconsole("%08zx: %s\n", (size_t)hash_, node->str());
			node = hxstring_literal_hashes_().find(hash_, node);
		}
	}
	else {
		hxlogconsole("%08zx: not found\n", (size_t)hash_);
		return false;
	}
	return true;
}

// Use the debug console to emit and check file hashes.
hxconsole_command_named(hxprint_hashes, printhashes);
hxconsole_command_named(hxcheck_hash, checkhash);

#endif

// ----------------------------------------------------------------------------
// Initialization, shutdown, exit, assert, and logging.

extern "C"
void hxinit_internal(int version_) {
	// Check if compiled in expected_version matches callers.
	const long expected_version = HATCHLING_VER;
	hxassertrelease(expected_version == version_, "HATCHLING_VER mismatch.");
	hxassertrelease(!g_hxinit_ver_ || g_hxinit_ver_ == version_, "HATCHLING_VER mismatch.");
	(void)version_; (void)expected_version;

	if(!g_hxinit_ver_) {
		hxsettings_construct();

#if HX_FLOATING_POINT_TRAPS
		// You need the math library -lm. This is a nonstandard glibc/_GNU_SOURCE extension.
		::feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

		hxmemory_manager_init();
		g_hxinit_ver_ = HATCHLING_VER;
	}
}

extern "C"
hxattr_noexcept void hxloghandler(hxloglevel_t level, const char* format, ...) {
	va_list args;
	va_start(args, format);
	hxloghandler_v(level, format, args);
	va_end(args);
}

extern "C"
hxattr_noexcept void hxloghandler_v(hxloglevel_t level, const char* format, va_list args) {
	if(g_hxinit_ver_ && g_hxsettings.log_level > level) {
		return;
	}

	// vsnprintf leaves a trailing NUL that may be overwritten below.
	char buf[HX_MAX_LINE];
	int len = ::vsnprintf(buf, HX_MAX_LINE, format, args);

	// Do not try to print the format string because it may be invalid.
	hxassertrelease(len >= 0 && len < (int)HX_MAX_LINE, "vsnprintf");

	hxfile& f = level == hxloglevel_log ? hxout : hxerr;
	if(level == hxloglevel_warning) {
		f << "WARNING ";
		buf[len++] = '\n';
	}
	else if(level == hxloglevel_assert) {
		f.write("ASSERT_FAIL ", (sizeof "ASSERT_FAIL ") - 1u);
		buf[len++] = '\n';
	}
	f.write(buf, (size_t)len);
}

// HX_RELEASE < 3 provides facilities for testing teardown. Just call _Exit() otherwise.
extern "C"
void hxshutdown(void) {
	if(g_hxinit_ver_) {
#if (HX_RELEASE) < 3
		hxmemory_manager_shut_down();
		// Try to trap further activity. This breaks global destructors that call
		// hxfree. There is no easier way to track leaks.
#endif
		g_hxinit_ver_ = false;
	}
}

#if (HX_RELEASE) == 0
extern "C"
hxattr_noexcept bool hxasserthandler(const char* file, size_t line) {
	const char* f = hxbasename(file);
	if(g_hxinit_ver_ && g_hxsettings.asserts_to_be_skipped > 0) {
		--g_hxsettings.asserts_to_be_skipped;
		hxloghandler(hxloglevel_assert, "skipped %s(%zu) hash %08zx",
			f, line, (size_t)hxstring_literal_hash_debug(file));
		return 1;
	}
	hxloghandler(hxloglevel_assert, "breakpoint %s(%zu) hash %08zx\n",
		f, line, (size_t)hxstring_literal_hash_debug(file));

	// Return to hxbreakpoint at the calling line.
	return 0;
}
#else
extern "C"
hxattr_noexcept hxattr_noreturn void hxasserthandler(hxhash_t file, size_t line) {
	hxloghandler(hxloglevel_assert, "exit file %08zx line %zu\n", (size_t)file, line);
	_Exit(EXIT_FAILURE);
}
#endif
