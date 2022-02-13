//---------------------------------------------------------------------------
#pragma hdrstop

#include "HttpModule.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma classgroup "FMX.Controls.TControl"
#pragma resource "*.dfm"
TDataModule1 *DataModule1;
//---------------------------------------------------------------------------
__fastcall TDataModule1::TDataModule1(TComponent* Owner)
    : TDataModule(Owner)
    , IdHTTP1(new TIdHTTP(this))
{
    IdHTTP1->HTTPOptions = TIdHTTPOptions() << TIdHTTPOption::hoForceEncodeParams;
    IdHTTP1->HandleRedirects = true;
    IdHTTP1->IOHandler = IdSSLIOHandlerSocketOpenSSL1;
    IdHTTP1->Request->UserAgent =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/90.0.4430.93 Safari/537.36";
}
//---------------------------------------------------------------------------

