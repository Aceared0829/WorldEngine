#pragma once

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class WIReflectedTypeAccessor;
class WDocumentObject;
class WAbstractObjectGraph;

/// Helper functions for handling reflection related operations.
///
/// Also check out WToolsSerializationUtils for related functionality.
class W_TOOLSFOUNDATION_DLL WToolsReflectionUtils
{
public:
  /// Returns the type under which the property is stored on the editor side.
  static WVariantType::Enum GetStorageType(const WAbstractProperty* pProperty);

  /// Returns the default value for the entire property as it is stored on the editor side.
  static WVariant GetStorageDefault(const WAbstractProperty* pProperty);

  static bool GetFloatFromVariant(const WVariant& val, double& out_fValue);
  static bool GetVariantFromFloat(double fValue, WVariantType::Enum type, WVariant& out_val);

  /// Creates a ReflectedTypeDescriptor from an WRTTI instance that can be serialized and registered at the WPhantomRttiManager.
  static void GetReflectedTypeDescriptorFromRtti(const WRTTI* pRtti, WReflectedTypeDescriptor& out_desc); // [tested]
  static void GetMinimalReflectedTypeDescriptorFromRtti(const WRTTI* pRtti, WReflectedTypeDescriptor& out_desc);

  static void GatherObjectTypes(const WDocumentObject* pObject, WSet<const WRTTI*>& inout_types);

  static bool DependencySortTypeDescriptorArray(WDynamicArray<WReflectedTypeDescriptor*>& ref_descriptors);
};
