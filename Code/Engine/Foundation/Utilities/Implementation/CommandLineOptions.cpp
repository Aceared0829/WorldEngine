#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/ConversionUtils.h>

W_ENUMERABLE_CLASS_IMPLEMENTATION(WCommandLineOption);

void WCommandLineOption::GetSortingGroup(WStringBuilder& ref_sOut) const
{
  ref_sOut = m_sSortingGroup;
}

void WCommandLineOption::GetSplitOptions(WStringBuilder& out_sAll, WDynamicArray<WStringView>& ref_splitOptions) const
{
  GetOptions(out_sAll);
  out_sAll.Split(false, ref_splitOptions, ";", "|");
}

bool WCommandLineOption::IsHelpRequested(const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/)
{
  return pUtils->GetBoolOption("-help") || pUtils->GetBoolOption("--help") || pUtils->GetBoolOption("-h") || pUtils->GetBoolOption("-?");
}

WResult WCommandLineOption::RequireOptions(WStringView sRequiredOptions, WString* pMissingOption /*= nullptr*/, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/)
{
  WStringBuilder tmp;
  WStringBuilder allOpts = sRequiredOptions;
  WHybridArray<WStringView, 16> options;
  allOpts.Split(false, options, ";");

  for (auto opt : options)
  {
    opt.Trim(" ");

    if (pUtils->GetOptionIndex(opt.GetData(tmp)) < 0)
    {
      if (pMissingOption)
      {
        *pMissingOption = opt;
      }

      return W_FAILURE;
    }
  }

  if (pMissingOption)
  {
    pMissingOption->Clear();
  }

  return W_SUCCESS;
}

bool WCommandLineOption::LogAvailableOptions(LogAvailableModes mode, WStringView sGroupFilter0 /*= {} */, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/)
{
  if (mode == LogAvailableModes::IfHelpRequested)
  {
    if (!IsHelpRequested(pUtils))
      return false;
  }

  // Note: nothing in here may use the temp allocator. Printing the command line help is the typical
  // reason for an application to abort in BeforeCoreSystemsStartup(), i.e. before the core systems -
  // and with them the temp allocator - exist.
  WMap<WString, WHybridArray<WCommandLineOption*, 16>> sorted;

  WStringBuilder sGroupFilter;
  if (!sGroupFilter0.IsEmpty())
  {
    sGroupFilter.Set(";", sGroupFilter0, ";");
  }

  for (WCommandLineOption* pOpt = WCommandLineOption::GetFirstInstance(); pOpt != nullptr; pOpt = pOpt->GetNextInstance())
  {
    WStringBuilder sGroup;
    pOpt->GetSortingGroup(sGroup);
    sGroup.Prepend(";");
    sGroup.Append(";");

    if (!sGroupFilter.IsEmpty())
    {
      if (sGroupFilter.FindSubString_NoCase(sGroup) == nullptr)
        continue;
    }

    sorted[sGroup].PushBack(pOpt);
  }

  if (WApplication::GetApplicationInstance())
  {
    WLog::Info("");
    WLog::Info("{} command line options:", WApplication::GetApplicationInstance()->GetApplicationName());
  }

  if (sorted.IsEmpty())
  {
    WLog::Info("This application has no documented command line options.");
    return true;
  }

  WStringBuilder sLine;

  for (auto optIt : sorted)
  {
    for (auto pOpt : optIt.Value())
    {
      WStringBuilder sOptions, sParamShort, sParamDefault, sLongDesc;

      sLine.Clear();

      pOpt->GetOptions(sOptions);
      pOpt->GetParamShortDesc(sParamShort);
      pOpt->GetParamDefaultValueDesc(sParamDefault);
      pOpt->GetLongDesc(sLongDesc);

      WHybridArray<WStringView, 4> lines;

      sOptions.Split(false, lines, ";", "|");

      for (auto o : lines)
      {
        sLine.AppendWithSeparator(", ", o);
      }

      if (!sParamShort.IsEmpty())
      {
        sLine.Append(" ", sParamShort);

        if (!sParamDefault.IsEmpty())
        {
          sLine.Append(" = ", sParamDefault);
        }
      }

      WLog::Info("");
      WLog::Info(sLine);

      sLongDesc.Trim(" \t\n\r");
      sLongDesc.Split(true, lines, "\n");

      for (auto o : lines)
      {
        sLine = o;
        sLine.Trim("\t\n\r");
        sLine.Prepend("    ");

        WLog::Info(sLine);
      }
    }

    WLog::Info("");
  }

  WLog::Info("");

  return true;
}


