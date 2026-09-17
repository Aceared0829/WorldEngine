#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WRTTI;
class WPhantomRttiManager;
class WReflectedTypeStorageManager;

/// Event message used by the WPhantomRttiManager.
struct W_TOOLSFOUNDATION_DLL WPhantomTypeChange
{
  const WRTTI* m_pChangedType = nullptr;
};

struct W_TOOLSFOUNDATION_DLL WAttributeHolder
{
  WAttributeHolder();
  WAttributeHolder(const WAttributeHolder& rhs);
  virtual ~WAttributeHolder();

  WUInt32 GetCount() const;
  const WPropertyAttribute* GetValue(WUInt32 uiIndex) const;
  void SetValue(WUInt32 uiIndex, const WPropertyAttribute* value);
  void Insert(WUInt32 uiIndex, const WPropertyAttribute* value);
  void Remove(WUInt32 uiIndex);

  void operator=(const WAttributeHolder& rhs);

  mutable WHybridArray<const WPropertyAttribute*, 2> m_Attributes;
  WArrayPtr<const WPropertyAttribute* const> m_ReferenceAttributes;
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WAttributeHolder);

/// Stores the description of a reflected property in a serializable form, used by WReflectedTypeDescriptor.
struct W_TOOLSFOUNDATION_DLL WReflectedPropertyDescriptor : public WAttributeHolder
{
  WReflectedPropertyDescriptor() = default;
  WReflectedPropertyDescriptor(WPropertyCategory::Enum category, WStringView sName, WStringView sType, WBitflags<WPropertyFlags> flags);
  WReflectedPropertyDescriptor(WPropertyCategory::Enum category, WStringView sName, WStringView sType, WBitflags<WPropertyFlags> flags,
    WArrayPtr<const WPropertyAttribute* const> attributes); // [tested]
  /// Initialize to a constant.
  WReflectedPropertyDescriptor(WStringView sName, const WVariant& constantValue, WArrayPtr<const WPropertyAttribute* const> attributes); // [tested]
  WReflectedPropertyDescriptor(const WReflectedPropertyDescriptor& rhs);
  ~WReflectedPropertyDescriptor();

  void operator=(const WReflectedPropertyDescriptor& rhs);

  WEnum<WPropertyCategory> m_Category;
  WString m_sName; ///< The name of this property. E.g. what WAbstractProperty::GetPropertyName() returns.
  WString m_sType; ///< The name of the type of the property. E.g. WAbstractProperty::GetSpecificType().GetTypeName()

  /// WPropertyFlags::Phantom is not part of a descriptor: it is added by the WPhantom*Property classes when the
  /// descriptor is registered, so setting it here has no effect other than writing it into serialized documents.
  WBitflags<WPropertyFlags> m_Flags;
  WVariant m_ConstantValue;
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WReflectedPropertyDescriptor);

struct W_TOOLSFOUNDATION_DLL WFunctionArgumentDescriptor
{
  WFunctionArgumentDescriptor();
  WFunctionArgumentDescriptor(WStringView sType, WBitflags<WPropertyFlags> flags);
  WString m_sType;
  WBitflags<WPropertyFlags> m_Flags;
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WFunctionArgumentDescriptor);

/// Stores the description of a reflected function in a serializable form, used by WReflectedTypeDescriptor.
struct W_TOOLSFOUNDATION_DLL WReflectedFunctionDescriptor : public WAttributeHolder
{
  WReflectedFunctionDescriptor();
  WReflectedFunctionDescriptor(WStringView sName, WBitflags<WPropertyFlags> flags, WEnum<WFunctionType> type, WArrayPtr<const WPropertyAttribute* const> attributes);

  WReflectedFunctionDescriptor(const WReflectedFunctionDescriptor& rhs);
  ~WReflectedFunctionDescriptor();

  void operator=(const WReflectedFunctionDescriptor& rhs);

  WString m_sName;
  WBitflags<WPropertyFlags> m_Flags;
  WEnum<WFunctionType> m_Type;
  WFunctionArgumentDescriptor m_ReturnValue;
  WDynamicArray<WFunctionArgumentDescriptor> m_Arguments;
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WReflectedFunctionDescriptor);


/// Stores the description of a reflected type in a serializable form. Used by WPhantomRttiManager to add new types.
struct W_TOOLSFOUNDATION_DLL WReflectedTypeDescriptor : public WAttributeHolder
{
  ~WReflectedTypeDescriptor();

  WString m_sTypeName;
  WString m_sPluginName;
  WString m_sParentTypeName;

  /// WTypeFlags::Phantom is not part of a descriptor: WPhantomRTTI adds it to every type it creates, so setting it
  /// here has no effect other than writing it into serialized documents.
  WBitflags<WTypeFlags> m_Flags;
  WDynamicArray<WReflectedPropertyDescriptor> m_Properties;
  WDynamicArray<WReflectedFunctionDescriptor> m_Functions;
  WUInt32 m_uiTypeVersion = 1;
};
W_DECLARE_REFLECTABLE_TYPE(W_TOOLSFOUNDATION_DLL, WReflectedTypeDescriptor);
