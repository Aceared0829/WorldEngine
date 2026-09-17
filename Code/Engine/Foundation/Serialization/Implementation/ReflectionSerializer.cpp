#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

////////////////////////////////////////////////////////////////////////
// WReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void WReflectionSerializer::WriteObjectToDDL(WStreamWriter& inout_stream, const WRTTI* pRtti, const void* pObject, bool bCompactMmode /*= true*/, WOpenDdlWriter::TypeStringMode typeMode /*= WOpenDdlWriter::TypeStringMode::Shortest*/)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;
  WRttiConverterWriter conv(&graph, &context, false, true);

  context.RegisterObject(WUuid::MakeUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  WAbstractGraphDdlSerializer::Write(inout_stream, &graph, nullptr, bCompactMmode, typeMode);
}

void WReflectionSerializer::WriteObjectToDDL(WOpenDdlWriter& ref_ddl, const WRTTI* pRtti, const void* pObject, WUuid guid /*= WUuid()*/)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;
  WRttiConverterWriter conv(&graph, &context, false, true);

  if (!guid.IsValid())
  {
    guid = WUuid::MakeUuid();
  }

  context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  WAbstractGraphDdlSerializer::Write(ref_ddl, &graph, nullptr);
}

void WReflectionSerializer::WriteObjectToBinary(WStreamWriter& inout_stream, const WRTTI* pRtti, const void* pObject)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;
  WRttiConverterWriter conv(&graph, &context, false, true);

  context.RegisterObject(WUuid::MakeUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  WAbstractGraphBinarySerializer::Write(inout_stream, &graph);
}

void* WReflectionSerializer::ReadObjectFromDDL(WStreamReader& inout_stream, const WRTTI*& ref_pRtti)
{
  WOpenDdlReader reader;
  if (reader.ParseDocument(inout_stream, 0, WLog::GetThreadLocalLogSystem()).Failed())
  {
    WLog::Error("Failed to parse DDL graph");
    return nullptr;
  }

  return ReadObjectFromDDL(reader.GetRootElement(), ref_pRtti);
}

void* WReflectionSerializer::ReadObjectFromDDL(const WOpenDdlReaderElement* pRootElement, const WRTTI*& ref_pRtti)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;

  WAbstractGraphDdlSerializer::Read(pRootElement, &graph).IgnoreResult();

  WRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  W_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = WRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void* WReflectionSerializer::ReadObjectFromBinary(WStreamReader& inout_stream, const WRTTI*& ref_pRtti)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;

  WAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  WRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  W_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = WRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void WReflectionSerializer::ReadObjectPropertiesFromDDL(WStreamReader& inout_stream, const WRTTI& rtti, void* pObject)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;

  WAbstractGraphDdlSerializer::Read(inout_stream, &graph).IgnoreResult();

  WRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  W_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  if (pRootNode == nullptr)
    return;

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}

void WReflectionSerializer::ReadObjectPropertiesFromBinary(WStreamReader& inout_stream, const WRTTI& rtti, void* pObject)
{
  WAbstractObjectGraph graph;
  WRttiConverterContext context;

  WAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  WRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  W_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}


