//---------------------------------------------------------------------------
#ifndef GitInternalH
#define GitInternalH
//---------------------------------------------------------------------------
#include <System.Sysutils.hpp>
//----------------------------------------------------------------------
struct git_repository;
struct git_remote;
struct git_credential;
struct git_clone_options;

class TRepositoryHandle : public System::TObject
{
private:
    git_repository* FPointer;
public:
    __fastcall TRepositoryHandle(git_repository* APointer);
    virtual __fastcall ~TRepositoryHandle();

    __property git_repository* Handle = {read=FPointer};
};

class TRemoteHandle : public System::TObject
{
private:
    git_remote* FPointer;
public:
    __fastcall TRemoteHandle(git_remote* APointer);
    virtual __fastcall ~TRemoteHandle();

    __property git_remote* Handle = {read=FPointer};
};

class TEnsure
{
public:
    static void HandleError(int AResult);
    static void ZeroResult(int AResult);
    static void ArgumentNotNull(void* ArgumentValue, const String AArgumentName);
    static void ArgumentEmptyString(const String ArgumentValue, const String AArgumentName);
};

class TProxy
{
public:
    static TRepositoryHandle* git_clone(const String AUrl, const String AWorkDir, git_clone_options AOptions);
    static TRepositoryHandle* git_repository_open(const String Apath);
    static TRepositoryHandle* git_repository_init(const String Apath, unsigned int AIsBare);

    static TRemoteHandle* git_remote_create(TRepositoryHandle* ARepo, const String AName, const String AUrl);
    static TRemoteHandle* git_remote_create_with_fetchspec(TRepositoryHandle* ARepo, const String AName, const String AUrl, const String ARefSpec);
    static TRemoteHandle* git_remote_create_anonymous(TRepositoryHandle* ARepo, const String AUrl);
    static TRemoteHandle* git_remote_lookup(TRepositoryHandle* ARepo, const String AName);

    static String git_remote_name(TRemoteHandle* ARemote);

    static String git_repository_path(TRepositoryHandle* ARepo);
};
//---------------------------------------------------------------------------
#endif
