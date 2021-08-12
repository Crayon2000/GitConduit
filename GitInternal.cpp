//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitInternal.h"
#include <System.RTLConsts.hpp>
#define _MSC_VER 1600
#include <git2.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)

/**
 * Constructor.
 */
__fastcall TRepositoryHandle::TRepositoryHandle(git_repository* APointer) :
    System::TObject(),
    FPointer(APointer)
{
}

/**
 * Destructor.
 */
__fastcall TRepositoryHandle::~TRepositoryHandle()
{
    git_repository_free(FPointer);
}

/**
 * Constructor.
 */
__fastcall TRemoteHandle::TRemoteHandle(git_remote* APointer) :
    System::TObject(),
    FPointer(APointer)
{
}

/**
 * Destructor.
 */
__fastcall TRemoteHandle::~TRemoteHandle()
{
    git_remote_free(FPointer);
}

/**
 * Raise an exception.
 * @param AResult The result to examine.
 */
void TEnsure::HandleError(int AResult)
{
    String LErrorMessage;
    const git_error* LError = git_error_last();
    if(LError == nullptr || LError->message == nullptr)
    {
        LErrorMessage = "No error message has been provided by the native library";
    }
    else
    {
        LErrorMessage = UTF8ToString(LError->message).Trim();
    }
    throw Exception("Error " + String(AResult) + ": " + LErrorMessage);
}

/**
 * Check that the result of a C call was successful.
 * @param AResult The result to examine.
 */
void TEnsure::ZeroResult(int AResult)
{
    if(AResult == GIT_OK)
    {
        return;
    }
    HandleError(AResult);
}

void TEnsure::ArgumentNotNull(void* ArgumentValue, const String AArgumentName)
{
    if(ArgumentValue == nullptr)
    {
        throw EArgumentNilException(Sysutils::Format(System_Rtlconsts_SParamIsNil, ARRAYOFCONST((AArgumentName))));
    }
}

void TEnsure::ArgumentEmptyString(const String ArgumentValue, const String AArgumentName)
{
    if(ArgumentValue.IsEmpty() == true)
    {
        throw EArgumentException(String("Parameter ") + AArgumentName + " is empty!");
    }
}

/**
 * Clone a remote repository.
 * @param AUrl The remote repository to clone.
 * @param AWorkDir The local directory to clone to.
 * @param AOptions Controls the clone behavior.
 * @return Returns a TRepositoryHandle.
 */
TRepositoryHandle* TProxy::git_clone(const String AUrl, const String AWorkDir, git_clone_options AOptions)
{
    const RawByteString LUtf8Url = System::UTF8Encode(AUrl);
    const RawByteString LUtf8Directory = System::UTF8Encode(AWorkDir);

    git_repository *LRepo;
    int LRes = ::git_clone(&LRepo, LUtf8Url.c_str(), LUtf8Directory.c_str(), &AOptions);
    TEnsure::ZeroResult(LRes);
    return new TRepositoryHandle(LRepo);
}

/**
 * Open a git repository.
 * @param Apath The path to the repository.
 * @return Returns a TRepositoryHandle.
 */
TRepositoryHandle* TProxy::git_repository_open(const String Apath)
{
    git_repository *LRepo;
    int LRes = ::git_repository_open(&LRepo, System::UTF8Encode(Apath).c_str());
    TEnsure::ZeroResult(LRes);
    return new TRepositoryHandle(LRepo);
}

/**
 * Creates a new Git repository in the given folder.
 * @param Apath The path to the repository.
 * @param AIsBare If true, a Git repository without a working directory is created at the pointed path.
 *                If false, provided path will be considered as the working directory into which the .git directory will be created.
 * @return Returns a TRepositoryHandle.
 */
TRepositoryHandle* TProxy::git_repository_init(const String Apath, unsigned int AIsBare)
{
    git_repository *LRepo;
    int LRes = ::git_repository_init(&LRepo, System::UTF8Encode(Apath).c_str(), AIsBare);
    TEnsure::ZeroResult(LRes);
    return new TRepositoryHandle(LRepo);
}

/**
 * TBD
 */
TRemoteHandle* TProxy::git_remote_create(TRepositoryHandle* ARepo, const String AName, const String AUrl)
{
    git_remote* LHandle;
    int LRes = ::git_remote_create(&LHandle, ARepo->Handle,
        System::UTF8Encode(AName).c_str(), System::UTF8Encode(AUrl).c_str());
    TEnsure::ZeroResult(LRes);
    return new TRemoteHandle(LHandle);
}

/**
 * TBD
 */
TRemoteHandle* TProxy::git_remote_create_with_fetchspec(TRepositoryHandle* ARepo, const String AName, const String AUrl, const String ARefSpec)
{
    git_remote* LHandle;
    int LRes = ::git_remote_create_with_fetchspec(&LHandle, ARepo->Handle,
        System::UTF8Encode(AName).c_str(), System::UTF8Encode(AUrl).c_str(),
        System::UTF8Encode(ARefSpec).c_str());
    TEnsure::ZeroResult(LRes);
    return new TRemoteHandle(LHandle);
}

/**
 * TBD
 */
TRemoteHandle* TProxy::git_remote_create_anonymous(TRepositoryHandle* ARepo, const String AUrl)
{
    git_remote* LHandle;
    int LRes = ::git_remote_create_anonymous(&LHandle, ARepo->Handle, System::UTF8Encode(AUrl).c_str());
    TEnsure::ZeroResult(LRes);
    return new TRemoteHandle(LHandle);
}

/**
 * TBD
 */
TRemoteHandle* TProxy::git_remote_lookup(TRepositoryHandle* ARepo, const String AName)
{
    git_remote* LHandle;
    int LRes = ::git_remote_lookup(&LHandle, ARepo->Handle, System::UTF8Encode(AName).c_str());
    TEnsure::ZeroResult(LRes);
    return new TRemoteHandle(LHandle);
}

/**
 * Get the remote's name.
 * @param ARemote The remote handle.
 * @return The remote's name.
 */
String TProxy::git_remote_name(TRemoteHandle* ARemote)
{
    return UTF8ToUnicodeString(::git_remote_name(ARemote->Handle));
}

/**
 * Set the remote's URL for pushing in the configuration.
 * @param ARepo The repository in which to perform the change.
 * @param AName The remote's name.
 * @param AUrl The url to set.
 */
void TProxy::git_remote_set_pushurl(TRepositoryHandle* ARepo, const String AName, const String AUrl)
{
    int LRes = ::git_remote_set_pushurl(ARepo->Handle,
        System::UTF8Encode(AName).c_str(),
        System::UTF8Encode(AUrl).c_str());
    TEnsure::ZeroResult(LRes);
}

/**
 * Get the path of the repository.
 * @param ARepo A repository handle.
 * @return The path to the repository.
 */
String TProxy::git_repository_path(TRepositoryHandle* ARepo)
{
    return UTF8ToUnicodeString(::git_repository_path(ARepo->Handle));
}

/**
 * Perform a push.
 * @param ARemote The remote to push to.
 * @param ARefSpecs The refspecs to use for pushing. If nullptr or an empty array, the configured refspecs will be used.
 * @param AOptions Options to use for this push.
 */
void TProxy::git_remote_push(TRemoteHandle* ARemote, const git_strarray* ARefSpecs, const git_push_options* AOptions)
{
    int LRes = ::git_remote_push(ARemote->Handle, ARefSpecs, AOptions);
    TEnsure::ZeroResult(LRes);
}

