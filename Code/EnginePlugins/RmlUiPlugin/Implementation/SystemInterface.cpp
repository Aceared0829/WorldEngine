#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <RmlUiPlugin/Implementation/SystemInterface.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Time/Clock.h>

namespace WRmlUiInternal
{
  double SystemInterface::GetElapsedTime()
  {
    return WClock::GetGlobalClock()->GetAccumulatedTime().GetSeconds();
  }

  int SystemInterface::TranslateString(Rml::String& out_sTranslated, const Rml::String& sInput)
  {
    WStringView sTrimmedInput = WRmlUiConversionUtils::ToStringView(sInput);
    sTrimmedInput.Trim(" \t\r\n");
    if (sTrimmedInput.IsEmpty() == false)
    {
      WStringView sTranslated = WTranslate(sTrimmedInput);
      if (sTranslated != sTrimmedInput)
      {
        out_sTranslated = WRmlUiConversionUtils::ToString(sTranslated);
        return 1;
      }
    }

    out_sTranslated = sInput;
    return 0;
  }

  void SystemInterface::JoinPath(Rml::String& out_sTranslatedPath, const Rml::String& sDocumentPath, const Rml::String& sPath)
  {
    if (WFileSystem::ExistsFile(WRmlUiConversionUtils::ToStringView(sPath)))
    {
      // path is already a valid path for W file system so don't join with document path
      out_sTranslatedPath = sPath;
      return;
    }

    Rml::SystemInterface::JoinPath(out_sTranslatedPath, sDocumentPath, sPath);
  }

  bool SystemInterface::LogMessage(Rml::Log::Type type, const Rml::String& sMessage)
  {
    switch (type)
    {
      case Rml::Log::LT_ERROR:
        WLog::Error("{}", WRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_ASSERT:
        WLog::Error("{}", WRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_WARNING:
        WLog::Warning("{}", WRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_ALWAYS:
      case Rml::Log::LT_INFO:
        WLog::Info("{}", WRmlUiConversionUtils::ToStringView(sMessage));
        break;

      case Rml::Log::LT_DEBUG:
        WLog::Debug("{}", WRmlUiConversionUtils::ToStringView(sMessage));
        break;
      default:
        break;
    }

    return true;
  }

} // namespace WRmlUiInternal
