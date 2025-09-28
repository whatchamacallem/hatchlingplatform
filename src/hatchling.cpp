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

// HX_FLOATING_POINT_TRAPS - Traps (FE_DIVBYZERO|FE_INVALID|FE_OVERFLOW) in
// debug so you can safely run without checks for them in release. Use
// -DHX_FLOATING_POINT_TRAPS=0 to disable this debug facility. There is no C++
// standard conforming way to disable floating point error checking. That
// requires a gcc/clang extension. Using -fno-math-errno and -fno-trapping-math
// will work if you require C++ conforming accuracy without the overhead of
// error checking. -ffast-math includes both those switches. You need the math
// library -lm. Causing or explicit checking for floating point exceptions is
// not recommended.
#if (HX_RELEASE) == 0 && defined __GLIBC__ && !defined __FAST_MATH__
#include <fenv.h>
#if !defined HX_FLOATING_POINT_TRAPS
#define HX_FLOATING_POINT_TRAPS 1
#endif
#else
#undef HX_FLOATING_POINT_TRAPS
#define HX_FLOATING_POINT_TRAPS 0
#endif

// There are exception handling semantics in use in case they are on. However
// you are advised to use -fno-exceptions. This library does not provide the
// exception handling functions expected by the C++ ABI. In this codebase memory
// allocation cannot fail. And it tries to get you to allocate enough memory for
// everything in advance. The creation of hxthread.h classes cannot fail. By
// design there are no exceptions to handle... Although there are lots of
// asserts.
#if (HX_RELEASE) >= 1 && defined __cpp_exceptions && !defined __INTELLISENSE__
static_assert(0, "warning: C++ exceptions are not recommended for embedded use.");
#endif

// No reason for this to be visible.
void hxsettings_construct();

// ----------------------------------------------------------------------------
// When not hosted, provide no locking around the initialization of function
// scope static variables. Provide a release mode assert to enforce that locking
// is not required. Also provide a release mode assert to enforce that function
// static constructors do not throw exceptions. Also provides error handling for
// virtual tables.

extern "C" {

#if HX_NO_LIBCXX

int __cxa_guard_acquire(size_t *guard) {
	// Return 0 if already constructed.
	if(*guard == 1u) { return 0; }

	// Function scope statics must be initialized before calling worker threads.
	// Checks if the constructor is already in progress and flags any potential
	// race condition or reentrance.
	hxassertrelease(*guard != 2u, "__cxa_guard_acquire no function scope static lock");
	*guard = 2u;
	return 1; // Signal construction required.
}

void __cxa_guard_release(size_t *guard) {
	// Flag constructor as done. Clear in progress flag.
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
// Hook clang's sanitizers in the debugger. This overrides a weak library symbol
// in the sanitizer support library. This provides clickable error messages
// in vscode. Unused otherwise.

void __sanitizer_report_error_summary(const char *error_summary) {
	// A clickable message has already been printed to standard out.
	hxbreakpoint(); (void)error_summary;
}

} // extern "C"

// ----------------------------------------------------------------------------
#if (HX_RELEASE) < 1

// Implements HX_REGISTER_FILENAME_HASH in debug. See hxstring_literal_hash.h.
namespace hxdetail_ {

class hxhash_string_literal_
		: public hxhash_table<hxregister_string_literal_hash, 5, hxdo_not_delete> { };

class hxfilename_less {
public:
	bool operator()(const char* lhs, const char* rhs) const {
		return hxstring_literal_hash_debug(lhs) < hxstring_literal_hash_debug(rhs);
	}
};

hxhash_string_literal_& hxstring_literal_hashes_(void) {
	static hxhash_string_literal_ s_hxstring_literal_hashes_;
	return s_hxstring_literal_hashes_;
}

} // hxdetail_ {

// The key for the table is a string hash.
hxregister_string_literal_hash::hxregister_string_literal_hash(const char* str_)
		: m_hash_next_(0), m_hash_(hxstring_literal_hash_debug(str_)), m_str_(str_) {
	hxdetail_::hxstring_literal_hashes_().insert_node(this);
}

// The hash table code expects to be able to hash a `key_t` and compare it equal
// to the `node_t`'s hash value. That results in double hashing here. It is just
// another multiply.
hxhash_t hxregister_string_literal_hash::hash(void) const {
	return hxkey_hash(m_hash_);
};

bool hxprint_hashes(void) {
	hxinit();

	// sort by hash.
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

bool hxcheck_hash(hxconsolehex_t hash_) {
	hxregister_string_literal_hash* node = hxstring_literal_hashes_().find(hash_);
	if(node) {
		while(node) {
			hxlogconsole("%08zx: %s\n", (size_t)hash_, node->str());
			node = hxstring_literal_hashes_().find(hash_, node);
		}
	}
	else {
		hxlogconsole("%08zx: not found\n", (size_t)hash_);
	}
	return true;
}

// use the debug console to emit and check file hashes.
hxconsole_command_named(hxprint_hashes, printhashes);
hxconsole_command_named(hxcheck_hash, checkhash);

#endif

// ----------------------------------------------------------------------------
// init, shutdown, exit, assert and logging.

extern "C"
void hxinit_internal(void) {
	hxassertrelease(!g_hxisinit, "not_init");
	hxsettings_construct();
	g_hxisinit = true;

#if HX_FLOATING_POINT_TRAPS
	// You need the math library -lm. This is nonstandard glibc/_GNU_SOURCE.
	::feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#endif

	hxmemory_manager_init();
}

extern "C"
hxnoexcept_unchecked void hxloghandler(hxloglevel_t level, const char* format, ...) {
	va_list args;
	va_start(args, format);
	hxloghandler_v(level, format, args);
	va_end(args);
}

extern "C"
hxnoexcept_unchecked void hxloghandler_v(hxloglevel_t level, const char* format, va_list args) {
	if(g_hxisinit && g_hxsettings.log_level > level) {
		return;
	}

	// vsnprintf leaves a trailing NUL which may be overwritten below.
	char buf[HX_MAX_LINE];
	int len = ::vsnprintf(buf, HX_MAX_LINE, format, args);


	// Don't try and print the format string as it may be bad.
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
	f.write(buf, len);
}

// HX_RELEASE < 3 facilities for testing tear down. Just call _Exit() otherwise.
extern "C"
void hxshutdown(void) {
	if(g_hxisinit) {
#if (HX_RELEASE) < 3
		// Will trap further activity and leaks.
		hxmemory_manager_shut_down();
#endif
	}
}

#if (HX_RELEASE) == 0
extern "C"
hxnoexcept_unchecked bool hxasserthandler(const char* file, size_t line) {
	const char* f = hxbasename(file);
	if(g_hxisinit && g_hxsettings.asserts_to_be_skipped > 0) {
		--g_hxsettings.asserts_to_be_skipped;
		hxloghandler(hxloglevel_assert, "skipped %s(%zu) hash %08zx",
			f, line, (size_t)hxstring_literal_hash_debug(file));
		return 1;
	}
	hxloghandler(hxloglevel_assert, "breakpoint %s(%zu) hash %08zx\n",
		f, line, (size_t)hxstring_literal_hash_debug(file));

	// return to hxbreakpoint at calling line.
	return 0;
}
#else
extern "C"
hxnoexcept_unchecked hxattr_noreturn void hxasserthandler(hxhash_t file, size_t line) {
	hxloghandler(hxloglevel_assert, "exit file %08zx line %zu\n", (size_t)file, line);
	_Exit(EXIT_FAILURE);
}
#endif
