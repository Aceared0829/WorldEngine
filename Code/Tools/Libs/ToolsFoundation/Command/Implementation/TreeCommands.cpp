#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAddObjectCommand, 1, WRTTIDefaultAllocator<WAddObjectCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Type", GetType, SetType),
    W_MEMBER_PROPERTY("ParentGuid", m_Parent),
    W_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    W_MEMBER_PROPERTY("Index", m_Index),
    W_MEMBER_PROPERTY("NewGuid", m_NewObjectGuid),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPasteObjectsCommand, 1, WRTTIDefaultAllocator<WPasteObjectsCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ParentGuid", m_Parent),
    W_MEMBER_PROPERTY("TextGraph", m_sGraphTextFormat),
    W_MEMBER_PROPERTY("Mime", m_sMimeType),
    W_MEMBER_PROPERTY("AllowPickedPosition", m_bAllowPickedPosition),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInstantiatePrefabCommand, 1, WRTTIDefaultAllocator<WInstantiatePrefabCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ParentGuid", m_Parent),
    W_MEMBER_PROPERTY("CreateFromPrefab", m_CreateFromPrefab),
    W_MEMBER_PROPERTY("BaseGraph", m_sBasePrefabGraph),
    W_MEMBER_PROPERTY("ObjectGraph", m_sObjectGraph),
    W_MEMBER_PROPERTY("RemapGuid", m_RemapGuid),
    W_MEMBER_PROPERTY("CreatedObjects", m_CreatedRootObject),
    W_MEMBER_PROPERTY("AllowPickedPos", m_bAllowPickedPosition),
    W_MEMBER_PROPERTY("Index", m_Index),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WUnlinkPrefabCommand, 1, WRTTIDefaultAllocator<WUnlinkPrefabCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Object", m_Object),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRemoveObjectCommand, 1, WRTTIDefaultAllocator<WRemoveObjectCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMoveObjectCommand, 1, WRTTIDefaultAllocator<WMoveObjectCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("NewParentGuid", m_NewParent),
    W_MEMBER_PROPERTY("ParentProperty", m_sParentProperty),
    W_MEMBER_PROPERTY("Index", m_Index),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSetObjectPropertyCommand, 1, WRTTIDefaultAllocator<WSetObjectPropertyCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("NewValue", m_NewValue),
    W_MEMBER_PROPERTY("Index", m_Index),
    W_MEMBER_PROPERTY("Property", m_sProperty),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WResizeAndSetObjectPropertyCommand, 1, WRTTIDefaultAllocator<WResizeAndSetObjectPropertyCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("NewValue", m_NewValue),
    W_MEMBER_PROPERTY("Index", m_Index),
    W_MEMBER_PROPERTY("Property", m_sProperty),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInsertObjectPropertyCommand, 1, WRTTIDefaultAllocator<WInsertObjectPropertyCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("NewValue", m_NewValue),
    W_MEMBER_PROPERTY("Index", m_Index),
    W_MEMBER_PROPERTY("Property", m_sProperty),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRemoveObjectPropertyCommand, 1, WRTTIDefaultAllocator<WRemoveObjectPropertyCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("Index", m_Index),
    W_MEMBER_PROPERTY("Property", m_sProperty),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMoveObjectPropertyCommand, 1, WRTTIDefaultAllocator<WMoveObjectPropertyCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("ObjectGuid", m_Object),
    W_MEMBER_PROPERTY("OldIndex", m_OldIndex),
    W_MEMBER_PROPERTY("NewIndex", m_NewIndex),
    W_MEMBER_PROPERTY("Property", m_sProperty),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

////////////////////////////////////////////////////////////////////////
// WAddObjectCommand
////////////////////////////////////////////////////////////////////////

WAddObjectCommand::WAddObjectCommand()

  = default;

WStringView WAddObjectCommand::GetType() const
{
  if (m_pType == nullptr)
    return {};

  return m_pType->GetTypeName();
}

void WAddObjectCommand::SetType(WStringView sType)
{
  m_pType = WRTTI::FindTypeByName(sType);
}

WStatus WAddObjectCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (!m_NewObjectGuid.IsValid())
      m_NewObjectGuid = WUuid::MakeUuid();
  }

  WDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return WStatus("Add Object: The given parent does not exist!");
  }

  W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pType, pParent, m_sParentProperty, m_Index));

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->CreateObject(m_pType, m_NewObjectGuid);
  }

  pDocument->GetObjectManager()->AddObject(m_pObject, pParent, m_sParentProperty, m_Index);
  return WStatus(W_SUCCESS);
}

