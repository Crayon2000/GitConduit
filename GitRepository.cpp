//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitRepository.h"
#include <System.JSON.Writers.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TUser::TUser() :
    System::TObject()
{
}

void __fastcall JsonToUser(const String AJson, TUser* AUser)
{
    TJSONObject* LUser = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToUser(LUser, AUser);
}

void __fastcall JsonToUser(TJSONObject* AJsonObject, TUser* AUser)
{
    TJSONPair* Pair;
    TJSONObject* LUser = AJsonObject;

    if(LUser == NULL)
    {
        throw Exception("Invalid JSON input!");
    }

    if((Pair = LUser->Get("login")) != NULL)
    {
        AUser->Login = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
    else
    {
        throw Exception("login not found");
    }
}

__fastcall TRepository::TRepository() :
    System::TObject(),
    Owner(new TUser()),
    Private(true),
    Fork(false),
    OpenIssueCount(0),
    HasWiki(true)
{
}

__fastcall TRepository::~TRepository()
{
    delete Owner;
}

void __fastcall JsonToRepo(const String AJson, TRepository* ARepository)
{
    TJSONObject* LRepo = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToRepo(LRepo, ARepository);
}

void __fastcall JsonToRepo(TJSONObject* AJsonObject, TRepository* ARepository)
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
        JsonToUser(LOwner, ARepository->Owner);
    }
#ifdef _DEBUG
    else
    {
        throw Exception("owner not found");
    }
#endif

    if((Pair = LRepo->Get("name")) != NULL)
    {
        ARepository->Name = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("name not found");
    }
#endif

    if((Pair = LRepo->Get("full_name")) != NULL)
    {
        ARepository->FullName = static_cast<TJSONString*>(Pair->JsonValue)->Value();
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
            ARepository->Private = false;
        }
        else
        {
            ARepository->Private = true;
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
            ARepository->Description = LJsonString->Value();
        }
        else
        {
            ARepository->Description = "";
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
        ARepository->Fork = true;
    }
    else
    {   // Default value is false, this is the most commun case
        ARepository->Fork = false;
    }

    if((Pair = LRepo->Get("clone_url")) != NULL)
    {
        ARepository->CloneUrl = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
#ifdef _DEBUG
    else
    {
        throw Exception("clone_url not found");
    }
#endif

    if((Pair = LRepo->Get("mirror_url")) != NULL &&
        dynamic_cast<TJSONNull*>(Pair->JsonValue) == NULL)
    {
        ARepository->MirrorUrl = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
    else
    {
        ARepository->MirrorUrl = "";
    }

    if((Pair = LRepo->Get("open_issues_count")) != NULL)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        ARepository->OpenIssueCount = LIssueNumber->AsInt;
    }
    else
    {
        ARepository->OpenIssueCount = 0;
    }

    if((Pair = LRepo->Get("has_wiki")) != NULL &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != NULL)
    {
        ARepository->HasWiki = false;
    }
    else
    {   // Default value is true, let's presume there is a Wiki
        ARepository->HasWiki = true;
    }

    if((Pair = LRepo->Get("has_issues")) != NULL &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != NULL)
    {
        ARepository->HasIssues = false;
    }
    else
    {   // Default value is true, let's presume issues are enabled
        ARepository->HasIssues = true;
    }

    if((Pair = LRepo->Get("has_projects")) != NULL &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != NULL)
    {
        ARepository->HasProjects = false;
    }
    else
    {   // Default value is true, let's presume projects are enabled
        ARepository->HasProjects = true;
    }

    if((Pair = LRepo->Get("has_downloads")) != NULL &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != NULL)
    {
        ARepository->HasDownloads = false;
    }
    else
    {   // Default value is true, let's presume downloads are enabled
        ARepository->HasDownloads = true;
    }

    if((Pair = LRepo->Get("homepage")) != NULL &&
        dynamic_cast<TJSONNull*>(Pair->JsonValue) == NULL)
    {
        ARepository->Homepage = static_cast<TJSONString*>(Pair->JsonValue)->Value();
    }
    else
    {
        ARepository->Homepage = "";
    }
}

__fastcall TIssue::TIssue() :
    System::TObject(),
    Number(0)
{
}

void __fastcall JsonToIssue(const String AJson, TIssue* AIssue)
{
    TJSONObject* LIssue = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToIssue(LIssue, AIssue);
}

void __fastcall JsonToIssue(TJSONObject* AJsonObject, TIssue* AIssue)
{
    TJSONPair* Pair;
    TJSONObject* LIssue = AJsonObject;

    if(LIssue == NULL)
    {
        throw Exception("Invalid JSON input!");
    }

    AIssue->Title = "";
    if((Pair = LIssue->Get("title")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue->Title = LJsonString->Value();
        }
    }

    AIssue->Body = "";
    if((Pair = LIssue->Get("body")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue->Body = LJsonString->Value();
        }
    }

    AIssue->State = "";
    if((Pair = LIssue->Get("state")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue->State = LJsonString->Value();
        }
    }

    if((Pair = LIssue->Get("number")) != NULL)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        AIssue->Number = LIssueNumber->AsInt;
    }
    else
    {
        AIssue->Number = 0;
    }
}

__fastcall TOrganization::TOrganization() :
    System::TObject()
{
}

void __fastcall JsonToOrganization(const String AJson, TOrganization* AOrganization)
{
    TJSONObject* LOrg = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));
    JsonToOrganization(LOrg, AOrganization);
}

void __fastcall JsonToOrganization(TJSONObject* AJsonObject, TOrganization* AOrganization)
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
            AOrganization->Login = LJsonString->Value();
        }
    }
    else if((Pair = LOrg->Get("username")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization->Login = LJsonString->Value();
        }
    }

    if((Pair = LOrg->Get("description")) != NULL)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization->Description = LJsonString->Value();
        }
    }
}

void __fastcall RepoToJson(const TRepository* ARepository, String& AJson)
{
    TStringWriter* LStringWriter =  new TStringWriter();
    TJsonTextWriter* LJsonTextWriter = new TJsonTextWriter(LStringWriter, false);

#ifdef _DEBUG
    LJsonTextWriter->Formatting = TJsonFormatting::Indented;
#endif

    LJsonTextWriter->WriteStartObject();

    LJsonTextWriter->WritePropertyName("name");
    LJsonTextWriter->WriteValue(ARepository->Name);

    LJsonTextWriter->WritePropertyName("description");
    LJsonTextWriter->WriteValue(ARepository->Description);

    LJsonTextWriter->WritePropertyName("homepage");
    LJsonTextWriter->WriteValue(ARepository->Homepage);

    LJsonTextWriter->WritePropertyName("private");
    LJsonTextWriter->WriteValue(ARepository->Private);

    LJsonTextWriter->WritePropertyName("has_issues");
    LJsonTextWriter->WriteValue(ARepository->HasIssues);

    LJsonTextWriter->WritePropertyName("has_projects");
    LJsonTextWriter->WriteValue(ARepository->HasProjects);

    LJsonTextWriter->WritePropertyName("has_wiki");
    LJsonTextWriter->WriteValue(ARepository->HasWiki);

    LJsonTextWriter->WritePropertyName("has_downloads");
    LJsonTextWriter->WriteValue(ARepository->HasDownloads);

    LJsonTextWriter->WriteEndObject();

    AJson = LStringWriter->ToString();

    delete LJsonTextWriter;
    delete LStringWriter;
}

