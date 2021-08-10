//---------------------------------------------------------------------------
#ifndef GitH
#define GitH
//---------------------------------------------------------------------------
#include <System.Sysutils.hpp>
//---------------------------------------------------------------------------
class TRepositoryHandle;
class TRemoteHandle;
struct git_credential;
struct git_remote;
struct git_repository;

class TCloneOptions
{
public:
    __fastcall TCloneOptions() : IsBare(false) {}
    bool IsBare;
};

class TGit : public System::TObject
{
    typedef System::TObject inherited;

private:    // User declarations
    TRepositoryHandle* FRepository;
    TRemoteHandle* FRemote;
    RawByteString FUsername;
    RawByteString FPassword;

    static int CredentialCallback(git_credential **ACred, const char *AUrl, const char *AUsernameFromUrl, unsigned int AAllowedTypes, void *APayload);
    static int RemoteCallback(git_remote **AOut, git_repository *ARepo, const char *AName, const char *AUrl, void *APayload);

protected:
    void __fastcall SetUsername(String AUsername);
    String __fastcall GetUsername();
    void __fastcall SetPassword(String APassword);
    String __fastcall GetPassword();

    int __fastcall DoFetchCredentials(git_credential **ACred, const RawByteString AUrl, const RawByteString AUsernameFromUrl, unsigned int AAllowedTypes);
public:
    __fastcall TGit();
    virtual __fastcall ~TGit();

    String __fastcall Clone(const String AUrl, const String ADirectory);
    String __fastcall Clone(const String AUrl, const String ADirectory, TCloneOptions AOptions);
    void __fastcall OpenRepository(const String ADirectory);
    String __fastcall Init(const String ADirectory, bool AIsBare = false);
    void __fastcall AddRemote(const String AUrl, const String AName, const String AFetch = "");
    void __fastcall AddAnonymousRemote(const String AUrl);
    void __fastcall SetRemote(const String AName);
    void __fastcall SetRemotePushUrl(const String AName, const String AUrl);
    String __fastcall GetRemoteName();
    void __fastcall Push();

    __property String Username = {read = GetUsername, write = SetUsername}; /**< Username. */
    __property String Password = {read = GetPassword, write = SetPassword}; /**< Password. */
};
//---------------------------------------------------------------------------
#endif
