#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>

WQtAnimationGraphNode::WQtAnimationGraphNode() = default;

void WQtAnimationGraphNode::UpdateState()
{
  TitleFormat format;
  format.m_uiMaxStringLength = 0;
  format.m_uiMaxTitleLength = 30;
  format.m_bQuoteStrings = false;
  format.m_bIgnoreInvalidProperties = true;

  WStringBuilder sTemplate;
  if (!TryGetTitleTemplateFromProperty("CustomTitle", sTemplate) && !TryGetTitleTemplateFromAttribute(sTemplate))
  {
    GetDefaultTitleTemplate(sTemplate);
  }

  WStringBuilder sTitle;
  WTokenParseUtils::RenderTemplate(sTemplate, [&](WStringView sPlaceholder, WVariant index, bool bOptional, WStringBuilder& ref_sOutput)
    { ResolvePropertyPlaceholder(sPlaceholder, index, bOptional, format, ref_sOutput); }, sTitle);

  // placeholders that resolved to nothing leave empty quotes and duplicate spaces behind
  sTitle.ReplaceAll("''", "");
  sTitle.ReplaceAll("\"\"", "");
  sTitle.ReplaceAll("  ", " ");
  sTitle.Trim(" ");

  if (sTitle.IsEmpty())
    return;

  SetTitleAndSubtitle(sTitle, format);
}
