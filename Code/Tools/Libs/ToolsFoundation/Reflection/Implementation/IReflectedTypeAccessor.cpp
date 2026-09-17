#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>

bool WIReflectedTypeAccessor::GetValues(WStringView sProperty, WDynamicArray<WVariant>& out_values) const
{
  WTempHybridArray<WVariant, 16> keys;
  if (!GetKeys(sProperty, keys))
    return false;

  out_values.Clear();
  out_values.Reserve(keys.GetCount());
  for (const WVariant& key : keys)
  {
    out_values.PushBack(GetValue(sProperty, key));
  }
  return true;
}
