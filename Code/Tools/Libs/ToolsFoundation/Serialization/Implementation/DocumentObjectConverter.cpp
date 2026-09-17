#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

WAbstractObjectNode* WDocumentObjectConverterWriter::AddObjectToGraph(const WDocumentObject* pObject, WStringView sNodeName)
{
  WAbstractObjectNode* pNode = AddSubObjectToGraph(pObject, sNodeName);

  while (!m_QueuedObjects.IsEmpty())
  {
    auto itCur = m_QueuedObjects.GetIterator();

    AddSubObjectToGraph(itCur.Key(), nullptr);

    m_QueuedObjects.Remove(itCur);
  }

  return pNode;
}

void WDocumentObjectConverterWriter::AddProperty(WAbstractObjectNode* pNode, const WAbstractProperty* pProp, const WDocumentObject* pObject)
{
  if (m_Filter.IsValid() && !m_Filter(pObject, pProp))
    return;

  const WRTTI* pPropType = pProp->GetSpecificType();
  const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
        {
          const WUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<WUuid>();

          pNode->AddProperty(pProp->GetPropertyName(), guid);
          if (guid.IsValid())
            m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
        else
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
        {
          WStringBuilder sTemp;
          WReflectionUtils::EnumerationToString(
            pPropType, pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).ConvertTo<WInt64>(), sTemp);
          pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
        }
        else if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()));
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
        {
          const WUuid guid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<WUuid>();
          W_ASSERT_DEV(guid.IsValid(), "Embedded class cannot be null.");
          pNode->AddProperty(pProp->GetPropertyName(), guid);
          m_QueuedObjects.Insert(m_pManager->GetObject(guid));
        }
      }
    }

    break;

    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      const WInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      W_ASSERT_DEV(iCount >= 0, "Invalid array property size {0}", iCount);

      WVariantArray values;
      values.SetCount(iCount);

      for (WInt32 i = 0; i < iCount; ++i)
      {
        values[i] = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
        if (!bIsValueType)
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(values[i].Get<WUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
    case WPropertyCategory::Map:
    {
      const WInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      W_ASSERT_DEV(iCount >= 0, "Invalid map property size {0}", iCount);

      WVariantDictionary values;
      values.Reserve(iCount);
      WTempHybridArray<WVariant, 16> keys;
      pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), keys);

      for (const WVariant& key : keys)
      {
        WVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), key);
        values.Insert(key.Get<WString>(), value);
        if (!bIsValueType)
        {
          m_QueuedObjects.Insert(m_pManager->GetObject(value.Get<WUuid>()));
        }
      }
      pNode->AddProperty(pProp->GetPropertyName(), values);
    }
    break;
    case WPropertyCategory::Constant:
      // Nothing to do here.
      break;
    default:
      W_ASSERT_NOT_IMPLEMENTED
  }
}

void WDocumentObjectConverterWriter::AddProperties(WAbstractObjectNode* pNode, const WDocumentObject* pObject)
{
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);

  for (const auto* pProp : properties)
  {
    AddProperty(pNode, pProp, pObject);
  }
}

WAbstractObjectNode* WDocumentObjectConverterWriter::AddSubObjectToGraph(const WDocumentObject* pObject, WStringView sNodeName)
{
  WAbstractObjectNode* pNode = m_pGraph->AddNode(pObject->GetGuid(), pObject->GetType()->GetTypeName(), pObject->GetType()->GetTypeVersion(), sNodeName);
  AddProperties(pNode, pObject);
  return pNode;
}

WDocumentObjectConverterReader::WDocumentObjectConverterReader(const WAbstractObjectGraph* pGraph, WDocumentObjectManager* pManager, Mode mode)
{
  m_pManager = pManager;
  m_pGraph = pGraph;
  m_Mode = mode;
  m_uiUnknownTypeInstances = 0;
}

