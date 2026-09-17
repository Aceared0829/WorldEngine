#include <Core/CorePCH.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>

void WWorldWriter::Clear()
{
  m_AllRootObjects.Clear();
  m_AllChildObjects.Clear();
  m_AllComponents.Clear();

  m_pStream = nullptr;
  m_pExclude = nullptr;

  // invalid handles
  {
    m_WrittenGameObjectHandles.Clear();
    m_WrittenGameObjectHandles[WGameObjectHandle()] = 0;
  }
}

void WWorldWriter::WriteWorld(WStreamWriter& inout_stream, WWorld& ref_world, const WTagSet* pExclude)
{
  Clear();

  m_pStream = &inout_stream;
  m_pExclude = pExclude;

  W_LOCK(ref_world.GetReadMarker());

  ref_world.Traverse(WMakeDelegate(&WWorldWriter::ObjectTraverser, this), WWorld::TraversalMethod::DepthFirst);

  WriteToStream().IgnoreResult();
}

void WWorldWriter::WriteObjects(WStreamWriter& inout_stream, const WDeque<const WGameObject*>& rootObjects)
{
  Clear();

  m_pStream = &inout_stream;

  for (const WGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<WGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

void WWorldWriter::WriteObjects(WStreamWriter& inout_stream, WArrayPtr<const WGameObject*> rootObjects)
{
  Clear();

  m_pStream = &inout_stream;

  for (const WGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<WGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

WResult WWorldWriter::WriteToStream()
{
  const WUInt8 uiVersion = 10;
  *m_pStream << uiVersion;

  // version 8: use string dedup instead of handle writer
  WStringDeduplicationWriteContext stringDedupWriteContext(*m_pStream);
  m_pStream = &stringDedupWriteContext.Begin();

  IncludeAllComponentBaseTypes();

  WUInt32 uiNumRootObjects = m_AllRootObjects.GetCount();
  WUInt32 uiNumChildObjects = m_AllChildObjects.GetCount();
  WUInt32 uiNumComponentTypes = m_AllComponents.GetCount();

  *m_pStream << uiNumRootObjects;
  *m_pStream << uiNumChildObjects;
  *m_pStream << uiNumComponentTypes;

  // this is used to sort all component types by name, to make the file serialization deterministic
  WMap<WString, const WRTTI*> sortedTypes;

  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    sortedTypes[it.Key()->GetTypeName()] = it.Key();
  }

  AssignGameObjectIndices();
  AssignComponentHandleIndices(sortedTypes);

  for (const auto* pObject : m_AllRootObjects)
  {
    WriteGameObject(pObject);
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    WriteGameObject(pObject);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentTypeInfo(it.Value());
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentCreationData(m_AllComponents[it.Value()].m_Components);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentSerializationData(m_AllComponents[it.Value()].m_Components);
  }

  W_SUCCEED_OR_RETURN(stringDedupWriteContext.End());
  m_pStream = &stringDedupWriteContext.GetOriginalStream();

  return W_SUCCESS;
}


void WWorldWriter::AssignGameObjectIndices()
{
  WUInt32 uiGameObjectIndex = 1;
  for (const auto* pObject : m_AllRootObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }
}

void WWorldWriter::AssignComponentHandleIndices(const WMap<WString, const WRTTI*>& sortedTypes)
{
  WUInt16 uiTypeIndex = 0;

  W_ASSERT_DEV(m_AllComponents.GetCount() <= WMath::MaxValue<WUInt16>(), "Too many types for world writer");

  // assign the component handle indices in the order in which the components are written
  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    auto& components = m_AllComponents[it.Value()];

    components.m_uiSerializedTypeIndex = uiTypeIndex;
    ++uiTypeIndex;

    WUInt32 uiComponentIndex = 1;
    components.m_HandleToIndex[WComponentHandle()] = 0;

    for (const WComponent* pComp : components.m_Components)
    {
      components.m_HandleToIndex[pComp->GetHandle()] = uiComponentIndex;
      ++uiComponentIndex;
    }
  }
}


void WWorldWriter::IncludeAllComponentBaseTypes()
{
  WDynamicArray<const WRTTI*> allNow;
  allNow.Reserve(m_AllComponents.GetCount());
  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    allNow.PushBack(it.Key());
  }

  for (auto pRtti : allNow)
  {
    IncludeAllComponentBaseTypes(pRtti->GetParentType());
  }
}


void WWorldWriter::IncludeAllComponentBaseTypes(const WRTTI* pRtti)
{
  if (pRtti == nullptr || !pRtti->IsDerivedFrom<WComponent>() || m_AllComponents.Contains(pRtti))
    return;

  // this is actually used to insert the type, but we have no component of this type
  m_AllComponents[pRtti];

  IncludeAllComponentBaseTypes(pRtti->GetParentType());
}


void WWorldWriter::Traverse(WGameObject* pObject)
{
  if (ObjectTraverser(pObject) == WVisitorExecution::Continue)
  {
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      Traverse(&(*it));
    }
  }
}

