#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class WPhantomRTTI : public WRTTI
{
  friend class WPhantomRttiManager;

public:
  ~WPhantomRTTI();

private:
  WPhantomRTTI(WStringView sName, const WRTTI* pParentType, WUInt32 uiTypeSize, WUInt32 uiTypeVersion, WUInt8 uiVariantType,
    WBitflags<WTypeFlags> flags, WStringView sPluginName);

  void SetProperties(WDynamicArray<WReflectedPropertyDescriptor>& properties);
  void SetFunctions(WDynamicArray<WReflectedFunctionDescriptor>& functions);
  void SetAttributes(WDynamicArray<const WPropertyAttribute*>& attributes);
  bool IsEqualToDescriptor(const WReflectedTypeDescriptor& desc);

  void UpdateType(WReflectedTypeDescriptor& desc);

private:
  WString m_sTypeNameStorage;
  WString m_sPluginNameStorage;
  WDynamicArray<WAbstractProperty*> m_PropertiesStorage;
  WDynamicArray<WAbstractFunctionProperty*> m_FunctionsStorage;
  WDynamicArray<const WPropertyAttribute*> m_AttributesStorage;
};
