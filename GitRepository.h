//---------------------------------------------------------------------------
#ifndef GitRepositoryH
#define GitRepositoryH
//---------------------------------------------------------------------------
#include <System.hpp>
#include <System.JSON.hpp>
//---------------------------------------------------------------------------
class TOwner
{
public:
    __fastcall TOwner();
    inline __fastcall virtual ~TOwner(void) { }

    String Login;
};

class TRepository
{
public:
    __fastcall TRepository();
    inline __fastcall virtual ~TRepository(void) { }

    TOwner Owner;
    String Name;
    String FullName;
    bool Private;
    String Description;
    bool Fork;
    String CloneUrl;
    int OpenIssueCount;
    bool HasWiki;
};

class TIssue
{
public:
    __fastcall TIssue();
    inline __fastcall virtual ~TIssue(void) { }

    String Title;
    String Body;
    String State;
    int Number;
};

class TOrganization
{
public:
    __fastcall TOrganization();
    inline __fastcall virtual ~TOrganization(void) { }

    String Login;
    String Description;
};

void __fastcall JsonToRepo(const String AJson, TRepository& ARepository);
void __fastcall JsonToRepo(TJSONObject* AJsonObject, TRepository& ARepository);

void __fastcall JsonToIssue(const String AJson, TIssue& AIssue);
void __fastcall JsonToIssue(TJSONObject* AJsonObject, TIssue& AIssue);

void __fastcall JsonToOrganization(const String AJson, TOrganization& AOrganization);
void __fastcall JsonToOrganization(TJSONObject* AJsonObject, TOrganization& AOrganization);
//---------------------------------------------------------------------------
#endif
