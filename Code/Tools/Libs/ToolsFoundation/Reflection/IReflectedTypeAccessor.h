#pragma once

#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

class WDocumentObject;
struct WStatus;

/// Provides access to the properties of an WRTTI compatible data storage.
class W_TOOLSFOUNDATION_DLL WIReflectedTypeAccessor
{
public:
  /// Constructor for the WIReflectedTypeAccessor.
  ///
  /// It is a valid implementation to pass an invalid handle. Note that in this case there is no way to determine
  /// what is actually stored inside. However, it can be useful to use e.g. the WReflectedTypeDirectAccessor
  /// to set properties on the engine runtime side without having the WPhantomRttiManager initialized.
  WIReflectedTypeAccessor(const WRTTI* pRtti, WDocumentObject* pOwner)
    : m_pRtti(pRtti)
    , m_pOwner(pOwner)
  {
  } // [tested]

  /// Returns the WRTTI* of the wrapped instance type.
  const WRTTI* GetType() const { return m_pRtti; } // [tested]

  /// Returns the value of the property defined by its path. Return value is invalid iff the path was invalid.
  virtual const WVariant GetValue(WStringView sProperty, WVariant index = WVariant(), WStatus* pRes = nullptr) const = 0;

  /// Sets a property defined by its path to the given value. Returns whether the operation was successful.
  virtual bool SetValue(WStringView sProperty, const WVariant& value, WVariant index = WVariant()) = 0;

  virtual WInt32 GetCount(WStringView sProperty) const = 0;
  virtual bool GetKeys(WStringView sProperty, WDynamicArray<WVariant>& out_keys) const = 0;

  virtual bool InsertValue(WStringView sProperty, WVariant index, const WVariant& value) = 0;
  virtual bool RemoveValue(WStringView sProperty, WVariant index) = 0;
  virtual bool MoveValue(WStringView sProperty, WVariant oldIndex, WVariant newIndex) = 0;

  virtual WVariant GetPropertyChildIndex(WStringView sProperty, const WVariant& value) const = 0;

  const WDocumentObject* GetOwner() const { return m_pOwner; }

  bool GetValues(WStringView sProperty, WDynamicArray<WVariant>& out_values) const;


private:
  friend class WDocumentObjectManager;
  friend class WDocumentObject;

  const WRTTI* m_pRtti;
  WDocumentObject* m_pOwner;
};
