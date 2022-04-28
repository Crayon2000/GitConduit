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
#include <FMX.Edit.hpp>
#include <FMX.Memo.hpp>
#include <FMX.ScrollBox.hpp>
#include <FMX.Layouts.hpp>
#include <FMX.ListBox.hpp>
#include <FMX.TabControl.hpp>
#include <FMX.Objects.hpp>
#include <FMX.ImgList.hpp>
#include <System.ImageList.hpp>
#include <FMX.ComboEdit.hpp>
#include <FMX.Memo.Types.hpp>
#include <string>
//---------------------------------------------------------------------------
namespace Idhttp
{
    class TIdHTTP;
};
class TDataModule1;
class TGitApplication;
class TRepository;

class TListBoxDataItem : public TListBoxItem
{
    typedef TListBoxItem inherited;

protected:
    virtual void __fastcall DoApplyStyleLookup();

public:     // User declarations
    inline __fastcall virtual TListBoxDataItem(System::Classes::TComponent* AOwner) : TListBoxItem(AOwner) { }
    __fastcall virtual ~TListBoxDataItem()
    {
        delete Data;
    }
};

class TForm2 : public TForm
{
__published:    // IDE-managed Components
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
    TLabel *Label1;
    TLabel *Label2;
    TLabel *Label7;
    TEdit *txtSourceToken;
    TEdit *txtSourceUrl;
    TComboBox *cboDestinationApp;
    TLabel *Label3;
    TLabel *Label4;
    TLabel *Label8;
    TEdit *txtDestinationToken;
    TEdit *txtDestinationUrl;
    TImageList *ImageList1;
    TLabel *Label14;
    TEdit *txtSourceUsername;
    TEdit *txtSourcePassword;
    TLabel *Label15;
    TLabel *Label16;
    TLabel *Label17;
    TEdit *txtDestinationUsername;
    TEdit *txtDestinationPassword;
    TTabItem *TabItemSourceOwner;
    TLabel *Label18;
    TLabel *Label5;
    TRadioButton *chkSourceTypeUser;
    TRadioButton *chkSourceTypeOrg;
    TComboEdit *cboeSourceName;
    TLayout *Layout1;
    TSpeedButton *btnSourceOwnerBack;
    TSpeedButton *btnSourceOwnerNext;
    TTabItem *TabItemDestinationOwner;
    TLayout *Layout7;
    TSpeedButton *btnDestinationOwnerBack;
    TSpeedButton *btnDestinationOwnerNext;
    TLabel *Label19;
    TLabel *Label20;
    TRadioButton *chkDestinationTypeUser;
    TRadioButton *chkDestinationTypeOrg;
    TComboEdit *cboeDestinationName;
    TComboEdit *cboeSourceUser;
    void __fastcall TabControl1Change(TObject *Sender);
    void __fastcall WizardButtonClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall btnErrorOkClick(TObject *Sender);
    void __fastcall btnCreateRepoCloseClick(TObject *Sender);
private:    // User declarations
    TGitApplication* SourceApplication;
    TGitApplication* DestinationApplication;
    int FTabAction;

    TDataModule1* FHTTPModule;
    Idhttp::TIdHTTP *FHTTPClient;

    void __fastcall ActionSource();
    void __fastcall ActionSourceOwner();
    void __fastcall ActionRepositories();
    void __fastcall ActionDestination();
    void __fastcall ActionDestinationOwner();
    void __fastcall ActionCreateRepo();
protected:
    bool __fastcall CreateRepo(const TRepository* ASourceRepository, TRepository* ADestinationRepository);
    String __fastcall GetAuthenticatedUser(const TGitApplication& AGitApplication);
    void __fastcall GetOrganizations(TGitApplication* AGitApplication, System::Classes::TStrings* AItems);
    void __fastcall PrepareRequest(const TGitApplication& AGitApplication);
    HANDLE __fastcall ExecuteProgramEx(const std::wstring ACmd, const std::wstring ADirectory = L".");
    DWORD __fastcall Wait(HANDLE AHandle);
    void __fastcall Clone(const std::wstring ADirectory, const std::wstring AGitRepo, bool AIsBare = false);
    void __fastcall AddRemote(const std::wstring AGitRepo, const std::wstring ADirectory);
    void __fastcall Push(const std::wstring ADirectory);
    bool __fastcall CheckGitExe();
    void __fastcall ShowMessage(const String AMessage);
    void __fastcall HideMessage();
    void __fastcall OnApplicationIdle(System::TObject* Sender, bool &Done);
    void __fastcall ListBoxItemApplyStyleLookup(TObject *Sender);
    void __fastcall PrintIssues(const TGitApplication& AGitApplication, const TRepository& ARepository);
    std::wstring __fastcall GetNextUrl();
public:     // User declarations
    __fastcall TForm2(TComponent* Owner);
    __fastcall virtual ~TForm2();
};
//---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
//---------------------------------------------------------------------------
#endif
