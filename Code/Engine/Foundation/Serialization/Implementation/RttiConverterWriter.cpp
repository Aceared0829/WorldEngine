#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

void WRttiConverterContext::Clear()
{
  m_GuidToObject.Clear();
  m_ObjectToGuid.Clear();
  m_QueuedObjects.Clear();
}

void WRttiConverterContext::OnUnknownTypeError(WStringView sTypeName)
{
  WLog::Error("RTTI type '{0}' is unknown, CreateObjectFromNode failed.", sTypeName);
}

WUuid WRttiConverterContext::GenerateObjectGuid(const WUuid& parentGuid, const WAbstractProperty* pProp, WVariant index, void* pObject) const
{
  W_IGNORE_UNUSED(pObject);

  WUuid guid = parentGuid;
  guid.HashCombine(WUuid::MakeStableUuidFromString(pProp->GetPropertyName()));
  if (index.IsA<WString>())
  {
    guid.HashCombine(WUuid::MakeStableUuidFromString(index.Get<WString>()));
  }
  else if (index.CanConvertTo<WUInt32>())
  {
    guid.HashCombine(WUuid::MakeStableUuidFromInt(index.ConvertTo<WUInt32>()));
  }
  else if (index.IsValid())
  {
    W_REPORT_FAILURE("Index type must be WUInt32 or WString.");
  }
  // WLog::Warning("{0},{1},{2} -> {3}", parentGuid, pProp->GetPropertyName(), index, guid);
  return guid;
}

WInternal::NewInstance<void> WRttiConverterContext::CreateObject(const WUuid& guid, const WRTTI* pRtti)
{
  W_ASSERT_DEBUG(pRtti != nullptr, "Cannot create object, RTTI type is unknown");
  if (!pRtti->GetAllocator() || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pObj = pRtti->GetAllocator()->Allocate<void>();
  RegisterObject(guid, pRtti, pObj);
  return pObj;
}

void WRttiConverterContext::DeleteObject(const WUuid& guid)
{
  auto object = GetObjectByGUID(guid);
  if (object.m_pObject)
  {
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);
  }
  UnregisterObject(guid);
}

void WRttiConverterContext::RegisterObject(const WUuid& guid, const WRTTI* pRtti, void* pObject)
{
  W_ASSERT_DEV(pObject != nullptr, "cannot register null object!");
  WRttiConverterObject& co = m_GuidToObject[guid];

  if (pRtti->IsDerivedFrom<WReflectedClass>())
  {
    pRtti = static_cast<WReflectedClass*>(pObject)->GetDynamicRTTI();
  }

  // TODO: Actually remove child owner ptr from register when deleting an object
  // W_ASSERT_DEV(co.m_pObject == nullptr || (co.m_pObject == pObject && co.m_pType == pRtti), "Registered same guid twice with different
  // values");

  co.m_pObject = pObject;
  co.m_pType = pRtti;

  m_ObjectToGuid[pObject] = guid;
}

void WRttiConverterContext::UnregisterObject(const WUuid& guid)
{
  WRttiConverterObject* pObj;
  if (m_GuidToObject.TryGetValue(guid, pObj))
  {
    m_GuidToObject.Remove(guid);
    m_ObjectToGuid.Remove(pObj->m_pObject);
  }
}

WRttiConverterObject WRttiConverterContext::GetObjectByGUID(const WUuid& guid) const
{
  WRttiConverterObject object;
  m_GuidToObject.TryGetValue(guid, object);
  return object;
}

WUuid WRttiConverterContext::GetObjectGUID(const WRTTI* pRtti, const void* pObject) const
{
  W_IGNORE_UNUSED(pRtti);

  WUuid guid;

  if (pObject != nullptr)
    m_ObjectToGuid.TryGetValue(pObject, guid);

  return guid;
}

const WRTTI* WRttiConverterContext::FindTypeByName(WStringView sName) const
{
  return WRTTI::FindTypeByName(sName);
}

WUuid WRttiConverterContext::EnqueObject(const WUuid& guid, const WRTTI* pRtti, void* pObject)
{
  W_ASSERT_DEBUG(guid.IsValid(), "For stable serialization, guid must be well defined");
  WUuid res = guid;

  if (pObject != nullptr)
  {
    // In the rare case that this succeeds we already encountered the object with a different guid before.
    // This can happen if two pointer owner point to the same object.
    if (!m_ObjectToGuid.TryGetValue(pObject, res))
    {
      RegisterObject(guid, pRtti, pObject);
    }

    m_QueuedObjects.Insert(res);
  }
  else
  {
    // Replace nullptr with invalid uuid.
    res = WUuid();
  }
  return res;
}

WRttiConverterObject WRttiConverterContext::DequeueObject()
{
  if (!m_QueuedObjects.IsEmpty())
  {
    auto it = m_QueuedObjects.GetIterator();
    auto object = GetObjectByGUID(it.Key());
    W_ASSERT_DEV(object.m_pObject != nullptr, "Enqueued object was never registered!");

    m_QueuedObjects.Remove(it);

    return object;
  }

  return WRttiConverterObject();
}


WRttiConverterWriter::WRttiConverterWriter(WAbstractObjectGraph* pGraph, WRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs)
{
  m_pGraph = pGraph;
  m_pContext = pContext;

  m_Filter = [bSerializeReadOnly, bSerializeOwnerPtrs](const void* pObject, const WAbstractProperty* pProp)
  {
    W_IGNORE_UNUSED(pObject);

    if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly) && !bSerializeReadOnly)
      return false;

    if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner) && !bSerializeOwnerPtrs)
      return false;

    return true;
  };
}

