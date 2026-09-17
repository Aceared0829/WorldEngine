#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

struct WMaterialResourceSlot
{
  WString m_sLabel;
  WString m_sResource;
  bool m_bHighlight = false;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WMaterialResourceSlot);
