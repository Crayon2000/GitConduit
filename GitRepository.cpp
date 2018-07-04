//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitRepository.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TOwner::TOwner()
{
}

__fastcall TRepository::TRepository() :
    Private(true),
    Fork(false),
    OpenIssueCount(0),
    HasWiki(true)
{
}

void __fastcall JsonToRepo(const String AJson, TRepository& ARepository)
{
    TJSONObject* LRepo = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToRepo(LRepo, ARepository);
}

void __fastcall JsonToRepo(TJSONObject* AJsonObject, TRepository& ARepository)
{
    TJSONPair* Pair;
    TJSONObject* LRepo = AJsonObject;

    if(LRepo == NULL)
    {
        throw Exception("Invalid JSON input!");
    }

    if((Pair = LRepo->Get("owner")) != NULL)
    {
        TJSONObject* LOwner = static_cast<TJSONObject*>(Pair->JsonValue);
        if((Pair = LOwner->Get("login")) != NULL)
        {
            ARepository.Owner.Login =
                static_cast<TJSONString*>(Pair->JsonValue)->Value();
        }
#ifdef _DEBUG
        else
        {
            throw Exception("login not found");
        }
#endif
    }
#ifdef _DEBUG
    else
    {
        throw Exception("owner not found");
    }
#endif

    if((Pair = LRepo->Get("name")) != NULL)
    {
        ARepository.Name = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("name not found");
    }
#endif

    if((Pair = LRepo->Get("full_name")) != NULL)
    {
        ARepository.FullName = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("full_name not found");
    }
#endif

    if((Pair = LRepo->Get("private")) != NULL)
    {
        if(dynamic_cast<TJSONFalse*>(Pair->JsonValue) != NULL)
        {
            ARepository.Private = false;
        }
        else
        {
            ARepository.Private = true;
        }
    }
#ifdef _DEBUG
    else
    {
        throw Exception("private not found");
    }
#endif

    if((Pair = LRepo->Get("description")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            ARepository.Description = LJsonString->Value();
        }
        else
        {
            ARepository.Description = "";
        }
    }
#ifdef _DEBUG
    else
    {
        throw Exception("description not found");
    }
#endif

    if((Pair = LRepo->Get("fork")) != NULL &&
        dynamic_cast<TJSONTrue*>(Pair->JsonValue) != NULL)
    {
        ARepository.Fork = true;
    }
    else
    {   // Default value is false, this is the most commun case
        ARepository.Fork = false;
    }

    if((Pair = LRepo->Get("clone_url")) != NULL)
    {
        ARepository.CloneUrl = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("clone_url not found");
    }
#endif

    if((Pair = LRepo->Get("open_issues_count")) != NULL)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        ARepository.OpenIssueCount = LIssueNumber->AsInt;
    }
    else
    {
        ARepository.OpenIssueCount = 0;
    }

    if((Pair = LRepo->Get("has_wiki")) != NULL &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != NULL)
    {
        ARepository.HasWiki = false;
    }
    else
    {   // Default value is true, let's presume there is a Wiki
        ARepository.HasWiki = true;
    }
}

__fastcall TIssue::TIssue() :
    Number(0)
{
}

void __fastcall JsonToIssue(const String AJson, TIssue& AIssue)
{
    TJSONObject* LIssue = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToIssue(LIssue, AIssue);
}

void __fastcall JsonToIssue(TJSONObject* AJsonObject, TIssue& AIssue)
{
    TJSONPair* Pair;
    TJSONObject* LIssue = AJsonObject;

    if(LIssue == NULL)
    {
        throw Exception("Invalid JSON input!");
    }

    AIssue.Title = "";
    if((Pair = LIssue->Get("title")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue.Title = LJsonString->Value();
        }
    }

    AIssue.Body = "";
    if((Pair = LIssue->Get("body")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue.Body = LJsonString->Value();
        }
    }

    AIssue.State = "";
    if((Pair = LIssue->Get("state")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue.State = LJsonString->Value();
        }
    }

    if((Pair = LIssue->Get("number")) != NULL)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        AIssue.Number = LIssueNumber->AsInt;
    }
    else
    {
        AIssue.Number = 0;
    }
}

__fastcall TOrganization::TOrganization()
{
}

void __fastcall JsonToOrganization(const String AJson, TOrganization& AOrganization)
{
    TJSONObject* LOrg = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToOrganization(LOrg, AOrganization);
}

void __fastcall JsonToOrganization(TJSONObject* AJsonObject, TOrganization& AOrganization)
{
    TJSONObject* LOrg = AJsonObject;
    TJSONPair* Pair;

    if(LOrg == NULL)
    {
        throw Exception("Invalid JSON input!");
    }

    if((Pair = LOrg->Get("login")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization.Login = LJsonString->Value();
        }
    }
    else if((Pair = LOrg->Get("username")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization.Login = LJsonString->Value();
        }
    }

    if((Pair = LOrg->Get("description")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization.Description = LJsonString->Value();
        }
    }
}

