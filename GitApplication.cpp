//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitApplication.h"
#include <System.SysUtils.hpp>
#include <IdURI.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TGitApplication::TGitApplication() :
    System::TObject(),
    ApplicationType(TGitApplicationType::Gogs),
    FEndpoint(TApiEndpoint::User)
{
}

void __fastcall TGitApplication::SetApplicationType(TGitApplicationType AApplicationType)
{
    switch(AApplicationType)
    {
        case TGitApplicationType::Gogs:
            FApplicationName = "Gogs";
            break;
        case TGitApplicationType::GitBucket:
            FApplicationName = "GitBucket";
            break;
        case TGitApplicationType::GitHub:
            FApplicationName = "GitHub";
            break;
        default:
            throw Exception("Invalid application type!");
    }
    FApplicationType = AApplicationType;
}

void __fastcall TGitApplication::SetApiUrl(String AApiUrl)
{
    FApiUrl = TIdURI::URLEncode(AApiUrl.Trim());
    if(*FApiUrl.LastChar() == L'/')
    {
        FApiUrl = FApiUrl.SubString(0, FApiUrl.Length() - 1);
    }
}

