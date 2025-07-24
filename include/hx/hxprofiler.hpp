#pragma once
// Copyright 2017-2025 Adrian Johnston

#include <hx/hatchling.h>

#if defined __EMSCRIPTEN__
// from "emscripten/emscripten.h"
extern "C" double emscripten_get_now(void);
#elif defined __x86_64__ || defined __i386__
#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

/// `hxcycles_t` - Stores approx. 3 seconds to 300 years worth of processor cycles
/// starting from an unspecified origin and wrapping using unsigned rules. This is
/// intended for profiling, not calendaring. Used by the following include.
typedef size_t hxcycles_t;

#if HX_PROFILE
#include <hx/detail/hxprofiler_internal.hpp>
#define HX_PROFILE_ONLY_(x_) x_
#else
#define HX_PROFILE_ONLY_(x_) ((void)0)
#endif

/// `hxcycles_per_second` - Please customize for your processor speed. This assumes
/// 2.8ghz. These are really only used with printf which promotes everything to
/// double anyhow.
static const double hxcycles_per_second = 2.8e+9;
static const double hxmilliseconds_per_cycle = 1.0e+3 / hxcycles_per_second;
static const double hxmicroseconds_per_cycle = 1.0e+6 / hxcycles_per_second;
static const hxcycles_t hxdefault_cycles_cutoff = 1000;

/// `hxtime_sample_cycles()` - Set up the processor cycle counter for your
/// architecture. This is callable without enabling HX_PROFILE.
inline hxcycles_t hxtime_sample_cycles(void);

/// `hxprofile_scope(const char* label_string_literal)` - Declares an RAII-style
/// profiling sample. WARNING: A pointer to label_string_literal is kept.
/// Compiles to a NOP when not in use.
/// - `label_string_literal` : A string literal label for the sample.
#define hxprofile_scope(label_string_literal_) \
    HX_PROFILE_ONLY_(hxprofiler_scope_internal_<> \
        HX_APPEND_COUNTER_(hxprofile_scope_)(label_string_literal_))

/// `hxprofile_scope_min(const char* label_string_literal, hxcycles_t min_cycles)` -
/// Declares an RAII-style profiling sample with a minimum cycle cutoff. Compiles
/// to a NOP when not in use.
/// - `label_string_literal` : A string literal label for the sample.
/// - `min_cycles` : A minimum number of cycles required for a sample to be recorded.
#define hxprofile_scope_min(label_string_literal_, min_cycles_) \
    HX_PROFILE_ONLY_(hxprofiler_scope_internal_<min_cycles_> \
        HX_APPEND_COUNTER_(hxprofile_scope_)(label_string_literal_))

/// `hxprofiler_start()` - Clears samples and begins sampling. Compiles to a NOP when
/// not in use.
#define hxprofiler_start() HX_PROFILE_ONLY_(g_hxprofiler_.start_())

/// `hxprofiler_stop()` - Ends sampling. Does not clear samples. Compiles to a NOP
/// when not in use.
#define hxprofiler_stop() HX_PROFILE_ONLY_(g_hxprofiler_.stop_())

/// `hxprofiler_log()` - Stops sampling and writes samples to the system log.
/// Compiles to a NOP when not in use.
#define hxprofiler_log() HX_PROFILE_ONLY_(g_hxprofiler_.log_())

/// `hxprofiler_write_to_chrome_tracing(const char* filename)` - Stops sampling and
/// writes samples to the provided file. Writes profiling data in a format usable
/// by Chrome's chrome://tracing view. Usage: In Chrome go to "chrome://tracing/".
/// Load the generated json file. Use the W/A/S/D keys. See
/// http://www.chromium.org/developers/how-tos/trace-event-profiling-tool
/// Compiles to a NOP when not in use.
#define hxprofiler_write_to_chrome_tracing(filename_) \
    HX_PROFILE_ONLY_( g_hxprofiler_.write_to_chrome_tracing_(filename_) )
