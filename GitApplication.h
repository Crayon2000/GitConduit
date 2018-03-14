//---------------------------------------------------------------------------
#ifndef GitApplicationH
#define GitApplicationH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
enum class TApiEndpoint : unsigned char
{
    User,
    Organization
};

class TGitApplication : public System::TObject
{
    typedef System::TObject inherited;

private:
    String FApplicationName;
    String FApiVersion;
    String FUrl;
    String FToken;
    String FUser;
    TApiEndpoint FEndpoint;
protected:

public:
    __fastcall TGitApplication();
    inline __fastcall virtual ~TGitApplication(void) { }

    __property String ApplicationName = {read=FApplicationName, write=FApplicationName};
    __property String ApiVersion = {read=FApiVersion, write=FApiVersion};
    __property String Url = {read=FUrl, write=FUrl};
    __property String Token = {read=FToken, write=FToken};
    __property String User = {read=FUser, write=FUser};
    __property TApiEndpoint Endpoint = {read=FEndpoint, write=FEndpoint};
};
//---------------------------------------------------------------------------
#endif
