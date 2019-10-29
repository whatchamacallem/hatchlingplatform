// \author (c) Marco Paland (info@paland.com)
//             2014-2018, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

// Modifications Copyright 2019 Adrian Johnston

#if !HATCHLING_VER
#error #include <hx/hatchling.h>
#endif

#if __cplusplus
extern "C" {
#endif

// Tiny snprintf/vsnprintf implementation.
//
//   Configuration: <hx/hxSettings.h>
//   Docs: https://en.cppreference.com/w/c/io/vfprintf
//   Source: https://github.com/mpaland/printf

int  snprintf_(char* buffer, size_t count, const char* format, ...);

int vsnprintf_(char* buffer, size_t count, const char* format, va_list va);

#if __cplusplus
}
#endif
