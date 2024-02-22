//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitRepository.h"
#include "StringUtils.h"
#include <json.hpp>
#include <memory>
#include <exception>
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TUser::TUser()
{
}

void __fastcall JsonToUser(const std::wstring AJson, TUser* AUser)
{
    TJSONObject* LUser = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(String(AJson.c_str())));
    JsonToUser(LUser, AUser);
}

void __fastcall JsonToUser(TJSONObject* AJsonObject, TUser* AUser)
{
    TJSONPair* Pair;
    TJSONObject* LUser = AJsonObject;

    if(LUser == nullptr)
    {
        throw std::exception("Invalid JSON input!");
    }

    if((Pair = LUser->Get("login")) != nullptr)
    {
        AUser->Login = static_cast<TJSONString*>(Pair->JsonValue)->Value().c_str();
    }
    else
    {
        throw std::exception("login not found");
    }
}

__fastcall TRepository::TRepository() :
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

void __fastcall JsonToRepo(const std::wstring AJson, TRepository* ARepository)
{
    TJSONObject* LRepo = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(String(AJson.c_str())));
    JsonToRepo(LRepo, ARepository);
}

void __fastcall JsonToRepo(TJSONObject* AJsonObject, TRepository* ARepository)
{
    TJSONPair* Pair;
    TJSONObject* LRepo = AJsonObject;

    if(LRepo == nullptr)
    {
        throw std::exception("Invalid JSON input!");
    }

    if((Pair = LRepo->Get("owner")) != nullptr)
    {
        TJSONObject* LOwner = static_cast<TJSONObject*>(Pair->JsonValue);
        JsonToUser(LOwner, ARepository->Owner);
    }
#ifdef _DEBUG
    else
    {
        throw std::exception("owner not found");
    }
#endif

    if((Pair = LRepo->Get("name")) != nullptr)
    {
        ARepository->Name = static_cast<TJSONString*>(Pair->JsonValue)->Value().c_str();
    }
#ifdef _DEBUG
    else
    {
        throw std::exception("name not found");
    }
#endif

    if((Pair = LRepo->Get("full_name")) != nullptr)
    {
        ARepository->FullName = static_cast<TJSONString*>(Pair->JsonValue)->Value().c_str();
    }
#ifdef _DEBUG
    else
    {
        throw std::exception("full_name not found");
    }
#endif

    if((Pair = LRepo->Get("private")) != nullptr)
    {
        if(dynamic_cast<TJSONFalse*>(Pair->JsonValue) != nullptr)
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
        throw std::exception("private not found");
    }
#endif

    if((Pair = LRepo->Get("description")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            ARepository->Description = LJsonString->Value().c_str();
        }
        else
        {
            ARepository->Description = L"";
        }
    }
#ifdef _DEBUG
    else
    {
        throw std::exception("description not found");
    }
#endif

    if((Pair = LRepo->Get("fork")) != nullptr &&
        dynamic_cast<TJSONTrue*>(Pair->JsonValue) != nullptr)
    {
        ARepository->Fork = true;
    }
    else
    {   // Default value is false, this is the most commun case
        ARepository->Fork = false;
    }

    if((Pair = LRepo->Get("clone_url")) != nullptr)
    {
        ARepository->CloneUrl = static_cast<TJSONString*>(Pair->JsonValue)->Value().c_str();
    }
#ifdef _DEBUG
    else
    {
        throw std::exception("clone_url not found");
    }
#endif

    if((Pair = LRepo->Get("mirror_url")) != nullptr &&
        dynamic_cast<TJSONNull*>(Pair->JsonValue) == nullptr)
    {
        ARepository->MirrorUrl = static_cast<TJSONString*>(Pair->JsonValue)->Value().c_str();
    }
    else
    {
        ARepository->MirrorUrl = L"";
    }

    if((Pair = LRepo->Get("open_issues_count")) != nullptr)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        ARepository->OpenIssueCount = LIssueNumber->AsInt;
    }
    else
    {
        ARepository->OpenIssueCount = 0;
    }

    if((Pair = LRepo->Get("has_wiki")) != nullptr &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != nullptr)
    {
        ARepository->HasWiki = false;
    }
    else
    {   // Default value is true, let's presume there is a Wiki
        ARepository->HasWiki = true;
    }

    if((Pair = LRepo->Get("has_issues")) != nullptr &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != nullptr)
    {
        ARepository->HasIssues = false;
    }
    else
    {   // Default value is true, let's presume issues are enabled
        ARepository->HasIssues = true;
    }

    if((Pair = LRepo->Get("has_projects")) != nullptr &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != nullptr)
    {
        ARepository->HasProjects = false;
    }
    else
    {   // Default value is true, let's presume projects are enabled
        ARepository->HasProjects = true;
    }

    if((Pair = LRepo->Get("has_downloads")) != nullptr &&
        dynamic_cast<TJSONFalse*>(Pair->JsonValue) != nullptr)
    {
        ARepository->HasDownloads = false;
    }
    else
    {   // Default value is true, let's presume downloads are enabled
        ARepository->HasDownloads = true;
    }

    if((Pair = LRepo->Get("homepage")) != nullptr &&
        dynamic_cast<TJSONNull*>(Pair->JsonValue) == nullptr)
    {
        ARepository->Homepage = static_cast<TJSONString*>(Pair->JsonValue)->Value().c_str();
    }
    else
    {
        ARepository->Homepage = L"";
    }
}

