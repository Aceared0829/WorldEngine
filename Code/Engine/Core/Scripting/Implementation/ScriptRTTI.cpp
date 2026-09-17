#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptRTTI.h>
#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/ReflectionUtils.h>

WScriptRTTI::WScriptRTTI(WStringView sName, const WRTTI* pParentType, FunctionList&& functions, MessageHandlerList&& messageHandlers)
  : WRTTI(nullptr, pParentType, 0, 1, WVariantType::Invalid, WTypeFlags::Class, nullptr, WArrayPtr<const WAbstractProperty*>(), WArrayPtr<const WAbstractFunctionProperty*>(), WArrayPtr<const WPropertyAttribute*>(), WArrayPtr<WAbstractMessageHandler*>(), WArrayPtr<WMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_FunctionStorage(std::move(functions))
  , m_MessageHandlerStorage(std::move(messageHandlers))
{
  m_sTypeName = m_sTypeNameStorage.GetData();

  for (auto& pFunction : m_FunctionStorage)
  {
    if (pFunction != nullptr)
    {
      m_FunctionRawPtrs.PushBack(pFunction.Borrow());
    }
  }

  for (auto& pMessageHandler : m_MessageHandlerStorage)
  {
    if (pMessageHandler != nullptr)
    {
      m_MessageHandlerRawPtrs.PushBack(pMessageHandler.Borrow());
    }
  }

  m_Functions = m_FunctionRawPtrs;
  m_MessageHandlers = m_MessageHandlerRawPtrs;

  RegisterType();

  SetupParentHierarchy();
  GatherDynamicMessageHandlers();
}

WScriptRTTI::~WScriptRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;

  // RTTI base class will try to delete the contents of these arrays under the assumption that they were created during static init. Dynamically created types must ensure that these arrays are cleared out before the base class is executed.
  m_Properties.Clear();
  m_Functions.Clear();
  m_Attributes.Clear();
}

const WAbstractFunctionProperty* WScriptRTTI::GetFunctionByIndex(WUInt32 uiIndex) const
{
  if (uiIndex < m_FunctionStorage.GetCount())
  {
    return m_FunctionStorage.GetData()[uiIndex].Borrow();
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

WScriptFunctionProperty::WScriptFunctionProperty(WStringView sName)
  : WAbstractFunctionProperty(nullptr)
{
  m_sPropertyNameStorage.Assign(sName);
  m_szPropertyName = m_sPropertyNameStorage.GetData();
}

WScriptFunctionProperty::~WScriptFunctionProperty() = default;

//////////////////////////////////////////////////////////////////////////

WScriptMessageHandler::WScriptMessageHandler(const WScriptMessageDesc& desc)
  : m_Properties(desc.m_Properties)
{
  WUniquePtr<WMessage> pMessage = desc.m_pType->GetAllocator()->Allocate<WMessage>();

  m_Id = pMessage->GetId();
  m_bIsConst = true;
}

WScriptMessageHandler::~WScriptMessageHandler() = default;

void WScriptMessageHandler::FillMessagePropertyValues(const WMessage& msg, WDynamicArray<WVariant>& out_propertyValues)
{
  out_propertyValues.Clear();

  for (auto pProp : m_Properties)
  {
    if (pProp->GetCategory() == WPropertyCategory::Member)
    {
      // Special handling for WGameObjectHandle and WComponentHandle to avoid unnecessary allocations that happen when converting them to WVariant in WReflectionUtils::GetMemberPropertyValue.
      if (pProp->GetSpecificType() == WGetStaticRTTI<WGameObjectHandle>())
      {
        WGameObjectHandle hObject;
        static_cast<const WAbstractMemberProperty*>(pProp)->GetValuePtr(&msg, &hObject);
        out_propertyValues.PushBack(WVariant(hObject));
      }
      else if (pProp->GetSpecificType() == WGetStaticRTTI<WComponentHandle>())
      {
        WComponentHandle hComponent;
        static_cast<const WAbstractMemberProperty*>(pProp)->GetValuePtr(&msg, &hComponent);
        out_propertyValues.PushBack(WVariant(hComponent));
      }
      else
      {
        out_propertyValues.PushBack(WReflectionUtils::GetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), &msg));
      }
    }
    else if (pProp->GetCategory() == WPropertyCategory::Array)
    {
      auto pArrayProp = static_cast<const WAbstractArrayProperty*>(pProp);

      WVariantArray a;
      for (WUInt32 i = 0; i < pArrayProp->GetCount(&msg); ++i)
      {
        a.PushBack(WReflectionUtils::GetArrayPropertyValue(pArrayProp, &msg, i));
      }

      out_propertyValues.PushBack(a);
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

WScriptInstance::WScriptInstance(WReflectedClass& inout_owner, WWorld* pWorld)
  : m_Owner(inout_owner)
  , m_pWorld(pWorld)
{
}

void WScriptInstance::SetInstanceVariables(const WArrayMap<WHashedString, WVariant>& parameters)
{
  for (auto it : parameters)
  {
    SetInstanceVariable(it.key, it.value);
  }
}

//////////////////////////////////////////////////////////////////////////

// static
WAllocator* WScriptAllocator::GetAllocator()
{
  static WProxyAllocator s_ScriptAllocator("Script", WFoundation::GetDefaultAllocator());
  return &s_ScriptAllocator;
}
