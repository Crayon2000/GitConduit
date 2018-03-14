//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitApplication.h"
#include <System.SysUtils.hpp>
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
            FApiVersion = "v1";
            break;
        case TGitApplicationType::GitBucket:
            FApplicationName = "GitBucket";
            FApiVersion = "v3";
            break;
        default:
            throw Exception("Invalid application type!");
    }
    FApplicationType = AApplicationType;
}

