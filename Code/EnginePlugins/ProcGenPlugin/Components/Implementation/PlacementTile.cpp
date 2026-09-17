#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/World/World.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <ProcGenPlugin/Components/Implementation/PlacementTile.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>

using namespace WProcGenInternal;

PlacementTile::PlacementTile()
  : m_pOutput(nullptr)

{
}

PlacementTile::PlacementTile(PlacementTile&& other)
{
  m_Desc = other.m_Desc;
  m_pOutput = other.m_pOutput;

  m_State = other.m_State;
  other.m_State = State::Invalid;

  m_PlacedObjects = std::move(other.m_PlacedObjects);
}

PlacementTile::~PlacementTile()
{
  W_ASSERT_DEV(m_State == State::Invalid, "Implementation error");
}

void PlacementTile::Initialize(const PlacementTileDesc& desc, WSharedPtr<const PlacementOutput>& ref_pOutput)
{
  m_Desc = desc;
  m_pOutput = ref_pOutput;

  m_State = State::Initialized;
}

void PlacementTile::Deinitialize(WWorld& ref_world)
{
  for (auto hObject : m_PlacedObjects)
  {
    ref_world.DeleteObjectDelayed(hObject);
  }
  m_PlacedObjects.Clear();

  m_Desc.m_hComponent.Invalidate();
  m_pOutput = nullptr;
  m_State = State::Invalid;
}

bool PlacementTile::IsValid() const
{
  return !m_Desc.m_hComponent.IsInvalidated() && m_pOutput != nullptr;
}

const PlacementTileDesc& PlacementTile::GetDesc() const
{
  return m_Desc;
}

const PlacementOutput* PlacementTile::GetOutput() const
{
  return m_pOutput;
}

WArrayPtr<const WGameObjectHandle> PlacementTile::GetPlacedObjects() const
{
  return m_PlacedObjects;
}

WBoundingBox PlacementTile::GetBoundingBox() const
{
  return m_Desc.GetBoundingBox();
}

WColor PlacementTile::GetDebugColor() const
{
  switch (m_State)
  {
    case State::Initialized:
      return WColor::Orange;
    case State::Scheduled:
      return WColor::Yellow;
    case State::Finished:
      return WColor::Green;
    default:
      return WColor::DarkRed;
  }
}

void PlacementTile::PreparePlacementData(const WWorld* pWorld, const WPhysicsWorldModuleInterface* pPhysicsModule, bool bDebugVisualization, PlacementData& ref_placementData)
{
  const WUInt64 uiOutputNameHash = m_pOutput->m_sName.GetHash();
  WUInt32 hashData[] = {
    static_cast<WUInt32>(m_Desc.m_iPosX),
    static_cast<WUInt32>(m_Desc.m_iPosY),
    static_cast<WUInt32>(uiOutputNameHash),
    static_cast<WUInt32>(uiOutputNameHash >> 32),
  };

  ref_placementData.m_pPhysicsModule = pPhysicsModule;
  ref_placementData.m_pWorld = pWorld;
  ref_placementData.m_pOutput = m_pOutput;
  ref_placementData.m_uiTileSeed = WHashingUtils::xxHash32(hashData, sizeof(hashData));
  ref_placementData.m_TileBoundingBox = GetBoundingBox();
  ref_placementData.m_bDebugVisualization = bDebugVisualization;
  ref_placementData.m_GlobalToLocalBoxTransforms = m_Desc.m_GlobalToLocalBoxTransforms;

  m_State = State::Scheduled;
}

WUInt32 PlacementTile::PlaceObjects(WWorld& ref_world, WArrayPtr<const PlacementTransform> objectTransforms)
{
  W_PROFILE_SCOPE("PlacementTile::PlaceObjects");

  WGameObjectDesc desc;
  auto& objectsToPlace = m_pOutput->m_ObjectsToPlace;

  WTempHybridArray<WPrefabResource*, 4> prefabs;
  prefabs.SetCount(objectsToPlace.GetCount());



  for (auto& objectTransform : objectTransforms)
  {
    const WUInt32 uiObjectIndex = objectTransform.m_uiObjectIndex;
    WPrefabResource* pPrefab = prefabs[uiObjectIndex];

    if (pPrefab == nullptr)
    {
      pPrefab = WResourceManager::BeginAcquireResource(objectsToPlace[uiObjectIndex], WResourceAcquireMode::BlockTillLoaded);
      prefabs[uiObjectIndex] = pPrefab;
    }

    WTransform transform = WSimdConversion::ToTransform(objectTransform.m_Transform);
    WTempHybridArray<WGameObject*, 8> rootObjects;

    WPrefabInstantiationOptions options;
    options.m_pCreatedRootObjectsOut = &rootObjects;

    pPrefab->InstantiatePrefab(ref_world, transform, options);

    // only send the color message, if we actually have a custom color
    if (objectTransform.m_bHasValidColor)
    {
      for (auto pRootObject : rootObjects)
      {
        // Set the color
        WMsgSetColor msg;
        msg.m_Color = objectTransform.m_ObjectColor.ToLinearFloat();
        pRootObject->PostMessageRecursive(msg, WTime::MakeZero(), WObjectMsgQueueType::AfterInitialized);
      }
    }

    for (auto pRootObject : rootObjects)
    {
      m_PlacedObjects.PushBack(pRootObject->GetHandle());
    }
  }

  for (auto pPrefab : prefabs)
  {
    if (pPrefab != nullptr)
    {
      WResourceManager::EndAcquireResource(pPrefab);
    }
  }

  m_State = State::Finished;

  return m_PlacedObjects.GetCount();
}
