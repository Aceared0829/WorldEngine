#include <TexConv/TexConvPCH.h>

#include <TexConv/TexConv.h>

WResult WTexConv::ParseUIntOption(WStringView sOption, WInt32 iMinValue, WInt32 iMaxValue, WUInt32& ref_uiResult) const
{
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();
  const WUInt32 uiDefault = ref_uiResult;

  const WInt32 val = pCmd->GetIntOption(sOption, ref_uiResult);

  if (!WMath::IsInRange(val, iMinValue, iMaxValue))
  {
    WLog::Error("'{}' value {} is out of valid range [{}; {}]", sOption, val, iMinValue, iMaxValue);
    return W_FAILURE;
  }

  ref_uiResult = static_cast<WUInt32>(val);

  if (ref_uiResult == uiDefault)
  {
    WLog::Info("Using default '{}': '{}'.", sOption, ref_uiResult);
    return W_SUCCESS;
  }

  WLog::Info("Selected '{}': '{}'.", sOption, ref_uiResult);

  return W_SUCCESS;
}

WResult WTexConv::ParseStringOption(WStringView sOption, const WDynamicArray<KeyEnumValuePair>& allowed, WInt32& ref_iResult) const
{
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();
  const WStringBuilder sValue = pCmd->GetStringOption(sOption, 0);

  if (sValue.IsEmpty())
  {
    ref_iResult = allowed[0].m_iEnumValue;

    WLog::Info("Using default '{}': '{}'", sOption, allowed[0].m_sKey);
    return W_SUCCESS;
  }

  for (WUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (sValue.IsEqual_NoCase(allowed[i].m_sKey))
    {
      ref_iResult = allowed[i].m_iEnumValue;

      WLog::Info("Selected '{}': '{}'", sOption, allowed[i].m_sKey);
      return W_SUCCESS;
    }
  }

  WLog::Error("Unknown value for option '{}': '{}'.", sOption, sValue);

  PrintOptionValues(sOption, allowed);

  return W_FAILURE;
}

void WTexConv::PrintOptionValues(WStringView sOption, const WDynamicArray<KeyEnumValuePair>& allowed) const
{
  WLog::Info("Valid values for option '{}' are:", sOption);

  for (WUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    WLog::Info("  {}", allowed[i].m_sKey);
  }
}

void WTexConv::PrintOptionValuesHelp(WStringView sOption, const WDynamicArray<KeyEnumValuePair>& allowed) const
{
  WStringBuilder out(sOption, " ");

  for (WUInt32 i = 0; i < allowed.GetCount(); ++i)
  {
    if (i > 0)
      out.Append(" | ");

    out.Append(allowed[i].m_sKey);
  }

  WLog::Info(out);
}

bool WTexConv::ParseFile(WStringView sOption, WString& ref_sResult) const
{
  const auto pCmd = WCommandLineUtils::GetGlobalInstance();
  ref_sResult = pCmd->GetAbsolutePathOption(sOption);

  if (!ref_sResult.IsEmpty())
  {
    WLog::Info("'{}' file: '{}'", sOption, ref_sResult);
    return true;
  }
  else
  {
    WLog::Info("No '{}' file specified.", sOption);
    return false;
  }
}
