#pragma once

#include <Foundation/Strings/String.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A helper class to implement a multi-part, case-insensitive search pattern filter with support for exclusions.
///
/// The search text is split into multiple parts by spaces. A text passes the filter if it contains all parts.
/// The check is always case insensitive and the order of the parts does not matter.
/// It is also possible to exclude parts by prefixing them with a minus.
/// E.g. "com mesh" would pass all texts that contain "com" and "mesh" like WMeshComponent.
/// "com -mesh" would pass WLightComponent but would fail WMeshComponent.
class W_TOOLSFOUNDATION_DLL WSearchPatternFilter
{
public:
  /// Sets the search text and splits it into its part for faster checks.
  void SetSearchText(WStringView sSearchText);

  /// Returns the current search text.
  const WString& GetSearchText() const { return m_sSearchText; }
  /// Returns true if the search text is empty.
  bool IsEmpty() const { return m_sSearchText.IsEmpty(); }

  /// Returns true if the filter contains any exclusion patterns.
  bool ContainsExclusions() const;

  /// Determines whether the given text matches the filter patterns.
  bool PassesFilters(WStringView sText) const;

private:
  WString m_sSearchText;

  struct Part
  {
    WStringView m_sPart;
    bool m_bExclude = false;
  };

  WHybridArray<Part, 4> m_Parts;
};
