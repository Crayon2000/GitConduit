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
//---------------------------------------------------------------------------
class TGitApplication;

class TForm2 : public TForm
{
__published:    // IDE-managed Components
    TIdHTTP *IdHTTP1;
    TIdSSLIOHandlerSocketOpenSSL *IdSSLIOHandlerSocketOpenSSL1;
    TButton *Button1;
    TGroupBox *GroupBox1;
    TLabel *Label1;
    TLabel *Label2;
    TEdit *txtSourceUrl;
    TEdit *txtSourceToken;
    TGroupBox *GroupBox2;
    TLabel *Label3;
    TLabel *Label4;
    TEdit *txtDestinationUrl;
    TEdit *txtDestinationToken;
    TMemo *memoLog;
    TGroupBox *GroupBox4;
    TLayout *Layout1;
    TLabel *Label5;
    TRadioButton *chkSourceTypeUser;
    TRadioButton *chkSourceTypeOrg;
    TEdit *txtSourceName;
    TLabel *Label6;
    TRadioButton *chkDestinationTypeUser;
    TRadioButton *chkDestinationTypeOrg;
    TEdit *txtDestinationName;
    TLabel *Label7;
    TComboBox *cboSourceApp;
    TLabel *Label8;
    TComboBox *cboDestinationApp;
    void __fastcall Button1Click(TObject *Sender);
private:    // User declarations
    TGitApplication* SourceApplication;
    TGitApplication* DestinationApplication;
protected:
    bool __fastcall CreateRepo(const String AJson);
    String __fastcall GetAuthenticatedUser(TGitApplication* AGitApplication);
    void __fastcall PrepareRequest(TGitApplication* AGitApplication);
    String __fastcall GitUrl(TGitApplication* AGitApplication, const String AFullName);
    String __fastcall GitWikiUrl(TGitApplication* AGitApplication, const String AFullName);
    HANDLE __fastcall ExecuteProgramEx(const String ACmd, const String ADirectory = ".");
    DWORD __fastcall Wait(HANDLE AHandle);
    void __fastcall Clone(const String AGitRepo);
    void __fastcall AddRemote(const String AGitRepo, const String ADirectory);
    void __fastcall Push(const String ADirectory);
public:     // User declarations
    __fastcall TForm2(TComponent* Owner);
    __fastcall virtual ~TForm2(void);
};
//---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
//---------------------------------------------------------------------------
#endif
