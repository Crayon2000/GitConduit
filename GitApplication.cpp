//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitApplication.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TGitApplication::TGitApplication() :
    System::TObject(),
    FEndpoint(TApiEndpoint::User)
{
}

