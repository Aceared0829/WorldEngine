#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/PhantomRtti.h>

WPhantomRTTI::WPhantomRTTI(WStringView sName, const WRTTI* pParentType, WUInt32 uiTypeSize, WUInt32 uiTypeVersion, WUInt8 uiVariantType,
  WBitflags<WTypeFlags> flags, WStringView sPluginName)
  : WRTTI(nullptr, pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags | WTypeFlags::Phantom, nullptr, WArrayPtr<const WAbstractProperty*>(),
      WArrayPtr<const WAbstractFunctionProperty*>(), WArrayPtr<const WPropertyAttribute*>(), WArrayPtr<WAbstractMessageHandler*>(),
      WArrayPtr<WMessageSenderInfo>(), nullptr)
{
  m_sTypeNameStorage = sName;
  m_sPluginNameStorage = sPluginName;

  m_sTypeName = m_sTypeNameStorage.GetData();
  m_sPluginName = m_sPluginNameStorage.GetData();

  RegisterType();
}

WPhantomRTTI::~WPhantomRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;

  for (auto pProp : m_PropertiesStorage)
  {
    W_DEFAULT_DELETE(pProp);
  }
  m_PropertiesStorage.Clear();
  m_Properties.Clear();

  for (auto pFunc : m_FunctionsStorage)
  {
    W_DEFAULT_DELETE(pFunc);
  }
  m_FunctionsStorage.Clear();
  m_Functions.Clear();

  for (auto pAttrib : m_AttributesStorage)
  {
    auto pAttribNonConst = const_cast<WPropertyAttribute*>(pAttrib);
    W_DEFAULT_DELETE(pAttribNonConst);
  }
  m_AttributesStorage.Clear();
  m_Attributes.Clear();
}

void WPhantomRTTI::SetProperties(WDynamicArray<WReflectedPropertyDescriptor>& properties)
{
  for (auto pProp : m_PropertiesStorage)
  {
    W_DEFAULT_DELETE(pProp);
  }
  m_PropertiesStorage.Clear();

  const WUInt32 iCount = properties.GetCount();
  m_PropertiesStorage.Reserve(iCount);

  for (WUInt32 i = 0; i < iCount; i++)
  {
    switch (properties[i].m_Category)
    {
      case WPropertyCategory::Constant:
      {
        m_PropertiesStorage.PushBack(W_DEFAULT_NEW(WPhantomConstantProperty, &properties[i]));
      }
      break;
      case WPropertyCategory::Member:
      {
        m_PropertiesStorage.PushBack(W_DEFAULT_NEW(WPhantomMemberProperty, &properties[i]));
      }
      break;
      case WPropertyCategory::Array:
      {
        m_PropertiesStorage.PushBack(W_DEFAULT_NEW(WPhantomArrayProperty, &properties[i]));
      }
      break;
      case WPropertyCategory::Set:
      {
        m_PropertiesStorage.PushBack(W_DEFAULT_NEW(WPhantomSetProperty, &properties[i]));
      }
      break;
      case WPropertyCategory::Map:
      {
        m_PropertiesStorage.PushBack(W_DEFAULT_NEW(WPhantomMapProperty, &properties[i]));
      }
      break;
      case WPropertyCategory::Function:
        break; // Handled in SetFunctions
    }
  }

  m_Properties = m_PropertiesStorage.GetArrayPtr();
}


void WPhantomRTTI::SetFunctions(WDynamicArray<WReflectedFunctionDescriptor>& functions)
{
  for (auto pProp : m_FunctionsStorage)
  {
    W_DEFAULT_DELETE(pProp);
  }
  m_FunctionsStorage.Clear();

  const WUInt32 iCount = functions.GetCount();
  m_FunctionsStorage.Reserve(iCount);

  for (WUInt32 i = 0; i < iCount; i++)
  {
    m_FunctionsStorage.PushBack(W_DEFAULT_NEW(WPhantomFunctionProperty, &functions[i]));
  }

  m_Functions = m_FunctionsStorage.GetArrayPtr();
}

void WPhantomRTTI::SetAttributes(WDynamicArray<const WPropertyAttribute*>& attributes)
{
  for (auto pAttrib : m_AttributesStorage)
  {
    auto pAttribNonConst = const_cast<WPropertyAttribute*>(pAttrib);
    W_DEFAULT_DELETE(pAttribNonConst);
  }
  m_AttributesStorage.Clear();
  m_AttributesStorage = attributes;
  m_Attributes = m_AttributesStorage;
  attributes.Clear();
}

