#include <afx.h>         // MFC core and standard components
#include <string>
#include <algorithm>
#include "StrUtil.h"
#include <shlobj.h>


#if defined(_MSC_VER) && defined(_DEBUG)
#define new DEBUG_NEW
#endif

using namespace std;

namespace utils {

bool WideStrToAnsiStr(std::string &str, const WCHAR * wstr)
{
    bool result = false;
    char *buff = NULL;

	str.clear();

    if( NULL != wstr )
    {
        int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
        if (size > 0)
        {
            buff = new char[size];
            if (NULL != buff)
            {
                if (0 < WideCharToMultiByte(CP_ACP, 0, wstr, -1, buff, size, NULL, NULL))
                {
                    str = buff;
                    result = TRUE;
                }
            }
        }
    }

    if (buff) delete []buff;
    return result;
}

std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
    std::wstring r(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, &r[0], len);
    return r;
}

std::string ws2s(const std::wstring& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, 0, 0, 0, 0); 
    std::string r(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, s.c_str(), slength, &r[0], len, 0, 0); 
    return r;
}

void StringReplace(string &strBase, const string &strSrc, const string &strDes)
{
    string::size_type pos = 0;
    string::size_type srcLen = strSrc.size();
    string::size_type desLen = strDes.size();
    pos = strBase.find(strSrc, pos); 
    while ((pos != string::npos)) {
        strBase.replace(pos, srcLen, strDes);
        pos=strBase.find(strSrc, (pos+desLen));
    }
}

void ReplaceCharInStr(std::string& str, char ch1, char ch2)
{
    std::replace(str.begin(), str.end(), ch1, ch2);
}

// "    too much\t   \tspace\t\t\t  " => "too much\t   \tspace"
std::string &TrimStr(std::string& str, const char *whitespace /*= " \t"*/)
{
    const size_t strBegin = str.find_first_not_of(whitespace);
    if (strBegin == std::string::npos) {
        str.clear(); // no content
    } else {
        const size_t strEnd = str.find_last_not_of(whitespace);
        const size_t strRange = strEnd - strBegin + 1;
        str = str.substr(strBegin, strRange);
    }
    return str;
}

// "    too much\t   \tspace\t\t\t  " => "too-much-space" if fill is "-"
std::string &ReduceStr(std::string& str, const char *fill/*= " "*/, const char *whitespace /*=" \t"*/)
{
    // trim first
    TrimStr(str, whitespace);

    // replace sub ranges
    auto beginSpace = str.find_first_of(whitespace);
    while (beginSpace != std::string::npos)
    {
        const auto endSpace = str.find_first_not_of(whitespace, beginSpace);
        const auto range = endSpace - beginSpace;

        str.replace(beginSpace, range, fill);

        const auto newStart = beginSpace + strlen(fill);
        beginSpace = str.find_first_of(whitespace, newStart);
    }

    return str;
}

}