__fastcall TIssue::TIssue() :
    Number(0)
{
}

void __fastcall JsonToIssue(const std::wstring AJson, TIssue* AIssue)
{
    TJSONObject* LIssue = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(String(AJson.c_str())));
    JsonToIssue(LIssue, AIssue);
}

void __fastcall JsonToIssue(TJSONObject* AJsonObject, TIssue* AIssue)
{
    TJSONPair* Pair;
    TJSONObject* LIssue = AJsonObject;

    if(LIssue == nullptr)
    {
        throw std::exception("Invalid JSON input!");
    }

    AIssue->Title = L"";
    if((Pair = LIssue->Get("title")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue->Title = LJsonString->Value().c_str();
        }
    }

    AIssue->Body = L"";
    if((Pair = LIssue->Get("body")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue->Body = LJsonString->Value().c_str();
        }
    }

    AIssue->State = L"";
    if((Pair = LIssue->Get("state")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AIssue->State = LJsonString->Value().c_str();
        }
    }

    if((Pair = LIssue->Get("number")) != nullptr)
    {
        TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
        AIssue->Number = LIssueNumber->AsInt;
    }
    else
    {
        AIssue->Number = 0;
    }
}

__fastcall TOrganization::TOrganization()
{
}

void __fastcall JsonToOrganization(const std::wstring AJson, TOrganization* AOrganization)
{
    TJSONObject* LOrg = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(String(AJson.c_str())));
    JsonToOrganization(LOrg, AOrganization);
}

void __fastcall JsonToOrganization(TJSONObject* AJsonObject, TOrganization* AOrganization)
{
    TJSONObject* LOrg = AJsonObject;
    TJSONPair* Pair;

    if(LOrg == nullptr)
    {
        throw std::exception("Invalid JSON input!");
    }

    if((Pair = LOrg->Get("login")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization->Login = LJsonString->Value().c_str();
        }
    }
    else if((Pair = LOrg->Get("username")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization->Login = LJsonString->Value().c_str();
        }
    }

    if((Pair = LOrg->Get("description")) != nullptr)
    {
        TJSONString* LJsonString = static_cast<TJSONString*>(Pair->JsonValue);
        if(LJsonString->Null == false)
        {
            AOrganization->Description = LJsonString->Value().c_str();
        }
    }
}

std::string __fastcall RepoToJson(const TRepository& ARepository)
{
    nlohmann::json LJson;
    LJson["name"] = ToStdString(ARepository.Name);
    LJson["description"] = ToStdString(ARepository.Description);
    LJson["homepage"] = ToStdString(ARepository.Homepage);
    LJson["private"] = ARepository.Private;
    LJson["has_issues"] = ARepository.HasIssues;
    LJson["has_projects"] = ARepository.HasProjects;
    LJson["has_wiki"] = ARepository.HasWiki;
    LJson["has_downloads"] = ARepository.HasDownloads;
#ifdef _DEBUG
        return LJson.dump(4);
#else
        return LJson.dump();
#endif
}

