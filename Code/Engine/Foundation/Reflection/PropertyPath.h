#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Variant.h>

class WAbstractProperty;


///Reflected property step that can be used to init an WPropertyPath
struct W_FOUNDATION_DLL WPropertyPathStep
{
  WString m_sProperty;
  WVariant m_Index;
};
W_DECLARE_REFLECTABLE_TYPE(W_FOUNDATION_DLL, WPropertyPathStep);

/// Stores a path from an object of a given type to a property inside of it.
///
/// Once initialized to a specific path, the target property/object of the path can be read or written on
/// multiple root objects. This is useful for implementing property binding, serialization, and generic
/// property editors.
///
/// Path syntax: 'propertyName[index]/propertyName[index]/...'
/// - The '[index]' part is only required for properties that need indices (arrays and maps)
/// - Example: "Transform/Position[0]" accesses the X component of a Position vector in a Transform property
/// - An empty path is allowed, in which case operations work directly on the root object
///
/// Usage pattern:
/// 1. Create WPropertyPath instance
/// 2. Initialize with InitializeFromPath() using root type and path string
/// 3. Use SetValue()/GetValue() for simple access or WriteProperty()/ReadProperty() for complex operations
/// 4. Reuse the same path instance for multiple objects of the same root type
class W_FOUNDATION_DLL WPropertyPath
{
public:
  WPropertyPath();
  ~WPropertyPath();

  /// Returns true if InitializeFromPath() has been successfully called and it is therefore possible to use the other functions.
  bool IsValid() const;

  /// Resolves a path string into property steps and validates them against the root type.
  ///
  /// The path syntax is 'propertyName[index]/propertyName[index]/...'. The '[index]' part is only
  /// required for properties that need indices (arrays and maps). Returns failure if any property
  /// in the path doesn't exist or has incompatible types.
  WResult InitializeFromPath(const WRTTI& rootObjectRtti, const char* szPath);

  /// Resolves a path provided as an array of WPropertyPathStep and validates it.
  ///
  /// This overload allows programmatic construction of paths. Each step must have a valid property
  /// name and appropriate index (if required by the property type).
  WResult InitializeFromPath(const WRTTI* pRootObjectRtti, const WArrayPtr<const WPropertyPathStep> path);

  ///Applies the entire path and allows writing to the target object.
  WResult WriteToLeafObject(void* pRootObject, const WRTTI* pType, WDelegate<void(void* pLeaf, const WRTTI& pType)> func) const;
  ///Applies the entire path and allows reading from the target object.
  WResult ReadFromLeafObject(void* pRootObject, const WRTTI* pType, WDelegate<void(void* pLeaf, const WRTTI& pType)> func) const;

  ///Applies the path up to the last step and allows a functor to write to the final property.
  WResult WriteProperty(
    void* pRootObject, const WRTTI& type, WDelegate<void(void* pLeafObject, const WRTTI& pLeafType, const WAbstractProperty* pProp, const WVariant& index)> func) const;
  ///Applies the path up to the last step and allows a functor to read from the final property.
  WResult ReadProperty(
    void* pRootObject, const WRTTI& type, WDelegate<void(void* pLeafObject, const WRTTI& pLeafType, const WAbstractProperty* pProp, const WVariant& index)> func) const;

  ///Convenience function that writes 'value' to the 'pRootObject' at the current path.
  void SetValue(void* pRootObject, const WRTTI& type, const WVariant& value) const;
  ///Convenience function that writes 'value' to the 'pRootObject' at the current path.
  template <typename T>
  W_ALWAYS_INLINE void SetValue(T* pRootObject, const WVariant& value) const
  {
    SetValue(pRootObject, *WGetStaticRTTI<T>(), value);
  }

  ///Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  void GetValue(void* pRootObject, const WRTTI& type, WVariant& out_value) const;
  ///Convenience function that reads the value from 'pRootObject' at the current path and stores it in 'out_value'.
  template <typename T>
  W_ALWAYS_INLINE void GetValue(T* pRootObject, WVariant& out_value) const
  {
    GetValue(pRootObject, *WGetStaticRTTI<T>(), out_value);
  }

private:
  struct ResolvedStep
  {
    const WAbstractProperty* m_pProperty = nullptr;
    WVariant m_Index;
  };

  static WResult ResolvePath(void* pCurrentObject, const WRTTI* pType, const WArrayPtr<const ResolvedStep> path, bool bWriteToObject,
    const WDelegate<void(void* pLeaf, const WRTTI& pType)>& func);

  bool m_bIsValid = false;
  WHybridArray<ResolvedStep, 2> m_PathSteps;
};