WRttiConverterWriter::WRttiConverterWriter(WAbstractObjectGraph* pGraph, WRttiConverterContext* pContext, FilterFunction filter)
  : m_pContext(pContext)
  , m_pGraph(pGraph)
  , m_Filter(filter)
{
  W_ASSERT_DEBUG(filter.IsValid(), "Either filter function must be valid or a different ctor must be chosen.");
}

WAbstractObjectNode* WRttiConverterWriter::AddObjectToGraph(const WRTTI* pRtti, const void* pObject, const char* szNodeName)
{
  const WUuid guid = m_pContext->GetObjectGUID(pRtti, pObject);
  W_ASSERT_DEV(guid.IsValid(), "The object was not registered. Call WRttiConverterContext::RegisterObject before adding.");
  WAbstractObjectNode* pNode = AddSubObjectToGraph(pRtti, pObject, guid, szNodeName);

  WRttiConverterObject obj = m_pContext->DequeueObject();
  while (obj.m_pObject != nullptr)
  {
    const WUuid objectGuid = m_pContext->GetObjectGUID(obj.m_pType, obj.m_pObject);
    AddSubObjectToGraph(obj.m_pType, obj.m_pObject, objectGuid, nullptr);

    obj = m_pContext->DequeueObject();
  }

  return pNode;
}

WAbstractObjectNode* WRttiConverterWriter::AddSubObjectToGraph(const WRTTI* pRtti, const void* pObject, const WUuid& guid, const char* szNodeName)
{
  WAbstractObjectNode* pNode = m_pGraph->AddNode(guid, pRtti->GetTypeName(), pRtti->GetTypeVersion(), szNodeName);
  AddProperties(pNode, pRtti, pObject);
  return pNode;
}

void WRttiConverterWriter::AddProperty(WAbstractObjectNode* pNode, const WAbstractProperty* pProp, const void* pObject)
{
  if (!m_Filter(pObject, pProp))
    return;

  WVariant vTemp;
  WStringBuilder sTemp;
  const WRTTI* pPropType = pProp->GetSpecificType();
  const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      const WAbstractMemberProperty* pSpecific = static_cast<const WAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        vTemp = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();

        WUuid guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, WVariant(), pRefrencedObject);
        if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
        {
          guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
        else
        {
          guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
        {
          vTemp = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          WReflectionUtils::EnumerationToString(pPropType, vTemp.Get<WInt64>(), sTemp);

          pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
        }
        else if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject));
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Class) && pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);


          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            const WUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, WVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();

            pSpecific->GetValuePtr(pObject, pSubObject);
            const WUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, WVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
    }
    break;
    case WPropertyCategory::Array:
    {
      const WAbstractArrayProperty* pSpecific = static_cast<const WAbstractArrayProperty*>(pProp);
      WUInt32 uiCount = pSpecific->GetCount(pObject);
      WVariantArray values;
      values.SetCount(uiCount);

      if (pSpecific->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        for (WUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();

          WUuid guid;
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          values[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), values);
      }
      else
      {
        if (bIsValueType)
        {
          for (WUInt32 i = 0; i < uiCount; ++i)
          {
            values[i] = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
        }
        else if (pSpecific->GetFlags().IsSet(WPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          for (WUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            const WUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            values[i] = SubObjectGuid;
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
          pPropType->GetAllocator()->Deallocate(pSubObject);
        }
      }
    }
    break;
    case WPropertyCategory::Set:
    {
      const WAbstractSetProperty* pSpecific = static_cast<const WAbstractSetProperty*>(pProp);

      WTempHybridArray<WVariant, 16> values;
      pSpecific->GetValues(pObject, values);

      WVariantArray ValuesCopied(values);

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        for (WUInt32 i = 0; i < values.GetCount(); ++i)
        {
          void* pRefrencedObject = values[i].ConvertTo<void*>();

          WUuid guid;
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case WPropertyCategory::Map:
    {
      const WAbstractMapProperty* pSpecific = static_cast<const WAbstractMapProperty*>(pProp);

      WTempHybridArray<WString, 16> keys;
      pSpecific->GetKeys(pObject, keys);

      WVariantDictionary ValuesCopied;
      ValuesCopied.Reserve(keys.GetCount());

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        for (WUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          WVariant value = WReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
          void* pRefrencedObject = value.ConvertTo<void*>();

          WUuid guid;
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, WVariant(keys[i]), pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied.Insert(keys[i], guid);
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (bIsValueType)
        {
          for (WUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            WVariant value = WReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            ValuesCopied.Insert(keys[i], value);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
        {
          for (WUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
            W_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pSubObject););
            W_VERIFY(pSpecific->GetValue(pObject, keys[i], pSubObject), "Key should be valid.");

            const WUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, WVariant(keys[i]), pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
            ValuesCopied.Insert(keys[i], SubObjectGuid);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case WPropertyCategory::Constant:
      // Nothing to do here.
      break;
    default:
      break;
  }
}

void WRttiConverterWriter::AddProperties(WAbstractObjectNode* pNode, const WRTTI* pRtti, const void* pObject)
{
  if (pRtti->GetParentType())
    AddProperties(pNode, pRtti->GetParentType(), pObject);

  for (const auto* pProp : pRtti->GetProperties())
  {
    AddProperty(pNode, pProp, pObject);
  }
}
