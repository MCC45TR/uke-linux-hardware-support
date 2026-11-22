// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <climits>
#include <cwchar>
#include <cwctype>
#include <string>
#include <string_view>
#include <vector>

namespace uke::tui_text {
inline std::wstring wide(std::string_view input) {
    std::wstring result;mbstate_t state{};
    for(size_t at=0;at<input.size();) {
        wchar_t character{};const auto n=std::mbrtowc(&character,input.data()+at,input.size()-at,&state);
        if(n==size_t(-1)||n==size_t(-2)){result+=L'\ufffd';++at;state={};continue;}
        at+=n?n:1;
        if(character==L'\n'){result+=character;continue;}
        if(character==L'\t'){result+=L"    ";continue;}
        if(character<32||character==127||(character>=0x80&&character<0xa0)||(character>=0x202a&&character<=0x202e)||(character>=0x2066&&character<=0x2069)) {
            wchar_t escaped[16]{};std::swprintf(escaped,16,L"\\u%04x",unsigned(character));result+=escaped;
        } else result+=character;
    }
    return result;
}
inline std::string utf8(const std::wstring& input) {
    std::string result;mbstate_t state{};
    for(wchar_t character:input){char bytes[MB_LEN_MAX]{};const auto n=std::wcrtomb(bytes,character,&state);if(n==size_t(-1)){result+="?";state={};}else result.append(bytes,n);}
    return result;
}
inline int cells(std::wstring_view text){int result=0;for(wchar_t c:text)result+=std::max(0,wcwidth(c));return result;}
inline std::wstring clipped(std::wstring_view text,int maximum) {
    std::wstring result;int width=0;
    for(wchar_t c:text){if(c==L'\n')break;const auto n=std::max(0,wcwidth(c));if(width+n>maximum)break;result+=c;width+=n;}
    return result;
}
inline std::vector<std::wstring> wrap(std::string_view text,int maximum,size_t bound=8192) {
    maximum=std::max(1,maximum);const auto source=wide(text);std::vector<std::wstring> lines;
    size_t start=0;
    while(start<source.size()&&lines.size()<bound) {
        size_t end=start,space=std::wstring::npos;int width=0;
        while(end<source.size()&&source[end]!=L'\n') {
            const auto n=std::max(0,wcwidth(source[end]));if(width+n>maximum)break;
            width+=n;if(source[end]==L' ')space=end;++end;
        }
        if(end==start&&source[end]!=L'\n'){lines.push_back(L"?");++start;continue;}
        const bool newline=end<source.size()&&source[end]==L'\n';
        if(!newline&&end<source.size()&&space!=std::wstring::npos&&space>start)end=space;
        lines.push_back(source.substr(start,end-start));
        start=end;if(start<source.size()&&source[start]==L'\n')++start;else while(start<source.size()&&source[start]==L' ')++start;
    }
    if(start<source.size())lines.push_back(L"[Display limit / Görüntüleme sınırı]");
    if(lines.empty())lines.push_back(L"");
    return lines;
}
}
