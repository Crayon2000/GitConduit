//---------------------------------------------------------------------------
#ifndef GitRepositoryH
#define GitRepositoryH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.JSON.hpp>
//---------------------------------------------------------------------------
class TUser : public System::TObject
{
    typedef System::TObject inherited;

public:
    __fastcall TUser();
    inline __fastcall virtual ~TUser() { }

    String Login;
};

class TRepository : public System::TObject
{
    typedef System::TObject inherited;

public:
    __fastcall TRepository();
    __fastcall virtual ~TRepository();

    TUser* Owner;
    String Name;
    String FullName;
    bool Private;
    String Description;
    bool Fork;
    String CloneUrl;
    String MirrorUrl;
    int OpenIssueCount;
    bool HasWiki;
    bool HasIssues;
    bool HasProjects;
    bool HasDownloads;
    String Homepage;
};

class TIssue : public System::TObject
{
    typedef System::TObject inherited;

public:
    __fastcall TIssue();
    inline __fastcall virtual ~TIssue() { }

    String Title;
    String Body;
    String State;
    int Number;
};

class TOrganization : public System::TObject
{
    typedef System::TObject inherited;

public:
    __fastcall TOrganization();
    inline __fastcall virtual ~TOrganization() { }

    String Login;
    String Description;
};

void __fastcall JsonToUser(const String AJson, TUser* AUser);
void __fastcall JsonToUser(TJSONObject* AJsonObject, TUser* AUser);

void __fastcall JsonToRepo(const String AJson, TRepository* ARepository);
void __fastcall JsonToRepo(TJSONObject* AJsonObject, TRepository* ARepository);

void __fastcall JsonToIssue(const String AJson, TIssue* AIssue);
void __fastcall JsonToIssue(TJSONObject* AJsonObject, TIssue* AIssue);

void __fastcall JsonToOrganization(const String AJson, TOrganization* AOrganization);
void __fastcall JsonToOrganization(TJSONObject* AJsonObject, TOrganization* AOrganization);

void __fastcall RepoToJson(const TRepository* ARepository, String& AJson);
//---------------------------------------------------------------------------
#endif
