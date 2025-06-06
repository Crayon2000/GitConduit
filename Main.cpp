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
#include <memory>
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

    String LUrl = DestinationApplication->ApiUrl;

    if(DestinationApplication->Endpoint == TApiEndpoint::Organization)
    {
        LUrl += "/orgs/" + DestinationApplication->User + "/repos";
    }
    else
    {
        LUrl += "/user/repos";
    }

    String LJson;
    RepoToJson(*ASourceRepository, LJson);

    String LAnswer;
    try
    {
        PrepareRequest(*DestinationApplication);
        auto SourceFile = std::make_unique<System::Classes::TMemoryStream>();
        WriteStringToStream(SourceFile.get(), LJson, enUTF8);
        SourceFile->Position = 0;
        FHTTPClient->Request->ContentType = "application/json";
        LAnswer = FHTTPClient->Post(LUrl, SourceFile.get());
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
            LUrl = DestinationApplication->ApiUrl + "/repos/" +
                DestinationApplication->User + "/" + ASourceRepository->Name;

            try
            {
                LAnswer = FHTTPClient->Get(LUrl);
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

    TJSONObject* LObject = dynamic_cast<TJSONObject*>(TJSONObject::ParseJSONValue(LAnswer));
    if(LObject != nullptr)
    {
        TJSONPair* Pair;
        if((Pair = LObject->Get("full_name")) != nullptr)
        {
            JsonToRepo(LAnswer, ADestinationRepository);

            const String LLog = "Created repository " + ADestinationRepository->FullName +
                " on " + DestinationApplication->ApplicationName;
            memoLog->Lines->Add(LLog);
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

String __fastcall TForm2::GetAuthenticatedUser(const TGitApplication& AGitApplication)
{
    String LJson;
    const String LUrl = AGitApplication.ApiUrl + "/user";

    PrepareRequest(AGitApplication);
    LJson = FHTTPClient->Get(LUrl); // May throw exception

    if(LJson.IsEmpty() == true)
    {
        return "";
    }

    auto LUser = std::make_unique<TUser>();
    JsonToUser(LJson, LUser.get());

    return LUser->Login;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::GetOrganizations(TGitApplication* AGitApplication,
    System::Classes::TStrings* AItems)
{
    String LJson;
    const String LUrl = AGitApplication->ApiUrl + "/user/orgs";

    AItems->Clear();

    PrepareRequest(*AGitApplication);
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
    if(LOrgs != nullptr)
    {
        TJSONArray::TEnumerator* LOrgsEnumerator = LOrgs->GetEnumerator();
        while(LOrgsEnumerator->MoveNext() == true)
        {
            TJSONObject* LOrg = static_cast<TJSONObject*>(LOrgsEnumerator->Current);

            auto Org = std::make_unique<TOrganization>();
            JsonToOrganization(LOrg, Org.get());

            AItems->AddObject(Org->Login, nullptr);
        }
    }
}
//---------------------------------------------------------------------------

void __fastcall TForm2::PrepareRequest(const TGitApplication& AGitApplication)
{
    // Set authorization token
    FHTTPClient->Request->CustomHeaders->Values["Authorization"] =
        "token " + AGitApplication.Token;
}
//---------------------------------------------------------------------------

HANDLE __fastcall TForm2::ExecuteProgramEx(const String ACmd, String& AOut, String& AErr, const String ADirectory)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    SECURITY_ATTRIBUTES saAttr;
    HANDLE hStdOutRead, hStdOutWrite;
    HANDLE hStdErrRead, hStdErrWrite;

    // Set up security attributes to allow handle inheritance
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = nullptr;

    // Create pipes for stdout and stderr
    CreatePipe(&hStdOutRead, &hStdOutWrite, &saAttr, 0);
    CreatePipe(&hStdErrRead, &hStdErrWrite, &saAttr, 0);

    // Ensure the read handles to the pipes are not inherited
    SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(hStdErrRead, HANDLE_FLAG_INHERIT, 0);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.lpReserved = nullptr;
    si.lpDesktop = nullptr;
    si.lpTitle = const_cast<LPWSTR>(L"Git");
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.cbReserved2 = 0;
    si.lpReserved2 = nullptr;
    si.hStdInput = nullptr;
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdErrWrite;

    ZeroMemory(&pi, sizeof(pi));

    bool ProcResult = CreateProcess(nullptr, ACmd.c_str(), nullptr, nullptr, true, 0,
        nullptr, ADirectory.c_str(), &si, &pi);
    if(ProcResult == true)
    {
        // Close the write ends of the pipes so the child process can exit
        CloseHandle(hStdOutWrite);
        CloseHandle(hStdErrWrite);

        // Read from stdout and stderr pipes
        char buffer[4096];
        DWORD bytesRead;

        while(ReadFile(hStdOutRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            AOut += String(buffer);
        }
        AOut = AOut.Trim();

        while(ReadFile(hStdErrRead, buffer, sizeof(buffer) - 1, &bytesRead, nullptr) && bytesRead > 0)
        {
            buffer[bytesRead] = '\0';
            AErr += String(buffer);
        }
        AErr = AErr.Trim();

        CloseHandle(hStdOutRead);
        CloseHandle(hStdErrRead);

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
    if(si.hStdError != nullptr)
    {
        CloseHandle(si.hStdError);
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

bool __fastcall TForm2::CheckGitExe()
{
    const String LCmd = "gitconduit-cli";
    String LOut;
    return (ExecuteProgramEx(LCmd, LOut, LOut) != nullptr);
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
        ShowMessage("gitconduit-cli.exe not found in path!\n\nCorrect the problem and try again.");
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
        LUser = GetAuthenticatedUser(*SourceApplication);
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
    cboeSourceUser->Items->AddObject(LUser, nullptr);
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
            PrepareRequest(*SourceApplication);
            const String LContent = FHTTPClient->Get(LUrl);

            TJSONArray* LRepos = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
            if(LRepos != nullptr)
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

                    TListBoxItem* LListBoxItem = new TListBoxDataItem(this);
                    LListBoxItem->Parent = ListBoxRepo;
                    LListBoxItem->IsChecked = true;
                    LListBoxItem->Text = LSourceRepository->FullName;
                    LListBoxItem->Data = LSourceRepository; // List box is owner of memory
                    if(LSourceRepository->Private == true)
                    {   // Private
                        LListBoxItem->ImageIndex = 0;
                    }
                    else
                    {
                        if(LSourceRepository->MirrorUrl.IsEmpty() == false)
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
        LUser = GetAuthenticatedUser(*DestinationApplication);
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

        try
        {
            auto LDestinationRepository = std::make_unique<TRepository>();
            TRepository *LSourceRepository = static_cast<TRepository*>(LItem->Data);

            const String LLog = String().sprintf(L"====== %s ======",
                LSourceRepository->FullName.c_str());
            memoLog->Lines->Add(LLog);

            bool LIsCreated = CreateRepo(LSourceRepository, LDestinationRepository.get());

            if(LIsCreated == false)
            {   // Don't do the rest if the issue was not created
                continue;
            }

            if(LSourceRepository->OpenIssueCount > 0)
            {
                const String LIssueLog =
                    String().sprintf(L"%d open issue(s) not created!", LSourceRepository->OpenIssueCount);
                memoLog->Lines->Add(LIssueLog);
                PrintIssues(*SourceApplication, *LSourceRepository);
            }

            const String LCmd = String().sprintf(
                L"gitconduit-cli cloneandpush --sourcerepo=%s --sourceusername=%s --sourcepassword=%s --destrepo=%s --destusername=%s --destpassword=%s --haswiki=%s",
                LSourceRepository->CloneUrl.c_str(), SourceApplication->Username.c_str(), SourceApplication->Password.c_str(),
                LDestinationRepository->CloneUrl.c_str(), DestinationApplication->Username.c_str(), DestinationApplication->Password.c_str(),
                LSourceRepository->HasWiki ? L"true" : L"false");
            String LOut;
            String LErr;
            HANDLE LHandle = ExecuteProgramEx(LCmd, LOut, LErr);
            DWORD LExitCode = Wait(LHandle);
            if(LExitCode == 0)
            {
                memoLog->Lines->Add("Pushed repository");
            }
            else
            {
                if(LOut.IsEmpty() == false)
                {
                    memoLog->Lines->Add(LOut);
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

void __fastcall TForm2::PrintIssues(const TGitApplication& AGitApplication, const TRepository& ARepository)
{
    const String LUrl = AGitApplication.ApiUrl + "/repos/" +
        AGitApplication.User + "/" + ARepository.Name + "/issues";
    try
    {
        PrepareRequest(AGitApplication);
        const String LContent = FHTTPClient->Get(LUrl);

        TJSONArray* LIssues = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
        if(LIssues != nullptr)
        {
            TJSONArray::TEnumerator* LIssueEnumerator = LIssues->GetEnumerator();
            while(LIssueEnumerator->MoveNext() == true)
            {
                TJSONObject* LIssue = static_cast<TJSONObject*>(LIssueEnumerator->Current);

                auto LIssueToPrint = std::make_unique<TIssue>();
                JsonToIssue(LIssue, LIssueToPrint.get());

                const String LLog = String().sprintf(L"    Issue #%d: %s", LIssueToPrint->Number, LIssueToPrint->Title.c_str());
                memoLog->Lines->Add(LLog);
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

