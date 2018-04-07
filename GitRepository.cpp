//---------------------------------------------------------------------------
#pragma hdrstop

#include "GitRepository.h"
#include <System.JSON.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)

__fastcall TOwner::TOwner()
{
}

__fastcall TRepository::TRepository() :
    Private(true),
    Fork(false),
    OpenIssueCount(0)
{
}

void __fastcall JsonToRepo(const String AJson, TRepository& ARepository)
{
    TJSONPair* Pair;
    TJSONObject* LRepo = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(AJson));

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

    if((Pair = LRepo->Get("fork")) != NULL)
    {
        if(dynamic_cast<TJSONTrue*>(Pair->JsonValue) != NULL)
        {
            ARepository.Fork = true;
        }
        else
        {
            ARepository.Fork = false;
        }
    }
#ifdef _DEBUG
    else
    {
        throw Exception("fork not found");
    }
#endif

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
}

