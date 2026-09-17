#include <Mcp/McpPCH.h>

#include <Mcp/McpJson.h>

#include <Foundation/Utilities/ConversionUtils.h>

WStringView WMcpJson::GetString(const WVariantDictionary& dict, WStringView sKey, WStringView sFallback)
{
  const WVariant* pValue = nullptr;

  if (dict.TryGetValue(sKey, pValue) && pValue->IsA<WString>())
  {
    return pValue->Get<WString>().GetView();
  }

  return sFallback;
}

WInt64 WMcpJson::GetInt(const WVariantDictionary& dict, WStringView sKey, WInt64 iFallback)
{
  const WVariant* pValue = nullptr;

  if (!dict.TryGetValue(sKey, pValue) || !pValue->IsValid())
    return iFallback;

  if (pValue->IsNumber())
    return static_cast<WInt64>(pValue->ConvertTo<double>());

  if (pValue->IsA<WString>())
  {
    WInt64 iResult = 0;
    if (WConversionUtils::StringToInt64(pValue->Get<WString>(), iResult).Succeeded())
      return iResult;
  }

  return iFallback;
}

bool WMcpJson::GetBool(const WVariantDictionary& dict, WStringView sKey, bool bFallback)
{
  const WVariant* pValue = nullptr;

  if (!dict.TryGetValue(sKey, pValue) || !pValue->IsValid())
    return bFallback;

  if (pValue->IsA<bool>())
    return pValue->Get<bool>();

  if (pValue->IsNumber())
    return pValue->ConvertTo<double>() != 0.0;

  if (pValue->IsA<WString>())
  {
    bool bResult = false;
    if (WConversionUtils::StringToBool(pValue->Get<WString>(), bResult) == W_SUCCESS)
      return bResult;
  }

  return bFallback;
}

bool WMcpJson::GetStringArray(const WVariantDictionary& dict, WStringView sKey, WDynamicArray<WString>& out_values)
{
  const WVariant* pValue = nullptr;

  if (!dict.TryGetValue(sKey, pValue) || !pValue->IsValid())
    return false;

  if (pValue->IsA<WString>())
  {
    out_values.PushBack(pValue->Get<WString>());
    return true;
  }

  if (!pValue->IsA<WVariantArray>())
    return false;

  for (const WVariant& element : pValue->Get<WVariantArray>())
  {
    if (element.CanConvertTo<WString>())
      out_values.PushBack(element.ConvertTo<WString>());
  }

  return true;
}

const WVariantDictionary* WMcpJson::GetDict(const WVariantDictionary& dict, WStringView sKey)
{
  const WVariant* pValue = nullptr;

  if (dict.TryGetValue(sKey, pValue) && pValue->IsA<WVariantDictionary>())
  {
    return &pValue->Get<WVariantDictionary>();
  }

  return nullptr;
}

const WVariantArray* WMcpJson::GetArray(const WVariantDictionary& dict, WStringView sKey)
{
  const WVariant* pValue = nullptr;

  if (dict.TryGetValue(sKey, pValue) && pValue->IsA<WVariantArray>())
  {
    return &pValue->Get<WVariantArray>();
  }

  return nullptr;
}
