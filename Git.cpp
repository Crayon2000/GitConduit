//---------------------------------------------------------------------------
#pragma hdrstop

#include "Git.h"

#include "GitInternal.h"
#define _MSC_VER 1600
#include <git2.h>
//---------------------------------------------------------------------------
#pragma comment(lib, "libgit2") // Include the libgit2 library
//---------------------------------------------------------------------------
#pragma package(smart_init)

/**
 * Constructor.
 */
__fastcall TGit::TGit() :
    FRepository(nullptr),
    FRemote(nullptr)
{
    git_libgit2_init();
}

/**
 * Destructor.
 */
__fastcall TGit::~TGit()
{
    delete FRemote;
    delete FRepository;

    git_libgit2_shutdown();
}

/**
 * Credential callback.
 * @param ACred The newly created credential object.
 * @param AUrl The resource for which we are demanding a credential.
 * @param AUsernameFromUrl The username that was embedded in a "user\@host" remote url, or nullptr if not included.
 * @param AAllowedTypes A bitmask stating which cred types are OK to return.
 * @param APayload The payload provided when specifying this callback.
 */
int TGit::CredentialCB(git_credential **ACred, const char *AUrl, const char *AUsernameFromUrl,
    unsigned int AAllowedTypes, void *APayload)
{
    TGit* LClass = static_cast<TGit*>(APayload);
    if(LClass != nullptr)
    {
        return LClass->DoFetchCredentials(ACred, AUrl, AUsernameFromUrl, AAllowedTypes);
    }
    else
    {
        return GIT_EUSER;
    }
}

/**
 * Fetch credentials.
 * @param ACred The newly created credential object.
 * @param AUrl The resource for which we are demanding a credential.
 * @param AUsernameFromUrl The username that was embedded in a "user\@host" remote url, or nullptr if not included.
 * @param AAllowedTypes A bitmask stating which cred types are OK to return.
 */
int __fastcall TGit::DoFetchCredentials(git_credential **ACred, const RawByteString AUrl,
    const RawByteString AUsernameFromUrl, unsigned int AAllowedTypes)
{
    int Result = git_cred_userpass_plaintext_new(ACred,
        FUsername.c_str(), FPassword.c_str());

    return Result;
}

/**
 * Clone a remote repository.
 * @param AUrl The remote repository to clone.
 * @param ADirectory The local directory to clone to.
 * @return The path to the created repository.
 */
String __fastcall TGit::Clone(const String AUrl, const String ADirectory)
{
    TCloneOptions LOptions;
    LOptions.IsBare = false;
    return Clone(AUrl, ADirectory, LOptions);
}

/**
 * Clone a remote repository.
 * @param AUrl The remote repository to clone.
 * @param ADirectory The local directory to clone to.
 * @param AOptions Controls the clone behavior.
 * @return The path to the created repository.
 */
String __fastcall TGit::Clone(const String AUrl, const String ADirectory, TCloneOptions AOptions)
{
    String LClonedRepoPath;
    TRepositoryHandle* LRepository = nullptr;

    git_clone_options LGitCloneOptions = GIT_CLONE_OPTIONS_INIT;
    LGitCloneOptions.fetch_opts.callbacks.credentials = CredentialCB;
    LGitCloneOptions.fetch_opts.callbacks.payload = this;
    LGitCloneOptions.bare = AOptions.IsBare;

    try
    {
        LRepository = TProxy::git_clone(AUrl, ADirectory, LGitCloneOptions);
        LClonedRepoPath = TProxy::git_repository_path(LRepository);
    }
    __finally
    {
        delete LRepository;
    }

    return LClonedRepoPath;
}

/**
 * Open a git repository.
 * @param ADirectory The path to the repository.
 */
void __fastcall TGit::OpenRepository(const String ADirectory)
{
    delete FRemote;
    FRemote = nullptr;
    delete FRepository;
    FRepository = nullptr;

    FRepository = TProxy::git_repository_open(ADirectory);
}

