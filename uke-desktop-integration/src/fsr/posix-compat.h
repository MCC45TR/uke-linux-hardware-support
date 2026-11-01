// SPDX-License-Identifier: MIT
// SDK CRT compatibility; adapted from ychwch/FidelityFX-SDK-Linux (MIT).
#pragma once
#include <bit>
#include <cmath>
#include <locale>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <mutex>
#include <iterator>

#ifndef _WIN32
#define _countof(value) std::size(value)
inline int wcscpy_s(wchar_t* output, size_t capacity, const wchar_t* input)
{
    if (!output || !input || !capacity) return EINVAL;
    if (std::wcslen(input) >= capacity) { output[0] = L'\0'; return ERANGE; }
    std::wcscpy(output, input);
    return 0;
}
template<size_t N> int wcscpy_s(wchar_t (&output)[N], const wchar_t* input)
{ return wcscpy_s(output, N, input); }
inline int strcpy_s(char* output, size_t capacity, const char* input)
{
    if (!output || !input || !capacity) return EINVAL;
    if (std::strlen(input) >= capacity) { output[0] = '\0'; return ERANGE; }
    std::strcpy(output, input);
    return 0;
}
#define sprintf_s std::snprintf
#endif
