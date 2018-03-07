//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include <System.JSON.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TForm2 *Form2;
//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner)
    : TForm(Owner)
    , SourceApplication(NULL)
    , DestinationApplication(NULL)
{
    Caption = "Gogs To GitBucket";

    SourceApplication = new TGitApplication();
    DestinationApplication = new TGitApplication();

    IdHTTP1->HandleRedirects = true;
    IdHTTP1->IOHandler = IdSSLIOHandlerSocketOpenSSL1;
}
//---------------------------------------------------------------------------

__fastcall TForm2::~TForm2()
{
    delete SourceApplication;
    delete DestinationApplication;
}
//---------------------------------------------------------------------------


void __fastcall TForm2::Button1Click(TObject *Sender)
{
    SourceApplication->ApplicationName = "Gogs";
    SourceApplication->ApiVersion = "v1";
    SourceApplication->Url = txtSourceUrl->Text;
    SourceApplication->Token = txtSourceToken->Text;

    DestinationApplication->ApplicationName = "GitBucket";
    DestinationApplication->ApiVersion = "v3";
    DestinationApplication->Url = txtDestinationUrl->Text;
    DestinationApplication->Token = txtDestinationToken->Text;

    String LUser;
    String LUrl = SourceApplication->Url + "/api/" + SourceApplication->ApiVersion + "/";
    if(chkTypeOrg->IsChecked == true)
    {
        LUrl += "orgs/" + txtName->Text + "/repos";
    }
    else
    {
        LUrl += "user/repos";
        LUser = GetAuthenticatedUser(SourceApplication);
        if(LUser.IsEmpty() == true)
        {
            return;
        }
    }

    try
    {
        PrepareRequest(SourceApplication);
        const String LContent = IdHTTP1->Get(LUrl);

        TJSONArray* LRepos = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
        if(LRepos != NULL)
        {
            TJSONArrayEnumerator* LRepoEnumerator = LRepos->GetEnumerator();
            while(LRepoEnumerator->MoveNext() == true)
            {
                TJSONPair* Pair;
                TJSONObject* LRepo = static_cast<TJSONObject*>(LRepoEnumerator->Current);

                if(chkTypeOrg->IsChecked == false)
                {
                    if((Pair = LRepo->Get("owner")) != NULL)
                    {
                        TJSONObject* LOwner = static_cast<TJSONObject*>(Pair->JsonValue);
                        if((Pair = LOwner->Get("username")) != NULL)
                        {
                            String LUserName =
                                static_cast<TJSONString*>(Pair->JsonValue)->Value();
                            if(LUserName != LUser)
                            {
                                continue;
                            }
                        }
                    }
                }

                String LFullName;
                if((Pair = LRepo->Get("full_name")) != NULL)
                {
                    LFullName =
                        static_cast<TJSONString*>(Pair->JsonValue)->Value();
                }

                const String LLog = String().sprintf(L"====== %s ======", LFullName.c_str());
                memoLog->Lines->Add(LLog);

                LRepo->RemovePair("owner");
                LRepo->RemovePair("permissions");
                const String LJson = LRepo->ToJSON();
                bool LIsCreated = CreateRepo(LJson);

                if(LIsCreated == false)
                {   // Don't do rest if issue was not created
                    continue;
                }

                if((Pair = LRepo->Get("open_issues_count")) != NULL)
                {
                    TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
                    const LIssueCount = LIssueNumber->AsInt;
                    if(LIssueCount > 0)
                    {
                        const String LLog = String().sprintf(L"%d issue(s) not created!", LIssueCount);
                        memoLog->Lines->Add(LLog);
                    }
                }

                Clone(GitUrl(SourceApplication, LFullName));

                //break; // TEST ONE
                Application->ProcessMessages();
            }
        }
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        const String LLog = "Get repository exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }
}
//---------------------------------------------------------------------------

bool __fastcall TForm2::CreateRepo(const String AJson)
{
    bool Result = false;

    String LUrl = DestinationApplication->Url + "/api/" +
        DestinationApplication->ApiVersion + "/";
    if(chkTypeOrg->IsChecked == true)
    {
        LUrl += "orgs/" + txtName->Text + "/repos";
    }
    else
    {
        LUrl += "user/repos";
    }

    String LAnswer;
    System::Classes::TStringStream* SourceFile = NULL;
    try
    {
        PrepareRequest(DestinationApplication);
        SourceFile = new System::Classes::TStringStream(AJson);
        LAnswer = IdHTTP1->Post(LUrl, SourceFile);
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        const String LLog = "Repository creation exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }
    delete SourceFile;

    TJSONObject* LObject = dynamic_cast<TJSONObject*>(TJSONObject::ParseJSONValue(LAnswer));
    if(LObject != NULL)
    {
        TJSONPair* Pair;
        if((Pair = LObject->Get("full_name")) != NULL)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            const String LLog = "Created repository: " + Answer->Value();
            memoLog->Lines->Add(LLog);
            Result = true;
        }
        else if((Pair = LObject->Get("message")) != NULL)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            const String LLog = "Repository creation message: " + Answer->Value();
            memoLog->Lines->Add(LLog);
        }
    }

    return Result;
}
//---------------------------------------------------------------------------

