#pragma once

#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

struct WStatus;

/// Helper class to modify an WVariant as if it was a container.
/// GetValue and SetValue are valid for all variant types.
/// The remaining accessor functions require an VariantArray or VariantDictionary type.
class W_TOOLSFOUNDATION_DLL WVariantStorageAccessor
{
public:
  WVariantStorageAccessor(WStringView sProperty, WVariant& value);
  WVariantStorageAccessor(WStringView sProperty, const WVariant& value);

  WVariant GetValue(WVariant index = WVariant(), WStatus* pRes = nullptr) const;
  WStatus SetValue(const WVariant& value, WVariant index = WVariant());

  WInt32 GetCount() const;
  WStatus GetKeys(WDynamicArray<WVariant>& out_keys) const;
  WStatus InsertValue(const WVariant& index, const WVariant& value);
  WStatus RemoveValue(const WVariant& index);
  WStatus MoveValue(const WVariant& oldIndex, const WVariant& newIndex);

private:
  WStringView m_sProperty;
  WVariant& m_Value;
};
