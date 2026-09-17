#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsFunctionDispatch.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

//////////////////////////////////////////////////////////////////////////
// WGameObject
//////////////////////////////////////////////////////////////////////////

void WGameObject_TryGetComponentOfBaseType(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  int typeId = pGen->GetArgTypeId(0);

  if (auto info = pGen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const WRTTI* pRtti = static_cast<const WRTTI*>(info->GetUserData(WAsUserData::RttiPtr)))
    {
      WComponent* pComponent;
      if (pObj->TryGetComponentOfBaseType(pRtti, pComponent))
      {
        WComponent** ref = (WComponent**)pGen->GetArgAddress(0);
        *ref = pComponent;

        pGen->SetReturnByte(1);
        return;
      }
    }
  }

  pGen->SetReturnByte(0);
}

void WGameObject_CreateComponent(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  WWorld* pWorld = pObj->GetWorld();
  const int typeId = pGen->GetArgTypeId(0);

  if (auto info = pGen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const WRTTI* pRtti = static_cast<const WRTTI*>(info->GetUserData(WAsUserData::RttiPtr)))
    {
      if (auto pCompMan = pWorld->GetOrCreateManagerForComponentType(pRtti))
      {
        WComponentHandle hComp = pCompMan->CreateComponent(pObj);

        WComponent* pComponent;
        if (pWorld->TryGetComponent(hComp, pComponent))
        {
          WComponent** ref = (WComponent**)pGen->GetArgAddress(0);
          *ref = pComponent;

          pGen->SetReturnByte(1);
          return;
        }
      }
    }
  }

  pGen->SetReturnByte(0);
}

void WGameObjectHandle_Construct(void* pMemory)
{
  new (pMemory) WGameObjectHandle();
}

void WComponentHandle_Construct(void* pMemory)
{
  new (pMemory) WComponentHandle();
}

void WGameObject_SendAsMessage(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  if (pObj->SendMessage(msg))
  {
    pGen->SetReturnByte(1);
  }
  else
  {
    pGen->SetReturnByte(0);
  }
}

