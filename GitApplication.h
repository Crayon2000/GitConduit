//---------------------------------------------------------------------------
#ifndef GitApplicationH
#define GitApplicationH
//---------------------------------------------------------------------------
#include <System.hpp>
//---------------------------------------------------------------------------
enum class TGitApplicationType : unsigned char
{
    Gogs,
    GitBucket,
    GitHub
};

enum class TApiEndpoint : unsigned char
{
    User,
    Organization
};

class TGitApplication : public System::TObject
{
    typedef System::TObject inherited;

private:
    TGitApplicationType FApplicationType;
    String FApplicationName;
    String FApiVersion;
    String FUrl;
    String FToken;
    String FUser;
    TApiEndpoint FEndpoint;
protected:
    void __fastcall SetApplicationType(TGitApplicationType AApplicationType);
    String __fastcall GetApiUrl();
public:
    __fastcall TGitApplication();
    inline __fastcall virtual ~TGitApplication(void) { }

    __property TGitApplicationType ApplicationType = {read=FApplicationType, write=SetApplicationType};
    __property String ApplicationName = {read=FApplicationName};
    __property String ApiVersion = {read=FApiVersion, write=FApiVersion};
    __property String Url = {read=FUrl, write=FUrl};
    __property String ApiUrl = {read=GetApiUrl};
    __property String Token = {read=FToken, write=FToken};
    __property String User = {read=FUser, write=FUser};
    __property TApiEndpoint Endpoint = {read=FEndpoint, write=FEndpoint};
};
//---------------------------------------------------------------------------
#endif