void WPhantomRTTI::UpdateType(WReflectedTypeDescriptor& desc)
{
  // WRTTI::UpdateType overwrites the type flags, so Phantom has to be added here just like in the constructor,
  // otherwise a type would stop being phantom when it is registered a second time with a changed descriptor
  WRTTI::UpdateType(WRTTI::FindTypeByName(desc.m_sParentTypeName), 0, desc.m_uiTypeVersion, WVariantType::Invalid, desc.m_Flags | WTypeFlags::Phantom);

  m_sPluginNameStorage = desc.m_sPluginName;
  m_sPluginName = m_sPluginNameStorage.GetData();

  SetProperties(desc.m_Properties);
  SetFunctions(desc.m_Functions);
  SetAttributes(desc.m_Attributes);
  SetupParentHierarchy();
}

bool WPhantomRTTI::IsEqualToDescriptor(const WReflectedTypeDescriptor& desc)
{
  if ((desc.m_Flags.GetValue() & ~WTypeFlags::Phantom) != (GetTypeFlags().GetValue() & ~WTypeFlags::Phantom))
    return false;

  if (desc.m_sParentTypeName.IsEmpty() && GetParentType() != nullptr)
    return false;

  if (GetParentType() != nullptr && desc.m_sParentTypeName != GetParentType()->GetTypeName())
    return false;

  if (desc.m_sPluginName != GetPluginName())
    return false;

  if (desc.m_sTypeName != GetTypeName())
    return false;

  if (desc.m_Properties.GetCount() != GetProperties().GetCount())
    return false;

  for (WUInt32 i = 0; i < GetProperties().GetCount(); i++)
  {
    if (desc.m_Properties[i].m_Category != GetProperties()[i]->GetCategory())
      return false;

    if (desc.m_Properties[i].m_sName != GetProperties()[i]->GetPropertyName())
      return false;

    if ((desc.m_Properties[i].m_Flags.GetValue() & ~WPropertyFlags::Phantom) !=
        (GetProperties()[i]->GetFlags().GetValue() & ~WPropertyFlags::Phantom))
      return false;

    switch (desc.m_Properties[i].m_Category)
    {
      case WPropertyCategory::Constant:
      {
        auto pProp = (WPhantomConstantProperty*)GetProperties()[i];

        if (pProp->GetSpecificType() != WRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;

        if (pProp->GetConstant() != desc.m_Properties[i].m_ConstantValue)
          return false;
      }
      break;
      case WPropertyCategory::Member:
      {
        if (GetProperties()[i]->GetSpecificType() != WRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case WPropertyCategory::Array:
      {
        if (GetProperties()[i]->GetSpecificType() != WRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case WPropertyCategory::Set:
      {
        if (GetProperties()[i]->GetSpecificType() != WRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case WPropertyCategory::Map:
      {
        if (GetProperties()[i]->GetSpecificType() != WRTTI::FindTypeByName(desc.m_Properties[i].m_sType))
          return false;
      }
      break;
      case WPropertyCategory::Function:
        break; // Functions handled below
    }

    if (desc.m_Functions.GetCount() != GetFunctions().GetCount())
      return false;

    for (WUInt32 j = 0; j < GetFunctions().GetCount(); j++)
    {
      const WAbstractFunctionProperty* pProp = GetFunctions()[j];
      if (desc.m_Functions[j].m_sName != pProp->GetPropertyName())
        return false;
      if ((desc.m_Functions[j].m_Flags.GetValue() & ~WPropertyFlags::Phantom) != (pProp->GetFlags().GetValue() & ~WPropertyFlags::Phantom))
        return false;
      if (desc.m_Functions[j].m_Type != pProp->GetFunctionType())
        return false;

      if (pProp->GetReturnType() != WRTTI::FindTypeByName(desc.m_Functions[j].m_ReturnValue.m_sType))
        return false;
      if (pProp->GetReturnFlags() != desc.m_Functions[j].m_ReturnValue.m_Flags)
        return false;
      if (desc.m_Functions[j].m_Arguments.GetCount() != pProp->GetArgumentCount())
        return false;
      for (WUInt32 a = 0; a < pProp->GetArgumentCount(); a++)
      {
        if (pProp->GetArgumentType(a) != WRTTI::FindTypeByName(desc.m_Functions[j].m_Arguments[a].m_sType))
          return false;
        if (pProp->GetArgumentFlags(a) != desc.m_Functions[j].m_Arguments[a].m_Flags)
          return false;
      }
    }

    if (desc.m_Properties[i].m_Attributes.GetCount() != GetProperties()[i]->GetAttributes().GetCount())
      return false;

    for (WUInt32 i2 = 0; i2 < desc.m_Properties[i].m_Attributes.GetCount(); i2++)
    {
      if (!WReflectionUtils::IsEqual(desc.m_Properties[i].m_Attributes[i2], GetProperties()[i]->GetAttributes()[i2]))
        return false;
    }
  }

  if (desc.m_Attributes.GetCount() != GetAttributes().GetCount())
    return false;

  // TODO: compare attribute values?
  for (WUInt32 i = 0; i < GetAttributes().GetCount(); i++)
  {
    if (desc.m_Attributes[i]->GetDynamicRTTI() != GetAttributes()[i]->GetDynamicRTTI())
      return false;
  }
  return true;
}
