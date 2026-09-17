#pragma once

#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class WDocumentObject;

struct W_GUIFOUNDATION_DLL WPropertySelection
{
  const WDocumentObject* m_pObject;
  WVariant m_Index;

  bool operator==(const WPropertySelection& rhs) const { return m_pObject == rhs.m_pObject && m_Index == rhs.m_Index; }

  bool operator<(const WPropertySelection& rhs) const
  {
    // Qt6 requires the less than operator but never calls it, so we use this dummy for now.
    W_ASSERT_NOT_IMPLEMENTED;
    return false;
  }
};

struct W_GUIFOUNDATION_DLL WPropertyClipboard
{
  WString m_Type;
  WVariant m_Value;
  /// When set, contains a DDL serialized WAbstractObjectGraph of an entire object (e.g. a component).
  /// In that case m_Value is empty and m_Type is the RTTI type name of the serialized object.
  WString m_ObjectGraph;
};
W_DECLARE_REFLECTABLE_TYPE(W_GUIFOUNDATION_DLL, WPropertyClipboard)
