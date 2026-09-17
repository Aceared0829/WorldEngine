#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <ToolsFoundation/Reflection/PhantomProperty.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

WPhantomConstantProperty::WPhantomConstantProperty(const WReflectedPropertyDescriptor* pDesc)
  : WAbstractConstantProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_Value = pDesc->m_ConstantValue;
  m_pPropertyType = WRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(WPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

WPhantomConstantProperty::~WPhantomConstantProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
  m_Attributes.Clear();
}

const WRTTI* WPhantomConstantProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

void* WPhantomConstantProperty::GetPropertyPointer() const
{
  return nullptr;
}



WPhantomMemberProperty::WPhantomMemberProperty(const WReflectedPropertyDescriptor* pDesc)
  : WAbstractMemberProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = WRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(WPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

WPhantomMemberProperty::~WPhantomMemberProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
  m_Attributes.Clear();
}

const WRTTI* WPhantomMemberProperty::GetSpecificType() const
{
  return m_pPropertyType;
}



WPhantomFunctionProperty::WPhantomFunctionProperty(WReflectedFunctionDescriptor* pDesc)
  : WAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_FunctionType = pDesc->m_Type;
  m_Flags = pDesc->m_Flags;
  m_Flags.Add(WPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();

  m_ReturnValue = pDesc->m_ReturnValue;
  m_Arguments.Swap(pDesc->m_Arguments);
}



WPhantomFunctionProperty::~WPhantomFunctionProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
  m_Attributes.Clear();
}

WFunctionType::Enum WPhantomFunctionProperty::GetFunctionType() const
{
  return m_FunctionType;
}

const WRTTI* WPhantomFunctionProperty::GetReturnType() const
{
  return WRTTI::FindTypeByName(m_ReturnValue.m_sType);
}

WBitflags<WPropertyFlags> WPhantomFunctionProperty::GetReturnFlags() const
{
  return m_ReturnValue.m_Flags;
}

WUInt32 WPhantomFunctionProperty::GetArgumentCount() const
{
  return m_Arguments.GetCount();
}

const WRTTI* WPhantomFunctionProperty::GetArgumentType(WUInt32 uiParamIndex) const
{
  return WRTTI::FindTypeByName(m_Arguments[uiParamIndex].m_sType);
}

WBitflags<WPropertyFlags> WPhantomFunctionProperty::GetArgumentFlags(WUInt32 uiParamIndex) const
{
  return m_Arguments[uiParamIndex].m_Flags;
}

void WPhantomFunctionProperty::Execute(void* pInstance, WArrayPtr<WVariant> values, WVariant& ref_returnValue) const
{
  W_ASSERT_NOT_IMPLEMENTED;
}

WPhantomArrayProperty::WPhantomArrayProperty(const WReflectedPropertyDescriptor* pDesc)
  : WAbstractArrayProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = WRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(WPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

WPhantomArrayProperty::~WPhantomArrayProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
  m_Attributes.Clear();
}

const WRTTI* WPhantomArrayProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

WPhantomSetProperty::WPhantomSetProperty(const WReflectedPropertyDescriptor* pDesc)
  : WAbstractSetProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = WRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(WPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

WPhantomSetProperty::~WPhantomSetProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
  m_Attributes.Clear();
}

const WRTTI* WPhantomSetProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

WPhantomMapProperty::WPhantomMapProperty(const WReflectedPropertyDescriptor* pDesc)
  : WAbstractMapProperty(nullptr)
{
  m_sPropertyNameStorage = pDesc->m_sName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = WRTTI::FindTypeByName(pDesc->m_sType);

  m_Flags = pDesc->m_Flags;
  m_Flags.Add(WPropertyFlags::Phantom);
  m_Attributes = pDesc->m_Attributes;
  pDesc->m_Attributes.Clear();
}

WPhantomMapProperty::~WPhantomMapProperty()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(const_cast<WPropertyAttribute*>(pAttr));
  }
  m_Attributes.Clear();
}

const WRTTI* WPhantomMapProperty::GetSpecificType() const
{
  return m_pPropertyType;
}