void WGameObject_SendAsMessageConst(asIScriptGeneric* pGen)
{
  const WGameObject* pObj = (const WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  if (pObj->SendMessage(msg))
  {
    pGen->SetReturnByte(1);
  }
  else
  {
    pGen->SetReturnByte(0);
  }
}

void WGameObject_SendAsMessageRecursive(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  if (pObj->SendMessageRecursive(msg))
  {
    pGen->SetReturnByte(1);
  }
  else
  {
    pGen->SetReturnByte(0);
  }
}

void WGameObject_SendAsMessageRecursiveConst(asIScriptGeneric* pGen)
{
  const WGameObject* pObj = (const WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  if (pObj->SendMessageRecursive(msg))
  {
    pGen->SetReturnByte(1);
  }
  else
  {
    pGen->SetReturnByte(0);
  }
}

void WGameObject_PostAsMessage(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  WTime delay = *((const WTime*)pGen->GetArgObject(1));
  WInt32 delivery = (WInt32)pGen->GetArgDWord(2);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  W_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  WMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostMessage(msg, delay, static_cast<WObjectMsgQueueType::Enum>(delivery));
}

void WGameObject_PostAsMessageRecursive(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  WTime delay = *((const WTime*)pGen->GetArgObject(1));
  WInt32 delivery = (WInt32)pGen->GetArgDWord(2);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  W_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  WMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostMessageRecursive(msg, delay, static_cast<WObjectMsgQueueType::Enum>(delivery));
}

void WGameObject_SendAsEventMessage(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();

  void* pAsMsg = pGen->GetArgObject(0);
  const WComponent* pSender = (const WComponent*)pGen->GetArgObject(1);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  if (pObj->SendEventMessage(msg, pSender))
  {
    pGen->SetReturnByte(1);
  }
  else
  {
    pGen->SetReturnByte(0);
  }
}

void WGameObject_SendAsEventMessageConst(asIScriptGeneric* pGen)
{
  const WGameObject* pObj = (const WGameObject*)pGen->GetObject();

  void* pAsMsg = pGen->GetArgObject(0);
  const WComponent* pSender = (const WComponent*)pGen->GetArgObject(1);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  if (pObj->SendEventMessage(msg, pSender))
  {
    pGen->SetReturnByte(1);
  }
  else
  {
    pGen->SetReturnByte(0);
  }
}

void WGameObject_PostAsEventMessage(asIScriptGeneric* pGen)
{
  WGameObject* pObj = (WGameObject*)pGen->GetObject();
  void* pAsMsg = pGen->GetArgObject(0);

  const WComponent* pSender = (const WComponent*)pGen->GetArgObject(1);
  WTime delay = *((const WTime*)pGen->GetArgObject(2));
  WInt32 delivery = (WInt32)pGen->GetArgDWord(3);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  W_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  WMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostEventMessage(msg, pSender, delay, static_cast<WObjectMsgQueueType::Enum>(delivery));
}

void WAngelScriptEngineSingleton::Register_GameObject()
{
  WAngelScriptUtils::RegisterEnumType(m_pEngine, WGetStaticRTTI<WObjectMsgQueueType>());
  WAngelScriptUtils::RegisterEnumType(m_pEngine, WGetStaticRTTI<WTransformPreservation>());

  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WGameObjectHandle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WGameObjectHandle_Construct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObjectHandle", "void Invalidate()", asMETHOD(WGameObjectHandle, Invalidate), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObjectHandle", "bool IsInvalidated() const", asMETHOD(WGameObjectHandle, IsInvalidated), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObjectHandle", "bool opEquals(WGameObjectHandle) const", asMETHODPR(WGameObjectHandle, operator==, (WGameObjectHandle) const, bool), asCALL_THISCALL));
  }

  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WComponentHandle", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WComponentHandle_Construct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WComponentHandle", "void Invalidate()", asMETHOD(WComponentHandle, Invalidate), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WComponentHandle", "bool IsInvalidated() const", asMETHOD(WComponentHandle, IsInvalidated), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WComponentHandle", "bool opEquals(WComponentHandle) const", asMETHODPR(WComponentHandle, operator==, (WComponentHandle) const, bool), asCALL_THISCALL));
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WGameObjectHandle GetHandle() const", asMETHOD(WGameObject, GetHandle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void MakeDynamic()", asMETHOD(WGameObject, MakeDynamic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void MakeStatic()", asMETHOD(WGameObject, MakeStatic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool IsDynamic() const", asMETHOD(WGameObject, IsDynamic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool IsStatic() const", asMETHOD(WGameObject, IsStatic), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetActiveFlag(bool bActive)", asMETHOD(WGameObject, SetActiveFlag), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool GetActiveFlag() const", asMETHOD(WGameObject, GetActiveFlag), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool IsActive() const", asMETHOD(WGameObject, IsActive), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetName(WStringView sName)", asMETHODPR(WGameObject, SetName, (WStringView), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetName(const WHashedString& in sName)", asMETHODPR(WGameObject, SetName, (const WHashedString&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WStringView GetName() const", asMETHOD(WGameObject, GetName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool HasName(const WTempHashedString& in sName) const", asMETHOD(WGameObject, HasName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalKey(WStringView sKey)", asMETHODPR(WGameObject, SetGlobalKey, (WStringView), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalKey(const WHashedString& in sKey)", asMETHODPR(WGameObject, SetGlobalKey, (const WHashedString&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WStringView GetGlobalKey() const", asMETHOD(WGameObject, GetGlobalKey), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetParent(const WGameObjectHandle& in hParent, WTransformPreservation preserve = WTransformPreservation::PreserveGlobal)", asMETHOD(WGameObject, SetParent), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WGameObject@ GetParent() const", asMETHODPR(WGameObject, GetParent, () const, const WGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WGameObject@ GetParent()", asMETHODPR(WGameObject, GetParent, (), WGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void AddChild(const WGameObjectHandle& in hChild, WTransformPreservation preserve = WTransformPreservation::PreserveGlobal)", asMETHOD(WGameObject, AddChild), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void DetachChild(const WGameObjectHandle& in hChild, WTransformPreservation preserve = WTransformPreservation::PreserveGlobal)", asMETHOD(WGameObject, DetachChild), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "uint32 GetChildCount()", asMETHOD(WGameObject, GetChildCount), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void GetChildren()", asMETHOD(WGameObject, GetChildren), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WGameObject@ FindChildByName(const WTempHashedString& in sName, bool bRecursive = true)", asMETHODPR(WGameObject, FindChildByName, (const WTempHashedString&, bool), WGameObject*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WGameObject@ FindChildByPath(WStringView sPath)", asMETHODPR(WGameObject, FindChildByPath, (WStringView), WGameObject*), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WGameObject@ SearchForChildByNameSequence(string)", asMETHOD(WGameObject, SearchForChildByNameSequence), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WWorld@ GetWorld()", asMETHODPR(WGameObject, GetWorld, (), WWorld*), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "const WWorld@ GetWorld() const", asMETHODPR(WGameObject, GetWorld, () const, const WWorld*), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetLocalPosition(const WVec3& in)", asMETHODPR(WGameObject, SetLocalPosition, (WVec3), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetLocalPosition() const", asMETHOD(WGameObject, GetLocalPosition), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetLocalRotation(const WQuat& in)", asMETHODPR(WGameObject, SetLocalRotation, (WQuat), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WQuat GetLocalRotation() const", asMETHOD(WGameObject, GetLocalRotation), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetLocalScaling(const WVec3& in)", asMETHODPR(WGameObject, SetLocalScaling, (WVec3), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetLocalScaling() const", asMETHOD(WGameObject, GetLocalScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetLocalUniformScaling(float fScale)", asMETHODPR(WGameObject, SetLocalUniformScaling, (float), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "float GetLocalUniformScaling() const", asMETHOD(WGameObject, GetLocalUniformScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WTransform GetLocalTransform() const", asMETHOD(WGameObject, GetLocalTransform), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalPosition(const WVec3& in)", asMETHODPR(WGameObject, SetGlobalPosition, (const WVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetGlobalPosition() const", asMETHOD(WGameObject, GetGlobalPosition), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalRotation(const WQuat& in)", asMETHODPR(WGameObject, SetGlobalRotation, (const WQuat&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WQuat GetGlobalRotation() const", asMETHODPR(WGameObject, GetGlobalRotation, () const, WQuat), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalScaling(const WVec3& in)", asMETHODPR(WGameObject, SetGlobalScaling, (const WVec3&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetGlobalScaling() const", asMETHOD(WGameObject, GetGlobalScaling), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalTransform(const WTransform& in)", asMETHODPR(WGameObject, SetGlobalTransform, (const WTransform&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WTransform GetGlobalTransform() const", asMETHOD(WGameObject, GetGlobalTransform), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WTransform GetLastGlobalTransform() const", asMETHOD(WGameObject, GetLastGlobalTransform), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetGlobalDirForwards() const", asMETHOD(WGameObject, GetGlobalDirForwards), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetGlobalDirRight() const", asMETHOD(WGameObject, GetGlobalDirRight), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetGlobalDirUp() const", asMETHOD(WGameObject, GetGlobalDirUp), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalRotationToLookAt(const WVec3& in vTargetPosition, const WVec3& in vUp = WVec3(0, 0, 1))", asMETHOD(WGameObject, SetGlobalRotationToLookAt), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetGlobalTransformToLookAt(const WVec3& in vOwnPosition, const WVec3& in vTargetPosition, const WVec3& in vUp = WVec3(0, 0, 1))", asMETHOD(WGameObject, SetGlobalTransformToLookAt), asCALL_THISCALL));

#if W_ENABLED(W_GAMEOBJECT_VELOCITY)
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetLinearVelocity() const", asMETHOD(WGameObject, GetLinearVelocity), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WVec3 GetAngularVelocity() const", asMETHOD(WGameObject, GetAngularVelocity), asCALL_THISCALL));
#endif

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void UpdateGlobalTransform()", asMETHOD(WGameObject, UpdateGlobalTransform), asCALL_THISCALL));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WBoundingBoxSphere GetLocalBounds() const", asMETHOD(WGameObject, GetLocalBounds), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "WBoundingBoxSphere GetGlobalBounds() const", asMETHOD(WGameObject, GetGlobalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void UpdateLocalBounds()", asMETHOD(WGameObject, UpdateLocalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void UpdateGlobalBounds()", asMETHOD(WGameObject, UpdateGlobalBounds), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void UpdateGlobalTransformAndBounds()", asMETHOD(WGameObject, UpdateGlobalTransformAndBounds), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool TryGetComponentOfBaseType(const WRTTI@ pType, WComponent@& out pComponent)", asMETHODPR(WGameObject, TryGetComponentOfBaseType, (const WRTTI*, WComponent*&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool TryGetComponentOfBaseType(?& out pTypedComponent)", asFUNCTION(WGameObject_TryGetComponentOfBaseType), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool CreateComponent(?& out pTypedComponent)", asFUNCTION(WGameObject_CreateComponent), asCALL_GENERIC));

  // AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void GetComponents()", asMETHOD(WGameObject, GetComponents), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "uint16 GetComponentVersion()", asMETHOD(WGameObject, GetComponentVersion), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessage(WMessage& inout)", asMETHODPR(WGameObject, SendMessage, (WMessage&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessage(WMessage& inout) const", asMETHODPR(WGameObject, SendMessage, (WMessage&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessageRecursive(WMessage& inout)", asMETHODPR(WGameObject, SendMessageRecursive, (WMessage&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessageRecursive(WMessage& inout) const", asMETHODPR(WGameObject, SendMessageRecursive, (WMessage&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void PostMessage(const WMessage& in, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame) const", asMETHOD(WGameObject, PostMessage), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void PostMessageRecursive(const WMessage& in, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame) const", asMETHOD(WGameObject, PostMessageRecursive), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendEventMessage(WMessage& inout, const WComponent@ pSender)", asMETHODPR(WGameObject, SendEventMessage, (WMessage&, const WComponent*), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendEventMessage(WMessage& inout, const WComponent@ pSender) const", asMETHODPR(WGameObject, SendEventMessage, (WMessage&, const WComponent*) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void PostEventMessage(const WMessage& in msg, const WComponent@ pSender, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame) const", asMETHOD(WGameObject, PostEventMessage), asCALL_THISCALL));

  // GetTags
  // SetTags
  // SetTag
  // RemoveTag
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool HasTag(const WTempHashedString& in sTagName) const", asMETHOD(WGameObject, HasTag), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "uint16 GetTeamID()", asMETHOD(WGameObject, GetTeamID), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetTeamID(uint16 id)", asMETHOD(WGameObject, SetTeamID), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "uint32 GetStableRandomSeed()", asMETHOD(WGameObject, GetStableRandomSeed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void SetStableRandomSeed(uint32 seed)", asMETHOD(WGameObject, SetStableRandomSeed), asCALL_THISCALL));

  // GetVisibilityState

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessage(WAngelScriptMessage& inout)", asFUNCTION(WGameObject_SendAsMessage), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessage(WAngelScriptMessage& inout) const", asFUNCTION(WGameObject_SendAsMessageConst), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessageRecursive(WAngelScriptMessage& inout)", asFUNCTION(WGameObject_SendAsMessageRecursive), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendMessageRecursive(WAngelScriptMessage& inout) const", asFUNCTION(WGameObject_SendAsMessageRecursiveConst), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendEventMessage(WAngelScriptMessage& inout, const WComponent@ pSender)", asFUNCTION(WGameObject_SendAsEventMessage), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "bool SendEventMessage(WAngelScriptMessage& inout, const WComponent@ pSender) const", asFUNCTION(WGameObject_SendAsEventMessageConst), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void PostMessage(const WAngelScriptMessage& in, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame)", asFUNCTION(WGameObject_PostAsMessage), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void PostMessageRecursive(const WAngelScriptMessage& in, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame)", asFUNCTION(WGameObject_PostAsMessageRecursive), asCALL_GENERIC));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WGameObject", "void PostEventMessage(WAngelScriptMessage& inout, const WComponent@ pSender, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame)", asFUNCTION(WGameObject_PostAsEventMessage), asCALL_GENERIC));
}

//////////////////////////////////////////////////////////////////////////
// WWorld
//////////////////////////////////////////////////////////////////////////

static void WGameObjectDesc_Construct(void* pMemory)
{
  new (pMemory) WGameObjectDesc();
}

static void WWorld_TryGetComponent(asIScriptGeneric* pGen)
{
  WWorld* pWorld = (WWorld*)pGen->GetObject();
  WComponentHandle* hComponent = (WComponentHandle*)pGen->GetArgObject(0);
  int typeId = pGen->GetArgTypeId(1);

  if (auto info = pGen->GetEngine()->GetTypeInfoById(typeId))
  {
    if (const WRTTI* pRtti = static_cast<const WRTTI*>(info->GetUserData(WAsUserData::RttiPtr)))
    {
      WComponent* pComponent;
      if (pWorld->TryGetComponent(*hComponent, pComponent))
      {
        WComponent** ref = (WComponent**)pGen->GetArgAddress(1);
        *ref = pComponent;

        pGen->SetReturnByte((*ref)->GetDynamicRTTI()->IsDerivedFrom(pRtti) ? 1 : 0);
        return;
      }
    }
  }

  pGen->SetReturnByte(0);
}

static void WWorld_SendAsMessage(asIScriptGeneric* pGen)
{
  WWorld* pObj = (WWorld*)pGen->GetObject();
  WGameObjectHandle* pReceiver = (WGameObjectHandle*)pGen->GetArgObject(0);
  void* pAsMsg = pGen->GetArgObject(1);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  pObj->SendMessage(*pReceiver, msg);
}

static void WWorld_SendAsMessageRecursive(asIScriptGeneric* pGen)
{
  WWorld* pObj = (WWorld*)pGen->GetObject();
  WGameObjectHandle* pReceiver = (WGameObjectHandle*)pGen->GetArgObject(0);
  void* pAsMsg = pGen->GetArgObject(1);

  WMsgDeliverAngelScriptMsg msg;
  msg.m_pAsMsg = pAsMsg;
  pObj->SendMessageRecursive(*pReceiver, msg);
}

static void WWorld_PostAsMessage(asIScriptGeneric* pGen)
{
  WWorld* pObj = (WWorld*)pGen->GetObject();
  WGameObjectHandle* pReceiver = (WGameObjectHandle*)pGen->GetArgObject(0);
  void* pAsMsg = pGen->GetArgObject(1);

  WTime delay = *((const WTime*)pGen->GetArgObject(2));
  WInt32 delivery = (WInt32)pGen->GetArgDWord(3);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  W_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  WMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostMessage(*pReceiver, msg, delay, static_cast<WObjectMsgQueueType::Enum>(delivery));
}

static void WWorld_PostAsMessageRecursive(asIScriptGeneric* pGen)
{
  WWorld* pObj = (WWorld*)pGen->GetObject();
  WGameObjectHandle* pReceiver = (WGameObjectHandle*)pGen->GetArgObject(0);
  void* pAsMsg = pGen->GetArgObject(1);

  WTime delay = *((const WTime*)pGen->GetArgObject(2));
  WInt32 delivery = (WInt32)pGen->GetArgDWord(3);

  void* pMsgCopy = pGen->GetEngine()->CreateScriptObjectCopy(pAsMsg, reinterpret_cast<asIScriptObject*>(pAsMsg)->GetObjectType());
  W_ASSERT_DEV(pMsgCopy != nullptr, "Failed to create copy of message");

  WMsgDeliverAngelScriptMsg msg;
  msg.m_bRelease = true;
  msg.m_pAsMsg = pMsgCopy;
  pObj->PostMessageRecursive(*pReceiver, msg, delay, static_cast<WObjectMsgQueueType::Enum>(delivery));
}


void WAngelScriptEngineSingleton::Register_World()
{
  {
    AS_CHECK(m_pEngine->RegisterObjectType("WGameObjectDesc", sizeof(WGameObjectDesc), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<WGameObjectDesc>()));

    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "bool m_bActiveFlag", asOFFSET(WGameObjectDesc, m_bActiveFlag)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "bool m_bDynamic", asOFFSET(WGameObjectDesc, m_bDynamic)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "uint16 m_uiTeamID", asOFFSET(WGameObjectDesc, m_uiTeamID)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "WHashedString m_sName", asOFFSET(WGameObjectDesc, m_sName)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "WGameObjectHandle m_hParent", asOFFSET(WGameObjectDesc, m_hParent)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "WVec3 m_LocalPosition", asOFFSET(WGameObjectDesc, m_LocalPosition)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "WQuat m_LocalRotation", asOFFSET(WGameObjectDesc, m_LocalRotation)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "WVec3 m_LocalScaling", asOFFSET(WGameObjectDesc, m_LocalScaling)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "float m_LocalUniformScaling", asOFFSET(WGameObjectDesc, m_LocalUniformScaling)));
    // AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "WTagSet m_Tags", asOFFSET(WGameObjectDesc, m_Tags)));
    AS_CHECK(m_pEngine->RegisterObjectProperty("WGameObjectDesc", "uint32 m_uiStableRandomSeed", asOFFSET(WGameObjectDesc, m_uiStableRandomSeed)));

    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WGameObjectDesc", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WGameObjectDesc_Construct), asCALL_CDECL_OBJFIRST));
  }

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "WStringView GetName()", asMETHOD(WWorld, GetName), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "WGameObjectHandle CreateObject(const WGameObjectDesc& in desc)", asMETHODPR(WWorld, CreateObject, (const WGameObjectDesc& desc), WGameObjectHandle), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "WGameObjectHandle CreateObject(const WGameObjectDesc& in desc, WGameObject@& out object)", asMETHODPR(WWorld, CreateObject, (const WGameObjectDesc& desc, WGameObject*&), WGameObjectHandle), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void DeleteObjectDelayed(const WGameObjectHandle& in hObject, bool bAlsoDeleteEmptyParents = true)", asMETHOD(WWorld, DeleteObjectDelayed), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool IsValidObject(const WGameObjectHandle& in hObject)", asMETHOD(WWorld, IsValidObject), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool TryGetObject(const WGameObjectHandle& in, WGameObject@& out pObject)", asMETHODPR(WWorld, TryGetObject, (const WGameObjectHandle&, WGameObject*&), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool TryGetObject(const WGameObjectHandle& in, const WGameObject@& out pObject) const", asMETHODPR(WWorld, TryGetObject, (const WGameObjectHandle&, const WGameObject*&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool TryGetObjectWithGlobalKey(const WTempHashedString& in sGlobalKey, WGameObject@& out pObject)", asMETHODPR(WWorld, TryGetObjectWithGlobalKey, (const WTempHashedString& sGlobalKey, WGameObject*& out_pObject), bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool TryGetObjectWithGlobalKey(const WTempHashedString& in sGlobalKey, const WGameObject@& out pObject)", asMETHODPR(WWorld, TryGetObjectWithGlobalKey, (const WTempHashedString& sGlobalKey, const WGameObject*& out_pObject) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "WGameObject@ SearchForObject(WStringView sSearchPath, WGameObject@ pStartSearchObj = null, const WRTTI@ pExpectedComponent = null)", asMETHODPR(WWorld, SearchForObject, (WStringView, WGameObject*, const WRTTI*), WGameObject*), asCALL_THISCALL));

  // GetOrCreateModule

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool IsValidComponent(WComponentHandle& in)", asMETHOD(WWorld, IsValidComponent), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "bool TryGetComponent(const WComponentHandle& in hComponent, ?& out component)", asFUNCTION(WWorld_TryGetComponent), asCALL_GENERIC));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void SendMessage(const WGameObjectHandle& in hReceiverObject, WMessage& inout msg)", asMETHODPR(WWorld, SendMessage, (const WGameObjectHandle&, WMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void SendMessageRecursive(const WGameObjectHandle& in hReceiverObject, WMessage& inout msg)", asMETHODPR(WWorld, SendMessageRecursive, (const WGameObjectHandle&, WMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void PostMessage(const WGameObjectHandle& in hReceiverObject, const WMessage& in msg, WTime delay, WObjectMsgQueueType queueType = WObjectMsgQueueType::NextFrame) const", asMETHODPR(WWorld, PostMessage, (const WGameObjectHandle&, const WMessage&, WTime, WObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void PostMessageRecursive(const WGameObjectHandle& in hReceiverObject, const WMessage& in msg, WTime delay, WObjectMsgQueueType queueType = WObjectMsgQueueType::NextFrame) const", asMETHODPR(WWorld, PostMessageRecursive, (const WGameObjectHandle&, const WMessage&, WTime, WObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void SendMessage(const WComponentHandle& in hReceiverObject, WMessage& inout msg)", asMETHODPR(WWorld, SendMessage, (const WComponentHandle&, WMessage&), void), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void PostMessage(const WComponentHandle& in hReceiverObject, const WMessage& in msg, WTime delay, WObjectMsgQueueType queueType = WObjectMsgQueueType::NextFrame) const", asMETHODPR(WWorld, PostMessage, (const WComponentHandle&, const WMessage&, WTime, WObjectMsgQueueType::Enum) const, void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "WClock@ GetClock()", asMETHODPR(WWorld, GetClock, (), WClock&), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "const WClock@ GetClock() const", asMETHODPR(WWorld, GetClock, () const, const WClock&),
    asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "WRandom@ GetRandomNumberGenerator()", asMETHOD(WWorld, GetRandomNumberGenerator), asCALL_THISCALL));

  // AS messages
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void SendMessage(const WGameObjectHandle& in hReceiverObject, WAngelScriptMessage& inout)", asFUNCTION(WWorld_SendAsMessage), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void SendMessageRecursive(const WGameObjectHandle& in hReceiverObject, WAngelScriptMessage& inout)", asFUNCTION(WWorld_SendAsMessageRecursive), asCALL_GENERIC));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void PostMessage(const WGameObjectHandle& in hReceiverObject, const WAngelScriptMessage& in, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame)", asFUNCTION(WWorld_PostAsMessage), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WWorld", "void PostMessageRecursive(const WGameObjectHandle& in hReceiverObject, const WAngelScriptMessage& in, WTime delay, WObjectMsgQueueType delivery = WObjectMsgQueueType::NextFrame)", asFUNCTION(WWorld_PostAsMessageRecursive), asCALL_GENERIC));
  }
}
