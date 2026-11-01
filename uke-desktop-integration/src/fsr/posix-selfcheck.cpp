// SPDX-License-Identifier: MIT
#include "posix-compat.h"
#include <cassert>

int main()
{
    char text[3] = {};
    wchar_t wide[3] = {};
    assert(strcpy_s(text, 3, "ab") == 0 && std::strcmp(text, "ab") == 0);
    assert(strcpy_s(text, 3, "abc") == ERANGE && text[0] == '\0');
    assert(strcpy_s(nullptr, 3, "a") == EINVAL);
    assert(strcpy_s(text, 0, "a") == EINVAL);
    assert(wcscpy_s(wide, L"ab") == 0 && std::wcscmp(wide, L"ab") == 0);
    assert(wcscpy_s(wide, L"abc") == ERANGE && wide[0] == L'\0');
    assert(wcscpy_s(nullptr, 3, L"a") == EINVAL);
    assert(wcscpy_s(wide, 0, L"a") == EINVAL);
}
