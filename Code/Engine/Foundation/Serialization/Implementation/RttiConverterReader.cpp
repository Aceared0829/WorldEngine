#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>

WRttiConverterReader::WRttiConverterReader(const WAbstractObjectGraph* pGraph, WRttiConverterContext* pContext)
{
  m_pGraph = pGraph;
  m_pContext = pContext;
}

WInternal::NewInstance<void> WRttiConverterReader::CreateObjectFromNode(const WAbstractObjectNode* pNode)
{
  const WRTTI* pRtti = m_pContext->FindTypeByName(pNode->GetType());
  if (pRtti == nullptr)
  {
    m_pContext->OnUnknownTypeError(pNode->GetType());
    return nullptr;
  }

  auto pObject = m_pContext->CreateObject(pNode->GetGuid(), pRtti);
  if (pObject)
  {
    ApplyPropertiesToObject(pNode, pRtti, pObject);
  }

  CallOnObjectCreated(pNode, pRtti, pObject);
  return pObject;
}

void WRttiConverterReader::ApplyPropertiesToObject(const WAbstractObjectNode* pNode, const WRTTI* pRtti, void* pObject)
{
  W_ASSERT_DEBUG(pNode != nullptr, "Invalid node");

  if (pRtti->GetParentType() != nullptr)
    ApplyPropertiesToObject(pNode, pRtti->GetParentType(), pObject);

  for (auto* prop : pRtti->GetProperties())
  {
    auto* pOtherProp = pNode->FindProperty(prop->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, prop, pOtherProp);
  }
}

