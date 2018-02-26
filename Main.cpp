//---------------------------------------------------------------------------
#include <fmx.h>
#pragma hdrstop

#include "Main.h"
#include <System.JSON.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.fmx"
TForm2 *Form2;
//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner)
    : TForm(Owner)
{
    Caption = "Gogs To GitBucket";

    IdHTTP1->HandleRedirects = true;
    IdHTTP1->IOHandler = IdSSLIOHandlerSocketOpenSSL1;
}
//---------------------------------------------------------------------------

void __fastcall TForm2::Button1Click(TObject *Sender)
{
    // Set authorization token
    IdHTTP1->Request->CustomHeaders->Values["Authorization"] =
        "token " + txtGogsToken->Text;

    String LUser;
    String LUrl = txtGogsUrl->Text + "/api/v1/";
    if(chkTypeOrg->IsChecked == true)
    {
        LUrl += "orgs/" + txtName->Text + "/repos";
    }
    else
    {
        LUrl += "user/repos";
        LUser = GetAuthenticatedUser();
    }

    try
    {
        const String LContent = IdHTTP1->Get(LUrl);

        TJSONArray* LRepos = static_cast<TJSONArray*>(TJSONObject::ParseJSONValue(LContent));
        if(LRepos != NULL)
        {
            TJSONArrayEnumerator* LRepoEnumerator = LRepos->GetEnumerator();
            while(LRepoEnumerator->MoveNext() == true)
            {
                TJSONObject* LRepo = static_cast<TJSONObject*>(LRepoEnumerator->Current);

                if(chkTypeOrg->IsChecked == false)
                {
                    TJSONPair* Pair;
                    if((Pair = LRepo->Get("owner")) != NULL)
                    {
                        TJSONObject* LOwner = static_cast<TJSONObject*>(Pair->JsonValue);
                        if((Pair = LOwner->Get("username")) != NULL)
                        {
                            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
                            String LUserName = Answer->Value();
                            if(LUserName != LUser)
                            {
                                continue;
                            }
                        }
                    }
                }

                LRepo->RemovePair("owner");
                LRepo->RemovePair("permissions");
                const String LJson = LRepo->ToJSON();
                CreateRepo(LJson);

                //break; // TEST ONE
            }
        }
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        const String LLog = "Get repository exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }
}
//---------------------------------------------------------------------------

bool __fastcall TForm2::CreateRepo(const String AJson)
{
    bool Result = false;

    // Set authorization token
    IdHTTP1->Request->CustomHeaders->Values["Authorization"] =
        "token " + txtGitBucketToken->Text;

    String LUrl = txtGitBucketUrl->Text + "/api/v3/";
    if(chkTypeOrg->IsChecked == true)
    {
        LUrl += "orgs/" + txtName->Text + "/repos";
    }
    else
    {
        LUrl += "user/repos";
    }

    String LAnswer;
    System::Classes::TStringStream* SourceFile = NULL;
    try
    {
        SourceFile = new System::Classes::TStringStream(AJson);
        LAnswer = IdHTTP1->Post(LUrl, SourceFile);
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        const String LLog = "Repository creation exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }
    delete SourceFile;

    TJSONObject* LObject = dynamic_cast<TJSONObject*>(TJSONObject::ParseJSONValue(LAnswer));
    if(LObject != NULL)
    {
        TJSONPair* Pair;
        if((Pair = LObject->Get("full_name")) != NULL)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            const String LLog = "Created repository: " + Answer->Value();
            memoLog->Lines->Add(LLog);
            Result = true;
        }
        else if((Pair = LObject->Get("message")) != NULL)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            const String LLog = "Repository creation message: " + Answer->Value();
            memoLog->Lines->Add(LLog);
        }
    }

    return Result;
}
//---------------------------------------------------------------------------

String __fastcall TForm2::GetAuthenticatedUser()
{
    String Result;

    // Set authorization token
    IdHTTP1->Request->CustomHeaders->Values["Authorization"] =
        "token " + txtGogsToken->Text;

    String LJson;
    String LUrl = txtGogsUrl->Text + "/api/v1/user";
    try
    {
        LJson = IdHTTP1->Get(LUrl);
    }
    catch(const Idhttp::EIdHTTPProtocolException& e)
    {
        const String LLog = "Get authenticated user exception: " + e.Message;
        memoLog->Lines->Add(LLog);
    }

    if(LJson.IsEmpty() == true)
    {
        return "";
    }

    TJSONObject* LUser = dynamic_cast<TJSONObject*>(TJSONObject::ParseJSONValue(LJson));
    if(LUser != NULL)
    {
        TJSONPair* Pair;
        if((Pair = LUser->Get("login")) != NULL)
        {
            TJSONString* Answer = static_cast<TJSONString*>(Pair->JsonValue);
            Result = Answer->Value();
        }
    }

    if(Result.IsEmpty() == false)
    {
        memoLog->Lines->Add("Authenticated user is " + Result);
    }
    else
    {
        memoLog->Lines->Add("Cannot get authenticated user");
    }

    return Result;
}
//---------------------------------------------------------------------------

