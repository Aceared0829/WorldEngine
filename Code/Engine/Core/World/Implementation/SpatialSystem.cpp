#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/GameObject.h>
#include <Core/World/SpatialSystem.h>
#include <Core/World/World.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSpatialSystem, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSpatialSystem::WSpatialSystem()
  : m_Allocator("Spatial System", WFoundation::GetDefaultAllocator())
{
}

WSpatialSystem::~WSpatialSystem() = default;

void WSpatialSystem::StartNewFrame()
{
  ++m_uiFrameCounter;
}

void WSpatialSystem::FindObjectsInSphere(const WBoundingSphere& sphere, const QueryParams& queryParams, WDynamicArray<WGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](WGameObject* pObject)
    {
      out_objects.PushBack(pObject);

      return WVisitorExecution::Continue;
    });
}

void WSpatialSystem::FindObjectsInBox(const WBoundingBox& box, const QueryParams& queryParams, WDynamicArray<WGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](WGameObject* pObject)
    {
      out_objects.PushBack(pObject);

      return WVisitorExecution::Continue;
    });
}

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
void WSpatialSystem::GetInternalStats(WStringBuilder& ref_sSb) const
{
  ref_sSb.Clear();
}
#endif

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_Spatial, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(FindClosestObjectInSphere, In, "World", In, "Category", In, "Center", In, "Radius"),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("Spatial"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on


WGameObject* WScriptExtensionClass_Spatial::FindClosestObjectInSphere(WWorld* pWorld, WStringView sCategory, const WVec3& vCenter, float fRadius)
{
  WGameObject* pClosest = nullptr;

  auto category = WSpatialData::FindCategory(sCategory);
  if (category != WInvalidSpatialDataCategory)
  {
    WSpatialSystem::QueryParams params;
    params.m_uiCategoryBitmask = category.GetBitmask();

    float fDistanceSqr = WMath::HighValue<float>();

    pWorld->GetSpatialSystem()->FindObjectsInSphere(WBoundingSphere::MakeFromCenterAndRadius(vCenter, fRadius), params, [&](WGameObject* go) -> WVisitorExecution::Enum
      {
        const float fSqr = go->GetGlobalPosition().GetSquaredDistanceTo(vCenter);

        if (fSqr < fDistanceSqr)
        {
          fDistanceSqr = fSqr;
          pClosest = go;
        }

        return WVisitorExecution::Continue;
        //
      });
  }

  return pClosest;
}

W_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
