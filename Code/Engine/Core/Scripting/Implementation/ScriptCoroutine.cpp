#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/Scripting/ScriptWorldModule.h>
#include <Foundation/Types/VariantTypeRegistry.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptCoroutineHandle, WNoBase, 1, WRTTIDefaultAllocator<WScriptCoroutineHandle>)
W_END_STATIC_REFLECTED_TYPE;
W_DEFINE_CUSTOM_VARIANT_TYPE(WScriptCoroutineHandle);

W_BEGIN_STATIC_REFLECTED_TYPE(WScriptCoroutine, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY_READ_ONLY("Name", GetName),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_FUNCTION_PROPERTY(UpdateAndSchedule),
  }
  W_END_FUNCTIONS;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WScriptCoroutine::WScriptCoroutine() = default;

WScriptCoroutine::~WScriptCoroutine()
{
  W_ASSERT_DEV(m_pOwnerModule == nullptr, "Deinitialize was not called");
}

void WScriptCoroutine::UpdateAndSchedule(WTime deltaTimeSinceLastUpdate)
{
  auto result = Update(deltaTimeSinceLastUpdate);

  // Has been deleted during update
  if (m_pOwnerModule == nullptr)
    return;

  if (result.m_State == Result::State::Running)
  {
    // We can safely pass false here since we would not end up here if the coroutine is used in a simulation only function
    // but the simulation is not running because then the outer function should not have been called.
    const bool bOnlyWhenSimulating = false;
    m_pOwnerModule->AddUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this, result.m_MaxDelay, bOnlyWhenSimulating);
  }
  else
  {
    m_pOwnerModule->StopAndDeleteCoroutine(GetHandle());
  }
}

void WScriptCoroutine::Initialize(WScriptCoroutineId id, WStringView sName, WScriptInstance& inout_instance, WScriptWorldModule& inout_ownerModule)
{
  m_Id = id;
  m_sName.Assign(sName);
  m_pInstance = &inout_instance;
  m_pOwnerModule = &inout_ownerModule;
}

void WScriptCoroutine::Deinitialize()
{
  m_pOwnerModule->RemoveUpdateFunctionToSchedule(GetUpdateFunctionProperty(), this);
  m_pOwnerModule = nullptr;
}

// static
const WAbstractFunctionProperty* WScriptCoroutine::GetUpdateFunctionProperty()
{
  static const WAbstractFunctionProperty* pUpdateFunctionProperty = []() -> const WAbstractFunctionProperty*
  {
    const WRTTI* pType = WGetStaticRTTI<WScriptCoroutine>();
    auto functions = pType->GetFunctions();
    for (auto pFunc : functions)
    {
      if (WStringUtils::IsEqual(pFunc->GetPropertyName(), "UpdateAndSchedule"))
      {
        return pFunc;
      }
    }
    return nullptr;
  }();

  return pUpdateFunctionProperty;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WScriptCoroutineCreationMode, 1)
  W_ENUM_CONSTANTS(WScriptCoroutineCreationMode::StopOther, WScriptCoroutineCreationMode::DontCreateNew, WScriptCoroutineCreationMode::AllowOverlap)
W_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WScriptCoroutineRTTI::WScriptCoroutineRTTI(WStringView sName, WUniquePtr<WRTTIAllocator>&& pAllocator)
  : WRTTI(nullptr, WGetStaticRTTI<WScriptCoroutine>(), 0, 1, WVariantType::Invalid, WTypeFlags::Class, nullptr, WArrayPtr<const WAbstractProperty*>(), WArrayPtr<const WAbstractFunctionProperty*>(), WArrayPtr<const WPropertyAttribute*>(), WArrayPtr<WAbstractMessageHandler*>(), WArrayPtr<WMessageSenderInfo>(), nullptr)
  , m_sTypeNameStorage(sName)
  , m_pAllocatorStorage(std::move(pAllocator))
{
  m_sTypeName = m_sTypeNameStorage;
  m_pAllocator = m_pAllocatorStorage.Borrow();

  RegisterType();

  SetupParentHierarchy();
}

WScriptCoroutineRTTI::~WScriptCoroutineRTTI()
{
  UnregisterType();
  m_sTypeName = nullptr;

  // RTTI base class will try to delete the contents of these arrays under the assumption that they were created during static init. Dynamically created types must ensure that these arrays are cleared out before the base class is executed.
  m_Properties.Clear();
  m_Functions.Clear();
  m_Attributes.Clear();
}

//////////////////////////////////////////////////////////////////////////

WScriptCoroutineFunctionProperty::WScriptCoroutineFunctionProperty(WStringView sName, const WSharedPtr<WScriptCoroutineRTTI>& pType, WScriptCoroutineCreationMode::Enum creationMode)
  : WScriptFunctionProperty(sName)
  , m_pType(pType)
  , m_CreationMode(creationMode)
{
}

WScriptCoroutineFunctionProperty::~WScriptCoroutineFunctionProperty() = default;

void WScriptCoroutineFunctionProperty::Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const
{
  W_IGNORE_UNUSED(out_returnValue);

  W_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pScriptInstance = static_cast<WScriptInstance*>(pInstance);

  WWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    WLog::Error("Script coroutines need a script instance with a valid WWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<WScriptWorldModule>();

  WScriptCoroutine* pCoroutine = nullptr;
  auto hCoroutine = pModule->CreateCoroutine(m_pType.Borrow(), m_szPropertyName, *pScriptInstance, m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    WTempHybridArray<WVariant, 8> finalArgs;
    finalArgs = arguments;
    finalArgs.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, finalArgs);
  }
}

//////////////////////////////////////////////////////////////////////////

WScriptCoroutineMessageHandler::WScriptCoroutineMessageHandler(WStringView sName, const WScriptMessageDesc& desc, const WSharedPtr<WScriptCoroutineRTTI>& pType, WScriptCoroutineCreationMode::Enum creationMode)
  : WScriptMessageHandler(desc)
  , m_pType(pType)
  , m_CreationMode(creationMode)
{
  m_sName.Assign(sName);
  m_DispatchFunc = &Dispatch;
}

WScriptCoroutineMessageHandler::~WScriptCoroutineMessageHandler() = default;

// static
void WScriptCoroutineMessageHandler::Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg)
{
  W_ASSERT_DEBUG(pInstance != nullptr, "Invalid instance");
  auto pHandler = static_cast<WScriptCoroutineMessageHandler*>(pSelf);
  auto pComponent = static_cast<WScriptComponent*>(pInstance);
  auto pScriptInstance = pComponent->GetScriptInstance();

  WWorld* pWorld = pScriptInstance->GetWorld();
  if (pWorld == nullptr)
  {
    WLog::Error("Script coroutines need a script instance with a valid WWorld");
    return;
  }

  auto pModule = pWorld->GetOrCreateModule<WScriptWorldModule>();

  WScriptCoroutine* pCoroutine = nullptr;
  auto hCoroutine = pModule->CreateCoroutine(pHandler->m_pType.Borrow(), pHandler->m_sName, *pScriptInstance, pHandler->m_CreationMode, pCoroutine);

  if (pCoroutine != nullptr)
  {
    WTempHybridArray<WVariant, 8> arguments;
    pHandler->FillMessagePropertyValues(ref_msg, arguments);
    arguments.PushBack(hCoroutine);

    pModule->StartCoroutine(hCoroutine, arguments);
  }
}


W_STATICLINK_FILE(Core, Core_Scripting_Implementation_ScriptCoroutine);
