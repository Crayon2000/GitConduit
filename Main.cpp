//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include "GitApplication.h"
#include "GitRepository.h"
#include "HttpModule.h"
#include <System.JSON.hpp>
#include <System.IOUtils.hpp>
#include <System.RegularExpressions.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TForm2 *Form2;

#define TABIDSOURCE             0
#define TABIDSOURCEOWNER        1
#define TABIDREPOSITORIES       2
#define TABIDDESTINATION        3
#define TABIDDESTINATIONOWNER   4
#define TABIDCREATE             5
//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner)
    : TForm(Owner)
    , SourceApplication(NULL)
    , DestinationApplication(NULL)
{
    Caption = "GitConduit";

    // TabPosition is set to None
    // We don't want to allow people to change tab with Tab + Arrow keys
    for(int i = 0; i < TabControl1->TabCount; ++i)
    {
        TabControl1->Tabs[i]->TabStop = false;
    }
    TabControl1->TabIndex = -1;

    HideMessage();

    FTabAction = -1;
    Fmx::Forms::Application->OnIdle = OnApplicationIdle;

    btnSourceNext->TagObject = TabItemSourceOwner;
    btnSourceOwnerNext->TagObject = TabItemRepo;
    btnRepoNext->TagObject = TabItemDestination;
    btnDestinationNext->TagObject = TabItemDestinationOwner;
    btnDestinationOwnerNext->TagObject = TabItemCreate;
    btnSourceOwnerBack->TagObject = TabItemSource;
    btnRepoBack->TagObject = TabItemSourceOwner;
    btnDestinationBack->TagObject = TabItemRepo;
    btnDestinationOwnerBack->TagObject = TabItemDestination;
    btnCreateRepoBack->TagObject = TabItemDestinationOwner;

    cboSourceApp->Items->AddObject("Gogs", reinterpret_cast<TObject*>(TGitApplicationType::Gogs));
    cboSourceApp->Items->AddObject("GitBucket", reinterpret_cast<TObject*>(TGitApplicationType::GitBucket));
    cboSourceApp->Items->AddObject("GitHub", reinterpret_cast<TObject*>(TGitApplicationType::GitHub));
    cboSourceApp->ItemIndex = 0;
    cboDestinationApp->Items->AddObject("Gogs", reinterpret_cast<TObject*>(TGitApplicationType::Gogs));
    cboDestinationApp->Items->AddObject("GitBucket", reinterpret_cast<TObject*>(TGitApplicationType::GitBucket));
    cboDestinationApp->Items->AddObject("GitHub", reinterpret_cast<TObject*>(TGitApplicationType::GitHub));
    cboDestinationApp->ItemIndex = 1;

    chkSourceTypeUser->GroupName = "Source";
    chkSourceTypeOrg->GroupName = "Source";
    chkSourceTypeUser->IsChecked = true;
    chkDestinationTypeUser->GroupName = "Destination";
    chkDestinationTypeOrg->GroupName = "Destination";
    chkDestinationTypeUser->IsChecked = true;

    SourceApplication = new TGitApplication();
    DestinationApplication = new TGitApplication();

    FHTTPModule = new TDataModule1(NULL);
    FHTTPClient = FHTTPModule->IdHTTP1;
}
//---------------------------------------------------------------------------

__fastcall TForm2::~TForm2()
{
    delete FHTTPModule;
    delete SourceApplication;
    delete DestinationApplication;
}
//---------------------------------------------------------------------------

