#pragma once

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>

/// An WIReflectedTypeAccessor implementation that also stores the actual data that is defined in the passed WRTTI.
///
/// This class is used to store data on the tool side for classes that are not known to the tool but exist outside of it
/// like engine components. As this is basically a complex value map the used type can be hot-reloaded. For this, the
/// WRTTI just needs to be updated with its new definition in the WPhantomRttiManager and all WReflectedTypeStorageAccessor
/// will be automatically rearranged to match the new class layout.
class W_TOOLSFOUNDATION_DLL WReflectedTypeStorageAccessor : public WIReflectedTypeAccessor
{
  friend class WReflectedTypeStorageManager;

public:
  WReflectedTypeStorageAccessor(const WRTTI* pReflectedType, WDocumentObject* pOwner);                                           // [tested]
  ~WReflectedTypeStorageAccessor();

  virtual const WVariant GetValue(WStringView sProperty, WVariant index = WVariant(), WStatus* pRes = nullptr) const override; // [tested]
  virtual bool SetValue(WStringView sProperty, const WVariant& value, WVariant index = WVariant()) override;                    // [tested]

  virtual WInt32 GetCount(WStringView sProperty) const override;
  virtual bool GetKeys(WStringView sProperty, WDynamicArray<WVariant>& out_keys) const override;

  virtual bool InsertValue(WStringView sProperty, WVariant index, const WVariant& value) override;
  virtual bool RemoveValue(WStringView sProperty, WVariant index) override;
  virtual bool MoveValue(WStringView sProperty, WVariant oldIndex, WVariant newIndex) override;

  virtual WVariant GetPropertyChildIndex(WStringView sProperty, const WVariant& value) const override;

private:
  WDynamicArray<WVariant> m_Data;
  const WReflectedTypeStorageManager::ReflectedTypeStorageMapping* m_pMapping;
};
