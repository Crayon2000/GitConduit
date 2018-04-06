//---------------------------------------------------------------------------
#ifndef MainH
#define MainH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <FMX.Controls.hpp>
#include <FMX.Forms.hpp>
#include <FMX.Controls.Presentation.hpp>
#include <FMX.StdCtrls.hpp>
#include <FMX.Types.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdHTTP.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <FMX.Edit.hpp>
#include <FMX.Memo.hpp>
#include <FMX.ScrollBox.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.TabControl.hpp>
#include <FMX.Objects.hpp>
//---------------------------------------------------------------------------
class TGitApplication;

class TOwner
{
public:
    String Login;
};

class TRepository
{
public:
    TOwner Owner;
    String Name;
    String FullName;
    bool Private;
    String Description;
    String CloneUrl;
    int OpenIssueCount;
};

class TForm2 : public TForm
{
__published:    // IDE-managed Components
    TIdHTTP *IdHTTP1;
    TIdSSLIOHandlerSocketOpenSSL *IdSSLIOHandlerSocketOpenSSL1;
    TMemo *memoLog;
    TGroupBox *GroupBox4;
    TTabControl *TabControl1;
    TTabItem *TabItemSource;
    TTabItem *TabItemDestination;
    TSpeedButton *btnSourceNext;
    TLayout *Layout2;
    TRectangle *Rectangle1;
    TRectangle *Rectangle2;
    TLabel *lblErrorTitle;
    TLabel *lblErrorMessage;
    TLayout *Layout3;
    TSpeedButton *btnDestinationBack;
    TSpeedButton *btnDestinationNext;
    TTabItem *TabItemRepo;
    TLayout *Layout4;
    TSpeedButton *btnRepoBack;
    TSpeedButton *btnRepoNext;
    TListBox *ListBoxRepo;
    TLabel *Label9;
    TLabel *Label10;
    TLabel *Label11;
    TLabel *Label12;
    TLayout *Layout5;
    TButton *btnErrorOk;
    TTabItem *TabItemCreate;
    TLabel *Label13;
    TLayout *Layout6;
    TSpeedButton *btnCreateRepoBack;
    TSpeedButton *btnCreateRepoClose;
    TComboBox *cboSourceApp;
    TRadioButton *chkSourceTypeOrg;
    TRadioButton *chkSourceTypeUser;
    TLabel *Label1;
    TLabel *Label2;
    TLabel *Label5;
    TLabel *Label7;
    TEdit *txtSourceName;
    TEdit *txtSourceToken;
    TEdit *txtSourceUrl;
    TComboBox *cboDestinationApp;
    TRadioButton *chkDestinationTypeOrg;
    TRadioButton *chkDestinationTypeUser;
    TLabel *Label3;
    TLabel *Label4;
    TLabel *Label6;
    TLabel *Label8;
    TEdit *txtDestinationName;
    TEdit *txtDestinationToken;
    TEdit *txtDestinationUrl;
    void __fastcall TabControl1Change(TObject *Sender);
    void __fastcall WizardButtonClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall btnErrorOkClick(TObject *Sender);
    void __fastcall btnCreateRepoCloseClick(TObject *Sender);
private:    // User declarations
    TGitApplication* SourceApplication;
    TGitApplication* DestinationApplication;
    int FTabAction;

    void __fastcall ActionSource();
    void __fastcall ActionRepositories();
    void __fastcall ActionDestination();
    void __fastcall ActionCreateRepo();
protected:
    bool __fastcall CreateRepo(const String AJson, TRepository& ARepository);
    String __fastcall GetAuthenticatedUser(TGitApplication* AGitApplication);
    void __fastcall PrepareRequest(TGitApplication* AGitApplication);
    HANDLE __fastcall ExecuteProgramEx(const String ACmd, const String ADirectory = ".");
    DWORD __fastcall Wait(HANDLE AHandle);
    void __fastcall Clone(const String AGitRepo);
    void __fastcall AddRemote(const String AGitRepo, const String ADirectory);
    void __fastcall Push(const String ADirectory);
    bool __fastcall CheckGitExe();
    void __fastcall JsonToRepo(const String AJson, TRepository& ARepository);
    void __fastcall ShowMessage(const String AMessage);
    void __fastcall HideMessage();
    void __fastcall OnApplicationIdle(System::TObject* Sender, bool &Done);
public:     // User declarations
    __fastcall TForm2(TComponent* Owner);
    __fastcall virtual ~TForm2(void);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
//---------------------------------------------------------------------------
#endif