WStatus WAddObjectCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  WDocument* pDocument = GetDocument();
  W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return WStatus(W_SUCCESS);
}

void WAddObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// WPasteObjectsCommand
////////////////////////////////////////////////////////////////////////

WPasteObjectsCommand::WPasteObjectsCommand() = default;

WStatus WPasteObjectsCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  WDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return WStatus("Paste Objects: The given parent does not exist!");
  }

  if (!bRedo)
  {
    WAbstractObjectGraph graph;

    {
      // Deserialize
      WRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
      W_SUCCEED_OR_RETURN(WAbstractGraphDdlSerializer::Read(memoryReader, &graph));
    }

    // Remap
    graph.ReMapNodeGuids(WUuid::MakeUuid());

    WDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateOnly);

    WTempHybridArray<WAbstractObjectNode*, 16> RootNodes;
    auto& nodes = graph.GetAllNodes();
    for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      if (pNode->GetNodeName() == "root")
      {
        RootNodes.PushBack(pNode);
      }
    }

    RootNodes.Sort([](const WAbstractObjectNode* a, const WAbstractObjectNode* b)
      {
      auto* pOrderA = a->FindProperty("__Order");
      auto* pOrderB = b->FindProperty("__Order");
      if (pOrderA && pOrderB && pOrderA->m_Value.CanConvertTo<WUInt32>() && pOrderB->m_Value.CanConvertTo<WUInt32>())
      {
        return pOrderA->m_Value.ConvertTo<WUInt32>() < pOrderB->m_Value.ConvertTo<WUInt32>();
      }
      return a < b; });

    WTempHybridArray<WDocument::PasteInfo, 16> ToBePasted;
    for (WAbstractObjectNode* pNode : RootNodes)
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        auto& ref = ToBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = pParent;
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, m_sMimeType))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }
    }

    if (m_PastedObjects.IsEmpty())
      return WStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WPasteObjectsCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  WDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return WStatus(W_SUCCESS);
}

void WPasteObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }
}

////////////////////////////////////////////////////////////////////////
// WInstantiatePrefabCommand
////////////////////////////////////////////////////////////////////////

WInstantiatePrefabCommand::WInstantiatePrefabCommand()
{
  m_bAllowPickedPosition = true;
}

WStatus WInstantiatePrefabCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  WDocumentObject* pParent = nullptr;
  if (m_Parent.IsValid())
  {
    pParent = pDocument->GetObjectManager()->GetObject(m_Parent);
    if (pParent == nullptr)
      return WStatus("Instantiate Prefab: The given parent does not exist!");
  }

  if (!bRedo)
  {
    // TODO: this is hard-coded, it only works for scene documents !
    const WRTTI* pRootObjectType = WRTTI::FindTypeByName("WGameObject");
    WStringView sParentProperty = "Children"_wsv;

    WDocumentObject* pRootObject = nullptr;
    WTempHybridArray<WDocument::PasteInfo, 16> ToBePasted;
    WAbstractObjectGraph graph;

    // create root object
    {
      W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(pRootObjectType, pParent, sParentProperty, m_Index));

      // use the same GUID for the root object ID as the remap GUID, this way the object ID is deterministic and reproducible
      m_CreatedRootObject = m_RemapGuid;

      pRootObject = pDocument->GetObjectManager()->CreateObject(pRootObjectType, m_CreatedRootObject);

      auto& ref = ToBePasted.ExpandAndGetRef();
      ref.m_pObject = pRootObject;
      ref.m_pParent = pParent;
      ref.m_Index = m_Index;
    }

    // update meta data
    // this is read when Paste is executed, to determine a good node name
    {
      // if prefabs are not allowed in this document, just create this as a regular object, with no link to the prefab template
      if (pDocument->ArePrefabsAllowed())
      {
        auto pMeta = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_CreatedRootObject);
        pMeta->m_CreateFromPrefab = m_CreateFromPrefab;
        pMeta->m_PrefabSeedGuid = m_RemapGuid;
        pMeta->m_sBasePrefab = m_sBasePrefabGraph;
        pDocument->m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::PrefabFlag);
      }
      else
      {
        pDocument->ShowDocumentStatus("Nested prefabs are not allowed. Instantiated object will not be linked to prefab template.");
      }
    }

    if (pDocument->Paste(ToBePasted, graph, m_bAllowPickedPosition, "application/WEditor.WAbstractGraph"))
    {
      for (const auto& item : ToBePasted)
      {
        auto& po = m_PastedObjects.ExpandAndGetRef();
        po.m_pObject = item.m_pObject;
        po.m_pParent = item.m_pParent;
        po.m_Index = item.m_pObject->GetPropertyIndex();
        po.m_sParentProperty = item.m_pObject->GetParentProperty();
      }
    }
    else
    {
      for (const auto& item : ToBePasted)
      {
        pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
      }

      ToBePasted.Clear();
    }

    if (m_PastedObjects.IsEmpty())
      return WStatus("Paste Objects: nothing was pasted!");

    if (!m_sObjectGraph.IsEmpty())
      WPrefabUtils::LoadGraph(graph, m_sObjectGraph);
    else
      WPrefabUtils::LoadGraph(graph, m_sBasePrefabGraph);

    graph.ReMapNodeGuids(m_RemapGuid);

    // a prefab can have multiple top level nodes
    WTempHybridArray<WAbstractObjectNode*, 4> rootNodes;
    WPrefabUtils::GetRootNodes(graph, rootNodes);

    for (auto* pPrefabRoot : rootNodes)
    {
      WDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateOnly);

      if (auto* pNewObject = reader.CreateObjectFromNode(pPrefabRoot))
      {
        reader.ApplyPropertiesToObject(pPrefabRoot, pNewObject);

        // attach all prefab nodes to the main group node
        pDocument->GetObjectManager()->AddObject(pNewObject, pRootObject, sParentProperty, -1);
      }
    }
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_PastedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  return WStatus(W_SUCCESS);
}