bool WCommandLineOption::LogAvailableOptionsToBuffer(WStringBuilder& out_sBuffer, LogAvailableModes mode, WStringView sGroupFilter /*= {} */, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/)
{
  WLogSystemToBuffer log;
  WLogSystemScope ls(&log);

  const bool res = WCommandLineOption::LogAvailableOptions(mode, sGroupFilter, pUtils);

  out_sBuffer = log.m_sBuffer;

  return res;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCommandLineOptionDoc::WCommandLineOptionDoc(WStringView sSortingGroup, WStringView sArgument, WStringView sParamShortDesc, WStringView sLongDesc, WStringView sDefaultValue, bool bCaseSensitive /*= false*/)
  : WCommandLineOption(sSortingGroup)
{
  m_sArgument = sArgument;
  m_sParamShortDesc = sParamShortDesc;
  m_sParamDefaultValue = sDefaultValue;
  m_sLongDesc = sLongDesc;
  m_bCaseSensitive = bCaseSensitive;
}

void WCommandLineOptionDoc::GetOptions(WStringBuilder& ref_sOut) const
{
  ref_sOut = m_sArgument;
}

void WCommandLineOptionDoc::GetParamShortDesc(WStringBuilder& ref_sOut) const
{
  ref_sOut = m_sParamShortDesc;
}

void WCommandLineOptionDoc::GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const
{
  ref_sOut = m_sParamDefaultValue;
}

void WCommandLineOptionDoc::GetLongDesc(WStringBuilder& ref_sOut) const
{
  ref_sOut = m_sLongDesc;
}

bool WCommandLineOptionDoc::IsOptionSpecified(WStringBuilder* out_pWhich, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  WStringBuilder sOptions, tmp;
  WHybridArray<WStringView, 4> eachOption;
  GetSplitOptions(sOptions, eachOption);

  for (auto o : eachOption)
  {
    if (pUtils->GetOptionIndex(o.GetData(tmp), m_bCaseSensitive) >= 0)
    {
      if (out_pWhich)
      {
        *out_pWhich = tmp;
      }

      return true;
    }
  }

  if (out_pWhich)
  {
    *out_pWhich = m_sArgument;
  }

  return false;
}


bool WCommandLineOptionDoc::ShouldLog(LogMode mode, bool bWasSpecified) const
{
  if (mode == LogMode::Never)
    return false;

  if (m_bLoggedOnce && (mode == LogMode::FirstTime || mode == LogMode::FirstTimeIfSpecified))
    return false;

  if (!bWasSpecified && (mode == LogMode::FirstTimeIfSpecified || mode == LogMode::AlwaysIfSpecified))
    return false;

  return true;
}

void WCommandLineOptionDoc::LogOption(WStringView sOption, WStringView sValue, bool bWasSpecified) const
{
  m_bLoggedOnce = true;

  if (bWasSpecified)
  {
    WLog::Info("Option '{}' is set to '{}'", sOption, sValue);
  }
  else
  {
    WLog::Info("Option '{}' is not set, default value is '{}'", sOption, sValue);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCommandLineOptionBool::WCommandLineOptionBool(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, bool bDefaultValue, bool bCaseSensitive /*= false*/)
  : WCommandLineOptionDoc(sSortingGroup, sArgument, "<bool>", sLongDesc, bDefaultValue ? "true" : "false", bCaseSensitive)
{
  m_bDefaultValue = bDefaultValue;
}

bool WCommandLineOptionBool::GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  bool result = m_bDefaultValue;

  WStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetBoolOption(sOption, m_bDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result ? "true" : "false", bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCommandLineOptionInt::WCommandLineOptionInt(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, int iDefaultValue, int iMinValue /*= WMath::MinValue<int>()*/, int iMaxValue /*= WMath::MaxValue<int>()*/, bool bCaseSensitive /*= false*/)
  : WCommandLineOptionDoc(sSortingGroup, sArgument, "<int>", sLongDesc, "0", bCaseSensitive)
{
  m_iDefaultValue = iDefaultValue;
  m_iMinValue = iMinValue;
  m_iMaxValue = iMaxValue;

  W_ASSERT_DEV(m_iMinValue < m_iMaxValue, "Invalid min/max value");
}

void WCommandLineOptionInt::GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const
{
  ref_sOut.SetFormat("{}", m_iDefaultValue);
}


void WCommandLineOptionInt::GetParamShortDesc(WStringBuilder& ref_sOut) const
{
  if (m_iMinValue == WMath::MinValue<int>() && m_iMaxValue == WMath::MaxValue<int>())
  {
    ref_sOut = "<int>";
  }
  else
  {
    ref_sOut.SetFormat("<int> [{} .. {}]", m_iMinValue, m_iMaxValue);
  }
}

int WCommandLineOptionInt::GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  int result = m_iDefaultValue;

  WStringBuilder sOption, tmp;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetIntOption(sOption, m_iDefaultValue, m_bCaseSensitive);

    if (result < m_iMinValue || result > m_iMaxValue)
    {
      if (ShouldLog(logMode, bSpecified))
      {
        WLog::Warning("Option '{}' selected value '{}' is outside valid range [{} .. {}]. Using default value instead.", sOption, result, m_iMinValue, m_iMaxValue);
      }

      result = m_iDefaultValue;
    }
  }

  if (ShouldLog(logMode, bSpecified))
  {
    tmp.SetFormat("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCommandLineOptionFloat::WCommandLineOptionFloat(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, float fDefaultValue, float fMinValue /*= WMath::MinValue<float>()*/, float fMaxValue /*= WMath::MaxValue<float>()*/, bool bCaseSensitive /*= false*/)
  : WCommandLineOptionDoc(sSortingGroup, sArgument, "<float>", sLongDesc, "0", bCaseSensitive)
{
  m_fDefaultValue = fDefaultValue;
  m_fMinValue = fMinValue;
  m_fMaxValue = fMaxValue;

  W_ASSERT_DEV(m_fMinValue < m_fMaxValue, "Invalid min/max value");
}

void WCommandLineOptionFloat::GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const
{
  ref_sOut.SetFormat("{}", m_fDefaultValue);
}

void WCommandLineOptionFloat::GetParamShortDesc(WStringBuilder& ref_sOut) const
{
  if (m_fMinValue == WMath::MinValue<float>() && m_fMaxValue == WMath::MaxValue<float>())
  {
    ref_sOut = "<float>";
  }
  else
  {
    ref_sOut.SetFormat("<float> [{} .. {}]", m_fMinValue, m_fMaxValue);
  }
}

float WCommandLineOptionFloat::GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  float result = m_fDefaultValue;

  WStringBuilder sOption, tmp;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = static_cast<float>(pUtils->GetFloatOption(sOption, m_fDefaultValue, m_bCaseSensitive));

    if (result < m_fMinValue || result > m_fMaxValue)
    {
      if (ShouldLog(logMode, bSpecified))
      {
        WLog::Warning("Option '{}' selected value '{}' is outside valid range [{} .. {}]. Using default value instead.", sOption, result, m_fMinValue, m_fMaxValue);
      }

      result = m_fDefaultValue;
    }
  }

  if (ShouldLog(logMode, bSpecified))
  {
    tmp.SetFormat("{}", result);
    LogOption(sOption, tmp, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCommandLineOptionString::WCommandLineOptionString(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, WStringView sDefaultValue, bool bCaseSensitive /*= false*/)
  : WCommandLineOptionDoc(sSortingGroup, sArgument, "<string>", sLongDesc, sDefaultValue, bCaseSensitive)
{
  m_sDefaultValue = sDefaultValue;
}

WStringView WCommandLineOptionString::GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  WStringView result = m_sDefaultValue;

  WStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetStringOption(sOption, 0, m_sDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WCommandLineOptionPath::WCommandLineOptionPath(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, WStringView sDefaultValue, bool bCaseSensitive /*= false*/)
  : WCommandLineOptionDoc(sSortingGroup, sArgument, "<path>", sLongDesc, sDefaultValue, bCaseSensitive)
{
  m_sDefaultValue = sDefaultValue;
}

WString WCommandLineOptionPath::GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  WString result = m_sDefaultValue;

  WStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  if (bSpecified)
  {
    result = pUtils->GetAbsolutePathOption(sOption, 0, m_sDefaultValue, m_bCaseSensitive);
  }

  if (ShouldLog(logMode, bSpecified))
  {
    LogOption(sOption, result, bSpecified);
  }

  return result;
}

WCommandLineOptionEnum::WCommandLineOptionEnum(WStringView sSortingGroup, WStringView sArgument, WStringView sLongDesc, WStringView sEnumKeysAndValues, WInt32 iDefaultValue, bool bCaseSensitive /*= false*/)
  : WCommandLineOptionDoc(sSortingGroup, sArgument, "<enum>", sLongDesc, "", bCaseSensitive)
{
  m_iDefaultValue = iDefaultValue;
  m_sEnumKeysAndValues = sEnumKeysAndValues;
}

WInt32 WCommandLineOptionEnum::GetOptionValue(LogMode logMode, const WCommandLineUtils* pUtils /*= WCommandLineUtils::GetGlobalInstance()*/) const
{
  WInt32 result = m_iDefaultValue;

  WStringBuilder sOption;
  const bool bSpecified = IsOptionSpecified(&sOption, pUtils);

  WHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  if (bSpecified)
  {
    WStringView selected = pUtils->GetStringOption(sOption, 0, "", m_bCaseSensitive);

    for (const auto& e : keysAndValues)
    {
      if (e.m_Key.IsEqual_NoCase(selected))
      {
        result = e.m_iValue;
        goto found;
      }
    }

    if (ShouldLog(logMode, bSpecified))
    {
      WLog::Warning("Option '{}' selected value '{}' is unknown. Using default value instead.", sOption, selected);
    }
  }

found:

  if (ShouldLog(logMode, bSpecified))
  {
    WStringBuilder opt;

    for (const auto& e : keysAndValues)
    {
      if (e.m_iValue == result)
      {
        opt = e.m_Key;
        break;
      }
    }

    LogOption(sOption, opt, bSpecified);
  }

  return result;
}

void WCommandLineOptionEnum::GetParamShortDesc(WStringBuilder& ref_sOut) const
{
  WHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    ref_sOut.AppendWithSeparator(" | ", e.m_Key);
  }

  ref_sOut.Prepend("<");
  ref_sOut.Append(">");
}

void WCommandLineOptionEnum::GetParamDefaultValueDesc(WStringBuilder& ref_sOut) const
{
  WHybridArray<EnumKeyValue, 16> keysAndValues;
  GetEnumKeysAndValues(keysAndValues);

  for (const auto& e : keysAndValues)
  {
    if (m_iDefaultValue == e.m_iValue)
    {
      ref_sOut = e.m_Key;
      return;
    }
  }
}

void WCommandLineOptionEnum::GetEnumKeysAndValues(WDynamicArray<EnumKeyValue>& out_keysAndValues) const
{
  WStringBuilder tmp = m_sEnumKeysAndValues;

  WHybridArray<WStringView, 16> enums;
  tmp.Split(false, enums, ";", "|");

  out_keysAndValues.SetCount(enums.GetCount());

  WInt32 eVal = 0;
  for (WUInt32 e = 0; e < enums.GetCount(); ++e)
  {
    WStringView eName;

    if (const char* eq = enums[e].FindSubString("="))
    {
      eName = WStringView(enums[e].GetStartPointer(), eq);

      W_VERIFY(WConversionUtils::StringToInt(eq + 1, eVal).Succeeded(), "Invalid enum declaration");
    }
    else
    {
      eName = enums[e];
    }

    eName.Trim(" \n\r\t=");

    const char* pStart = m_sEnumKeysAndValues.GetStartPointer();
    pStart += (WInt64)eName.GetStartPointer();
    pStart -= (WInt64)tmp.GetData();

    out_keysAndValues[e].m_iValue = eVal;
    out_keysAndValues[e].m_Key = WStringView(pStart, eName.GetElementCount());

    eVal++;
  }
}
