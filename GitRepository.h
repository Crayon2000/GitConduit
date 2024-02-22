//---------------------------------------------------------------------------
#ifndef GitRepositoryH
#define GitRepositoryH
//---------------------------------------------------------------------------
#include <string>
#include <System.JSON.hpp>
//---------------------------------------------------------------------------
/**
 * User.
 */
class TUser
{
public:
    __fastcall TUser();
    inline __fastcall virtual ~TUser() { }

    std::wstring Login;
};

/**
 * Repository.
 */
class TRepository
{
public:
    __fastcall TRepository();
    __fastcall virtual ~TRepository();

    TUser* Owner;
    std::wstring Name;
    std::wstring FullName;
    bool Private;
    std::wstring Description;
    bool Fork;
    std::wstring CloneUrl;
    std::wstring MirrorUrl;
    int OpenIssueCount;
    bool HasWiki;
    bool HasIssues;
    bool HasProjects;
    bool HasDownloads;
    std::wstring Homepage;
};

/**
 * Issue.
 */
class TIssue
{
public:
    __fastcall TIssue();
    inline __fastcall virtual ~TIssue() { }

    std::wstring Title;
    std::wstring Body;
    std::wstring State;
    int Number;
};

/**
 * Organization.
 */
class TOrganization
{
public:
    __fastcall TOrganization();
    inline __fastcall virtual ~TOrganization() { }

    std::wstring Login;
    std::wstring Description;
};

void __fastcall JsonToUser(const std::wstring AJson, TUser* AUser);
void __fastcall JsonToUser(TJSONObject* AJsonObject, TUser* AUser);

void __fastcall JsonToRepo(const std::wstring AJson, TRepository* ARepository);
void __fastcall JsonToRepo(TJSONObject* AJsonObject, TRepository* ARepository);

void __fastcall JsonToIssue(const std::wstring AJson, TIssue* AIssue);
void __fastcall JsonToIssue(TJSONObject* AJsonObject, TIssue* AIssue);

void __fastcall JsonToOrganization(const std::wstring AJson, TOrganization* AOrganization);
void __fastcall JsonToOrganization(TJSONObject* AJsonObject, TOrganization* AOrganization);

[[nodiscard]] std::string __fastcall RepoToJson(const TRepository& ARepository);
//---------------------------------------------------------------------------
#endif