WDocumentObject* WDocumentObjectConverterReader::CreateObjectFromNode(const WAbstractObjectNode* pNode)
{
  WDocumentObject* pObject = nullptr;
  const WRTTI* pType = WRTTI::FindTypeByName(pNode->GetType());
  if (pType)
  {
    pObject = m_pManager->CreateObject(pType, pNode->GetGuid());
  }
  else
  {
    if (!m_UnknownTypes.Contains(pNode->GetType()))
    {
      WLog::Error("Cannot create node of unknown type '{0}'.", pNode->GetType());
      m_UnknownTypes.Insert(pNode->GetType());
    }
    m_uiUnknownTypeInstances++;
  }
  return pObject;
}

void WDocumentObjectConverterReader::AddObject(WDocumentObject* pObject, WDocumentObject* pParent, WStringView sParentProperty, WVariant index)
{
  W_ASSERT_DEV(pObject && pParent, "Need to have valid objects to add them to the document");
  if (m_Mode == WDocumentObjectConverterReader::Mode::CreateAndAddToDocument && pParent->GetDocumentObjectManager()->GetObject(pParent->GetGuid()))
  {
    m_pManager->AddObject(pObject, pParent, sParentProperty, index);
  }
  else
  {
    pParent->InsertSubObject(pObject, sParentProperty, index);
  }
}

void WDocumentObjectConverterReader::ApplyPropertiesToObject(const WAbstractObjectNode* pNode, WDocumentObject* pObject)
{
  // W_ASSERT_DEV(pObject->GetChildren().GetCount() == 0, "Can only apply properties to empty objects!");
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);

  for (auto* pProp : properties)
  {
    auto* pOtherProp = pNode->FindProperty(pProp->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, pProp, pOtherProp);
  }
}

void WDocumentObjectConverterReader::ApplyDiffToObject(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pObject, WDeque<WAbstractGraphDiffOperation>& ref_diff)
{
  WTempHybridArray<WAbstractGraphDiffOperation*, 4> change;

  for (auto& op : ref_diff)
  {
    if (op.m_Operation == WAbstractGraphDiffOperation::Op::PropertyChanged && pObject->GetGuid() == op.m_Node)
      change.PushBack(&op);
  }

  for (auto* op : change)
  {
    const WAbstractProperty* pProp = pObject->GetTypeAccessor().GetType()->FindPropertyByName(op->m_sProperty);
    if (!pProp)
      continue;

    ApplyDiff(pObjectAccessor, pObject, pProp, *op, ref_diff);
  }

  // Recurse into owned sub objects (old or new)
  for (const WDocumentObject* pSubObject : pObject->GetChildren())
  {
    ApplyDiffToObject(pObjectAccessor, pSubObject, ref_diff);
  }
}