String __fastcall TForm2::GetAuthenticatedUser(TGitApplication* AGitApplication)
{
    String Result;

    String LJson;
    const String LUrl = AGitApplication->Url + "/api/" +
        AGitApplication->ApiVersion + "/user";
    try
    {
        PrepareRequest(AGitApplication);
        LJson = IdHTTP1->Get(LUrl);
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        const String LLog = "Get authenticated user exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }
    catch(const Sysutils::Exception& e)
    {
        const String LLog = "Get authenticated user exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }

    if(LJson.IsEmpty() == true)
    {
        return "";
    }

    TJSONObject* LUser = dynamic_cast<TJSONObject*>(TJSONObject::ParseJSONValue(LJson));
    if(LUser != NULL)
    {
        TJSONPair* Pair;
        if((Pair = LUser->Get("login")) != NULL)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            Result = Answer->Value();
        }
    }

    if(Result.IsEmpty() == false)
    {
        memoLog->Lines->Add("Authenticated user is " + Result);
    }
    else
    {
        memoLog->Lines->Add("Cannot get authenticated user");
    }

    return Result;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::PrepareRequest(TGitApplication* AGitApplication)
{
    // Set authorization token
    IdHTTP1->Request->CustomHeaders->Values["Authorization"] =
        "token " + AGitApplication->Token;
}
//---------------------------------------------------------------------------

String __fastcall TForm2::GitUrl(TGitApplication* AGitApplication, const String AFullName)
{
    return AGitApplication->Url + "/" + AFullName + ".git";
}
//---------------------------------------------------------------------------

String __fastcall TForm2::GitWikiUrl(TGitApplication* AGitApplication, const String AFullName)
{
    return AGitApplication->Url + "/" + AFullName + ".wiki.git";
}
//---------------------------------------------------------------------------

HANDLE __fastcall TForm2::ExecuteProgramEx(const String ACmd)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;

    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = L"Git";
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.cbReserved2 = 0;
    si.lpReserved2 = NULL;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = NULL;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = true;

    bool ProcResult = CreateProcess(NULL, ACmd.c_str(), NULL, NULL, true, 0, NULL, NULL, &si, &pi);
    if(ProcResult == true)
    {
        return pi.hProcess;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(si.hStdOutput);
    return NULL;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Wait(HANDLE AHandle)
{
    DWORD ExitCode;

    if(AHandle == NULL || AHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    bool Done = false;
    while(Done == false)
    {
        GetExitCodeProcess(
            AHandle,        // handle to the process
            &ExitCode       // address to receive termination status
        );

        if(ExitCode == STILL_ACTIVE)
        {
            Application->ProcessMessages();
        }
        else
        {
            Done = true;
        }
    }
    CloseHandle(AHandle);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Clone(const String AGitRepo)
{
    const String LCmd = String().sprintf(L"git clone %s --bare", AGitRepo.c_str());
    HANDLE LHandle = ExecuteProgramEx(LCmd);
    if(LHandle == NULL)
    {
        return;
    }
    Wait(LHandle);
}
//---------------------------------------------------------------------------

