#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/MessageHandler.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>

struct WTypeData
{
  WMutex m_Mutex;
  WHashTable<WUInt64, WRTTI*, WHashHelper<WUInt64>, WStaticsAllocatorWrapper> m_TypeNameHashToType;
  WDynamicArray<WRTTI*> m_AllTypes;

  bool m_bIterating = false;
};

WTypeData* GetTypeData()
{
  // Prevent static initialization hazard between first WRTTI instance
  // and type data and also make sure it is sufficiently sized before first use.
  auto CreateData = []() -> WTypeData*
  {
    WTypeData* pData = new WTypeData();
    pData->m_TypeNameHashToType.Reserve(512);
    pData->m_AllTypes.Reserve(512);
    return pData;
  };
  static WTypeData* pData = CreateData();
  return pData;
}

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Reflection)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "FileSystem"
  //END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    WPlugin::Events().AddEventHandler(WRTTI::PluginEventHandler);
    WRTTI::AssignPlugin("Static");
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WPlugin::Events().RemoveEventHandler(WRTTI::PluginEventHandler);
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WRTTI::WRTTI(WStringView sName, const WRTTI* pParentType, WUInt32 uiTypeSize, WUInt32 uiTypeVersion, WUInt8 uiVariantType,
  WBitflags<WTypeFlags> flags, WRTTIAllocator* pAllocator, WArrayPtr<const WAbstractProperty*> properties, WArrayPtr<const WAbstractFunctionProperty*> functions,
  WArrayPtr<const WPropertyAttribute*> attributes, WArrayPtr<WAbstractMessageHandler*> messageHandlers, WArrayPtr<WMessageSenderInfo> messageSenders,
  const WRTTI* (*fnVerifyParent)())
  : m_sTypeName(sName)
  , m_Properties(properties)
  , m_Functions(functions)
  , m_Attributes(attributes)
  , m_pAllocator(pAllocator)
  , m_VerifyParent(fnVerifyParent)
  , m_MessageHandlers(messageHandlers)
  , m_MessageSenders(messageSenders)
{
  UpdateType(pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags);

  // This part is not guaranteed to always work here!
  // pParentType is (apparently) always the correct pointer to the base class BUT it is not guaranteed to have been constructed at this
  // point in time! Therefore the message handler hierarchy is initialized delayed in DispatchMessage
  //
  // However, I don't know where we could do these debug checks where they are guaranteed to be executed.
  // For now they are executed here and one might also do that in e.g. the game application
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    VerifyCorrectness();
#endif
  }

  if (!m_sTypeName.IsEmpty())
  {
    RegisterType();
  }
}

WRTTI::~WRTTI()
{
  if (!m_sTypeName.IsEmpty())
  {
    UnregisterType();
  }

  // To ensure unloading plugins does not leak any heap allocated attributes etc, we need to properly clean up the RTTI members. The assumption is that anything that is deleted here was created using global 'new' when declaring the reflection information inside W_BEGIN_PROPERTIES etc. Thus, any derived WRTTI class must make sure these arrays are cleared out before this destructor is called.
  if (m_sPluginName != "Static")
  {
    // We only delete plugin types. For statically created types we can't ensure a proper destruction order so it's better to just leak the data and the the OS clean it up.
    for (auto pProp : m_Properties)
    {
      auto pPropNonConst = const_cast<WAbstractProperty*>(pProp);
      delete pPropNonConst;
    }
    for (auto pFunc : m_Functions)
    {
      auto pFuncNonConst = const_cast<WAbstractFunctionProperty*>(pFunc);
      delete pFuncNonConst;
    }
    for (auto pAttrib : m_Attributes)
    {
      auto pAttribNonConst = const_cast<WPropertyAttribute*>(pAttrib);
      delete pAttribNonConst;
    }
  }
}

