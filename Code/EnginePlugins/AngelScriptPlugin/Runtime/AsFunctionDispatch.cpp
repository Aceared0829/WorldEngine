#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptComponent.h>
#include <Core/Scripting/ScriptWorldModule.h>

// clang-format off
W_IMPLEMENT_MESSAGE_TYPE(WMsgDeliverAngelScriptMsg);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgDeliverAngelScriptMsg, 1, WRTTIDefaultAllocator<WMsgDeliverAngelScriptMsg>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WAngelScriptFunctionProperty::WAngelScriptFunctionProperty(WStringView sName, asIScriptFunction* pFunction)
  : WScriptFunctionProperty(sName)
{
  m_pAsFunction = pFunction;
  m_pAsFunction->AddRef();
}

WAngelScriptFunctionProperty::~WAngelScriptFunctionProperty()
{
  if (m_pAsFunction)
  {
    m_pAsFunction->Release();
    m_pAsFunction = nullptr;
  }
}

void WAngelScriptFunctionProperty::Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const
{
  if (m_pAsFunction)
  {
    auto pScriptInstance = static_cast<WAngelScriptInstance*>(pInstance);
    auto pContext = pScriptInstance->GetContext();

    bool bPush = false;
    if (pContext->GetState() == asEContextState::asEXECUTION_ACTIVE)
    {
      bPush = true;
      AS_CHECK(pContext->PushState());
    }

    WAngelScriptUtils::SetThreadLocalWorld(pScriptInstance->GetWorld());

    WTime tDiff;

    if (pContext->Prepare(m_pAsFunction) >= 0)
    {
      W_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
      pContext->SetObject(pScriptInstance->GetObject());

      if (m_pAsFunction->GetParamCount() > 0)
      {
        tDiff = arguments[0].Get<WTime>();
        AS_CHECK(pContext->SetArgObject(0, &tDiff));
      }

      AS_CHECK(pContext->Execute());
    }

    if (bPush)
    {
      AS_CHECK(pContext->PopState());
    }
  }
}

//////////////////////////////////////////////////////////////////////////

WAngelScriptMessageHandler::WAngelScriptMessageHandler(const WScriptMessageDesc& desc, asIScriptFunction* pFunction)
  : WScriptMessageHandler(desc)
{
  m_DispatchFunc = &Dispatch;

  m_pAsFunction = pFunction;
  m_pAsFunction->AddRef();
}

WAngelScriptMessageHandler::~WAngelScriptMessageHandler()
{
  if (m_pAsFunction)
  {
    m_pAsFunction->Release();
    m_pAsFunction = nullptr;
  }
}

void WAngelScriptMessageHandler::Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg)
{
  auto pScriptComp = static_cast<WScriptComponent*>(pInstance);

  auto pThis = static_cast<WAngelScriptMessageHandler*>(pSelf);
  auto pScriptInstance = static_cast<WAngelScriptInstance*>(pScriptComp->GetScriptInstance());
  auto pContext = pScriptInstance->GetContext();

  bool bPush = false;
  if (pContext->GetState() == asEContextState::asEXECUTION_ACTIVE)
  {
    bPush = true;
    AS_CHECK(pContext->PushState());
  }

  WAngelScriptUtils::SetThreadLocalWorld(pScriptInstance->GetWorld());

  if (pContext->Prepare(pThis->m_pAsFunction) >= 0)
  {
    W_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");
    AS_CHECK(pContext->SetObject(pScriptInstance->GetObject()));
    AS_CHECK(pContext->SetArgObject(0, &ref_msg));
    AS_CHECK(pContext->Execute());
  }

  if (bPush)
  {
    AS_CHECK(pContext->PopState());
  }
}

//////////////////////////////////////////////////////////////////////////

WAngelScriptCustomAsMessageHandler::WAngelScriptCustomAsMessageHandler(const WScriptMessageDesc& desc)
  : WScriptMessageHandler(desc)
{
  m_DispatchFunc = &Dispatch;
}

