#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

// clang-format off
W_ENUMERABLE_CLASS_IMPLEMENTATION(WCVar);

// The CVars need to be saved and loaded whenever plugins are loaded and unloaded.
// Therefore we register as early as possible (Base Startup) at the plugin system,
// to be informed about plugin changes.
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, CVars)

  // for saving and loading we need the filesystem, so make sure we are initialized after
  // and shutdown before the filesystem is
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WPlugin::Events().AddEventHandler(WCVar::PluginEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // save the CVars every time the core is shut down
    // at this point the filesystem might already be uninitialized by the user (data dirs)
    // in that case the variables cannot be saved, but it will fail silently
    // if it succeeds, the most recent state will be serialized though
    WCVar::SaveCVars();

    WPlugin::Events().RemoveEventHandler(WCVar::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // save the CVars every time the engine is shut down
    // at this point the filesystem should usually still be configured properly
    WCVar::SaveCVars();
  }

  // The user is responsible to call 'WCVar::SetStorageFolder' to define where the CVars are
  // actually stored. That call will automatically load all CVar states.

W_END_SUBSYSTEM_DECLARATION;
// clang-format on


WString WCVar::s_sStorageFolder;
WEvent<const WCVarEvent&> WCVar::s_AllCVarEvents;

void WCVar::AssignSubSystemPlugin(WStringView sPluginName)
{
  WCVar* pCVar = WCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->m_sPluginName.IsEmpty())
      pCVar->m_sPluginName = sPluginName;

    pCVar = pCVar->GetNextInstance();
  }
}

void WCVar::PluginEventHandler(const WPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case WPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all currently available CVars
      // are assigned to the proper plugin
      // all not-yet assigned cvars cannot be in any plugin, so assign them to the 'static' plugin
      AssignSubSystemPlugin("Static");
    }
    break;

    case WPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new CVars and assign them to that new plugin
      AssignSubSystemPlugin(EventData.m_sPluginBinary);

      // now load the state of all CVars
      LoadCVars();
    }
    break;

    case WPluginEvent::BeforeUnloading:
    {
      SaveCVars();
    }
    break;

    default:
      break;
  }
}

WCVar::WCVar(WStringView sName, WBitflags<WCVarFlags> Flags, WStringView sDescription)
  : m_sName(sName)
  , m_sDescription(sDescription)
  , m_Flags(Flags)
{
  W_ASSERT_DEV(!m_sDescription.IsEmpty(), "Please add a useful description for CVar '{}'.", sName);
}

WCVar* WCVar::FindCVarByName(WStringView sName)
{
  WCVar* pCVar = WCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->GetName().IsEqual_NoCase(sName))
      return pCVar;

    pCVar = pCVar->GetNextInstance();
  }

  return nullptr;
}

void WCVar::SetStorageFolder(WStringView sFolder)
{
  s_sStorageFolder = sFolder;
}

WCommandLineOptionBool opt_NoFileCVars("cvar", "-no-file-cvars", "Disables loading CVar values from the user-specific, persisted configuration file.", false);

void WCVar::SaveCVarsToFile(WStringView sPath, bool bIgnoreSaveFlag)
{
  WTempHybridArray<WCVar*, 128> allCVars;

  for (WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(WCVarFlags::Save))
    {
      allCVars.PushBack(pCVar);
    }
  }

  SaveCVarsToFileInternal(sPath, allCVars);
}