void WDocumentObjectConverterReader::ApplyDiff(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WAbstractGraphDiffOperation& op, WDeque<WAbstractGraphDiffOperation>& diff)
{
  WStringBuilder sTemp;

  const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

  auto NeedsToBeDeleted = [&diff](const WUuid& guid) -> bool
  {
    for (auto& op : diff)
    {
      if (op.m_Operation == WAbstractGraphDiffOperation::Op::NodeRemoved && guid == op.m_Node)
        return true;
    }
    return false;
  };
  auto NeedsToBeCreated = [&diff](const WUuid& guid) -> WAbstractGraphDiffOperation*
  {
    for (auto& op : diff)
    {
      if (op.m_Operation == WAbstractGraphDiffOperation::Op::NodeAdded && guid == op.m_Node)
        return &op;
    }
    return nullptr;
  };

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags) || bIsValueType)
      {
        const WVariantType::Enum memberType = WToolsReflectionUtils::GetStorageType(pProp);
        if (memberType != WVariantType::Invalid && bIsValueType && op.m_Value.GetType() != memberType)
        {
          op.m_Value = op.m_Value.ConvertTo(memberType);
        }

        pObjectAccessor->SetValue(pObject, pProp, op.m_Value).IgnoreResult();
      }
      else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
          {
            const WUuid oldGuid = pObjectAccessor->Get<WUuid>(pObject, pProp);
            const WUuid newGuid = op.m_Value.Get<WUuid>();
            if (oldGuid.IsValid())
            {
              if (NeedsToBeDeleted(oldGuid))
              {
                pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).IgnoreResult();
              }
            }

            if (newGuid.IsValid())
            {
              if (WAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(newGuid))
              {
                pObjectAccessor->AddObject(pObject, pProp, WVariant(), WRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
              }

              const WDocumentObject* pChild = pObject->GetChild(newGuid);
              W_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
            }
          }
          else
          {
            pObjectAccessor->SetValue(pObject, pProp, op.m_Value).IgnoreResult();
          }
        }
        else
        {
          // Noting to do here, value cannot change
        }
      }
      break;
    }
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      const WVariantArray& values = op.m_Value.Get<WVariantArray>();
      WInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      if (bIsValueType || (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
      {
        for (WUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (i < (WUInt32)iCurrentCount)
            pObjectAccessor->SetValue(pObject, pProp, values[i], i).IgnoreResult();
          else
            pObjectAccessor->InsertValue(pObject, pProp, values[i], i).IgnoreResult();
        }
        for (WInt32 i = iCurrentCount - 1; i >= (WInt32)values.GetCount(); --i)
        {
          pObjectAccessor->RemoveValue(pObject, pProp, i).IgnoreResult();
        }
      }
      else // Class
      {
        const WInt32 iCurrentCount2 = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

        WTempHybridArray<WVariant, 16> currentValues;
        pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
        for (WInt32 i = iCurrentCount2 - 1; i >= 0; --i)
        {
          if (NeedsToBeDeleted(currentValues[i].Get<WUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<WUuid>())).IgnoreResult();
          }
        }

        for (WUInt32 i = 0; i < values.GetCount(); ++i)
        {
          if (WAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(values[i].Get<WUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, i, WRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(values[i].Get<WUuid>()), pObject, pProp, i).IgnoreResult();
          }
        }
      }
      break;
    }
    case WPropertyCategory::Map:
    {
      const WVariantDictionary& values = op.m_Value.Get<WVariantDictionary>();
      WTempHybridArray<WVariant, 16> keys;
      W_VERIFY(pObjectAccessor->GetKeys(pObject, pProp, keys).Succeeded(), "Property is not a map, getting keys failed.");

      if (bIsValueType || (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
      {
        for (const WVariant& key : keys)
        {
          const WString& sKey = key.Get<WString>();
          if (!values.Contains(sKey))
          {
            W_VERIFY(pObjectAccessor->RemoveValue(pObject, pProp, key).Succeeded(), "RemoveValue failed.");
          }
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          WVariant variantKey(it.Key());
          if (keys.Contains(variantKey))
            pObjectAccessor->SetValue(pObject, pProp, it.Value(), variantKey).IgnoreResult();
          else
            pObjectAccessor->InsertValue(pObject, pProp, it.Value(), variantKey).IgnoreResult();
        }
      }
      else // Class
      {
        for (const WVariant& key : keys)
        {
          WVariant value;
          W_VERIFY(pObjectAccessor->GetValue(pObject, pProp, value, key).Succeeded(), "");
          if (NeedsToBeDeleted(value.Get<WUuid>()))
          {
            pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(value.Get<WUuid>())).IgnoreResult();
          }
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          const WVariant& value = it.Value();
          WVariant variantKey(it.Key());
          if (WAbstractGraphDiffOperation* pCreate = NeedsToBeCreated(value.Get<WUuid>()))
          {
            pObjectAccessor->AddObject(pObject, pProp, variantKey, WRTTI::FindTypeByName(pCreate->m_sProperty), pCreate->m_Node).IgnoreResult();
          }
          else
          {
            pObjectAccessor->MoveObject(pObjectAccessor->GetObject(value.Get<WUuid>()), pObject, pProp, variantKey).IgnoreResult();
          }
        }
      }
      break;
    }

    case WPropertyCategory::Function:
    case WPropertyCategory::Constant:
      break; // nothing to do
  }
}

