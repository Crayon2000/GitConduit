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
    , SourceApplication(NULL)
    , DestinationApplication(NULL)
{
    Caption = "Gogs To GitBucket";

    SourceApplication = new TGitApplication();
    DestinationApplication = new TGitApplication();

    IdHTTP1->HandleRedirects = true;
    IdHTTP1->IOHandler = IdSSLIOHandlerSocketOpenSSL1;
}
//---------------------------------------------------------------------------

__fastcall TForm2::~TForm2()
{
    delete SourceApplication;
    delete DestinationApplication;
}
//---------------------------------------------------------------------------


void __fastcall TForm2::Button1Click(TObject *Sender)
{
    SourceApplication->ApplicationName = "Gogs";
    SourceApplication->ApiVersion = "v1";
    SourceApplication->Url = txtSourceUrl->Text;
    SourceApplication->Token = txtSourceToken->Text;

    DestinationApplication->ApplicationName = "GitBucket";
    DestinationApplication->ApiVersion = "v3";
    DestinationApplication->Url = txtDestinationUrl->Text;
    DestinationApplication->Token = txtDestinationToken->Text;

    // Set authorization token
    IdHTTP1->Request->CustomHeaders->Values["Authorization"] =
        "token " + SourceApplication->Token;

    String LUser;
    String LUrl = SourceApplication->Url + "/api/" + SourceApplication->ApiVersion + "/";
    if(chkTypeOrg->IsChecked == true)
    {
        LUrl += "orgs/" + txtName->Text + "/repos";
    }
    else
    {
        LUrl += "user/repos";
        LUser = GetAuthenticatedUser(SourceApplication);
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
                TJSONPair* Pair;
                TJSONObject* LRepo = static_cast<TJSONObject*>(LRepoEnumerator->Current);

                if(chkTypeOrg->IsChecked == false)
                {
                    if((Pair = LRepo->Get("owner")) != NULL)
                    {
                        TJSONObject* LOwner = static_cast<TJSONObject*>(Pair->JsonValue);
                        if((Pair = LOwner->Get("username")) != NULL)
                        {
                            String LUserName =
                                static_cast<TJSONString*>(Pair->JsonValue)->Value();
                            if(LUserName != LUser)
                            {
                                continue;
                            }
                        }
                    }
                }

                String LFullName;
                if((Pair = LRepo->Get("full_name")) != NULL)
                {
                    LFullName =
                        static_cast<TJSONString*>(Pair->JsonValue)->Value();
                }

                const String LLog = String().sprintf(L"====== %s ======", LFullName.c_str());
                memoLog->Lines->Add(LLog);

                LRepo->RemovePair("owner");
                LRepo->RemovePair("permissions");
                const String LJson = LRepo->ToJSON();
                bool LIsCreated = CreateRepo(LJson);

                if(LIsCreated == false)
                {   // Don't do rest if issue was not created
                    continue;
                }

                if((Pair = LRepo->Get("open_issues_count")) != NULL)
                {
                    TJSONNumber* LIssueNumber = static_cast<TJSONNumber*>(Pair->JsonValue);
                    const LIssueCount = LIssueNumber->AsInt;
                    if(LIssueCount > 0)
                    {
                        const String LLog = String().sprintf(L"%d issue(s) not created!", LIssueCount);
                        memoLog->Lines->Add(LLog);
                    }
                }

                //break; // TEST ONE
                Application->ProcessMessages();
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
        "token " + DestinationApplication->Token;

    String LUrl = DestinationApplication->Url + "/api/" +
        DestinationApplication->ApiVersion + "/";
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

String __fastcall TForm2::GetAuthenticatedUser(TGitApplication* AGitApplication)
{
    String Result;

    // Set authorization token
    IdHTTP1->Request->CustomHeaders->Values["Authorization"] =
        "token " + AGitApplication->Token;

    String LJson;
    const String LUrl = AGitApplication->Url + "/api/" +
        AGitApplication->ApiVersion + "/user";
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

