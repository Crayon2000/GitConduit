//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include "GitApplication.h"
#include "GitRepository.h"
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

    btnSourceNext->TagObject = TabItemRepo;
    btnRepoNext->TagObject = TabItemDestination;
    btnDestinationNext->TagObject = TabItemCreate;
    btnRepoBack->TagObject = TabItemSource;
    btnDestinationBack->TagObject = TabItemRepo;
    btnCreateRepoBack->TagObject = TabItemDestination;

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
    chkSourceTypeUser->IsChecked = true;
    chkDestinationTypeUser->GroupName = "Destination";
    chkDestinationTypeOrg->GroupName = "Destination";
    chkDestinationTypeUser->IsChecked = true;

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

    PrepareRequest(AGitApplication);
    LJson = IdHTTP1->Get(LUrl); // May throw exception

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
        case 0:
            btnSourceNext->Enabled = false;
            break;
        case 1:
            btnRepoNext->Enabled = false;
            btnRepoBack->Enabled = false;

            ListBoxRepo->Clear();
            break;
        case 2:
            btnDestinationNext->Enabled = false;
            btnDestinationBack->Enabled = false;
            break;
        case 3:
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
        case 0:
            ActionSource();
            break;
        case 1:
            ActionRepositories();
            break;
        case 2:
            ActionDestination();
            break;
        case 3:
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

void __fastcall TForm2::ActionRepositories()
{
    const TGitApplicationType LSourceType =
        static_cast<TGitApplicationType>((unsigned char)cboSourceApp->Selected->Data);
    SourceApplication->ApplicationType = LSourceType;
    SourceApplication->ApiUrl = txtSourceUrl->Text;
    SourceApplication->Token = txtSourceToken->Text;
    SourceApplication->Username = txtSourceUsername->Text;
    SourceApplication->Password = txtSourcePassword->Text;

    String LUrl = SourceApplication->ApiUrl;
    if(chkSourceTypeOrg->IsChecked == true)
    {
        LUrl += "/orgs/" + txtSourceName->Text + "/repos";

        SourceApplication->Endpoint = TApiEndpoint::Organization;
        SourceApplication->User = txtSourceName->Text;
    }
    else
    {
        LUrl += "/user/repos";

        SourceApplication->Endpoint = TApiEndpoint::User;
        SourceApplication->User = "";

        String LExceptionMsg;
        try
        {
            SourceApplication->User = GetAuthenticatedUser(SourceApplication);
        }
        catch(const Exception& e)
        {
            LExceptionMsg = e.Message;
        }
        if(SourceApplication->User.IsEmpty() == true)
        {
            btnRepoBack->Enabled = true;
            String LMessage = "Cannot get authenticated user!";
            if(LExceptionMsg.IsEmpty() == false)
            {
                LMessage += String(EOL) + String(EOL) + LExceptionMsg;
            }
            LMessage += String(EOL) + String(EOL) + "Go Back, change the settings and try again.";
            ShowMessage(LMessage);

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

                const String LSourceJson = LRepo->ToString();
                TRepository LSourceRepository;
                JsonToRepo(LSourceJson, LSourceRepository);

                if(SourceApplication->Endpoint == TApiEndpoint::User &&
                    LSourceRepository.Owner.Login != SourceApplication->User)
                {
                    continue;
                }

                TListBoxItem* LListBoxItem = new TListBoxItem(this);
                LListBoxItem->Parent = ListBoxRepo;
                LListBoxItem->IsChecked = true;
                LListBoxItem->Text = LSourceRepository.FullName;
                LListBoxItem->TagString = LSourceJson;
                LListBoxItem->OnApplyStyleLookup = ListBoxItemApplyStyleLookup;
                if(LSourceRepository.Private == true)
                {   // Private
                    LListBoxItem->ImageIndex = 0;
                }
                else
                {
                    if(LSourceRepository.Fork == false)
                    {   // Public
                        LListBoxItem->ImageIndex = 1;
                    }
                    else
                    {   // Fork
                        LListBoxItem->ImageIndex = 2;
                    }
                }
                if(LSourceRepository.Description.IsEmpty() == false)
                {
                    LListBoxItem->ItemData->Detail = LSourceRepository.Description;
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
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        btnRepoBack->Enabled = true;
        ShowMessage("Get repository exception: " + e.Message + "\n\nGo Back, change the settings and try again.");
        return;
    }

    btnRepoNext->Enabled = true;
    btnRepoBack->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionDestination()
{
    btnDestinationNext->Enabled = true;
    btnDestinationBack->Enabled = true;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::ActionCreateRepo()
{
    const TGitApplicationType LDestinationype =
        static_cast<TGitApplicationType>((unsigned char)cboDestinationApp->Selected->Data);
    DestinationApplication->ApplicationType = LDestinationype;
    DestinationApplication->ApiUrl = txtDestinationUrl->Text;
    DestinationApplication->Token = txtDestinationToken->Text;
    DestinationApplication->Username = txtDestinationUsername->Text;
    DestinationApplication->Password = txtDestinationPassword->Text;

    if(chkDestinationTypeOrg->IsChecked == true)
    {
        DestinationApplication->Endpoint = TApiEndpoint::Organization;
        DestinationApplication->User = txtDestinationName->Text;
    }
    else
    {
        DestinationApplication->Endpoint = TApiEndpoint::User;
        DestinationApplication->User = "";

        String LExceptionMsg;
        try
        {
            DestinationApplication->User = GetAuthenticatedUser(DestinationApplication);
        }
        catch(const Exception& e)
        {
            LExceptionMsg = e.Message;
        }
        if(DestinationApplication->User.IsEmpty() == true)
        {
            btnCreateRepoBack->Enabled = true;

            String LMessage = "Cannot get authenticated user!";
            if(LExceptionMsg.IsEmpty() == false)
            {
                LMessage += String(EOL) + String(EOL) + LExceptionMsg;
            }
            LMessage += String(EOL) + String(EOL) + "Go Back, change the settings and try again.";
            ShowMessage(LMessage);

            return;
        }
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
            const String LSourceJson = LItem->TagString;

            TRepository LDestinationRepository;

            TRepository LSourceRepository;
            JsonToRepo(LSourceJson, LSourceRepository);

            const String LLog = String().sprintf(L"====== %s ======",
                LSourceRepository.FullName.c_str());
            memoLog->Lines->Add(LLog);

            bool LIsCreated = CreateRepo(LSourceJson, LDestinationRepository);

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
                String LSourceUrl = LSourceRepository.CloneUrl;
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

                Clone(LSourceUrl);

                String LDestinationUrl = LDestinationRepository.CloneUrl;
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

                if(LSourceRepository.HasWiki == true)
                {
                    try
                    {
                        const String LSourceWikiUrl = StringReplace(
                            LSourceUrl, ".git", ".wiki.git", TReplaceFlags());
                        const String LCDestinationWikiUrl = StringReplace(
                            LDestinationUrl, ".git", ".wiki.git", TReplaceFlags());

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

