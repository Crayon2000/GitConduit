//---------------------------------------------------------------------------
#ifndef GitApplicationH
#define GitApplicationH
//---------------------------------------------------------------------------
#include <string>
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

class TGitApplication
{
private:
    TGitApplicationType FApplicationType;
    std::wstring FApplicationName;
    std::wstring FApiUrl;
    std::wstring FToken;
    std::wstring FUser;
    std::wstring FUsername;
    std::wstring FPassword;
    TApiEndpoint FEndpoint;
protected:
    void __fastcall SetApplicationType(TGitApplicationType AApplicationType);
    void __fastcall SetApiUrl(const std::wstring AApiUrl);
public:
    __fastcall TGitApplication();
    inline __fastcall virtual ~TGitApplication() { }

    __property TGitApplicationType ApplicationType = {read=FApplicationType, write=SetApplicationType};
    __property std::wstring ApplicationName = {read=FApplicationName};
    __property std::wstring ApiUrl = {read=FApiUrl, write=SetApiUrl};
    __property std::wstring Token = {read=FToken, write=FToken};
    __property std::wstring User = {read=FUser, write=FUser};
    __property std::wstring Username = {read=FUsername, write=FUsername};
    __property std::wstring Password = {read=FPassword, write=FPassword};
    __property TApiEndpoint Endpoint = {read=FEndpoint, write=FEndpoint};
};
//---------------------------------------------------------------------------
#endif