void WRttiConverterReader::ApplyProperty(void* pObject, const WAbstractProperty* pProp, const WAbstractObjectNode::Property* pSource)
{
  const WRTTI* pPropType = pProp->GetSpecificType();

  if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
    return;

  const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const WAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        if (!pSource->m_Value.IsA<WUuid>())
          return;

        WUuid guid = pSource->m_Value.Get<WUuid>();
        void* pRefrencedObject = nullptr;

        if (guid.IsValid())
        {
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            W_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              // WLog::Error("Failed to set property '{0}', type could not be created!", pProp->GetPropertyName());
              return;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
        }

        void* pOldObject = nullptr;
        pSpecific->GetValuePtr(pObject, &pOldObject);
        pSpecific->SetValuePtr(pObject, &pRefrencedObject);
        if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          WReflectionUtils::DeleteObject(pOldObject, pProp);
      }
      else
      {
        if (bIsValueType || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
        {
          WReflectionUtils::SetMemberPropertyValue(pSpecific, pObject, pSource->m_Value);
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
        {
          if (!pSource->m_Value.IsA<WUuid>())
            return;

          void* pDirectPtr = pSpecific->GetPropertyPointer(pObject);
          bool bDelete = false;
          const WUuid sourceGuid = pSource->m_Value.Get<WUuid>();

          if (pDirectPtr == nullptr)
          {
            bDelete = true;
            pDirectPtr = m_pContext->CreateObject(sourceGuid, pPropType);
          }

          auto* pNode = m_pGraph->GetNode(sourceGuid);
          W_ASSERT_DEV(pNode != nullptr, "node must exist");

          ApplyPropertiesToObject(pNode, pPropType, pDirectPtr);

          if (bDelete)
          {
            pSpecific->SetValuePtr(pObject, pDirectPtr);
            m_pContext->DeleteObject(sourceGuid);
          }
        }
      }
    }
    break;
    case WPropertyCategory::Array:
    {
      auto pSpecific = static_cast<const WAbstractArrayProperty*>(pProp);
      if (!pSource->m_Value.IsA<WVariantArray>())
        return;
      const WVariantArray& array = pSource->m_Value.Get<WVariantArray>();
      // Delete old values
      if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
      {
        const WInt32 uiOldCount = (WInt32)pSpecific->GetCount(pObject);
        for (WInt32 i = uiOldCount - 1; i >= 0; --i)
        {
          void* pOldObject = nullptr;
          pSpecific->GetValue(pObject, i, &pOldObject);
          pSpecific->Remove(pObject, i);
          if (pOldObject)
            WReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }

      pSpecific->SetCount(pObject, array.GetCount());
      if (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer))
      {
        for (WUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<WUuid>())
            continue;
          WUuid guid = array[i].Get<WUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              W_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                WLog::Error("Failed to set array property '{0}' element, type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->SetValue(pObject, i, &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (WUInt32 i = 0; i < array.GetCount(); ++i)
          {
            WReflectionUtils::SetArrayPropertyValue(pSpecific, pObject, i, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(WPropertyFlags::Class))
        {
          const WUuid temp = WUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);
          W_ASSERT_DEBUG(pValuePtr != nullptr, "Failed to create value object. No allocator?");

          for (WUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<WUuid>())
              continue;

            const WUuid sourceGuid = array[i].Get<WUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            W_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->SetValue(pObject, i, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case WPropertyCategory::Set:
    {
      auto pSpecific = static_cast<const WAbstractSetProperty*>(pProp);
      if (!pSource->m_Value.IsA<WVariantArray>())
        return;

      const WVariantArray& array = pSource->m_Value.Get<WVariantArray>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
      {
        WTempHybridArray<WVariant, 16> keys;
        pSpecific->GetValues(pObject, keys);
        pSpecific->Clear(pObject);
        for (WVariant& value : keys)
        {
          void* pOldObject = value.ConvertTo<void*>();
          if (pOldObject)
            WReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer))
      {
        for (WUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<WUuid>())
            continue;

          WUuid guid = array[i].Get<WUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            W_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              WLog::Error("Failed to insert set element into property '{0}', type could not be created!", pProp->GetPropertyName());
              continue;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (WUInt32 i = 0; i < array.GetCount(); ++i)
          {
            WReflectionUtils::InsertSetPropertyValue(pSpecific, pObject, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(WPropertyFlags::Class))
        {
          const WUuid temp = WUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (WUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<WUuid>())
              continue;

            const WUuid sourceGuid = array[i].Get<WUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            W_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case WPropertyCategory::Map:
    {
      auto pSpecific = static_cast<const WAbstractMapProperty*>(pProp);
      if (!pSource->m_Value.IsA<WVariantDictionary>())
        return;

      const WVariantDictionary& dict = pSource->m_Value.Get<WVariantDictionary>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
      {
        WTempHybridArray<WString, 16> keys;
        pSpecific->GetKeys(pObject, keys);
        for (const WString& sKey : keys)
        {
          WVariant value = WReflectionUtils::GetMapPropertyValue(pSpecific, pObject, sKey);
          void* pOldClone = value.ConvertTo<void*>();
          pSpecific->Remove(pObject, sKey);
          if (pOldClone)
            WReflectionUtils::DeleteObject(pOldClone, pProp);
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer))
      {
        for (auto it = dict.GetIterator(); it.IsValid(); ++it)
        {
          if (!it.Value().IsA<WUuid>())
            continue;

          WUuid guid = it.Value().Get<WUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              W_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                WLog::Error("Failed to insert set element into property '{0}', type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, it.Key(), &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            WReflectionUtils::SetMapPropertyValue(pSpecific, pObject, it.Key(), it.Value());
          }
        }
        else if (pProp->GetFlags().IsAnySet(WPropertyFlags::Class))
        {
          const WUuid temp = WUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            if (!it.Value().IsA<WUuid>())
              continue;

            const WUuid sourceGuid = it.Value().Get<WUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            W_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, it.Key(), pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;

    default:
      W_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}

void WRttiConverterReader::CallOnObjectCreated(const WAbstractObjectNode* pNode, const WRTTI* pRtti, void* pObject)
{
  auto functions = pRtti->GetFunctions();
  for (auto pFunc : functions)
  {
    // TODO: Make this compare faster
    if (WStringUtils::IsEqual(pFunc->GetPropertyName(), "OnObjectCreated"))
    {
      WTempHybridArray<WVariant, 1> params;
      params.PushBack(WVariant(pNode));
      WVariant ret;
      pFunc->Execute(pObject, params, ret);
    }
  }
}
