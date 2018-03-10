//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include <System.JSON.hpp>
#include <System.IOUtils.hpp>
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
    IdHTTP1->Request->UserAgent =
        "Mozilla/5.0 (Windows NT 5.1) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/49.0.2623.112 Safari/537.36";
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

                try
                {
                    Clone(GitUrl(SourceApplication, LFullName));
                    AddRemote(GitUrl(DestinationApplication, LFullName), "temp.git");
                    Push("temp.git");
                    memoLog->Lines->Add("Pushed repository");
                    try
                    {
                        Ioutils::TDirectory::Delete("temp.git", true);

                        Clone(GitWikiUrl(SourceApplication, LFullName));
                        AddRemote(GitWikiUrl(DestinationApplication, LFullName), "temp.git");
                        Push("temp.git");
                        memoLog->Lines->Add("Pushed Wiki repository");
                    }
                    catch(...)
                    {
                        memoLog->Lines->Add("Wiki could not be exported");
                    }
                }
                __finally
                {
                    try
                    {
                        Ioutils::TDirectory::Delete("temp.git", true);
                    }
                    catch(...)
                    {
                    }
                }

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
            const String LLog = "Created repository " + Answer->Value() + " on " +
                DestinationApplication->ApplicationName;
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

HANDLE __fastcall TForm2::ExecuteProgramEx(const String ACmd, const String ADirectory)
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

    bool ProcResult = CreateProcess(NULL, ACmd.c_str(), NULL, NULL, true, 0,
        NULL, ADirectory.c_str(), &si, &pi);
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

DWORD __fastcall TForm2::Wait(HANDLE AHandle)
{
    DWORD Result;

    if(AHandle == NULL || AHandle == INVALID_HANDLE_VALUE)
    {
        return 1;
    }

    bool Done = false;
    while(Done == false)
    {
        GetExitCodeProcess(
            AHandle,        // handle to the process
            &Result       // address to receive termination status
        );

        if(Result == STILL_ACTIVE)
        {
            Application->ProcessMessages();
        }
        else
        {
            Done = true;
        }
    }
    CloseHandle(AHandle);

    return Result;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Clone(const String AGitRepo)
{
    const String LCmd = String().sprintf(L"git clone %s temp.git --bare", AGitRepo.c_str());
    HANDLE LHandle = ExecuteProgramEx(LCmd);
    DWORD LExitCode = Wait(LHandle);
    if(LExitCode != 0)
    {
        throw Exception("Clone command failed");
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::AddRemote(const String AGitRepo, const String ADirectory)
{
    const String LCmd = String().sprintf(L"git remote add origin2 %s", AGitRepo.c_str());
    HANDLE LHandle = ExecuteProgramEx(LCmd, ADirectory);
    DWORD LExitCode = Wait(LHandle);
    if(LExitCode != 0)
    {
        throw Exception("Add remote command failed");
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Push(const String ADirectory)
{
    const String LCmd = "git push origin2 --mirror";
    HANDLE LHandle = ExecuteProgramEx(LCmd, ADirectory);
    DWORD LExitCode = Wait(LHandle);
    if(LExitCode != 0)
    {
        throw Exception("Push command failed");
    }
}
//---------------------------------------------------------------------------

