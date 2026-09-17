#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WObjectChange, WNoBase, 1, WRTTIDefaultAllocator<WObjectChange>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Change", m_Change),
    W_MEMBER_PROPERTY("Root", m_Root),
    W_ARRAY_MEMBER_PROPERTY("Steps", m_Steps),
    W_MEMBER_PROPERTY("Graph", m_GraphData),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WObjectChange::WObjectChange(const WObjectChange&)
{
  W_REPORT_FAILURE("Not supported!");
}

void WObjectChange::GetGraph(WAbstractObjectGraph& ref_graph) const
{
  ref_graph.Clear();

  WRawMemoryStreamReader reader(m_GraphData);
  WAbstractGraphBinarySerializer::Read(reader, &ref_graph);
}

void WObjectChange::SetGraph(WAbstractObjectGraph& ref_graph)
{
  WContiguousMemoryStreamStorage storage;
  WMemoryStreamWriter writer(&storage);
  WAbstractGraphBinarySerializer::Write(writer, &ref_graph);

  m_GraphData = {storage.GetData(), storage.GetStorageSize32()};
}

WObjectChange::WObjectChange(WObjectChange&& rhs)
{
  m_Change = std::move(rhs.m_Change);
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void WObjectChange::operator=(WObjectChange&& rhs)
{
  m_Change = std::move(rhs.m_Change);
  m_Root = rhs.m_Root;
  m_Steps = std::move(rhs.m_Steps);
  m_GraphData = std::move(rhs.m_GraphData);
}

void WObjectChange::operator=(WObjectChange& rhs)
{
  W_REPORT_FAILURE("Not supported!");
}


WDocumentObjectMirror::WDocumentObjectMirror()
{
  m_pContext = nullptr;
  m_pManager = nullptr;
}

WDocumentObjectMirror::~WDocumentObjectMirror()
{
  W_ASSERT_DEV(m_pManager == nullptr && m_pContext == nullptr, "Need to call DeInit before d-tor!");
}

void WDocumentObjectMirror::InitSender(const WDocumentObjectManager* pManager)
{
  m_pManager = pManager;
  m_pManager->m_StructureEvents.AddEventHandler(WMakeDelegate(&WDocumentObjectMirror::TreeStructureEventHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WDocumentObjectMirror::TreePropertyEventHandler, this));
}

void WDocumentObjectMirror::InitReceiver(WRttiConverterContext* pContext)
{
  m_pContext = pContext;
}

void WDocumentObjectMirror::DeInit()
{
  if (m_pManager)
  {
    m_pManager->m_StructureEvents.RemoveEventHandler(WMakeDelegate(&WDocumentObjectMirror::TreeStructureEventHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(WMakeDelegate(&WDocumentObjectMirror::TreePropertyEventHandler, this));
    m_pManager = nullptr;
  }

  if (m_pContext)
  {
    m_pContext = nullptr;
  }
}

void WDocumentObjectMirror::SetFilterFunction(FilterFunction filter)
{
  m_Filter = filter;
}

void WDocumentObjectMirror::SendDocument()
{
  const auto* pRoot = m_pManager->GetRootObject();
  for (auto* pChild : pRoot->GetChildren())
  {
    if (IsDiscardedByFilter(pRoot, pChild->GetParentProperty()))
      continue;

    WObjectChange change;
    change.m_Change.m_Operation = WObjectChangeType::NodeAdded;
    change.m_Change.m_Value = pChild->GetGuid();

    WAbstractObjectGraph graph;
    WDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
    objectConverter.AddObjectToGraph(pChild, "Object");
    change.SetGraph(graph);

    ApplyOp(change);
  }
}

void WDocumentObjectMirror::Clear()
{
  if (m_pManager)
  {
    const auto* pRoot = m_pManager->GetRootObject();
    for (auto* pChild : pRoot->GetChildren())
    {
      WObjectChange change;
      change.m_Change.m_Operation = WObjectChangeType::NodeRemoved;
      change.m_Change.m_Value = pChild->GetGuid();

      /*WAbstractObjectGraph graph;
      WDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      WAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pChild, "Object");
      change.SetGraph(graph);*/

      ApplyOp(change);
    }
  }

  if (m_pContext)
  {
    m_pContext->Clear();
  }
}

void WDocumentObjectMirror::TreeStructureEventHandler(const WDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent && IsDiscardedByFilter(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent && IsDiscardedByFilter(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      if (IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty))
      {
        if (e.m_pNewParent == nullptr || e.m_pNewParent == m_pManager->GetRootObject())
        {
          // Object is now a root object, nothing to do to attach it to its new parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == WPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }
        WObjectChange change;
        CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

        change.m_Change.m_Operation = WObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = e.getInsertIndex();
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      // Intended falltrough as non ptr object might as well be destroyed and rebuild.
    }
      // case WDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case WDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      WObjectChange change;
      CreatePath(change, e.m_pNewParent, e.m_sParentProperty);

      change.m_Change.m_Operation = WObjectChangeType::NodeAdded;
      change.m_Change.m_Index = e.getInsertIndex();
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      WAbstractObjectGraph graph;
      WDocumentObjectConverterWriter objectConverter(&graph, m_pManager);
      objectConverter.AddObjectToGraph(e.m_pObject, "Object");
      change.SetGraph(graph);

      ApplyOp(change);
    }
    break;
    case WDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      if (IsHeapAllocated(e.m_pPreviousParent, e.m_sParentProperty))
      {
        W_ASSERT_DEBUG(IsHeapAllocated(e.m_pNewParent, e.m_sParentProperty), "Old and new parent must have the same heap allocation state!");
        if (e.m_pPreviousParent == nullptr || e.m_pPreviousParent == m_pManager->GetRootObject())
        {
          // Object is currently a root object, nothing to do to detach it from its parent.
          break;
        }

        if (e.GetProperty()->GetCategory() == WPropertyCategory::Set && e.m_pPreviousParent == e.m_pNewParent)
        {
          // Sets only have ordering in the editor. We can ignore set order changes in the mirror.
          break;
        }

        WObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        // Do not delete heap object, just remove it from its owner.
        change.m_Change.m_Operation = WObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_OldPropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
      else
      {
        WObjectChange change;
        CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

        change.m_Change.m_Operation = WObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = e.m_OldPropertyIndex;
        change.m_Change.m_Value = e.m_pObject->GetGuid();

        ApplyOp(change);
        break;
      }
    }
      // case WDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case WDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      WObjectChange change;
      CreatePath(change, e.m_pPreviousParent, e.m_sParentProperty);

      change.m_Change.m_Operation = WObjectChangeType::NodeRemoved;
      change.m_Change.m_Index = e.m_OldPropertyIndex;
      change.m_Change.m_Value = e.m_pObject->GetGuid();

      ApplyOp(change);
    }
    break;

    default:
      break;
  }
}

void WDocumentObjectMirror::TreePropertyEventHandler(const WDocumentObjectPropertyEvent& e)
{
  if (IsDiscardedByFilter(e.m_pObject, e.m_sProperty))
    return;

  switch (e.m_EventType)
  {
    case WDocumentObjectPropertyEvent::Type::PropertySet:
    {
      WObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = WObjectChangeType::PropertySet;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case WDocumentObjectPropertyEvent::Type::PropertyInserted:
    {
      WObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = WObjectChangeType::PropertyInserted;
      change.m_Change.m_Index = e.m_NewIndex;
      change.m_Change.m_Value = e.m_NewValue;
      ApplyOp(change);
    }
    break;
    case WDocumentObjectPropertyEvent::Type::PropertyRemoved:
    {
      WObjectChange change;
      CreatePath(change, e.m_pObject, e.m_sProperty);

      change.m_Change.m_Operation = WObjectChangeType::PropertyRemoved;
      change.m_Change.m_Index = e.m_OldIndex;
      change.m_Change.m_Value = e.m_OldValue;
      ApplyOp(change);
    }
    break;
    case WDocumentObjectPropertyEvent::Type::PropertyMoved:
    {
      WUInt32 uiOldIndex = e.m_OldIndex.ConvertTo<WUInt32>();
      WUInt32 uiNewIndex = e.m_NewIndex.ConvertTo<WUInt32>();
      // NewValue can be invalid if an invalid variant in a variant array is moved
      // W_ASSERT_DEBUG(e.m_NewValue.IsValid(), "Value must be valid");

      {
        WObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = WObjectChangeType::PropertyRemoved;
        change.m_Change.m_Index = uiOldIndex;
        change.m_Change.m_Value = e.m_NewValue;
        ApplyOp(change);
      }

      if (uiNewIndex > uiOldIndex)
      {
        uiNewIndex -= 1;
      }

      {
        WObjectChange change;
        CreatePath(change, e.m_pObject, e.m_sProperty);

        change.m_Change.m_Operation = WObjectChangeType::PropertyInserted;
        change.m_Change.m_Index = uiNewIndex;
        change.m_Change.m_Value = e.m_NewValue;
        ApplyOp(change);
      }

      return;
    }
    break;
  }
}

void* WDocumentObjectMirror::GetNativeObjectPointer(const WDocumentObject* pObject)
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

const void* WDocumentObjectMirror::GetNativeObjectPointer(const WDocumentObject* pObject) const
{
  auto object = m_pContext->GetObjectByGUID(pObject->GetGuid());
  return object.m_pObject;
}

bool WDocumentObjectMirror::IsRootObject(const WDocumentObject* pParent)
{
  return (pParent == nullptr || pParent == m_pManager->GetRootObject());
}

bool WDocumentObjectMirror::IsHeapAllocated(const WDocumentObject* pParent, WStringView sParentProperty)
{
  if (pParent == nullptr || pParent == m_pManager->GetRootObject())
    return true;

  const WRTTI* pRtti = pParent->GetTypeAccessor().GetType();

  auto* pProp = pRtti->FindPropertyByName(sParentProperty);
  return pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner);
}


bool WDocumentObjectMirror::IsDiscardedByFilter(const WDocumentObject* pObject, WStringView sProperty) const
{
  if (m_Filter.IsValid())
  {
    return !m_Filter(pObject, sProperty);
  }
  return false;
}

void WDocumentObjectMirror::CreatePath(WObjectChange& out_change, const WDocumentObject* pRoot, WStringView sProperty)
{
  if (pRoot && pRoot->GetDocumentObjectManager()->GetRootObject() != pRoot)
  {
    WTempHybridArray<const WDocumentObject*, 8> path;
    out_change.m_Root = FindRootOpObject(pRoot, path);
    FlattenSteps(path, out_change.m_Steps);
  }

  out_change.m_Change.m_sProperty = sProperty;
}

WUuid WDocumentObjectMirror::FindRootOpObject(const WDocumentObject* pParent, WDynamicArray<const WDocumentObject*>& out_path)
{
  out_path.PushBack(pParent);

  if (!pParent->IsOnHeap())
  {
    return FindRootOpObject(pParent->GetParent(), out_path);
  }
  else
  {
    return pParent->GetGuid();
  }
}

void WDocumentObjectMirror::FlattenSteps(const WArrayPtr<const WDocumentObject* const> path, WDynamicArray<WPropertyPathStep>& out_steps)
{
  WUInt32 uiCount = path.GetCount();
  W_ASSERT_DEV(uiCount > 0, "Path must not be empty!");
  W_ASSERT_DEV(path[uiCount - 1]->IsOnHeap(), "Root of steps must be on heap!");

  // Only root object? Then there is no path from it.
  if (uiCount == 1)
    return;

  for (WInt32 i = (WInt32)uiCount - 2; i >= 0; --i)
  {
    const WDocumentObject* pObject = path[i];
    out_steps.PushBack(WPropertyPathStep({pObject->GetParentProperty(), pObject->GetPropertyIndex()}));
  }
}

void WDocumentObjectMirror::ApplyOp(WObjectChange& change)
{
  WRttiConverterObject object;
  if (change.m_Root.IsValid())
  {
    object = m_pContext->GetObjectByGUID(change.m_Root);
    if (!object.m_pObject)
      return;
    // W_ASSERT_DEV(object.m_pObject != nullptr, "Root object does not exist in mirrored native object!");
  }

  WPropertyPath propPath;
  if (propPath.InitializeFromPath(object.m_pType, change.m_Steps).Failed())
  {
    WLog::Error("Failed to init property path on object of type '{0}'.", object.m_pType->GetTypeName());
    return;
  }

  propPath.WriteToLeafObject(object.m_pObject, object.m_pType, [this, &change](void* pLeaf, const WRTTI& type)
            { ApplyOp(WRttiConverterObject(&type, pLeaf), change); })
    .IgnoreResult();
}

void WDocumentObjectMirror::ApplyOp(WRttiConverterObject object, const WObjectChange& change)
{
  const WAbstractProperty* pProp = nullptr;

  if (object.m_pType != nullptr)
  {
    pProp = object.m_pType->FindPropertyByName(change.m_Change.m_sProperty);
    if (pProp == nullptr)
    {
      WLog::Error("Property '{0}' not found, can't apply mirror op!", change.m_Change.m_sProperty);
      return;
    }
  }

  switch (change.m_Change.m_Operation)
  {
    case WObjectChangeType::NodeAdded:
    {
      WAbstractObjectGraph graph;
      change.GetGraph(graph);
      WRttiConverterReader reader(&graph, m_pContext);
      const WAbstractObjectNode* pNode = graph.GetNodeByName("Object");
      const WRTTI* pType = m_pContext->FindTypeByName(pNode->GetType());
      void* pValue = reader.CreateObjectFromNode(pNode);
      if (!pValue)
      {
        // Can't create object, exiting.
        return;
      }

      if (!change.m_Root.IsValid())
      {
        // Create without parent (root element)
        return;
      }

      if (pProp->GetCategory() == WPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<const WAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
        }
        else
        {
          pSpecificProp->SetValuePtr(object.m_pObject, pValue);
        }
      }
      else if (pProp->GetCategory() == WPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const WAbstractArrayProperty*>(pProp);
        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<WUInt32>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.ConvertTo<WUInt32>(), pValue);
        }
      }
      else if (pProp->GetCategory() == WPropertyCategory::Set)
      {
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<const WAbstractSetProperty*>(pProp);
        WReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, WVariant(pValue, pType));
      }
      else if (pProp->GetCategory() == WPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const WAbstractMapProperty*>(pProp);
        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<WString>(), &pValue);
        }
        else
        {
          pSpecificProp->Insert(object.m_pObject, change.m_Change.m_Index.Get<WString>(), pValue);
        }
      }

      if (!pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(pNode->GetGuid());
      }
    }
    break;
    case WObjectChangeType::NodeRemoved:
    {
      if (!change.m_Root.IsValid())
      {
        // Delete root object
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<WUuid>());
        return;
      }

      if (pProp->GetCategory() == WPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<const WAbstractMemberProperty*>(pProp);
        if (!pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
        {
          WLog::Error("Property '{0}' not a pointer, can't remove object!", change.m_Change.m_sProperty);
          return;
        }

        void* pValue = nullptr;
        pSpecificProp->SetValuePtr(object.m_pObject, &pValue);
      }
      else if (pProp->GetCategory() == WPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const WAbstractArrayProperty*>(pProp);
        WReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<WUInt32>());
      }
      else if (pProp->GetCategory() == WPropertyCategory::Set)
      {
        W_ASSERT_DEV(pProp->GetFlags().IsSet(WPropertyFlags::Pointer), "Set object must always be pointers!");
        auto pSpecificProp = static_cast<const WAbstractSetProperty*>(pProp);
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<WUuid>());
        WReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, WVariant(valueObject.m_pObject, valueObject.m_pType));
      }
      else if (pProp->GetCategory() == WPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const WAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<WString>());
      }

      if (pProp->GetFlags().AreAllSet(WPropertyFlags::Pointer | WPropertyFlags::PointerOwner))
      {
        m_pContext->DeleteObject(change.m_Change.m_Value.Get<WUuid>());
      }
    }
    break;
    case WObjectChangeType::PropertySet:
    {
      if (pProp->GetCategory() == WPropertyCategory::Member)
      {
        auto pSpecificProp = static_cast<const WAbstractMemberProperty*>(pProp);
        WReflectionUtils::SetMemberPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == WPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const WAbstractArrayProperty*>(pProp);
        WReflectionUtils::SetArrayPropertyValue(
          pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<WUInt32>(), change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == WPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<const WAbstractSetProperty*>(pProp);
        WReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Value);
      }
      else if (pProp->GetCategory() == WPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const WAbstractMapProperty*>(pProp);
        WReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<WString>(), change.m_Change.m_Value);
      }
    }
    break;
    case WObjectChangeType::PropertyInserted:
    {
      WVariant value = change.m_Change.m_Value;
      if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
      {
        auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<WUuid>());
        value = WTypedPointer(valueObject.m_pObject, valueObject.m_pType);
      }

      if (pProp->GetCategory() == WPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const WAbstractArrayProperty*>(pProp);
        WReflectionUtils::InsertArrayPropertyValue(pSpecificProp, object.m_pObject, value, change.m_Change.m_Index.ConvertTo<WUInt32>());
      }
      else if (pProp->GetCategory() == WPropertyCategory::Set)
      {
        auto pSpecificProp = static_cast<const WAbstractSetProperty*>(pProp);
        WReflectionUtils::InsertSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == WPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const WAbstractMapProperty*>(pProp);
        WReflectionUtils::SetMapPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.Get<WString>(), value);
      }
    }
    break;
    case WObjectChangeType::PropertyRemoved:
    {
      if (pProp->GetCategory() == WPropertyCategory::Array)
      {
        auto pSpecificProp = static_cast<const WAbstractArrayProperty*>(pProp);
        WReflectionUtils::RemoveArrayPropertyValue(pSpecificProp, object.m_pObject, change.m_Change.m_Index.ConvertTo<WUInt32>());
      }
      else if (pProp->GetCategory() == WPropertyCategory::Set)
      {
        WVariant value = change.m_Change.m_Value;
        if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
        {
          auto valueObject = m_pContext->GetObjectByGUID(change.m_Change.m_Value.Get<WUuid>());
          value = WTypedPointer(valueObject.m_pObject, valueObject.m_pType);
        }

        auto pSpecificProp = static_cast<const WAbstractSetProperty*>(pProp);
        WReflectionUtils::RemoveSetPropertyValue(pSpecificProp, object.m_pObject, value);
      }
      else if (pProp->GetCategory() == WPropertyCategory::Map)
      {
        auto pSpecificProp = static_cast<const WAbstractMapProperty*>(pProp);
        pSpecificProp->Remove(object.m_pObject, change.m_Change.m_Index.Get<WString>());
      }
    }
    break;
  }
}
