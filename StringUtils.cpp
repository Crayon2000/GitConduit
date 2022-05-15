//---------------------------------------------------------------------------
#pragma hdrstop

#include "StringUtils.h"
#include <codecvt> // codecvt_utf8
#include <locale>  // wstring_convert
//---------------------------------------------------------------------------
#pragma package(smart_init)

std::string ToStdString(const std::wstring& AString)
{
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(AString);
}

std::string ToStdString(const UnicodeString& AString)
{
    return ToStdString(std::wstring(AString.c_str()));
}

std::wstring ToStdWString(const std::string& AString)
{
    static std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(AString);
}

std::wstring ToStdWString(const UnicodeString& AString)
{
    return AString.c_str();
}

UnicodeString ToUnicodeString(const std::string& AString)
{
    return ToStdWString(AString).c_str();
}

UnicodeString ToUnicodeString(const std::wstring& AString)
{
    return AString.c_str();
}