void WRTTI::GatherDynamicMessageHandlers()
{
  // This cannot be done in the constructor, because the parent types are not guaranteed to be initialized at that point

  if (m_uiMsgIdOffset != WSmallInvalidIndex)
    return;

  m_uiMsgIdOffset = 0;

  WUInt16 uiMinMsgId = WSmallInvalidIndex;
  WUInt16 uiMaxMsgId = 0;

  const WRTTI* pInstance = this;
  while (pInstance != nullptr)
  {
    for (WUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
    {
      WUInt16 id = pInstance->m_MessageHandlers[i]->GetMessageId();
      uiMinMsgId = WMath::Min(uiMinMsgId, id);
      uiMaxMsgId = WMath::Max(uiMaxMsgId, id);
    }

    pInstance = pInstance->m_pParentType;
  }

  if (uiMinMsgId != WSmallInvalidIndex)
  {
    m_uiMsgIdOffset = uiMinMsgId;
    WUInt16 uiNeededCapacity = uiMaxMsgId - uiMinMsgId + 1;

    m_DynamicMessageHandlers.SetCount(uiNeededCapacity);

    pInstance = this;
    while (pInstance != nullptr)
    {
      for (WUInt32 i = 0; i < pInstance->m_MessageHandlers.GetCount(); ++i)
      {
        WAbstractMessageHandler* pHandler = pInstance->m_MessageHandlers[i];
        WUInt16 uiIndex = pHandler->GetMessageId() - m_uiMsgIdOffset;

        // this check ensures that handlers in base classes do not override the derived handlers
        if (m_DynamicMessageHandlers[uiIndex] == nullptr)
        {
          m_DynamicMessageHandlers[uiIndex] = pHandler;
        }
      }

      pInstance = pInstance->m_pParentType;
    }
  }
}

void WRTTI::SetupParentHierarchy()
{
  m_ParentHierarchy.Clear();

  for (const WRTTI* rtti = this; rtti != nullptr; rtti = rtti->m_pParentType)
  {
    m_ParentHierarchy.PushBack(rtti);
  }
}

void WRTTI::VerifyCorrectness() const
{
  if (m_VerifyParent != nullptr)
  {
    W_ASSERT_DEV(m_VerifyParent() == m_pParentType, "Type '{0}': The given parent type '{1}' does not match the actual parent type '{2}'",
      m_sTypeName, (m_pParentType != nullptr) ? m_pParentType->GetTypeName() : "null",
      (m_VerifyParent() != nullptr) ? m_VerifyParent()->GetTypeName() : "null");
  }

  {
    WSet<WStringView> Known;

    const WRTTI* pInstance = this;

    while (pInstance != nullptr)
    {
      for (WUInt32 i = 0; i < pInstance->m_Properties.GetCount(); ++i)
      {
        const bool bNewProperty = !Known.Find(pInstance->m_Properties[i]->GetPropertyName()).IsValid();
        Known.Insert(pInstance->m_Properties[i]->GetPropertyName());

        W_IGNORE_UNUSED(bNewProperty);
        W_ASSERT_DEV(bNewProperty, "{0}: The property with name '{1}' is already defined in type '{2}'.", m_sTypeName,
          pInstance->m_Properties[i]->GetPropertyName(), pInstance->GetTypeName());
      }

      pInstance = pInstance->m_pParentType;
    }
  }

  {
    for (const WAbstractProperty* pFunc : m_Functions)
    {
      W_IGNORE_UNUSED(pFunc);
      W_ASSERT_DEV(pFunc->GetCategory() == WPropertyCategory::Function, "Invalid function property '{}'", pFunc->GetPropertyName());
    }
  }
}

void WRTTI::VerifyCorrectnessForAllTypes()
{
  WRTTI::ForEachType([](const WRTTI* pRtti)
    { pRtti->VerifyCorrectness(); });
}


void WRTTI::UpdateType(const WRTTI* pParentType, WUInt32 uiTypeSize, WUInt32 uiTypeVersion, WUInt8 uiVariantType, WBitflags<WTypeFlags> flags)
{
  m_pParentType = pParentType;
  m_uiVariantType = uiVariantType;
  m_uiTypeSize = uiTypeSize;
  m_uiTypeVersion = uiTypeVersion;
  m_TypeFlags = flags;
  m_ParentHierarchy.Clear();
}

void WRTTI::RegisterType()
{
  m_uiTypeNameHash = WHashingUtils::StringHash(m_sTypeName);

  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Insert(m_uiTypeNameHash, this);

  m_uiTypeIndex = pData->m_AllTypes.GetCount();
  pData->m_AllTypes.PushBack(this);
}

void WRTTI::UnregisterType()
{
  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);
  pData->m_TypeNameHashToType.Remove(m_uiTypeNameHash);

  W_ASSERT_DEV(pData->m_bIterating == false, "Unregistering types while iterating over types might cause unexpected behavior");
  pData->m_AllTypes.RemoveAtAndSwap(m_uiTypeIndex);
  if (m_uiTypeIndex != pData->m_AllTypes.GetCount())
  {
    pData->m_AllTypes[m_uiTypeIndex]->m_uiTypeIndex = m_uiTypeIndex;
  }
}

void WRTTI::GetAllProperties(WDynamicArray<const WAbstractProperty*>& out_properties) const
{
  out_properties.Clear();

  if (m_pParentType)
    m_pParentType->GetAllProperties(out_properties);

  out_properties.PushBackRange(GetProperties());
}

