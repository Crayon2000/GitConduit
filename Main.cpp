//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include "GitApplication.h"
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

    cboSourceApp->Items->AddObject("Gogs", (TObject*)TGitApplicationType::Gogs);
    cboSourceApp->Items->AddObject("GitBucket", (TObject*)TGitApplicationType::GitBucket);
    cboSourceApp->Items->AddObject("GitHub", (TObject*)TGitApplicationType::GitHub);
    cboSourceApp->ItemIndex = 0;
    cboDestinationApp->Items->AddObject("Gogs", (TObject*)TGitApplicationType::Gogs);
    cboDestinationApp->Items->AddObject("GitBucket", (TObject*)TGitApplicationType::GitBucket);
    cboDestinationApp->Items->AddObject("GitHub", (TObject*)TGitApplicationType::GitHub);
    cboDestinationApp->ItemIndex = 1;

    chkSourceTypeUser->GroupName = "Source";
    chkSourceTypeOrg->GroupName = "Source";
    chkDestinationTypeUser->GroupName = "Destination";
    chkDestinationTypeOrg->GroupName = "Destination";

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
    const TGitApplicationType LSourceType =
        static_cast<TGitApplicationType>((unsigned char)cboSourceApp->Selected->Data);
    SourceApplication->ApplicationType = LSourceType;
    SourceApplication->ApiUrl = txtSourceUrl->Text;
    SourceApplication->Token = txtSourceToken->Text;

    const TGitApplicationType LDestinationype =
        static_cast<TGitApplicationType>((unsigned char)cboDestinationApp->Selected->Data);
    DestinationApplication->ApplicationType = LDestinationype;
    DestinationApplication->ApiUrl = txtDestinationUrl->Text;
    DestinationApplication->Token = txtDestinationToken->Text;

    String LUrl = SourceApplication->ApiUrl;
    if(chkSourceTypeOrg->IsChecked == true)
    {
        LUrl += "/orgs/" + SourceApplication->User + "/repos";

        SourceApplication->Endpoint = TApiEndpoint::Organization;
        SourceApplication->User = txtSourceName->Text;
    }
    else
    {
        LUrl += "/user/repos";

        SourceApplication->Endpoint = TApiEndpoint::User;

        SourceApplication->User = GetAuthenticatedUser(SourceApplication);
        if(SourceApplication->User.IsEmpty() == true)
        {
            return;
        }
    }

    if(chkDestinationTypeOrg->IsChecked == true)
    {
        DestinationApplication->Endpoint = TApiEndpoint::Organization;
        DestinationApplication->User = txtDestinationName->Text;
    }
    else
    {
        DestinationApplication->Endpoint = TApiEndpoint::User;
        DestinationApplication->User = GetAuthenticatedUser(DestinationApplication);
        if(DestinationApplication->User.IsEmpty() == true)
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
                TJSONObject* LRepo = static_cast<TJSONObject*>(LRepoEnumerator->Current);

                TRepository LDestinationRepository;
                TRepository LSourceRepository;
                JsonToRepo(LRepo->ToString(), LSourceRepository);

                if(SourceApplication->Endpoint == TApiEndpoint::User &&
                    LSourceRepository.Owner.Login != SourceApplication->User)
                {
                    continue;
                }

                const String LLog = String().sprintf(L"====== %s ======", LSourceRepository.FullName.c_str());
                memoLog->Lines->Add(LLog);

                LRepo->RemovePair("owner");
                LRepo->RemovePair("permissions");
                const String LJson = LRepo->ToJSON();
                bool LIsCreated = CreateRepo(LJson, LDestinationRepository);

                if(LIsCreated == false)
                {   // Don't do rest if issue was not created
                    continue;
                }

                if(LSourceRepository.OpenIssueCount > 0)
                {
                    const String LLog = String().sprintf(L"%d issue(s) not created!", LSourceRepository.OpenIssueCount);
                    memoLog->Lines->Add(LLog);
                }

                try
                {
                    Clone(LSourceRepository.CloneUrl);
                    AddRemote(LDestinationRepository.CloneUrl, "temp.git");
                    Push("temp.git");
                    memoLog->Lines->Add("Pushed repository");
                    try
                    {
                        const String LSourceWikiUrl = StringReplace(
                            LSourceRepository.CloneUrl,
                            ".git", ".wiki.git", TReplaceFlags());
                        const String LCDestinationWikiUrl = StringReplace(
                            LDestinationRepository.CloneUrl,
                            ".git", ".wiki.git", TReplaceFlags());

                        Ioutils::TDirectory::Delete("temp.git", true);

                        Clone(LSourceWikiUrl);
                        AddRemote(LCDestinationWikiUrl, "temp.git");
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

bool __fastcall TForm2::CreateRepo(const String AJson, TRepository& ARepository)
{
    bool Result = false;

    String LUrl = DestinationApplication->ApiUrl;

    if(DestinationApplication->Endpoint == TApiEndpoint::Organization)
    {
        LUrl += "/orgs/" + DestinationApplication->User + "/repos";
    }
    else
    {
        LUrl += "/user/repos";
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
            JsonToRepo(LAnswer, ARepository);

            const String LLog = "Created repository " + ARepository.FullName +
                " on " + DestinationApplication->ApplicationName;
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
    const String LUrl = AGitApplication->ApiUrl + "/user";
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

HANDLE __fastcall TForm2::ExecuteProgramEx(const String ACmd, const String ADirectory)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES sa;

    si.cb = sizeof(si);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = const_cast<LPWSTR>(L"Git");
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

void __fastcall TForm2::JsonToRepo(const String AJson, TRepository& ARepository)
{
    TJSONPair* Pair;
    TJSONObject* LRepo = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));

    if((Pair = LRepo->Get("owner")) != NULL)
    {
        TJSONObject* LOwner = static_cast<TJSONObject*>(Pair->JsonValue);
        if((Pair = LOwner->Get("login")) != NULL)
        {
            ARepository.Owner.Login =
                static_cast<TJSONString*>(Pair->JsonValue)->Value();
        }
#ifdef _DEBUG
        else
        {
            throw Exception("login not found");
        }
#endif
    }
#ifdef _DEBUG
    else
    {
        throw Exception("owner not found");
    }
#endif

    if((Pair = LRepo->Get("name")) != NULL)
    {
        ARepository.Name = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("name not found");
    }
#endif

    if((Pair = LRepo->Get("full_name")) != NULL)
    {
        ARepository.FullName = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("full_name not found");
    }
#endif

    if((Pair = LRepo->Get("clone_url")) != NULL)
    {
        ARepository.CloneUrl = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("clone_url not found");
    }
#endif

    if((Pair = LRepo->Get("open_issues_count")) != NULL)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        ARepository.OpenIssueCount = LIssueNumber->AsInt;
    }
    else
    {
        ARepository.OpenIssueCount = 0;
    }
}
//---------------------------------------------------------------------------