WStatus WInstantiatePrefabCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  WDocument* pDocument = GetDocument();

  for (auto& po : m_PastedObjects)
  {
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return WStatus(W_SUCCESS);
}

void WInstantiatePrefabCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_PastedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_PastedObjects.Clear();
  }
}


//////////////////////////////////////////////////////////////////////////
// WUnlinkPrefabCommand
//////////////////////////////////////////////////////////////////////////

WStatus WUnlinkPrefabCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();
  WDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return WStatus("Unlink Prefab: The given object does not exist!");

  // store previous values
  if (!bRedo)
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData->BeginReadMetaData(m_Object);
    m_OldCreateFromPrefab = pMeta->m_CreateFromPrefab;
    m_OldRemapGuid = pMeta->m_PrefabSeedGuid;
    m_sOldGraphTextFormat = pMeta->m_sBasePrefab;
    pDocument->m_DocumentObjectMetaData->EndReadMetaData();
  }

  // unlink
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = WUuid();
    pMeta->m_PrefabSeedGuid = WUuid();
    pMeta->m_sBasePrefab.Clear();
    pDocument->m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::PrefabFlag);
  }

  return WStatus(W_SUCCESS);
}

WStatus WUnlinkPrefabCommand::UndoInternal(bool bFireEvents)
{
  WDocument* pDocument = GetDocument();
  WDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(m_Object);

  if (pObject == nullptr)
    return WStatus("Unlink Prefab: The given object does not exist!");

  // restore link
  {
    auto pMeta = pDocument->m_DocumentObjectMetaData->BeginModifyMetaData(m_Object);
    pMeta->m_CreateFromPrefab = m_OldCreateFromPrefab;
    pMeta->m_PrefabSeedGuid = m_OldRemapGuid;
    pMeta->m_sBasePrefab = m_sOldGraphTextFormat;
    pDocument->m_DocumentObjectMetaData->EndModifyMetaData(WDocumentObjectMetaData::PrefabFlag);
  }

  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WRemoveObjectCommand
////////////////////////////////////////////////////////////////////////

WRemoveObjectCommand::WRemoveObjectCommand()

  = default;

WStatus WRemoveObjectCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return WStatus("Remove Object: The given object does not exist!");
    }
    else
      return WStatus("Remove Object: The given object does not exist!");

    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(m_pObject));

    m_pParent = const_cast<WDocumentObject*>(m_pObject->GetParent());
    m_sParentProperty = m_pObject->GetParentProperty();
    const WIReflectedTypeAccessor& accessor = m_pObject->GetParent()->GetTypeAccessor();
    m_Index = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());
  }

  pDocument->GetObjectManager()->RemoveObject(m_pObject);
  return WStatus(W_SUCCESS);
}

WStatus WRemoveObjectCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  WDocument* pDocument = GetDocument();
  W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanAdd(m_pObject->GetTypeAccessor().GetType(), m_pParent, m_sParentProperty, m_Index));

  pDocument->GetObjectManager()->AddObject(m_pObject, m_pParent, m_sParentProperty, m_Index);
  return WStatus(W_SUCCESS);
}

void WRemoveObjectCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasDone)
  {
    GetDocument()->GetObjectManager()->DestroyObject(m_pObject);
    m_pObject = nullptr;
  }
}


////////////////////////////////////////////////////////////////////////
// WMoveObjectCommand
////////////////////////////////////////////////////////////////////////

WMoveObjectCommand::WMoveObjectCommand()
{
  m_pObject = nullptr;
  m_pOldParent = nullptr;
  m_pNewParent = nullptr;
}

WStatus WMoveObjectCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return WStatus("Move Object: The given object does not exist!");
    }

    if (m_NewParent.IsValid())
    {
      m_pNewParent = pDocument->GetObjectManager()->GetObject(m_NewParent);
      if (m_pNewParent == nullptr)
        return WStatus("Move Object: The new parent does not exist!");
    }

    m_pOldParent = const_cast<WDocumentObject*>(m_pObject->GetParent());
    m_sOldParentProperty = m_pObject->GetParentProperty();
    const WIReflectedTypeAccessor& accessor = m_pOldParent->GetTypeAccessor();
    m_OldIndex = accessor.GetPropertyChildIndex(m_pObject->GetParentProperty(), m_pObject->GetGuid());

    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pNewParent, m_sParentProperty, m_Index));
  }

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pNewParent, m_sParentProperty, m_Index);
  return WStatus(W_SUCCESS);
}

WStatus WMoveObjectCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  WDocument* pDocument = GetDocument();

  WVariant FinalOldPosition = m_OldIndex;

  if (m_Index.CanConvertTo<WInt32>() && m_pOldParent == m_pNewParent)
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same
    // position) so an object must always be moved by at least +2 moving UP can be done by -1, so when we undo that, we must ensure to move
    // +2

    WInt32 iNew = m_Index.ConvertTo<WInt32>();
    WInt32 iOld = m_OldIndex.ConvertTo<WInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }
  }

  W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanMove(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition));

  pDocument->GetObjectManager()->MoveObject(m_pObject, m_pOldParent, m_sOldParentProperty, FinalOldPosition);

  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

WSetObjectPropertyCommand::WSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

WStatus WSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    // if this assert triggers because of a stringview, check the caller and make sure to copy the stringview into a string first
    // something like this:
    // const WVariantType::Enum storageType = WToolsReflectionUtils::GetStorageType(pProp);
    // if (op.m_Value.GetType() != storageType)
    //{
    //  op.m_Value = op.m_Value.ConvertTo(storageType);
    //}
    W_ASSERT_DEBUG(m_NewValue.GetType() != WVariantType::StringView && m_NewValue.GetType() != WVariantType::TypedPointer, "Variants that are stored in the command history must hold ownership of their value.");

    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return WStatus("Set Property: The given object does not exist!");
    }
    else
      return WStatus("Set Property: The given object does not exist!");

    WIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    WStatus res(W_SUCCESS);
    m_OldValue = accessor0.GetValue(m_sProperty, m_Index, &res);

    if (res.Failed())
      return res;

    const WAbstractProperty* pProp = accessor0.GetType()->FindPropertyByName(m_sProperty);
    if (pProp == nullptr)
      return WStatus(WFmt("Set Property: The property '{0}' does not exist", m_sProperty));

    if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
    {
      return WStatus(WFmt("Set Property: The property '{0}' is a PointerOwner, use WAddObjectCommand instead", m_sProperty));
    }

    if (pProp->GetAttributeByType<WTemporaryAttribute>())
    {
      // if we modify a 'temporary' property, ie. one that is not serialized,
      // don't mark the document as modified
      m_bModifiedDocument = false;
    }
  }

  return pDocument->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

WStatus WSetObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->SetValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    WIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.SetValue(m_sProperty, m_OldValue, m_Index))
    {
      return WStatus(WFmt("Set Property: The property '{0}' does not exist", m_sProperty));
    }
  }
  return WStatus(W_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// WSetObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

WResizeAndSetObjectPropertyCommand::WResizeAndSetObjectPropertyCommand()
{
  m_pObject = nullptr;
}

WStatus WResizeAndSetObjectPropertyCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return WStatus("Set Property: The given object does not exist!");
    }
    else
      return WStatus("Set Property: The given object does not exist!");

    const WInt32 uiIndex = m_Index.ConvertTo<WInt32>();

    WIReflectedTypeAccessor& accessor0 = m_pObject->GetTypeAccessor();

    const WInt32 iCount = accessor0.GetCount(m_sProperty);

    for (WInt32 i = iCount; i <= uiIndex; ++i)
    {
      WInsertObjectPropertyCommand ins;
      ins.m_Object = m_Object;
      ins.m_sProperty = m_sProperty;
      ins.m_Index = i;
      ins.m_NewValue = WReflectionUtils::GetDefaultVariantFromType(m_NewValue.GetType());

      AddSubCommand(ins).AssertSuccess();
    }

    WSetObjectPropertyCommand set;
    set.m_sProperty = m_sProperty;
    set.m_Index = m_Index;
    set.m_NewValue = m_NewValue;
    set.m_Object = m_Object;

    AddSubCommand(set).AssertSuccess();
  }

  return WStatus(W_SUCCESS);
}