namespace
{
  static void CloneProperty(const void* pObject, void* pClone, const WAbstractProperty* pProp)
  {
    if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
      return;

    const WRTTI* pPropType = pProp->GetSpecificType();

    const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

    WVariant vTemp;
    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const WAbstractMemberProperty*>(pProp);

        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          vTemp = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);

          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner) && pRefrencedObject)
          {
            pRefrencedObject = WReflectionSerializer::Clone(pRefrencedObject, pPropType);
            vTemp = WVariant(pRefrencedObject, pPropType);
          }

          WVariant vOldValue = WReflectionUtils::GetMemberPropertyValue(pSpecific, pClone);
          WReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
            WReflectionUtils::DeleteObject(vOldValue.ConvertTo<void*>(), pProp);
        }
        else
        {
          if (bIsValueType || pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
          {
            vTemp = WReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
            WReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          }
          else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
          {
            void* pSubObject = pSpecific->GetPropertyPointer(pObject);
            // Do we have direct access to the property?
            if (pSubObject != nullptr)
            {
              void* pSubClone = pSpecific->GetPropertyPointer(pClone);
              WReflectionSerializer::Clone(pSubObject, pSubClone, pPropType);
            }
            // If the property is behind an accessor, we need to retrieve it first.
            else if (pPropType->GetAllocator()->CanAllocate())
            {
              pSubObject = pPropType->GetAllocator()->Allocate<void>();
              pSpecific->GetValuePtr(pObject, pSubObject);
              pSpecific->SetValuePtr(pClone, pSubObject);
              pPropType->GetAllocator()->Deallocate(pSubObject);
            }
          }
        }
      }
      break;
      case WPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const WAbstractArrayProperty*>(pProp);
        // Delete old values
        if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
        {
          const WInt32 iCloneCount = (WInt32)pSpecific->GetCount(pClone);
          for (WInt32 i = iCloneCount - 1; i >= 0; --i)
          {
            void* pOldSubClone = nullptr;
            pSpecific->GetValue(pClone, i, &pOldSubClone);
            pSpecific->Remove(pClone, i);
            if (pOldSubClone)
              WReflectionUtils::DeleteObject(pOldSubClone, pProp);
          }
        }

        const WUInt32 uiCount = pSpecific->GetCount(pObject);
        pSpecific->SetCount(pClone, uiCount);
        if (pSpecific->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          for (WUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            void* pRefrencedObject = vTemp.ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = WReflectionSerializer::Clone(pRefrencedObject, pPropType);
              vTemp = WVariant(pRefrencedObject, pPropType);
            }
            WReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
          }
        }
        else
        {
          if (bIsValueType)
          {
            for (WUInt32 i = 0; i < uiCount; ++i)
            {
              vTemp = WReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
              WReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
            }
          }
          else if (pProp->GetFlags().IsSet(WPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

            for (WUInt32 i = 0; i < uiCount; ++i)
            {
              pSpecific->GetValue(pObject, i, pSubObject);
              pSpecific->SetValue(pClone, i, pSubObject);
            }

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
      break;
      case WPropertyCategory::Set:
      {
        auto pSpecific = static_cast<const WAbstractSetProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
        {
          WTempHybridArray<WVariant, 16> keys;
          pSpecific->GetValues(pClone, keys);
          pSpecific->Clear(pClone);
          for (WVariant& value : keys)
          {
            void* pOldClone = value.ConvertTo<void*>();
            if (pOldClone)
              WReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
        pSpecific->Clear(pClone);

        WTempHybridArray<WVariant, 16> values;
        pSpecific->GetValues(pObject, values);


        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          for (WUInt32 i = 0; i < values.GetCount(); ++i)
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = WReflectionSerializer::Clone(pRefrencedObject, pPropType);
            }
            vTemp = WVariant(pRefrencedObject, pPropType);
            WReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, vTemp);
          }
        }
        else if (bIsValueType)
        {
          for (WUInt32 i = 0; i < values.GetCount(); ++i)
          {
            WReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, values[i]);
          }
        }
      }
      break;
      case WPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const WAbstractMapProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
        {
          WTempHybridArray<WString, 16> keys;
          pSpecific->GetKeys(pClone, keys);
          for (const WString& sKey : keys)
          {
            WVariant value = WReflectionUtils::GetMapPropertyValue(pSpecific, pClone, sKey);
            void* pOldClone = value.ConvertTo<void*>();
            pSpecific->Remove(pClone, sKey);
            if (pOldClone)
              WReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
        pSpecific->Clear(pClone);

        WTempHybridArray<WString, 16> keys;
        pSpecific->GetKeys(pObject, keys);

        for (WUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          if (bIsValueType ||
              (pProp->GetFlags().IsSet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
          {
            WVariant value = WReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            WReflectionUtils::SetMapPropertyValue(pSpecific, pClone, keys[i], value);
          }
          else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
          {
            if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
            {
              void* pValue = nullptr;
              pSpecific->GetValue(pObject, keys[i], &pValue);
              pValue = WReflectionSerializer::Clone(pValue, pPropType);
              pSpecific->Insert(pClone, keys[i], &pValue);
            }
            else
            {
              if (pPropType->GetAllocator()->CanAllocate())
              {
                void* pValue = pPropType->GetAllocator()->Allocate<void>();
                W_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pValue););
                W_VERIFY(pSpecific->GetValue(pObject, keys[i], pValue), "Previously retrieved key does not exist.");
                pSpecific->Insert(pClone, keys[i], pValue);
              }
              else
              {
                WLog::Error("The property '{0}' can not be cloned as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
              }
            }
          }
        }
      }
      break;
      default:
        break;
    }
  }

  static void CloneProperties(const void* pObject, void* pClone, const WRTTI* pType)
  {
    if (pType->GetParentType())
      CloneProperties(pObject, pClone, pType->GetParentType());

    for (auto* pProp : pType->GetProperties())
    {
      CloneProperty(pObject, pClone, pProp);
    }
  }
} // namespace

void* WReflectionSerializer::Clone(const void* pObject, const WRTTI* pType)
{
  if (!pObject)
    return nullptr;

  W_ASSERT_DEV(pType != nullptr, "invalid type.");
  if (pType->IsDerivedFrom<WReflectedClass>())
  {
    const WReflectedClass* pRefObject = static_cast<const WReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
  }

  W_ASSERT_DEV(pType->GetAllocator()->CanAllocate(), "The type '{0}' can't be cloned!", pType->GetTypeName());
  void* pClone = pType->GetAllocator()->Allocate<void>();
  CloneProperties(pObject, pClone, pType);
  return pClone;
}


void WReflectionSerializer::Clone(const void* pObject, void* pClone, const WRTTI* pType)
{
  W_ASSERT_DEV(pObject && pClone && pType, "invalid type.");
  if (pType->IsDerivedFrom<WReflectedClass>())
  {
    const WReflectedClass* pRefObject = static_cast<const WReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
    W_ASSERT_DEV(pType == static_cast<WReflectedClass*>(pClone)->GetDynamicRTTI(), "Object '{0}' and clone '{1}' have mismatching types!", pType->GetTypeName(), static_cast<WReflectedClass*>(pClone)->GetDynamicRTTI()->GetTypeName());
  }

  CloneProperties(pObject, pClone, pType);
}
