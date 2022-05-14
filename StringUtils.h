//---------------------------------------------------------------------------
#ifndef StringUtilsH
#define StringUtilsH
//---------------------------------------------------------------------------
#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
//---------------------------------------------------------------------------
/**
 * Removes blank and control characters from the left side of a string.
 */
static void TrimLeft(std::wstring &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t ch) {
        return !std::iswspace(ch);
    }));
}

/**
 * Removes blank and control characters from the right side of a string.
 */
static void TrimRight(std::wstring &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t ch) {
        return !std::iswspace(ch);
    }).base(), s.end());
}

/**
 * Trims leading and trailing spaces and control characters from a string.
 */
static auto Trim(std::wstring s)
{
    TrimLeft(s);
    TrimRight(s);
    return s;
} 

/**
 * Removes blank and control characters from the left side of a string.
 */
static void TrimLeft(std::string &s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

/**
 * Removes blank and control characters from the right side of a string.
 */
static void TrimRight(std::string &s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

/**
 * Trims leading and trailing spaces and control characters from a string.
 */
static auto Trim(std::string s)
{
    TrimLeft(s);
    TrimRight(s);
    return s;
}

//---------------------------------------------------------------------------
#endif