const WRTTI* WRTTI::FindTypeByName(WStringView sName)
{
  WUInt64 uiNameHash = WHashingUtils::StringHash(sName);

  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);

  WRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const WRTTI* WRTTI::FindTypeByNameHash(WUInt64 uiNameHash)
{
  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);

  WRTTI* pType = nullptr;
  pData->m_TypeNameHashToType.TryGetValue(uiNameHash, pType);
  return pType;
}

const WRTTI* WRTTI::FindTypeByNameHash32(WUInt32 uiNameHash)
{
  return FindTypeIf([=](const WRTTI* pRtti)
    { return (WHashingUtils::StringHashTo32(pRtti->GetTypeNameHash()) == uiNameHash); });
}

const WRTTI* WRTTI::FindTypeIf(PredicateFunc func)
{
  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);

  for (const WRTTI* pRtti : pData->m_AllTypes)
  {
    if (func(pRtti))
    {
      return pRtti;
    }
  }

  return nullptr;
}

const WAbstractProperty* WRTTI::FindPropertyByName(WStringView sName, bool bSearchBaseTypes /* = true */) const
{
  const WRTTI* pInstance = this;

  do
  {
    for (WUInt32 p = 0; p < pInstance->m_Properties.GetCount(); ++p)
    {
      if (pInstance->m_Properties[p]->GetPropertyName() == sName)
      {
        return pInstance->m_Properties[p];
      }
    }

    if (!bSearchBaseTypes)
      return nullptr;

    pInstance = pInstance->m_pParentType;
  } while (pInstance != nullptr);

  return nullptr;
}

bool WRTTI::DispatchMessage(void* pInstance, WMessage& ref_msg) const
{
  W_ASSERT_DEBUG(m_uiMsgIdOffset != WSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                          "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                          "you may have forgotten to instantiate an WPlugin object inside your plugin DLL.");

  const WUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    WAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr)
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

bool WRTTI::DispatchMessage(const void* pInstance, WMessage& ref_msg) const
{
  W_ASSERT_DEBUG(m_uiMsgIdOffset != WSmallInvalidIndex, "Message handler table should have been gathered at this point.\n"
                                                          "If this assert is triggered for a type loaded from a dynamic plugin,\n"
                                                          "you may have forgotten to instantiate an WPlugin object inside your plugin DLL.");

  const WUInt32 uiIndex = ref_msg.GetId() - m_uiMsgIdOffset;

  // m_DynamicMessageHandlers contains all message handlers of this type and all base types
  if (uiIndex < m_DynamicMessageHandlers.GetCount())
  {
    WAbstractMessageHandler* pHandler = m_DynamicMessageHandlers.GetData()[uiIndex];
    if (pHandler != nullptr && pHandler->IsConst())
    {
      (*pHandler)(pInstance, ref_msg);
      return true;
    }
  }

  return false;
}

void WRTTI::ForEachType(VisitorFunc func, WBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);

  pData->m_bIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (WUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];
    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(WTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIterating = false;
}

void WRTTI::ForEachDerivedType(const WRTTI* pBaseType, VisitorFunc func, WBitflags<ForEachOptions> options /*= ForEachOptions::Default*/)
{
  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);

  pData->m_bIterating = true;
  // Can't use ranged based for loop here since we might add new types while iterating and the m_AllTypes array might re-allocate.
  for (WUInt32 i = 0; i < pData->m_AllTypes.GetCount(); ++i)
  {
    auto pRtti = pData->m_AllTypes.GetData()[i];
    if (!pRtti->IsDerivedFrom(pBaseType))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeNonAllocatable) && (pRtti->GetAllocator() == nullptr || pRtti->GetAllocator()->CanAllocate() == false))
      continue;

    if (options.IsSet(ForEachOptions::ExcludeAbstract) && pRtti->GetTypeFlags().IsSet(WTypeFlags::Abstract))
      continue;

    func(pRtti);
  }
  pData->m_bIterating = false;
}

void WRTTI::AssignPlugin(WStringView sPluginName)
{
  // assigns the given plugin name to every WRTTI instance that has no plugin assigned yet

  auto pData = GetTypeData();
  W_LOCK(pData->m_Mutex);

  for (WRTTI* pRtti : pData->m_AllTypes)
  {
    if (pRtti->m_sPluginName.IsEmpty())
    {
      pRtti->m_sPluginName = sPluginName;
      SanityCheckType(pRtti);

      pRtti->SetupParentHierarchy();
      pRtti->GatherDynamicMessageHandlers();
    }
  }
}