bool __fastcall TForm2::CreateRepo(const String AJson, TRepository* ARepository)
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
    System::Classes::TMemoryStream* SourceFile = NULL;
    try
    {
        PrepareRequest(DestinationApplication);
        SourceFile = new System::Classes::TMemoryStream();
        WriteStringToStream(SourceFile, AJson, enUTF8);
        SourceFile->Position = 0;
        FHTTPClient->Request->ContentType = "application/json";
        LAnswer = FHTTPClient->Post(LUrl, SourceFile);
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
#ifdef GITBUCKET_FIX
// The repository is created but an error 500 is received.
// An issue was submitted to GitBucket.
        if(DestinationApplication->Endpoint == TApiEndpoint::Organization &&
            e.ErrorCode == 500)
        {
            // Get the repository name we wanted to create
            TRepository* LRepoInfo = new TRepository();
            JsonToRepo(AJson, LRepoInfo);

            LUrl = DestinationApplication->ApiUrl + "/repos/" +
                DestinationApplication->User + "/" + LRepoInfo->Name;

            try
            {
                LAnswer = FHTTPClient->Get(LUrl);
            }
            catch(...)
            {
            }

            delete LRepoInfo;
        }
#endif
        const String LLog = "Repository creation HTTP protocol exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }
    catch(const Exception& e)
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

            const String LLog = "Created repository " + ARepository->FullName +
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

    PrepareRequest(AGitApplication);
    LJson = FHTTPClient->Get(LUrl); // May throw exception

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

    return Result;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::GetOrganizations(TGitApplication* AGitApplication,
    System::Classes::TStrings* AItems)
{
    String LJson;
    const String LUrl = AGitApplication->ApiUrl + "/user/orgs";

    AItems->Clear();

    PrepareRequest(AGitApplication);
    try
    {
        LJson = FHTTPClient->Get(LUrl); // May throw exception
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        if(e.ErrorCode != 404)
        {   // GitBucket does not support this service
            // So error 404 is normal
            throw;
        }
    }

    TJSONArray* LOrgs = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LJson));
    if(LOrgs != NULL)
    {
        TJSONArray::TEnumerator* LOrgsEnumerator = LOrgs->GetEnumerator();
        while(LOrgsEnumerator->MoveNext() == true)
        {
            TJSONObject* LOrg = static_cast<TJSONObject*>(LOrgsEnumerator->Current);

            TOrganization* Org = new TOrganization();
            JsonToOrganization(LOrg, Org);

            AItems->AddObject(Org->Login, NULL);

            delete Org;
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::PrepareRequest(TGitApplication* AGitApplication)
{
    // Set authorization token
    FHTTPClient->Request->CustomHeaders->Values["Authorization"] =
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
    si.hStdInput = NULL;
    si.hStdOutput = NULL;
    si.hStdError = NULL;

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

void __fastcall TForm2::Clone(const String ADirectory, const String AGitRepo, bool AIsBare)
{
    String LCmd = String().sprintf(L"git clone %s %s", AGitRepo.c_str(), ADirectory.c_str());
    if(AIsBare == true)
    {
        LCmd += " --bare";
    }
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

bool __fastcall TForm2::CheckGitExe()
{
    const String LCmd = "git --version";
    if(ExecuteProgramEx(LCmd) == NULL)
    {
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::TabControl1Change(TObject *Sender)
{
    HideMessage();

    const int LIndex = TabControl1->TabIndex;
    switch(LIndex)
    {
        case TABIDSOURCE:
            btnSourceNext->Enabled = false;
            break;
        case TABIDSOURCEOWNER:
            btnSourceOwnerNext->Enabled = false;
            btnSourceOwnerBack->Enabled = false;

            cboeSourceUser->StyleLookup = "editstyle";
            cboeSourceName->StyleLookup = "editstyle";
            break;
        case TABIDREPOSITORIES:
            btnRepoNext->Enabled = false;
            btnRepoBack->Enabled = false;

            ListBoxRepo->Clear();
            break;
        case TABIDDESTINATION:
            btnDestinationNext->Enabled = false;
            btnDestinationBack->Enabled = false;
            break;
        case TABIDDESTINATIONOWNER:
            btnDestinationOwnerNext->Enabled = false;
            btnDestinationOwnerBack->Enabled = false;

            chkDestinationTypeUser->Text = "User";
            cboeDestinationName->StyleLookup = "editstyle";
            break;
        case TABIDCREATE:
            btnCreateRepoBack->Enabled = false;
            btnCreateRepoClose->Enabled = false;

            memoLog->Lines->Clear();
            break;
        default:
            break;
    }

    FTabAction = LIndex;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::WizardButtonClick(TObject *Sender)
{
    TSpeedButton* LButton = static_cast<TSpeedButton*>(Sender);
    TabControl1->ActiveTab = dynamic_cast<TTabItem*>(LButton->TagObject);
}
//---------------------------------------------------------------------------

void __fastcall TForm2::FormShow(TObject *Sender)
{
    TabControl1->TabIndex = 0;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ShowMessage(const String AMessage)
{
    Rectangle1->Visible = true;
    Rectangle1->Align = TAlignLayout::Contents;
    lblErrorMessage->Text = AMessage;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::HideMessage()
{
    Rectangle1->Visible = false;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::OnApplicationIdle(System::TObject* Sender, bool &Done)
{
    int LTabAction = FTabAction;
    FTabAction = -1;

    switch(LTabAction)
    {
        case TABIDSOURCE:
            ActionSource();
            break;
        case TABIDSOURCEOWNER:
            ActionSourceOwner();
            break;
        case TABIDREPOSITORIES:
            ActionRepositories();
            break;
        case TABIDDESTINATION:
            ActionDestination();
            break;
        case TABIDDESTINATIONOWNER:
            ActionDestinationOwner();
            break;
        case TABIDCREATE:
            ActionCreateRepo();
            break;
        default:
            break;
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionSource()
{
    if(CheckGitExe() == false)
    {
        ShowMessage("git.exe not found in path!\n\nCorrect the problem and try again.");
        return;
    }

    btnSourceNext->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionSourceOwner()
{
    const TGitApplicationType LSourceType =
        static_cast<TGitApplicationType>((unsigned char)cboSourceApp->Selected->Data);
    SourceApplication->ApplicationType = LSourceType;
    SourceApplication->ApiUrl = txtSourceUrl->Text;
    SourceApplication->Token = txtSourceToken->Text;
    SourceApplication->Username = txtSourceUsername->Text;
    SourceApplication->Password = txtSourcePassword->Text;

    String LUser;
    String LExceptionMsg;
    try
    {
        LUser = GetAuthenticatedUser(SourceApplication);
    }
    catch(const Exception& e)
    {
        LExceptionMsg = e.Message;
    }
    if(LUser.IsEmpty() == true)
    {
        btnSourceOwnerBack->Enabled = true;
        String LMessage = "Cannot get authenticated user!";
        if(LExceptionMsg.IsEmpty() == false)
        {
            LMessage += String(EOL) + String(EOL) + LExceptionMsg;
        }
        LMessage += String(EOL) + String(EOL) + "Go Back, change the settings and try again.";
        ShowMessage(LMessage);

        return;
    }

    cboeSourceUser->Items->Clear();
    cboeSourceUser->Items->AddObject(LUser, NULL);
    cboeSourceUser->ItemIndex = 0;
    cboeSourceUser->StyleLookup = "comboeditstyle";

    try
    {
        GetOrganizations(SourceApplication, cboeSourceName->Items);
    }
    catch(const Exception& e)
    {
        btnSourceOwnerBack->Enabled = true;
        String LMessage = "Cannot get organizations list!";
        LMessage += String(EOL) + String(EOL) + e.Message;
        LMessage += String(EOL) + String(EOL) + "Go Back, change the settings and try again.";
        ShowMessage(LMessage);

        return;
    }

    if(cboeSourceName->Items->Count > 0)
    {
        cboeSourceName->StyleLookup = "comboeditstyle";
    }

    btnSourceOwnerNext->Enabled = true;
    btnSourceOwnerBack->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionRepositories()
{
    String LUrl = SourceApplication->ApiUrl;
    if(chkSourceTypeOrg->IsChecked == true)
    {
        SourceApplication->Endpoint = TApiEndpoint::Organization;
        SourceApplication->User = cboeSourceName->Text;

        LUrl += "/orgs/" + cboeSourceName->Text + "/repos";
    }
    else
    {
        SourceApplication->Endpoint = TApiEndpoint::User;
        SourceApplication->User = cboeSourceUser->Text;

        if(cboeSourceUser->Items->Count > 0 &&
            cboeSourceUser->Items->Strings[0] == cboeSourceUser->Text)
        {
            LUrl += "/user/repos";
        }
        else
        {
            LUrl += "/users/" + cboeSourceUser->Text + "/repos";
        }
    }

    String LExceptionMsg;
    try
    {
        while(LUrl.IsEmpty() == false)
        {
            PrepareRequest(SourceApplication);
            const String LContent = FHTTPClient->Get(LUrl);

            TJSONArray* LRepos = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
            if(LRepos != NULL)
            {
                TJSONArray::TEnumerator* LRepoEnumerator = LRepos->GetEnumerator();
                while(LRepoEnumerator->MoveNext() == true)
                {
                    TJSONObject* LRepo = static_cast<TJSONObject*>(LRepoEnumerator->Current);

                    const String LSourceJson = LRepo->ToString();
                    TRepository* LSourceRepository = new TRepository();
                    JsonToRepo(LSourceJson, LSourceRepository);

                    if(SourceApplication->Endpoint == TApiEndpoint::User &&
                        LSourceRepository->Owner->Login != SourceApplication->User)
                    {
                        delete LSourceRepository;
                        continue;
                    }

                    TListBoxItem* LListBoxItem = new TListBoxItem(this);
                    LListBoxItem->Parent = ListBoxRepo;
                    LListBoxItem->IsChecked = true;
                    LListBoxItem->Text = LSourceRepository->FullName;
                    LListBoxItem->TagString = LSourceJson;
                    LListBoxItem->OnApplyStyleLookup = ListBoxItemApplyStyleLookup;
                    if(LSourceRepository->Private == true)
                    {   // Private
                        LListBoxItem->ImageIndex = 0;
                    }
                    else
                    {
                        if(LSourceRepository->Fork == false)
                        {   // Public
                            LListBoxItem->ImageIndex = 1;
                        }
                        else
                        {   // Fork
                            LListBoxItem->ImageIndex = 2;
                        }
                    }
                    if(LSourceRepository->Description.IsEmpty() == false)
                    {
                        LListBoxItem->ItemData->Detail = LSourceRepository->Description;
                        LListBoxItem->Height = 50.0f;
                        LListBoxItem->StyleLookup = "listboxitembottomdetail";
                    }
                    else
                    {
                        LListBoxItem->Height = 40.0f;
                        LListBoxItem->StyleLookup = "listboxitemnodetail";
                    }

                    delete LSourceRepository;

                    Application->ProcessMessages();
                }
            }

            LUrl = GetNextUrl();
        }
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        LExceptionMsg = "Get repository HTTP protocol exception: " + e.Message;
    }
    catch(const Idstack::EIdSocketError& e)
    {
        LExceptionMsg = "Get repository socket exception: " + e.Message;
    }
    catch(const Exception& e)
    {
        LExceptionMsg = "Get repository exception: " + e.Message;
    }

    btnRepoBack->Enabled = true;

    if(LExceptionMsg.IsEmpty() == false)
    {
        ShowMessage(LExceptionMsg + "\n\nGo Back, change the settings and try again.");
        return;
    }

    if(ListBoxRepo->Items->Count > 0)
    {
        btnRepoNext->Enabled = true;
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionDestination()
{
    btnDestinationNext->Enabled = true;
    btnDestinationBack->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionDestinationOwner()
{
    const TGitApplicationType LDestinationype =
        static_cast<TGitApplicationType>((unsigned char)cboDestinationApp->Selected->Data);
    DestinationApplication->ApplicationType = LDestinationype;
    DestinationApplication->ApiUrl = txtDestinationUrl->Text;
    DestinationApplication->Token = txtDestinationToken->Text;
    DestinationApplication->Username = txtDestinationUsername->Text;
    DestinationApplication->Password = txtDestinationPassword->Text;

    String LUser;
    String LExceptionMsg;
    try
    {
        LUser = GetAuthenticatedUser(DestinationApplication);
    }
    catch(const Exception& e)
    {
        LExceptionMsg = e.Message;
    }
    if(LUser.IsEmpty() == true)
    {
        btnDestinationOwnerBack->Enabled = true;
        String LMessage = "Cannot get authenticated user!";
        if(LExceptionMsg.IsEmpty() == false)
        {
            LMessage += String(EOL) + String(EOL) + LExceptionMsg;
        }
        LMessage += String(EOL) + String(EOL) + "Go Back, change the settings and try again.";
        ShowMessage(LMessage);

        return;
    }

    chkDestinationTypeUser->Text = String().sprintf(L"User (%s)", LUser.c_str());
    chkDestinationTypeUser->TagString = LUser;

    try
    {
        GetOrganizations(DestinationApplication, cboeDestinationName->Items);
    }
    catch(const Exception& e)
    {
        btnDestinationOwnerBack->Enabled = true;
        String LMessage = "Cannot get organizations list!";
        LMessage += String(EOL) + String(EOL) + e.Message;
        LMessage += String(EOL) + String(EOL) + "Go Back, change the settings and try again.";
        ShowMessage(LMessage);

        return;
    }

    if(cboeDestinationName->Items->Count > 0)
    {
        cboeDestinationName->StyleLookup = "comboeditstyle";
    }

    btnDestinationOwnerNext->Enabled = true;
    btnDestinationOwnerBack->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionCreateRepo()
{
    if(chkDestinationTypeOrg->IsChecked == true)
    {
        DestinationApplication->Endpoint = TApiEndpoint::Organization;
        DestinationApplication->User = cboeDestinationName->Text;
    }
    else
    {
        DestinationApplication->Endpoint = TApiEndpoint::User;
        DestinationApplication->User = chkDestinationTypeUser->TagString;
    }

    const int LCount = ListBoxRepo->Items->Count;
    for(int i = 0; i < LCount; ++i)
    {
        TListBoxItem *LItem = ListBoxRepo->ItemByIndex(i);
        if(LItem->IsChecked == false)
        {
            continue;
        }

        TRepository *LSourceRepository = NULL;
        TRepository* LDestinationRepository = NULL;
        try
        {
            const String LSourceJson = LItem->TagString;

            LDestinationRepository = new TRepository();

            LSourceRepository = new TRepository();
            JsonToRepo(LSourceJson, LSourceRepository);

            const String LLog = String().sprintf(L"====== %s ======",
                LSourceRepository->FullName.c_str());
            memoLog->Lines->Add(LLog);

            bool LIsCreated = CreateRepo(LSourceJson, LDestinationRepository);

            if(LIsCreated == false)
            {   // Don't do the rest if the issue was not created
                delete LDestinationRepository;
                delete LSourceRepository;
                continue;
            }

            if(LSourceRepository->OpenIssueCount > 0)
            {
                const String LIssueLog =
                    String().sprintf(L"%d open issue(s) not created!", LSourceRepository->OpenIssueCount);
                memoLog->Lines->Add(LIssueLog);
                PrintIssues(SourceApplication, LSourceRepository);
            }

            try
            {
                String LSourceUrl = LSourceRepository->CloneUrl;
                if(SourceApplication->Username.IsEmpty() == false &&
                    SourceApplication->Password.IsEmpty() == false)
                {
                    const String LSourceCredential =
                        SourceApplication->Username + ":" +
                        SourceApplication->Password + "@";
                    LSourceUrl = StringReplace(
                        LSourceUrl, "https://",
                        "https://" + LSourceCredential,
                        TReplaceFlags() << rfIgnoreCase);
                    LSourceUrl = StringReplace(
                        LSourceUrl, "http://",
                        "http://" + LSourceCredential,
                        TReplaceFlags() << rfIgnoreCase);
                }

                Clone("temp.git", LSourceUrl, true);

                String LDestinationUrl = LDestinationRepository->CloneUrl;
                if(DestinationApplication->Username.IsEmpty() == false &&
                    DestinationApplication->Password.IsEmpty() == false)
                {
                    const String LDestinationCredential =
                        DestinationApplication->Username + ":" +
                        DestinationApplication->Password + "@";
                    LDestinationUrl = StringReplace(
                        LDestinationUrl, "https://",
                        "https://" + LDestinationCredential,
                        TReplaceFlags() << rfIgnoreCase);
                    LDestinationUrl = StringReplace(
                        LDestinationUrl, "http://",
                        "http://" + LDestinationCredential,
                        TReplaceFlags() << rfIgnoreCase);
                }

                AddRemote(LDestinationUrl, "temp.git");
                Push("temp.git");

                memoLog->Lines->Add("Pushed repository");

                if(LSourceRepository->HasWiki == true)
                {
                    try
                    {
                        const String LSourceWikiUrl = StringReplace(
                            LSourceUrl, ".git", ".wiki.git", TReplaceFlags());
                        const String LCDestinationWikiUrl = StringReplace(
                            LDestinationUrl, ".git", ".wiki.git", TReplaceFlags());

                        Ioutils::TDirectory::Delete("temp.git", true);

                        Clone("temp.git", LSourceWikiUrl, true);
                        AddRemote(LCDestinationWikiUrl, "temp.git");
                        Push("temp.git");
                        memoLog->Lines->Add("Pushed Wiki repository");
                    }
                    catch(...)
                    {
                        memoLog->Lines->Add("Wiki could not be exported");
                    }
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

            Application->ProcessMessages();
        }
        catch(const Sysutils::Exception& e)
        {

        }

        delete LDestinationRepository;
        delete LSourceRepository;
    }

    memoLog->Lines->Add("");
    memoLog->Lines->Add("");
    memoLog->Lines->Add("Completed!");

    btnCreateRepoBack->Enabled = true;
    btnCreateRepoClose->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::btnErrorOkClick(TObject *Sender)
{
    HideMessage();
}
//---------------------------------------------------------------------------

void __fastcall TForm2::btnCreateRepoCloseClick(TObject *Sender)
{
    Close();
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ListBoxItemApplyStyleLookup(TObject *Sender)
{
    TListBoxItem* LListBoxItem = dynamic_cast<TListBoxItem*>(Sender);
    if(LListBoxItem == NULL)
    {
        return;
    }

    try
    {
        LListBoxItem->BeginUpdate();

        Fmx::Types::TFmxObject* GlyphStyleResource = LListBoxItem->FindStyleResource("glyphstyle");
        if(GlyphStyleResource != NULL && GlyphStyleResource->ClassNameIs("TGlyph") == true)
        {
            TGlyph* LGlyph = static_cast<TGlyph*>(GlyphStyleResource);
            LGlyph->Position->X = 20.0f; // Make sure checkbox is really MostLeft
        }

        Fmx::Types::TFmxObject* IconStyleResource = LListBoxItem->FindStyleResource("icon");
        if(IconStyleResource != NULL && IconStyleResource->ClassNameIs("TImage") == true)
        {
            TImage* LImage = static_cast<TImage*>(IconStyleResource);
            LImage->Position->X = 20.0f; // Make sure checkbox is really MostLeft
        }

        Fmx::Types::TFmxObject* CheckStyleResource = LListBoxItem->FindStyleResource("check");
        if(CheckStyleResource != NULL && CheckStyleResource->ClassNameIs("TCheckBox") == true)
        {
            TCheckBox* LCheckBox = static_cast<TCheckBox*>(CheckStyleResource);
            LCheckBox->Position->X = 0.0f;
        }
    }
    __finally
    {
        LListBoxItem->EndUpdate();
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::PrintIssues(TGitApplication* AGitApplication, TRepository* ARepository)
{
    const String LUrl = AGitApplication->ApiUrl + "/repos/" +
        AGitApplication->User + "/" + ARepository->Name + "/issues";
    try
    {
        PrepareRequest(AGitApplication);
        const String LContent = FHTTPClient->Get(LUrl);

        TJSONArray* LIssues = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
        if(LIssues != NULL)
        {
            TJSONArray::TEnumerator* LIssueEnumerator = LIssues->GetEnumerator();
            while(LIssueEnumerator->MoveNext() == true)
            {
                TJSONObject* LIssue = static_cast<TJSONObject*>(LIssueEnumerator->Current);

                TIssue* LIssueToPrint = new TIssue();
                JsonToIssue(LIssue, LIssueToPrint);

                const String LLog = String().sprintf(L"    Issue #%d: %s", LIssueToPrint->Number, LIssueToPrint->Title.c_str());
                memoLog->Lines->Add(LLog);

                delete LIssueToPrint;
            }
        }
    }
    catch(...)
    {
    }
}
//---------------------------------------------------------------------------

String __fastcall TForm2::GetNextUrl()
{
    String Result;
    const String LLink = FHTTPClient->Response->RawHeaders->Values["Link"];
    if(LLink.IsEmpty() == false)
    {
        const String LExpression = "<(\\S+)>; rel=\"next\"";
        TMatch LMatch = TRegEx::Match(LLink, LExpression, TRegExOptions() << TRegExOption::roSingleLine);
        if(LMatch.Success == true && LMatch.Groups.Count >= 1)
        {
            Result = LMatch.Groups[1].Value;
        }
    }
    return Result;
}
//---------------------------------------------------------------------------

