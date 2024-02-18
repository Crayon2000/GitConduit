//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include "GitApplication.h"
#include "GitRepository.h"
#include "HttpModule.h"
#include <System.JSON.hpp>
#include <System.IOUtils.hpp>
#include <regex>
#include <memory>
#include <vector>
#include <fmt/format.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TForm2 *Form2;

static constexpr int TABIDSOURCE =           0;
static constexpr int TABIDSOURCEOWNER =      1;
static constexpr int TABIDREPOSITORIES =     2;
static constexpr int TABIDDESTINATION =      3;
static constexpr int TABIDDESTINATIONOWNER = 4;
static constexpr int TABIDCREATE =           5;
//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner)
    : TForm(Owner)
    , SourceApplication(nullptr)
    , DestinationApplication(nullptr)
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

    FHTTPModule = new TDataModule1(nullptr);
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

bool __fastcall TForm2::CreateRepo(const TRepository* ASourceRepository, TRepository* ADestinationRepository)
{
    bool Result = false;

    std::wstring LUrl = DestinationApplication->ApiUrl.c_str();

    if(DestinationApplication->Endpoint == TApiEndpoint::Organization)
    {
        LUrl += fmt::format(L"/orgs/{}/repos", DestinationApplication->User);
    }
    else
    {
        LUrl += L"/user/repos";
    }

    std::wstring LJson;
    RepoToJson(*ASourceRepository, LJson);

    std::wstring LAnswer;
    try
    {
        PrepareRequest(*DestinationApplication);
        auto SourceFile = std::make_unique<System::Classes::TMemoryStream>();
        Idglobal::WriteStringToStream(SourceFile.get(), LJson.c_str(), IndyTextEncoding_UTF8());
        SourceFile->Position = 0;
        FHTTPClient->Request->ContentType = "application/json";
        LAnswer = FHTTPClient->Post(LUrl.c_str(), SourceFile.get()).c_str();
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
            LUrl = fmt::format(L"{}/repos/{}/{}", DestinationApplication->ApiUrl,
                DestinationApplication->User, ASourceRepository->Name);
            try
            {
                LAnswer = FHTTPClient->Get(LUrl.c_str()).c_str();
            }
            catch(...)
            {
            }
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

    TJSONObject* LObject = dynamic_cast<TJSONObject*>(TJSONObject::ParseJSONValue(String(LAnswer.c_str())));
    if(LObject != nullptr)
    {
        TJSONPair* Pair;
        if((Pair = LObject->Get("full_name")) != nullptr)
        {
            JsonToRepo(LAnswer, ADestinationRepository);

            const std::wstring LLog = fmt::format(L"Created repository {} on {}",
                ADestinationRepository->FullName, DestinationApplication->ApplicationName);
            memoLog->Lines->Add(LLog.c_str());
            Result = true;
        }
        else if((Pair = LObject->Get("message")) != nullptr)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            const String LLog = "Repository creation message: " + Answer->Value();
            memoLog->Lines->Add(LLog);
        }
    }

    return Result;
}
//---------------------------------------------------------------------------

