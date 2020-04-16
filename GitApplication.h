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
    GitHub,
    GitLab
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
    String FApiUrl;
    String FToken;
    String FUser;
    String FUsername;
    String FPassword;
    TApiEndpoint FEndpoint;
protected:
    void __fastcall SetApplicationType(TGitApplicationType AApplicationType);
    void __fastcall SetApiUrl(String AApiUrl);
public:
    __fastcall TGitApplication();
    inline __fastcall virtual ~TGitApplication(void) { }

    __property TGitApplicationType ApplicationType = {read=FApplicationType, write=SetApplicationType};
    __property String ApplicationName = {read=FApplicationName};
    __property String ApiUrl = {read=FApiUrl, write=SetApiUrl};
    __property String Token = {read=FToken, write=FToken};
    __property String User = {read=FUser, write=FUser};
    __property String Username = {read=FUsername, write=FUsername};
    __property String Password = {read=FPassword, write=FPassword};
    __property TApiEndpoint Endpoint = {read=FEndpoint, write=FEndpoint};
};
//---------------------------------------------------------------------------
#endif
