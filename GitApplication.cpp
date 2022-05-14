//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitApplication.h"
#include "StringUtils.h"
#include <exception>
#include <IdURI.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TGitApplication::TGitApplication() :
    FEndpoint(TApiEndpoint::User)
{
    SetApplicationType(TGitApplicationType::Gogs);
}

void __fastcall TGitApplication::SetApplicationType(TGitApplicationType AApplicationType)
{
    switch(AApplicationType)
    {
        case TGitApplicationType::Gogs:
            FApplicationName = L"Gogs";
            break;
        case TGitApplicationType::GitBucket:
            FApplicationName = L"GitBucket";
            break;
        case TGitApplicationType::GitHub:
            FApplicationName = L"GitHub";
            break;
        case TGitApplicationType::GitLab:
            FApplicationName = L"GitLab";
            break;
        default:
            throw std::exception("Invalid application type!");
    }
    FApplicationType = AApplicationType;
}

void __fastcall TGitApplication::SetApiUrl(const std::wstring AApiUrl)
{
    String LApiUrl = Trim(AApiUrl).c_str();

    try
    {
        // URLEncode may throw an exception
        LApiUrl = TIdURI::URLEncode(LApiUrl);
    }
    catch(...)
    {
    }

    if(LApiUrl.Length() > 0 && *LApiUrl.LastChar() == L'/')
    {
        LApiUrl = LApiUrl.SubString(0, LApiUrl.Length() - 1);
    }

    FApiUrl = LApiUrl.c_str();
}