void WDocumentObjectConverterReader::ApplyProperty(WDocumentObject* pObject, const WAbstractProperty* pProp, const WAbstractObjectNode::Property* pSource)
{
  WStringBuilder sTemp;

  const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
        {
          if (pSource->m_Value.IsA<WUuid>())
          {
            const WUuid guid = pSource->m_Value.Get<WUuid>();
            if (guid.IsValid())
            {
              auto* pSubNode = m_pGraph->GetNode(guid);
              W_ASSERT_DEV(pSubNode != nullptr, "invalid document");

              if (auto* pSubObject = CreateObjectFromNode(pSubNode))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
                AddObject(pSubObject, pObject, pProp->GetPropertyName(), WVariant());
              }
            }
          }
        }
        else
        {
          pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags) || bIsValueType)
        {
          pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), pSource->m_Value);
        }
        else if (pSource->m_Value.IsA<WUuid>()) // WPropertyFlags::Class
        {
          const WUuid& nodeGuid = pSource->m_Value.Get<WUuid>();
          if (nodeGuid.IsValid())
          {
            const WUuid subObjectGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName()).Get<WUuid>();
            WDocumentObject* pEmbeddedClassObject = pObject->GetChild(subObjectGuid);
            W_ASSERT_DEV(pEmbeddedClassObject != nullptr, "CreateObject should have created all embedded classes!");
            auto* pSubNode = m_pGraph->GetNode(nodeGuid);
            W_ASSERT_DEV(pSubNode != nullptr, "invalid document");

            ApplyPropertiesToObject(pSubNode, pEmbeddedClassObject);
          }
        }
      }
      break;
    }
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      const WVariantArray& array = pSource->m_Value.Get<WVariantArray>();
      const WInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      if (bIsValueType || (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
      {
        for (WUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (i < (WUInt32)iCurrentCount)
          {
            pObject->GetTypeAccessor().SetValue(pProp->GetPropertyName(), array[i], i);
          }
          else
          {
            pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), i, array[i]);
          }
        }
        for (WInt32 i = iCurrentCount - 1; i >= (WInt32)array.GetCount(); i--)
        {
          pObject->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), i);
        }
      }
      else
      {
        for (WUInt32 i = 0; i < array.GetCount(); ++i)
        {
          const WUuid guid = array[i].Get<WUuid>();
          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            W_ASSERT_DEV(pSubNode != nullptr, "invalid document");

            if (i < (WUInt32)iCurrentCount)
            {
              // Overwrite existing object
              WUuid childGuid = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i).ConvertTo<WUuid>();
              if (WDocumentObject* pSubObject = m_pManager->GetObject(childGuid))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
              }
            }
            else
            {
              if (WDocumentObject* pSubObject = CreateObjectFromNode(pSubNode))
              {
                ApplyPropertiesToObject(pSubNode, pSubObject);
                AddObject(pSubObject, pObject, pProp->GetPropertyName(), -1);
              }
            }
          }
        }
        for (WInt32 i = iCurrentCount - 1; i >= (WInt32)array.GetCount(); i--)
        {
          W_REPORT_FAILURE("Not implemented");
        }
      }
      break;
    }
    case WPropertyCategory::Map:
    {
      const WVariantDictionary& values = pSource->m_Value.Get<WVariantDictionary>();
      WTempHybridArray<WVariant, 16> keys;
      pObject->GetTypeAccessor().GetKeys(pProp->GetPropertyName(), keys);

      if (bIsValueType || (pProp->GetFlags().IsAnySet(WPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner)))
      {
        for (const WVariant& key : keys)
        {
          pObject->GetTypeAccessor().RemoveValue(pProp->GetPropertyName(), key);
        }
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          pObject->GetTypeAccessor().InsertValue(pProp->GetPropertyName(), WVariant(it.Key()), it.Value());
        }
      }
      else
      {
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          const WVariant& value = it.Value();
          const WUuid guid = value.Get<WUuid>();

          const WVariant variantKey(it.Key());

          if (guid.IsValid())
          {
            auto* pSubNode = m_pGraph->GetNode(guid);
            W_ASSERT_DEV(pSubNode != nullptr, "invalid document");
            if (WDocumentObject* pSubObject = CreateObjectFromNode(pSubNode))
            {
              ApplyPropertiesToObject(pSubNode, pSubObject);
              AddObject(pSubObject, pObject, pProp->GetPropertyName(), variantKey);
            }
          }
        }
      }
      break;
    }

    case WPropertyCategory::Function:
    case WPropertyCategory::Constant:
      break; // nothing to do
  }
}