#if W_ENABLED(W_COMPILE_FOR_DEBUG)

static bool IsValidIdentifierName(WStringView sIdentifier)
{
  // empty strings are not valid
  if (sIdentifier.IsEmpty())
    return false;

  // digits are not allowed as the first character
  WUInt32 uiChar = sIdentifier.GetCharacter();
  if (uiChar >= '0' && uiChar <= '9')
    return false;

  for (auto it = sIdentifier.GetIteratorFront(); it.IsValid(); ++it)
  {
    const WUInt32 c = it.GetCharacter();

    if (c >= 'a' && c <= 'z')
      continue;
    if (c >= 'A' && c <= 'Z')
      continue;
    if (c >= '0' && c <= '9')
      continue;
    if (c >= '_')
      continue;
    if (c >= ':')
      continue;

    return false;
  }

  return true;
}

#endif

void WRTTI::SanityCheckType(WRTTI* pType)
{
  W_ASSERT_DEV(pType->GetTypeFlags().IsSet(WTypeFlags::StandardType) + pType->GetTypeFlags().IsSet(WTypeFlags::IsEnum) +
                    pType->GetTypeFlags().IsSet(WTypeFlags::Bitflags) + pType->GetTypeFlags().IsSet(WTypeFlags::Class) ==
                  1,
    "Types are mutually exclusive!");

  for (auto pProp : pType->m_Properties)
  {
    const WRTTI* pSpecificType = pProp->GetSpecificType();

    W_ASSERT_DEBUG(IsValidIdentifierName(pProp->GetPropertyName()), "Property name is invalid: '{0}'", pProp->GetPropertyName());

    if (pProp->GetCategory() != WPropertyCategory::Function)
    {
      W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::StandardType) + pProp->GetFlags().IsSet(WPropertyFlags::IsEnum) +
                        pProp->GetFlags().IsSet(WPropertyFlags::Bitflags) + pProp->GetFlags().IsSet(WPropertyFlags::Class) <=
                      1,
        "Types are mutually exclusive!");
    }

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Constant:
      {
        W_IGNORE_UNUSED(pSpecificType);
        W_ASSERT_DEV(pSpecificType->GetTypeFlags().IsSet(WTypeFlags::StandardType), "Only standard type constants are supported!");
      }
      break;
      case WPropertyCategory::Member:
      {
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(WTypeFlags::StandardType),
          "Property-Type missmatch!");
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::IsEnum) == pSpecificType->GetTypeFlags().IsSet(WTypeFlags::IsEnum),
          "Property-Type missmatch! Use W_BEGIN_STATIC_REFLECTED_ENUM for type and W_ENUM_MEMBER_PROPERTY / "
          "W_ENUM_ACCESSOR_PROPERTY for property.");
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::Bitflags) == pSpecificType->GetTypeFlags().IsSet(WTypeFlags::Bitflags),
          "Property-Type missmatch! Use W_BEGIN_STATIC_REFLECTED_ENUM for type and W_BITFLAGS_MEMBER_PROPERTY / "
          "W_BITFLAGS_ACCESSOR_PROPERTY for property.");
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(WTypeFlags::Class),
          "If WPropertyFlags::Class is set, the property type must be WTypeFlags::Class and vise versa.");
      }
      break;
      case WPropertyCategory::Array:
      case WPropertyCategory::Set:
      case WPropertyCategory::Map:
      {
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::StandardType) == pSpecificType->GetTypeFlags().IsSet(WTypeFlags::StandardType),
          "Property-Type missmatch!");
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::Class) == pSpecificType->GetTypeFlags().IsSet(WTypeFlags::Class),
          "If WPropertyFlags::Class is set, the property type must be WTypeFlags::Class and vise versa.");
      }
      break;
      case WPropertyCategory::Function:
        W_REPORT_FAILURE("Functions need to be put into the W_BEGIN_FUNCTIONS / W_END_FUNCTIONS; block.");
        break;
    }
  }
}

void WRTTI::PluginEventHandler(const WPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case WPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all current WRTTI instances
      // are assigned to the proper plugin
      // all not-yet assigned rtti instances cannot be in any plugin, so assign them to the 'static' plugin
      AssignPlugin("Static");
    }
    break;

    case WPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new rtti instances and assign them to that new plugin
      AssignPlugin(EventData.m_sPluginBinary);

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
      WRTTI::VerifyCorrectnessForAllTypes();
#endif
    }
    break;

    default:
      break;
  }
}

WRTTIAllocator::~WRTTIAllocator() = default;


W_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_RTTI);
