#pragma once

#include <Foundation/Types/TypeTraits.h>

class WRTTI;

/// A typed raw pointer.
///
/// Common use case is the storage of object pointers inside an WVariant.
/// Has the same lifetime concerns that any other raw pointer.
/// \sa WVariant
struct WTypedPointer
{
  W_DECLARE_POD_TYPE();
  void* m_pObject = nullptr;
  const WRTTI* m_pType = nullptr;

  WTypedPointer() = default;
  WTypedPointer(void* pObject, const WRTTI* pType)
    : m_pObject(pObject)
    , m_pType(pType)
  {
  }

  bool operator==(const WTypedPointer& rhs) const
  {
    return m_pObject == rhs.m_pObject;
  }
  bool operator!=(const WTypedPointer& rhs) const
  {
    return m_pObject != rhs.m_pObject;
  }
};