void WCVar::SaveCVars()
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(WCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  // first gather all the cvars by plugin
  WMap<WString, WHybridArray<WCVar*, 128>> PluginCVars;

  {
    WCVar* pCVar = WCVar::GetFirstInstance();
    while (pCVar)
    {
      // only store cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(WCVarFlags::Save))
      {
        if (!pCVar->m_sPluginName.IsEmpty())
          PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
        else
          PluginCVars["Static"].PushBack(pCVar);
      }

      pCVar = pCVar->GetNextInstance();
    }
  }

  WMap<WString, WHybridArray<WCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

  WStringBuilder sTemp;

  // now save all cvars in their plugin specific file
  while (it.IsValid())
  {
    // create the plugin specific file
    sTemp.SetFormat("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

    SaveCVarsToFileInternal(sTemp, it.Value());

    // continue with the next plugin
    ++it;
  }
}

void WCVar::SaveCVarsToFileInternal(WStringView path, const WDynamicArray<WCVar*>& vars)
{
  WStringBuilder sTemp;
  WFileWriter File;
  if (File.Open(path.GetData(sTemp)) == W_SUCCESS)
  {
    // write one line for each cvar, to save its current value
    for (WUInt32 var = 0; var < vars.GetCount(); ++var)
    {
      WCVar* pCVar = vars[var];

      switch (pCVar->GetType())
      {
        case WCVarType::Int:
        {
          WCVarInt* pInt = (WCVarInt*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pInt->GetValue(WCVarValue::DelayedSync));
        }
        break;
        case WCVarType::Bool:
        {
          WCVarBool* pBool = (WCVarBool*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pBool->GetValue(WCVarValue::DelayedSync) ? "true" : "false");
        }
        break;
        case WCVarType::Float:
        {
          WCVarFloat* pFloat = (WCVarFloat*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pFloat->GetValue(WCVarValue::DelayedSync));
        }
        break;
        case WCVarType::String:
        {
          WCVarString* pString = (WCVarString*)pCVar;
          sTemp.SetFormat("{0} = \"{1}\"\n", pCVar->GetName(), pString->GetValue(WCVarValue::DelayedSync));
        }
        break;
        default:
          W_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      // add the one line for that cvar to the config file
      File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    }
  }
}

void WCVar::LoadCVars(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  LoadCVarsFromCommandLine(bOnlyNewOnes, bSetAsCurrentValue);
  LoadCVarsFromFile(bOnlyNewOnes, bSetAsCurrentValue);
}

static WResult ParseLine(const WString& sLine, WStringBuilder& out_sVarName, WStringBuilder& out_sVarValue)
{
  const char* szSign = sLine.FindSubString("=");

  if (szSign == nullptr)
    return W_FAILURE;

  {
    WStringView sSubString(sLine.GetData(), szSign);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);

    out_sVarName = sSubString;
  }

  {
    WStringView sSubString(szSign + 1);

    // remove all spaces
    while (sSubString.StartsWith(" "))
      sSubString.Shrink(1, 0);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);


    // remove " and start and end

    if (sSubString.StartsWith("\""))
      sSubString.Shrink(1, 0);

    if (sSubString.EndsWith("\""))
      sSubString.Shrink(0, 1);

    out_sVarValue = sSubString;
  }

  return W_SUCCESS;
}

void WCVar::LoadCVarsFromFile(bool bOnlyNewOnes, bool bSetAsCurrentValue, WDynamicArray<WCVar*>* pOutCVars)
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(WCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  WMap<WString, WHybridArray<WCVar*, 128>> PluginCVars;

  // first gather all the cvars by plugin
  {
    for (WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      // only load cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(WCVarFlags::Save))
      {
        if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
        {
          if (!pCVar->m_sPluginName.IsEmpty())
            PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
          else
            PluginCVars["Static"].PushBack(pCVar);
        }
      }

      // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
      pCVar->m_bHasNeverBeenLoaded = false;
    }
  }

  {
    WMap<WString, WHybridArray<WCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

    WStringBuilder sTemp;

    while (it.IsValid())
    {
      // create the plugin specific file
      sTemp.SetFormat("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

      LoadCVarsFromFileInternal(sTemp.GetView(), it.Value(), bSetAsCurrentValue, pOutCVars);

      // continue with the next plugin
      ++it;
    }
  }
}

void WCVar::LoadCVarsFromFile(WStringView sPath, bool bOnlyNewOnes, bool bSetAsCurrentValue, bool bIgnoreSaveFlag, WDynamicArray<WCVar*>* pOutCVars)
{
  WTempHybridArray<WCVar*, 128> allCVars;

  for (WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(WCVarFlags::Save))
    {
      if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
      {
        allCVars.PushBack(pCVar);
      }
    }

    // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
    pCVar->m_bHasNeverBeenLoaded = false;
  }

  LoadCVarsFromFileInternal(sPath, allCVars, bSetAsCurrentValue, pOutCVars);
}