std::wstring __fastcall TForm2::GetAuthenticatedUser(const TGitApplication& AGitApplication)
{
    std::wstring LJson;
    const std::wstring LUrl = AGitApplication.ApiUrl + L"/user";

    PrepareRequest(AGitApplication);
    LJson = FHTTPClient->Get(LUrl.c_str()).c_str(); // May throw exception

    if(LJson.empty() == true)
    {
        return L"";
    }

    auto LUser = std::make_unique<TUser>();
    JsonToUser(LJson, LUser.get());

    return LUser->Login;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::GetOrganizations(TGitApplication* AGitApplication,
    System::Classes::TStrings* AItems)
{
    std::wstring LJson;
    const std::wstring LUrl = AGitApplication->ApiUrl + L"/user/orgs";

    AItems->Clear();

    PrepareRequest(*AGitApplication);
    try
    {
        LJson = FHTTPClient->Get(LUrl.c_str()).c_str(); // May throw exception
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        if(e.ErrorCode != 404)
        {   // GitBucket does not support this service
            // So error 404 is normal
            throw;
        }
    }

    TJSONArray* LOrgs = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(String(LJson.c_str())));
    if(LOrgs != nullptr)
    {
        TJSONArray::TEnumerator* LOrgsEnumerator = LOrgs->GetEnumerator();
        while(LOrgsEnumerator->MoveNext() == true)
        {
            TJSONObject* LOrg = static_cast<TJSONObject*>(LOrgsEnumerator->Current);

            auto Org = std::make_unique<TOrganization>();
            JsonToOrganization(LOrg, Org.get());

            AItems->AddObject(Org->Login.c_str(), nullptr);
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::PrepareRequest(const TGitApplication& AGitApplication)
{
    // Set authorization token
    FHTTPClient->Request->CustomHeaders->Values["Authorization"] =
        fmt::format(L"token {}", AGitApplication.Token).c_str();
}
//---------------------------------------------------------------------------

HANDLE __fastcall TForm2::ExecuteProgramEx(const std::wstring ACmd, const std::wstring ADirectory)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    si.cb = sizeof(si);
    si.lpReserved = nullptr;
    si.lpDesktop = nullptr;
    si.lpTitle = const_cast<LPWSTR>(L"Git");
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.cbReserved2 = 0;
    si.lpReserved2 = nullptr;
    si.hStdInput = nullptr;
    si.hStdOutput = nullptr;
    si.hStdError = nullptr;

    ZeroMemory(&pi, sizeof(pi));

    std::vector<wchar_t> LCmd(ACmd.c_str(), ACmd.c_str() + ACmd.size() + 1);

    bool ProcResult = ::CreateProcess(nullptr, &LCmd[0], nullptr, nullptr, true, 0,
        nullptr, ADirectory.c_str(), &si, &pi);
    if(ProcResult == true)
    {
        return pi.hProcess;
    }

    if(pi.hProcess != nullptr)
    {
        CloseHandle(pi.hProcess);
    }
    if(pi.hThread != nullptr)
    {
        CloseHandle(pi.hThread);
    }
    if(si.hStdOutput != nullptr)
    {
        CloseHandle(si.hStdOutput);
    }
    return nullptr;
}
//---------------------------------------------------------------------------

DWORD __fastcall TForm2::Wait(HANDLE AHandle)
{
    DWORD Result;

    if(AHandle == nullptr || AHandle == INVALID_HANDLE_VALUE)
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

void __fastcall TForm2::Clone(const std::wstring ADirectory, const std::wstring AGitRepo, bool AIsBare)
{
    std::wstring LCmd = fmt::format(L"git clone {} {}{}",
        AGitRepo, ADirectory, (AIsBare == true) ? L" --bare" : L"");
    HANDLE LHandle = ExecuteProgramEx(LCmd);
    DWORD LExitCode = Wait(LHandle);
    if(LExitCode != 0)
    {
        throw Exception("Clone command failed");
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::AddRemote(const std::wstring AGitRepo, const std::wstring ADirectory)
{
    const std::wstring LCmd = fmt::format(L"git remote add origin2 {}", AGitRepo);
    HANDLE LHandle = ExecuteProgramEx(LCmd, ADirectory);
    DWORD LExitCode = Wait(LHandle);
    if(LExitCode != 0)
    {
        throw Exception("Add remote command failed");
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Push(const std::wstring ADirectory)
{
    const std::wstring LCmd = L"git push origin2 --mirror";
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
    return (ExecuteProgramEx(L"git --version") != nullptr);
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

void __fastcall TForm2::ShowMessage(const std::wstring AMessage)
{
    Rectangle1->Visible = true;
    Rectangle1->Align = TAlignLayout::Contents;
    lblErrorMessage->Text = AMessage.c_str();
}
//---------------------------------------------------------------------------

void __fastcall TForm2::HideMessage()
{
    Rectangle1->Visible = false;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::OnApplicationIdle(System::TObject* Sender, bool &Done)
{
    const int LTabAction = FTabAction;
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
        ShowMessage(L"git.exe not found in path!\n\nCorrect the problem and try again.");
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
    SourceApplication->ApiUrl = txtSourceUrl->Text.c_str();
    SourceApplication->Token = txtSourceToken->Text.c_str();
    SourceApplication->Username = txtSourceUsername->Text.c_str();
    SourceApplication->Password = txtSourcePassword->Text.c_str();

    std::wstring LUser;
    std::wstring LExceptionMsg;
    try
    {
        LUser = GetAuthenticatedUser(*SourceApplication);
    }
    catch(const Exception& e)
    {
        LExceptionMsg = e.Message.c_str();
    }
    if(LUser.empty() == true)
    {
        btnSourceOwnerBack->Enabled = true;
        std::wstring LMessage = L"Cannot get authenticated user!";
        if(LExceptionMsg.empty() == false)
        {
            LMessage += std::wstring(EOL) + std::wstring(EOL) + LExceptionMsg;
        }
        LMessage += std::wstring(EOL) + std::wstring(EOL) + std::wstring(L"Go Back, change the settings and try again.");
        ShowMessage(LMessage);

        return;
    }

    cboeSourceUser->Items->Clear();
    cboeSourceUser->Items->AddObject(LUser.c_str(), nullptr);
    cboeSourceUser->ItemIndex = 0;
    cboeSourceUser->StyleLookup = "comboeditstyle";

    try
    {
        GetOrganizations(SourceApplication, cboeSourceName->Items);
    }
    catch(const Exception& e)
    {
        btnSourceOwnerBack->Enabled = true;
        const std::wstring LMessage = fmt::format(
            L"Cannot get organizations list!\r\n\r\n"
            L"{}\r\n\r\n"
            L"Go Back, change the settings and try again.",
            e.Message.c_str());
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
    std::wstring LUrl = SourceApplication->ApiUrl;
    if(chkSourceTypeOrg->IsChecked == true)
    {
        SourceApplication->Endpoint = TApiEndpoint::Organization;
        SourceApplication->User = cboeSourceName->Text.c_str();

        LUrl += fmt::format(L"/orgs/{}/repos", cboeSourceName->Text.c_str());
    }
    else
    {
        SourceApplication->Endpoint = TApiEndpoint::User;
        SourceApplication->User = cboeSourceUser->Text.c_str();

        if(cboeSourceUser->Items->Count > 0 &&
            cboeSourceUser->Items->Strings[0] == cboeSourceUser->Text)
        {
            LUrl += L"/user/repos";
        }
        else
        {
            LUrl += fmt::format(L"/users/{}/repos", cboeSourceUser->Text.c_str());
        }
    }

    std::wstring LExceptionMsg;
    try
    {
        while(LUrl.empty() == false)
        {
            PrepareRequest(*SourceApplication);
            const String LContent = FHTTPClient->Get(LUrl.c_str());

            TJSONArray* LRepos = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
            if(LRepos != nullptr)
            {
                TJSONArray::TEnumerator* LRepoEnumerator = LRepos->GetEnumerator();
                while(LRepoEnumerator->MoveNext() == true)
                {
                    TJSONObject* LRepo = static_cast<TJSONObject*>(LRepoEnumerator->Current);

                    const std::wstring LSourceJson = LRepo->ToString().c_str();
                    TRepository* LSourceRepository = new TRepository();
                    JsonToRepo(LSourceJson, LSourceRepository);

                    if(SourceApplication->Endpoint == TApiEndpoint::User &&
                        LSourceRepository->Owner->Login != SourceApplication->User)
                    {
                        delete LSourceRepository;
                        continue;
                    }

                    TListBoxRepositoryItem* LListBoxItem = new TListBoxRepositoryItem(this);
                    LListBoxItem->Parent = ListBoxRepo;
                    LListBoxItem->IsChecked = true;
                    LListBoxItem->Text = LSourceRepository->FullName.c_str();
                    LListBoxItem->Repository = LSourceRepository; // List box is owner of memory
                    if(LSourceRepository->Private == true)
                    {   // Private
                        LListBoxItem->ImageIndex = 0;
                    }
                    else
                    {
                        if(LSourceRepository->MirrorUrl.empty() == false)
                        {   // Mirror
                            LListBoxItem->ImageIndex = 3;
                        }
                        else if(LSourceRepository->Fork == true)
                        {   // Fork
                            LListBoxItem->ImageIndex = 2;
                        }
                        else
                        {   // Public
                            LListBoxItem->ImageIndex = 1;
                        }
                    }
                    if(LSourceRepository->Description.empty() == false)
                    {
                        LListBoxItem->ItemData->Detail = LSourceRepository->Description.c_str();
                        LListBoxItem->Height = 50.0f;
                        LListBoxItem->StyleLookup = "listboxitembottomdetail";
                    }
                    else
                    {
                        LListBoxItem->Height = 40.0f;
                        LListBoxItem->StyleLookup = "listboxitemnodetail";
                    }

                    Application->ProcessMessages();
                }
            }

            LUrl = GetNextUrl();
        }
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        LExceptionMsg = std::wstring(L"Get repository HTTP protocol exception: ") + e.Message.c_str();
    }
    catch(const Idstack::EIdSocketError& e)
    {
        LExceptionMsg = std::wstring(L"Get repository socket exception: ") + e.Message.c_str();
    }
    catch(const Exception& e)
    {
        LExceptionMsg = std::wstring(L"Get repository exception: ") + e.Message.c_str();
    }

    btnRepoBack->Enabled = true;

    if(LExceptionMsg.empty() == false)
    {
        ShowMessage(LExceptionMsg + L"\n\nGo Back, change the settings and try again.");
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
    DestinationApplication->ApiUrl = txtDestinationUrl->Text.c_str();
    DestinationApplication->Token = txtDestinationToken->Text.c_str();
    DestinationApplication->Username = txtDestinationUsername->Text.c_str();
    DestinationApplication->Password = txtDestinationPassword->Text.c_str();

    std::wstring LUser;
    std::wstring LExceptionMsg;
    try
    {
        LUser = GetAuthenticatedUser(*DestinationApplication);
    }
    catch(const Exception& e)
    {
        LExceptionMsg = e.Message.c_str();
    }
    if(LUser.empty() == true)
    {
        btnDestinationOwnerBack->Enabled = true;
        std::wstring LMessage = L"Cannot get authenticated user!";
        if(LExceptionMsg.empty() == false)
        {
            LMessage += std::wstring(EOL) + std::wstring(EOL) + LExceptionMsg;
        }
        LMessage += std::wstring(EOL) + std::wstring(EOL) + L"Go Back, change the settings and try again.";
        ShowMessage(LMessage);

        return;
    }

    chkDestinationTypeUser->Text = fmt::format(L"User ({})", LUser.c_str()).c_str();
    chkDestinationTypeUser->TagString = LUser.c_str();

    try
    {
        GetOrganizations(DestinationApplication, cboeDestinationName->Items);
    }
    catch(const Exception& e)
    {
        btnDestinationOwnerBack->Enabled = true;
        const std::wstring LMessage = fmt::format(
            L"Cannot get organizations list!\r\n\r\n"
            L"{}\r\n\r\n"
            L"Go Back, change the settings and try again.",
            e.Message.c_str());
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
        DestinationApplication->User = cboeDestinationName->Text.c_str();
    }
    else
    {
        DestinationApplication->Endpoint = TApiEndpoint::User;
        DestinationApplication->User = chkDestinationTypeUser->TagString.c_str();
    }

    const int LCount = ListBoxRepo->Items->Count;
    for(int i = 0; i < LCount; ++i)
    {
        TListBoxRepositoryItem* LItem = static_cast<TListBoxRepositoryItem*>(ListBoxRepo->ItemByIndex(i));
        if(LItem->IsChecked == false)
        {
            continue;
        }

        try
        {
            auto LDestinationRepository = std::make_unique<TRepository>();
            TRepository *LSourceRepository = LItem->Repository;

            const std::wstring LLog = fmt::format(L"====== {} ======",
                LSourceRepository->FullName);
            memoLog->Lines->Add(LLog.c_str());

            bool LIsCreated = CreateRepo(LSourceRepository, LDestinationRepository.get());

            if(LIsCreated == false)
            {   // Don't do the rest if the issue was not created
                continue;
            }

            if(LSourceRepository->OpenIssueCount > 0)
            {
                const std::wstring LIssueLog =
                    fmt::format(L"{} open issue(s) not created!", LSourceRepository->OpenIssueCount);
                memoLog->Lines->Add(LIssueLog.c_str());
                PrintIssues(*SourceApplication, *LSourceRepository);
            }

            try
            {
                std::wstring LSourceUrl = LSourceRepository->CloneUrl;
                if(SourceApplication->Username.empty() == false &&
                    SourceApplication->Password.empty() == false)
                {
                    const std::wstring LSourceCredential = fmt::format(L"$1://{}:{}@",
                        SourceApplication->Username, SourceApplication->Password);
                    LSourceUrl = std::regex_replace(LSourceUrl,
                        std::wregex(L"(http|https)://", std::regex_constants::icase),
                        LSourceCredential);
                }

                Clone(L"temp.git", LSourceUrl, true);

                std::wstring LDestinationUrl = LDestinationRepository->CloneUrl;
                if(DestinationApplication->Username.empty() == false &&
                    DestinationApplication->Password.empty() == false)
                {
                    const std::wstring LDestinationCredential = fmt::format(L"$1://{}:{}@",
                        DestinationApplication->Username, DestinationApplication->Password);
                    LDestinationUrl = std::regex_replace(LDestinationUrl,
                        std::wregex(L"(http|https)://", std::regex_constants::icase),
                        LDestinationCredential);
                }

                AddRemote(LDestinationUrl, L"temp.git");
                Push(L"temp.git");

                memoLog->Lines->Add("Pushed repository");

                if(LSourceRepository->HasWiki == true)
                {
                    try
                    {
                        const std::wstring LSourceWikiUrl = std::regex_replace(
                            LSourceUrl.c_str(),
                            std::wregex(L".git", std::regex_constants::icase),
                            L".wiki.git");
                        const std::wstring LCDestinationWikiUrl = std::regex_replace(
                            LDestinationUrl.c_str(),
                            std::wregex(L".git", std::regex_constants::icase),
                            L".wiki.git");

                        Ioutils::TDirectory::Delete("temp.git", true);

                        Clone(L"temp.git", LSourceWikiUrl, true);
                        AddRemote(LCDestinationWikiUrl, L"temp.git");
                        Push(L"temp.git");
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

void __fastcall TListBoxDataItem::DoApplyStyleLookup()
{
    try
    {
        BeginUpdate();

        Fmx::Types::TFmxObject* GlyphStyleResource = FindStyleResource("glyphstyle");
        if(GlyphStyleResource != nullptr && GlyphStyleResource->ClassNameIs("TGlyph") == true)
        {
            TGlyph* LGlyph = static_cast<TGlyph*>(GlyphStyleResource);
            LGlyph->Position->X = 20.0f; // Make sure checkbox is really MostLeft
        }

        Fmx::Types::TFmxObject* IconStyleResource = FindStyleResource("icon");
        if(IconStyleResource != nullptr && IconStyleResource->ClassNameIs("TImage") == true)
        {
            TImage* LImage = static_cast<TImage*>(IconStyleResource);
            LImage->Position->X = 20.0f; // Make sure checkbox is really MostLeft
        }

        Fmx::Types::TFmxObject* CheckStyleResource = FindStyleResource("check");
        if(CheckStyleResource != nullptr && CheckStyleResource->ClassNameIs("TCheckBox") == true)
        {
            TCheckBox* LCheckBox = static_cast<TCheckBox*>(CheckStyleResource);
            LCheckBox->Position->X = 0.0f;
        }
    }
    __finally
    {
        EndUpdate();
    }

    inherited::DoApplyStyleLookup();
}
//---------------------------------------------------------------------------

__fastcall TListBoxRepositoryItem::~TListBoxRepositoryItem()
{
    delete Repository;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::PrintIssues(const TGitApplication& AGitApplication, const TRepository& ARepository)
{
    const std::wstring LUrl = fmt::format(L"{}/repos/{}/{}/issues",
        AGitApplication.ApiUrl, AGitApplication.User, ARepository.Name);
    try
    {
        PrepareRequest(AGitApplication);
        const String LContent = FHTTPClient->Get(LUrl.c_str());

        TJSONArray* LIssues = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
        if(LIssues != nullptr)
        {
            TJSONArray::TEnumerator* LIssueEnumerator = LIssues->GetEnumerator();
            while(LIssueEnumerator->MoveNext() == true)
            {
                TJSONObject* LIssue = static_cast<TJSONObject*>(LIssueEnumerator->Current);

                auto LIssueToPrint = std::make_unique<TIssue>();
                JsonToIssue(LIssue, LIssueToPrint.get());

                const std::wstring LLog = fmt::format(L"    Issue #{}: {}", LIssueToPrint->Number, LIssueToPrint->Title);
                memoLog->Lines->Add(LLog.c_str());
            }
        }
    }
    catch(...)
    {
    }
}
//---------------------------------------------------------------------------

std::wstring __fastcall TForm2::GetNextUrl()
{
    std::wstring Result;
    const std::wstring LLink = FHTTPClient->Response->RawHeaders->Values["Link"].c_str();
    if(LLink.empty() == false)
    {
        std::wcmatch LMatchResults;
        const auto LExpression = std::wregex(L"<([^ ]+)>; rel=\"next\"", std::regex_constants::extended);
        if(std::regex_search(LLink.c_str(), LMatchResults, LExpression,
            std::regex_constants::match_default) == true)
        {
            Result = LMatchResults[1];
        }
    }
    return Result;
}
//---------------------------------------------------------------------------

