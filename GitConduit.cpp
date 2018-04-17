//---------------------------------------------------------------------------
#include <fmx.h>
#ifdef _WIN32
#include <tchar.h>
#endif
#pragma hdrstop
#include <System.StartUpCopy.hpp>
//---------------------------------------------------------------------------
USEFORM("Main.cpp", Form2);
//---------------------------------------------------------------------------
void __fastcall UpdateStyle();
//---------------------------------------------------------------------------
extern "C" int FMXmain()
{
    try
    {
        UpdateStyle();
        Application->Initialize();
        Application->CreateForm(__classid(TForm2), &Form2);
         Application->Run();
    }
    catch (Exception &exception)
    {
        Application->ShowException(&exception);
    }
    catch (...)
    {
        try
        {
            throw Exception("");
        }
        catch (Exception &exception)
        {
            Application->ShowException(&exception);
        }
    }
    return 0;
}
//---------------------------------------------------------------------------

void __fastcall UpdateStyle()
{
    // If you call the SetStyle function in the initialization section of a unit on the main project file, before Application->Initialize, then it is applied to all forms.

    // Set the style
    Fmx::Types::TFmxObject* LStyle = TStyleStreaming::LoadFromResource(
        (NativeUInt)HInstance, L"DATA_STYLE", RT_RCDATA);
    TStyleManager::SetStyle(LStyle);
}
//---------------------------------------------------------------------------

