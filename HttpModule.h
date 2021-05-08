//---------------------------------------------------------------------------
#ifndef HttpModuleH
#define HttpModuleH
//---------------------------------------------------------------------------
#include <System.Classes.hpp>
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
//---------------------------------------------------------------------------
class TDataModule1 : public TDataModule
{
__published:    // IDE-managed Components
    TIdHTTP *IdHTTP1;
    TIdSSLIOHandlerSocketOpenSSL *IdSSLIOHandlerSocketOpenSSL1;
private:    // User declarations
public:     // User declarations
    __fastcall TDataModule1(TComponent* Owner);
    inline __fastcall virtual ~TDataModule1() { }
};
//---------------------------------------------------------------------------
extern PACKAGE TDataModule1 *DataModule1;
//---------------------------------------------------------------------------
#endif