/**
 * Initialize a repository in a given folder.
 * @param ADirectory The path to the repository.
 * @param AIsBare If true, a Git repository without a working directory is created at the pointed path.
 *                If false, provided path will be considered as the working directory into which the .git directory will be created.
 * @return The path to the created repository.
 */
String __fastcall TGit::Init(const String ADirectory, bool AIsBare)
{
    String LInitRepoPath;
    TRepositoryHandle* LRepository = nullptr;

    try
    {
        LRepository = TProxy::git_repository_init(ADirectory, AIsBare);
        LInitRepoPath = TProxy::git_repository_path(LRepository);
    }
    __finally
    {
        delete LRepository;
    }

    return LInitRepoPath;
}

/**
 * Add a remote to the repository's configuration.
 * @param AUrl The remote's URL.
 * @param AName The remote's name.
 * @param AFetch The remote fetch value. If empty the default fetch refspec will be used.
 */
void __fastcall TGit::AddRemote(const String AUrl, const String AName, const String AFetch)
{
    delete FRemote;
    FRemote = nullptr;

    if(FRepository == nullptr)
    {
        throw Exception("Repository cannot be nullptr");
    }

    if(AFetch.IsEmpty() == true)
    {
        FRemote = TProxy::git_remote_create(FRepository, AName, AUrl);
    }
    else
    {
        FRemote = TProxy::git_remote_create_with_fetchspec(FRepository,
            AName, AUrl, AFetch);
    }
}

/**
 * Create an anonymous remote with the given URL in-memory.
 * You can use this when you have a URL instead of a remote's name.
 * @param AUrl The remote repository's URL.
 */
void __fastcall TGit::AddAnonymousRemote(const String AUrl)
{
    delete FRemote;
    FRemote = nullptr;

    if(FRepository == nullptr)
    {
        throw Exception("Repository cannot be nullptr");
    }

    FRemote = TProxy::git_remote_create_anonymous(FRepository, AUrl);
}

/**
 *
 * @param AName The remote's name.
 */
void __fastcall TGit::SetRemote(const String AName)
{
    delete FRemote;
    FRemote = nullptr;

    if(FRepository == nullptr)
    {
        throw Exception("Repository cannot be nullptr");
    }

    FRemote = TProxy::git_remote_lookup(FRepository, AName);
}

/**
 * Get the remote name.
 */
String __fastcall TGit::GetRemoteName()
{
    if(FRemote == nullptr)
    {
        throw Exception("Remote cannot be nullptr");
    }

    return TProxy::git_remote_name(FRemote);
}

/**
 * Push to a repository.
 */
void __fastcall TGit::Push()
{
    if(FRemote == nullptr)
    {
        throw Exception("Remote cannot be nullptr");
    }

git_strarray LRefspecs;
::git_remote_get_fetch_refspecs(&LRefspecs, FRemote->Handle);


const int refcount = LRefspecs.count;
for(int i = 0; i < refcount; ++i)
{
    String ss = LRefspecs.strings[i];
    ss=ss;
}


/*
    git_push_options LPushOptions = GIT_PUSH_OPTIONS_INIT;
    LPushOptions.callbacks.credentials = CredentialCB;
    LPushOptions.callbacks.payload = this;

    int LRet = git_remote_push(FRemote, &LRefspecs, &LPushOptions);
    Ensure::ZeroResult(LRet);
*/

//git_strarray_free(&LRefspecs);
/*
GIT_EXTERN(int) git_remote_push(git_remote *remote,
                const git_strarray *refspecs,
                const git_push_options *opts);
*/
}

/**
 * Set username.
 */
void __fastcall TGit::SetUsername(String AUsername)
{
    FUsername = System::UTF8Encode(AUsername);
}

/**
 * Get username.
 */
String __fastcall TGit::GetUsername()
{
    return UTF8ToString(FUsername);
}

/**
 * Set password.
 */
void __fastcall TGit::SetPassword(String APassword)
{
    FPassword = System::UTF8Encode(APassword);
}

/**
 * Get password.
 */
String __fastcall TGit::GetPassword()
{
    return UTF8ToString(FPassword);
}