void WWorldWriter::WriteGameObjectHandle(const WGameObjectHandle& hObject)
{
  auto it = m_WrittenGameObjectHandles.Find(hObject);

  WUInt32 uiIndex = 0;

  W_ASSERT_DEV(it.IsValid(), "Referenced object does not exist in the scene. This can happen, if it was optimized away, because it had no name, no children and no essential components.");

  if (it.IsValid())
    uiIndex = it.Value();

  *m_pStream << uiIndex;
}

void WWorldWriter::WriteComponentHandle(const WComponentHandle& hComponent)
{
  WUInt16 uiTypeIndex = 0;
  WUInt32 uiIndex = 0;

  WComponent* pComponent = nullptr;
  if (WWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent))
  {
    if (auto* components = m_AllComponents.GetValue(pComponent->GetDynamicRTTI()))
    {
      auto it = components->m_HandleToIndex.Find(hComponent);
      W_ASSERT_DEBUG(it.IsValid(), "Handle should always be in the written map at this point");

      if (it.IsValid())
      {
        uiTypeIndex = components->m_uiSerializedTypeIndex;
        uiIndex = it.Value();
      }
    }
  }

  *m_pStream << uiTypeIndex;
  *m_pStream << uiIndex;
}

WVisitorExecution::Enum WWorldWriter::ObjectTraverser(WGameObject* pObject)
{
  if (m_pExclude && pObject->GetTags().IsAnySet(*m_pExclude))
    return WVisitorExecution::Skip;
  if (pObject->WasCreatedByPrefab())
    return WVisitorExecution::Skip;

  if (pObject->GetParent())
    m_AllChildObjects.PushBack(pObject);
  else
    m_AllRootObjects.PushBack(pObject);

  auto components = pObject->GetComponents();

  for (const WComponent* pComp : components)
  {
    if (pComp->WasCreatedByPrefab())
      continue;

    m_AllComponents[pComp->GetDynamicRTTI()].m_Components.PushBack(pComp);
  }

  return WVisitorExecution::Continue;
}

void WWorldWriter::WriteGameObject(const WGameObject* pObject)
{
  if (pObject->GetParent())
    WriteGameObjectHandle(pObject->GetParent()->GetHandle());
  else
    WriteGameObjectHandle(WGameObjectHandle());

  WStreamWriter& s = *m_pStream;

  s << pObject->GetName();
  s << pObject->GetGlobalKey();
  s << pObject->GetLocalPosition();
  s << pObject->GetLocalRotation();
  s << pObject->GetLocalScaling();
  s << pObject->GetLocalUniformScaling();
  s << pObject->GetActiveFlag();
  s << pObject->IsDynamic();
  pObject->GetTags().Save(s);
  s << pObject->GetTeamID();
  s << pObject->GetStableRandomSeed();
}

void WWorldWriter::WriteComponentTypeInfo(const WRTTI* pRtti)
{
  WStreamWriter& s = *m_pStream;

  s << pRtti->GetTypeName();
  s << pRtti->GetTypeVersion();
}

void WWorldWriter::WriteComponentCreationData(const WDeque<const WComponent*>& components)
{
  WDefaultMemoryStreamStorage storage;
  WMemoryStreamWriter memWriter(&storage);

  WStreamWriter* pPrevStream = m_pStream;
  m_pStream = &memWriter;

  // write to memory stream
  {
    WStreamWriter& s = *m_pStream;
    s << components.GetCount();

    WUInt32 uiComponentIndex = 1;
    for (auto pComponent : components)
    {
      WriteGameObjectHandle(pComponent->GetOwner()->GetHandle());
      s << uiComponentIndex;
      ++uiComponentIndex;

      s << pComponent->GetActiveFlag();

      // version 7
      {
        WUInt8 userFlags = 0;
        for (WUInt8 i = 0; i < 8; ++i)
        {
          userFlags |= pComponent->GetUserFlag(i) ? W_BIT(i) : 0;
        }

        s << userFlags;
      }
    }
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    WStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    W_ASSERT_ALWAYS(storage.GetStorageSize64() <= WMath::MaxValue<WUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}

void WWorldWriter::WriteComponentSerializationData(const WDeque<const WComponent*>& components)
{
  WDefaultMemoryStreamStorage storage;
  WMemoryStreamWriter memWriter(&storage);

  WStreamWriter* pPrevStream = m_pStream;
  m_pStream = &memWriter;

  // write to memory stream
  for (auto pComp : components)
  {
    pComp->SerializeComponent(*this);
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    WStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    W_ASSERT_ALWAYS(storage.GetStorageSize64() <= WMath::MaxValue<WUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}