WAngelScriptCustomAsMessageHandler::~WAngelScriptCustomAsMessageHandler()
{
  for (auto& r : m_Receivers)
  {
    r.m_pAsFunction->Release();
    r.m_pAsFunction = nullptr;
  }
}


void WAngelScriptCustomAsMessageHandler::AddReceiver(asIScriptFunction* pFunction, const char* szArgType)
{
  auto& r = m_Receivers.ExpandAndGetRef();
  r.m_pAsFunction = pFunction;
  r.m_pAsFunction->AddRef();
  r.m_sArgType.Assign(szArgType);
}

void WAngelScriptCustomAsMessageHandler::Dispatch(WAbstractMessageHandler* pSelf, void* pInstance, WMessage& ref_msg)
{
  auto pThis = static_cast<WAngelScriptCustomAsMessageHandler*>(pSelf);
  WMsgDeliverAngelScriptMsg& asMsg = static_cast<WMsgDeliverAngelScriptMsg&>(ref_msg);

  auto pMsgObj = reinterpret_cast<asIScriptObject*>(asMsg.m_pAsMsg);
  const char* szObjType = pMsgObj->GetObjectType()->GetName();
  const WTempHashedString sObjType(szObjType);

  for (const auto& r : pThis->m_Receivers)
  {
    if (r.m_sArgType != sObjType)
      continue;

    auto pScriptComp = static_cast<WScriptComponent*>(pInstance);
    auto pScriptInstance = static_cast<WAngelScriptInstance*>(pScriptComp->GetScriptInstance());
    W_ASSERT_DEBUG(pScriptInstance->GetObject(), "Invalid script object");

    auto pContext = pScriptInstance->GetContext();

    bool bPush = false;
    if (pContext->GetState() == asEContextState::asEXECUTION_ACTIVE)
    {
      bPush = true;
      AS_CHECK(pContext->PushState());
    }

    WAngelScriptUtils::SetThreadLocalWorld(pScriptInstance->GetWorld());

    if (pContext->Prepare(r.m_pAsFunction) >= 0)
    {

      AS_CHECK(pContext->SetObject(pScriptInstance->GetObject()));
      AS_CHECK(pContext->SetArgObject(0, pMsgObj));
      AS_CHECK(pContext->Execute());
    }

    if (bPush)
    {
      AS_CHECK(pContext->PopState());
    }

    break;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

WMsgDeliverAngelScriptMsg::~WMsgDeliverAngelScriptMsg()
{
  if (m_bRelease)
  {
    auto pObj = reinterpret_cast<asIScriptObject*>(m_pAsMsg);
    pObj->Release();
  }
}

WMsgDeliverAngelScriptMsg::WMsgDeliverAngelScriptMsg(const WMsgDeliverAngelScriptMsg& rhs)
{
  *this = rhs;
}

WMsgDeliverAngelScriptMsg::WMsgDeliverAngelScriptMsg(WMsgDeliverAngelScriptMsg&& rhs)
{
  *this = std::move(rhs);
}

void WMsgDeliverAngelScriptMsg::operator=(const WMsgDeliverAngelScriptMsg& rhs)
{
  WMemoryUtils::RawByteCopy(this, &rhs, sizeof(WMsgDeliverAngelScriptMsg));

  if (m_bRelease)
  {
    auto pObj = reinterpret_cast<asIScriptObject*>(m_pAsMsg);
    pObj->AddRef();
  }
}

void WMsgDeliverAngelScriptMsg::operator=(WMsgDeliverAngelScriptMsg&& rhs)
{
  m_bRelease = rhs.m_bRelease;
  m_pAsMsg = rhs.m_pAsMsg;
  rhs.m_bRelease = false;
  rhs.m_pAsMsg = nullptr;
}


W_STATICLINK_FILE(AngelScriptPlugin, AngelScriptPlugin_Runtime_AsFunctionDispatch);