void WCVar::LoadCVarsFromFileInternal(WStringView path, const WDynamicArray<WCVar*>& vars, bool bSetAsCurrentValue, WDynamicArray<WCVar*>* pOutCVars)
{
  WFileReader File;
  WStringBuilder sTemp;

  if (File.Open(path.GetData(sTemp)) == W_SUCCESS)
  {
    WStringBuilder sContent;
    sContent.ReadAll(File);

    WDynamicArray<WString> Lines;
    sContent.ReplaceAll("\r", ""); // remove carriage return

    // splits the string at occurrence of '\n' and adds each line to the 'Lines' container
    sContent.Split(true, Lines, "\n");

    WStringBuilder sVarName;
    WStringBuilder sVarValue;

    for (const WString& sLine : Lines)
    {
      if (ParseLine(sLine, sVarName, sVarValue) == W_FAILURE)
        continue;

      // now find a variable with the same name
      for (WUInt32 var = 0; var < vars.GetCount(); ++var)
      {
        WCVar* pCVar = vars[var];

        if (!sVarName.IsEqual(pCVar->GetName()))
          continue;

        // found the cvar, now convert the text into the proper value *sigh*
        switch (pCVar->GetType())
        {
          case WCVarType::Int:
          {
            WInt32 Value = 0;
            if (WConversionUtils::StringToInt(sVarValue, Value).Succeeded())
            {
              WCVarInt* pTyped = (WCVarInt*)pCVar;
              pTyped->m_Values[WCVarValue::Stored] = Value;
              *pTyped = Value;
            }
          }
          break;
          case WCVarType::Bool:
          {
            bool Value = sVarValue.IsEqual_NoCase("true");

            WCVarBool* pTyped = (WCVarBool*)pCVar;
            pTyped->m_Values[WCVarValue::Stored] = Value;
            *pTyped = Value;
          }
          break;
          case WCVarType::Float:
          {
            double Value = 0.0;
            if (WConversionUtils::StringToFloat(sVarValue, Value).Succeeded())
            {
              WCVarFloat* pTyped = (WCVarFloat*)pCVar;
              pTyped->m_Values[WCVarValue::Stored] = static_cast<float>(Value);
              *pTyped = static_cast<float>(Value);
            }
          }
          break;
          case WCVarType::String:
          {
            const char* Value = sVarValue.GetData();

            WCVarString* pTyped = (WCVarString*)pCVar;
            pTyped->m_Values[WCVarValue::Stored] = Value;
            *pTyped = Value;
          }
          break;
          default:
            W_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
            break;
        }

        if (pOutCVars)
        {
          pOutCVars->PushBack(pCVar);
        }

        if (bSetAsCurrentValue)
          pCVar->SetToDelayedSyncValue();
      }
    }
  }
}

WCommandLineOptionDoc opt_CVar("cvar", "-CVarName", "<value>", "Forces a CVar to the given value.\n\
Overrides persisted settings.\n\
Examples:\n\
-MyIntVar 42\n\
-MyStringVar \"Hello\"\n\
",
  nullptr);

void WCVar::LoadCVarsFromCommandLine(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/, WDynamicArray<WCVar*>* pOutCVars /*= nullptr*/)
{
  WStringBuilder sTemp;

  for (WCVar* pCVar = WCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bOnlyNewOnes && !pCVar->m_bHasNeverBeenLoaded)
      continue;

    sTemp.Set("-", pCVar->GetName());

    if (WCommandLineUtils::GetGlobalInstance()->GetOptionIndex(sTemp) != -1)
    {
      if (pOutCVars)
      {
        pOutCVars->PushBack(pCVar);
      }

      // has been specified on the command line -> mark it as 'has been loaded'
      pCVar->m_bHasNeverBeenLoaded = false;

      switch (pCVar->GetType())
      {
        case WCVarType::Int:
        {
          WCVarInt* pTyped = (WCVarInt*)pCVar;
          WInt32 Value = pTyped->m_Values[WCVarValue::Stored];
          Value = WCommandLineUtils::GetGlobalInstance()->GetIntOption(sTemp, Value);

          pTyped->m_Values[WCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        case WCVarType::Bool:
        {
          WCVarBool* pTyped = (WCVarBool*)pCVar;
          bool Value = pTyped->m_Values[WCVarValue::Stored];
          Value = WCommandLineUtils::GetGlobalInstance()->GetBoolOption(sTemp, Value);

          pTyped->m_Values[WCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        case WCVarType::Float:
        {
          WCVarFloat* pTyped = (WCVarFloat*)pCVar;
          double Value = pTyped->m_Values[WCVarValue::Stored];
          Value = WCommandLineUtils::GetGlobalInstance()->GetFloatOption(sTemp, Value);

          pTyped->m_Values[WCVarValue::Stored] = static_cast<float>(Value);
          *pTyped = static_cast<float>(Value);
        }
        break;
        case WCVarType::String:
        {
          WCVarString* pTyped = (WCVarString*)pCVar;
          WString Value = WCommandLineUtils::GetGlobalInstance()->GetStringOption(sTemp, 0, pTyped->m_Values[WCVarValue::Stored]);

          pTyped->m_Values[WCVarValue::Stored] = Value;
          *pTyped = Value;
        }
        break;
        default:
          W_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      if (bSetAsCurrentValue)
        pCVar->SetToDelayedSyncValue();
    }
  }
}

void WCVar::ListOfCVarsChanged(WStringView sSetPluginNameTo)
{
  AssignSubSystemPlugin(sSetPluginNameTo);

  LoadCVars();

  WCVarEvent e(nullptr);
  e.m_EventType = WCVarEvent::Type::ListOfVarsChanged;

  s_AllCVarEvents.Broadcast(e);
}


W_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_CVar);
