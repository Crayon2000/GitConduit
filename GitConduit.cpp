//---------------------------------------------------------------------------
#include <fmx.h>
#ifdef _WIN32
#include <tchar.h>
#endif
#pragma hdrstop
#include <System.StartUpCopy.hpp>
//---------------------------------------------------------------------------
USEFORM("Main.cpp", Form2);
USEFORM("HttpModule.cpp", DataModule1); /* TDataModule: File Type */
//---------------------------------------------------------------------------
void __fastcall UpdateStyle();
void __fastcall SetListBoxItemMargins(Fmx::Types::TFmxObject* AStyle, const String AStyleLookup);
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

    SetListBoxItemMargins(LStyle, "listboxitemnodetail");
    SetListBoxItemMargins(LStyle, "listboxitembottomdetail");
}
//---------------------------------------------------------------------------

void __fastcall SetListBoxItemMargins(Fmx::Types::TFmxObject* AStyle, const String AStyleLookup)
{
    Fmx::Types::TFmxObject* StyleResource = AStyle->FindStyleResource(AStyleLookup);
    if(StyleResource != NULL && StyleResource->ClassNameIs("TLayout") == true)
    {
        Fmx::Types::TFmxObject* GlyphStyleResource = StyleResource->FindStyleResource("glyphstyle");
        if(GlyphStyleResource != NULL && GlyphStyleResource->ClassNameIs("TGlyph") == true)
        {
            TGlyph* LGlyph = static_cast<TGlyph*>(GlyphStyleResource);
            LGlyph->Margins->Top = 4.0f;
            LGlyph->Margins->Bottom = 4.0f;
        }

        Fmx::Types::TFmxObject* IconStyleResource = StyleResource->FindStyleResource("icon");
        if(IconStyleResource != NULL && IconStyleResource->ClassNameIs("TImage") == true)
        {
            TImage* LImage = static_cast<TImage*>(IconStyleResource);
            LImage->Margins->Top = 4.0f;
            LImage->Margins->Bottom = 4.0f;
        }

        Fmx::Types::TFmxObject* CheckStyleResource = StyleResource->FindStyleResource("check");
        if(CheckStyleResource != NULL && CheckStyleResource->ClassNameIs("TCheckBox") == true)
        {
            TCheckBox* LCheckBox = static_cast<TCheckBox*>(CheckStyleResource);
            LCheckBox->Margins->Left = 8.0f;
            LCheckBox->Margins->Right = 0.0f;
        }
    }
    else
    {
        throw Exception("Style was changed, resource '" + AStyleLookup + "' is missing!");
    }
}
//---------------------------------------------------------------------------