////////////////////////////////////////////////////////////////////////
// WInsertObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

WInsertObjectPropertyCommand::WInsertObjectPropertyCommand()
{
  m_pObject = nullptr;
}

WStatus WInsertObjectPropertyCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return WStatus("Insert Property: The given object does not exist!");
    }
    else
      return WStatus("Insert Property: The given object does not exist!");

    if (m_Index.CanConvertTo<WInt32>() && m_Index.ConvertTo<WInt32>() == -1)
    {
      WIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
      m_Index = accessor.GetCount(m_sProperty.GetData());
    }
  }

  return pDocument->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_NewValue, m_Index);
}

WStatus WInsertObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
  }
  else
  {
    WIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.RemoveValue(m_sProperty, m_Index))
    {
      return WStatus(WFmt("Insert Property: The property '{0}' does not exist", m_sProperty));
    }
  }

  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WRemoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

WRemoveObjectPropertyCommand::WRemoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

WStatus WRemoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    if (m_Object.IsValid())
    {
      m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
      if (m_pObject == nullptr)
        return WStatus("Remove Property: The given object does not exist!");

      WStatus res(W_SUCCESS);

      m_OldValue = m_pObject->GetTypeAccessor().GetValue(m_sProperty, m_Index, &res);
      if (res.Failed())
        return res;
    }
    else
    {
      return WStatus("Remove Property: The given object does not exist!");
    }
  }

  return pDocument->GetObjectManager()->RemoveValue(m_pObject, m_sProperty, m_Index);
}

WStatus WRemoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  if (bFireEvents)
  {
    return GetDocument()->GetObjectManager()->InsertValue(m_pObject, m_sProperty, m_OldValue, m_Index);
  }
  else
  {
    WIReflectedTypeAccessor& accessor = m_pObject->GetTypeAccessor();
    if (!accessor.InsertValue(m_sProperty, m_Index, m_OldValue))
    {
      return WStatus(WFmt("Remove Property: Undo failed! The index '{0}' in property '{1}' does not exist", m_Index.ConvertTo<WString>(), m_sProperty));
    }
  }
  return WStatus(W_SUCCESS);
}


////////////////////////////////////////////////////////////////////////
// WMoveObjectPropertyCommand
////////////////////////////////////////////////////////////////////////

WMoveObjectPropertyCommand::WMoveObjectPropertyCommand()
{
  m_pObject = nullptr;
}

WStatus WMoveObjectPropertyCommand::DoInternal(bool bRedo)
{
  WDocument* pDocument = GetDocument();

  if (!bRedo)
  {
    m_pObject = pDocument->GetObjectManager()->GetObject(m_Object);
    if (m_pObject == nullptr)
      return WStatus("Move Property: The given object does not exist.");
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, m_OldIndex, m_NewIndex);
}

WStatus WMoveObjectPropertyCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");

  WVariant FinalOldPosition = m_OldIndex;
  WVariant FinalNewPosition = m_NewIndex;

  if (m_OldIndex.CanConvertTo<WInt32>())
  {
    // If we are moving an object downwards, we must move by more than 1 (+1 would be behind the same object, which is still the same
    // position) so an object must always be moved by at least +2 moving UP can be done by -1, so when we undo that, we must ensure to move
    // +2

    WInt32 iNew = m_NewIndex.ConvertTo<WInt32>();
    WInt32 iOld = m_OldIndex.ConvertTo<WInt32>();

    if (iNew < iOld)
    {
      FinalOldPosition = iOld + 1;
    }

    // The new position is relative to the original array, so we need to substract one to account for
    // the removal of the same element at the lower index.
    if (iNew > iOld)
    {
      FinalNewPosition = iNew - 1;
    }
  }

  return GetDocument()->GetObjectManager()->MoveValue(m_pObject, m_sProperty, FinalNewPosition, FinalOldPosition);
}
