#include <Mcp/McpPCH.h>

#include <Mcp/McpJsonWriter.h>
#include <Mcp/McpTranslation.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/TranslationLookup.h>

namespace
{
  /// The echo guard: an unknown key comes back unchanged, which is indistinguishable from a translation
  /// that happens to equal its key - and treating the latter as missing loses nothing, since repeating
  /// the key tells the caller nothing it did not send.
  ///
  /// Missing-translation logging is off for the duration: these tools ask about reflection data wholesale,
  /// where most keys are expected to have no entry, and WTranslatorLogMissing would fill the editor log
  /// with a warning per property. The property grid disables it for the same reason.
  WStringView Lookup(WStringView sKey, WTranslationUsage usage)
  {
    if (sKey.IsEmpty())
      return {};

    const bool bLogMissing = WTranslatorLogMissing::s_bActive;
    WTranslatorLogMissing::s_bActive = false;

    const WStringView sResult = WTranslationLookup::Translate(sKey, WHashingUtils::StringHash(sKey), usage);

    WTranslatorLogMissing::s_bActive = bLogMissing;

    if (sResult == sKey)
      return {};

    return sResult;
  }

  /// True when the translation is what WTranslatorMakeMoreReadable derives from the key itself: the
  /// part behind the last '::' with spaces inserted at CamelCase boundaries. That translator is
  /// registered in the editor, so a key with no entry anywhere still comes back as readable text - and
  /// 'Projection Axis' for 'ProjectionAxis' is a token per property that tells the reader nothing it
  /// could not have written down itself. Genuine overrides ('Base Color Texture' for 'BaseColor')
  /// survive the comparison.
  bool IsDerivableFromKey(WStringView sTranslation, WStringView sKey)
  {
    const char* szScope = sKey.FindLastSubString("::");

    if (szScope != nullptr)
    {
      sKey.SetStartPosition(szScope + 2);
    }

    WStringBuilder sStripped = sTranslation;
    sStripped.ReplaceAll(" ", "");

    return sStripped.IsEqual_NoCase(sKey);
  }

  /// Walks up the type hierarchy because the entry sits on the type that declares the property, which for
  /// an inherited property is not the type the caller asked about.
  WStringView LookupProperty(const WRTTI* pType, WStringView sPropertyName, WTranslationUsage usage)
  {
    if (sPropertyName.IsEmpty())
      return {};

    WStringBuilder sKey;

    for (const WRTTI* pCurrent = pType; pCurrent != nullptr; pCurrent = pCurrent->GetParentType())
    {
      sKey.Set(pCurrent->GetTypeName(), "::", sPropertyName);

      const WStringView sResult = Lookup(sKey, usage);

      // A readable-ified key is not an entry: it is produced for every key, so accepting it here would
      // stop the walk on the first type and never reach the base type that declares the property.
      if (!sResult.IsEmpty() && !IsDerivableFromKey(sResult, sKey))
        return sResult;
    }

    return {};
  }
} // namespace

WStringView WMcpTranslation::GetDisplayName(WStringView sKey)
{
  const WStringView sResult = Lookup(sKey, WTranslationUsage::Default);

  if (IsDerivableFromKey(sResult, sKey))
    return {};

  return sResult;
}

WStringView WMcpTranslation::GetTooltip(WStringView sKey)
{
  return Lookup(sKey, WTranslationUsage::Tooltip);
}

WStringView WMcpTranslation::GetHelpURL(WStringView sKey)
{
  return Lookup(sKey, WTranslationUsage::HelpURL);
}

WStringView WMcpTranslation::GetPropertyDisplayName(const WRTTI* pType, WStringView sPropertyName)
{
  return LookupProperty(pType, sPropertyName, WTranslationUsage::Default);
}

WStringView WMcpTranslation::GetPropertyTooltip(const WRTTI* pType, WStringView sPropertyName)
{
  return LookupProperty(pType, sPropertyName, WTranslationUsage::Tooltip);
}

void WMcpTranslation::AddOptionalString(WMcpJsonWriter& ref_writer, WStringView sFieldName, WStringView sValue)
{
  if (sValue.IsEmpty())
    return;

  ref_writer.AddVariableString(sFieldName, sValue);
}
