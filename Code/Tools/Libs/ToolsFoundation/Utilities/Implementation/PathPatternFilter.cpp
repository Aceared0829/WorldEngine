#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <ToolsFoundation/Utilities/PathPatternFilter.h>

void WPathPattern::Configure(const WStringView sText0)
{
  WStringView text = sText0;

  text.Trim(" \t\r\n");

  const bool bStart = text.StartsWith("*");
  const bool bEnd = text.EndsWith("*");

  text.Trim("*");
  m_sString = text;

  if (bStart && bEnd)
    m_MatchType = MatchType::Contains;
  else if (bStart)
    m_MatchType = MatchType::EndsWith;
  else if (bEnd)
    m_MatchType = MatchType::StartsWith;
  else
    m_MatchType = MatchType::Exact;
}

bool WPathPattern::Matches(const WStringView sText) const
{
  switch (m_MatchType)
  {
    case MatchType::Exact:
      return sText.IsEqual_NoCase(m_sString.GetView());
    case MatchType::StartsWith:
      return sText.StartsWith_NoCase(m_sString);
    case MatchType::EndsWith:
      return sText.EndsWith_NoCase(m_sString);
    case MatchType::Contains:
      return sText.FindSubString_NoCase(m_sString) != nullptr;
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return false;
}

//////////////////////////////////////////////////////////////////////////

bool WPathPatternFilter::PassesFilters(WStringView sText, WStringBuilder* pMatchingFilter) const
{
  for (const auto& filter : m_IncludePatterns)
  {
    // if any include pattern matches, that overrides the exclude patterns
    if (filter.Matches(sText))
    {
      if (pMatchingFilter)
      {
        pMatchingFilter->Clear();

        if (filter.m_MatchType == WPathPattern::EndsWith || filter.m_MatchType == WPathPattern::Contains)
          pMatchingFilter->Append("*");

        pMatchingFilter->Append(filter.m_sString);

        if (filter.m_MatchType == WPathPattern::StartsWith || filter.m_MatchType == WPathPattern::Contains)
          pMatchingFilter->Append("*");
      }

      return true;
    }
  }

  for (const auto& filter : m_ExcludePatterns)
  {
    // no include pattern matched, but any exclude pattern matches -> filter out
    if (filter.Matches(sText))
    {
      if (pMatchingFilter)
      {
        pMatchingFilter->Clear();

        if (filter.m_MatchType == WPathPattern::EndsWith || filter.m_MatchType == WPathPattern::Contains)
          pMatchingFilter->Append("*");

        pMatchingFilter->Append(filter.m_sString);

        if (filter.m_MatchType == WPathPattern::StartsWith || filter.m_MatchType == WPathPattern::Contains)
          pMatchingFilter->Append("*");
      }

      return false;
    }
  }

  // no filter matches at all -> include by default
  return true;
}

void WPathPatternFilter::AddFilter(WStringView sText, bool bIncludeFilter)
{
  WStringBuilder text = sText;
  text.MakeCleanPath();
  text.Trim(" \t\r\n");

  if (text.IsEmpty() || text.StartsWith("//"))
    return;

  if (!text.StartsWith("*") && !text.StartsWith("/"))
    text.Prepend("/");

  if (bIncludeFilter)
    m_IncludePatterns.ExpandAndGetRef().Configure(text);
  else
    m_ExcludePatterns.ExpandAndGetRef().Configure(text);
}

WResult WPathPatternFilter::ReadConfigFile(WStringView sFile, const WDynamicArray<WString>& preprocessorDefines)
{
  WStringBuilder content;

  WPreprocessor pp;
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);

  for (const auto& def : preprocessorDefines)
  {
    pp.AddCustomDefine(def).IgnoreResult();
  }

  // keep comments, because * and / can form a multi-line comment, and then we could lose vital information
  // instead only allow single-line comments and filter those out in AddFilter().
  if (pp.Process(sFile, content, true, true).Failed())
    return W_FAILURE;

  WDynamicArray<WStringView> lines;

  content.Split(false, lines, "\n", "\r");

  bool bIncludeFilter = false;

  for (auto line : lines)
  {
    if (line.IsEqual_NoCase("[INCLUDE]"))
    {
      bIncludeFilter = true;
      continue;
    }

    if (line.IsEqual_NoCase("[EXCLUDE]"))
    {
      bIncludeFilter = false;
      continue;
    }

    AddFilter(line, bIncludeFilter);
  }

  return W_SUCCESS;
}
