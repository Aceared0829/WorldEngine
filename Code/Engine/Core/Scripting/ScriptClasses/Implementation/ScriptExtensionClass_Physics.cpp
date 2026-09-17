#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Physics.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_Physics, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(GetGravity, In, "World"),
    W_SCRIPT_FUNCTION_PROPERTY(GetCollisionLayerByName, In, "World", In, "Name"),
    W_SCRIPT_FUNCTION_PROPERTY(GetWeightCategoryByName, In, "World", In, "Name"),
    W_SCRIPT_FUNCTION_PROPERTY(GetImpulseTypeByName, In, "World", In, "Name"),

    W_SCRIPT_FUNCTION_PROPERTY(Raycast, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Start", In, "Direction", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes(
      new WFunctionArgumentAttributes(6, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(7, new WDefaultValueAttribute((WInt32)WPhysicsShapeType::Static | (WInt32)WPhysicsShapeType::Dynamic)),
      new WFunctionArgumentAttributes(8, new WDefaultValueAttribute((WInt32)WInvalidIndex))),

    W_SCRIPT_FUNCTION_PROPERTY(OverlapTestLine, In, "World", In, "Start", In, "End", In, "CollisionLayer", In, "ShapeTypes", In, "IgnoreObjectID")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute((WInt32)WPhysicsShapeType::Static | (WInt32)WPhysicsShapeType::Dynamic)),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute((WInt32)WInvalidIndex))),

    W_SCRIPT_FUNCTION_PROPERTY(OverlapTestSphere, In, "World", In, "Radius", In, "Position", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute((WInt32)WPhysicsShapeType::Static | (WInt32)WPhysicsShapeType::Dynamic))),

    W_SCRIPT_FUNCTION_PROPERTY(OverlapTestCapsule, In, "World", In, "Radius", In, "Height", In, "Transform", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new WFunctionArgumentAttributes(4, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute((WInt32)WPhysicsShapeType::Static | (WInt32)WPhysicsShapeType::Dynamic))),

    W_SCRIPT_FUNCTION_PROPERTY(SweepTestSphere, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new WFunctionArgumentAttributes(8, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(9, new WDefaultValueAttribute((WInt32)WPhysicsShapeType::Static | (WInt32)WPhysicsShapeType::Dynamic))),

    W_SCRIPT_FUNCTION_PROPERTY(SweepTestCapsule, Out, "HitPosition", Out, "HitNormal", Out, "HitObject", In, "World", In, "Radius", In, "Height", In, "Start", In, "Direction", In, "Distance", In, "CollisionLayer", In, "ShapeTypes")->AddAttributes(
      new WFunctionArgumentAttributes(9, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(10, new WDefaultValueAttribute((WInt32)WPhysicsShapeType::Static | (WInt32)WPhysicsShapeType::Dynamic))),

    W_SCRIPT_FUNCTION_PROPERTY(RaycastSurfaceInteraction, In, "World", In, "RayStart", In, "RayDirection", In, "CollisionLayer", In, "ShapeTypes", In, "FallbackSurface", In, "Interaction", In, "Impulse", In, "IgnoreObjectID")->AddAttributes(
      new WFunctionArgumentAttributes(3, new WDynamicEnumAttribute("PhysicsCollisionLayer")),
      new WFunctionArgumentAttributes(7, new WDefaultValueAttribute(0.0f)),
      new WFunctionArgumentAttributes(8, new WDefaultValueAttribute((WInt32)WInvalidIndex))),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("Physics"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

WVec3 WScriptExtensionClass_Physics::GetGravity(WWorld* pWorld)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    return pModule->GetGravity();
  }

  return WVec3::MakeZero();
}

WUInt8 WScriptExtensionClass_Physics::GetCollisionLayerByName(WWorld* pWorld, WStringView sLayerName)
{
  if (WPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<WPhysicsWorldModuleInterface>())
  {
    return static_cast<WUInt8>(pInterface->GetCollisionLayerByName(sLayerName));
  }

  return 0;
}

WUInt8 WScriptExtensionClass_Physics::GetWeightCategoryByName(WWorld* pWorld, WStringView sCategoryName)
{
  if (WPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<WPhysicsWorldModuleInterface>())
  {
    return static_cast<WUInt8>(pInterface->GetWeightCategoryByName(sCategoryName));
  }

  return 255;
}

WUInt8 WScriptExtensionClass_Physics::GetImpulseTypeByName(WWorld* pWorld, WStringView sImpulseTypeName)
{
  if (WPhysicsWorldModuleInterface* pInterface = pWorld->GetModule<WPhysicsWorldModuleInterface>())
  {
    return static_cast<WUInt8>(pInterface->GetImpulseTypeByName(sImpulseTypeName));
  }

  return 255;
}

bool WScriptExtensionClass_Physics::Raycast(WVec3& out_vHitPosition, WVec3& out_vHitNormal, WGameObjectHandle& out_hHitObject, WWorld* pWorld, const WVec3& vStart, const WVec3& vDirection, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes /*= WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic*/, WUInt32 uiIgnoreObjectID)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap = true;

    if (pModule->Raycast(res, vStart, vDirection, 1.0f, params))
    {
      // res.m_hSurface
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal = res.m_vNormal;
      out_hHitObject = res.m_hActorObject;
      return true;
    }
  }

  return false;
}

bool WScriptExtensionClass_Physics::OverlapTestLine(WWorld* pWorld, const WVec3& vStart, const WVec3& vEnd, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes /*= WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic*/, WUInt32 uiIgnoreObjectID /*= WInvalidIndex*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap = true;

    WVec3 vDirection = vEnd - vStart;
    const float fDistance = vDirection.GetLengthAndNormalize();

    if (pModule->Raycast(res, vStart, vDirection, fDistance, params))
    {
      return true;
    }
  }

  return false;
}

bool WScriptExtensionClass_Physics::OverlapTestSphere(WWorld* pWorld, float fRadius, const WVec3& vPosition, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes /*= WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    return pModule->OverlapTestSphere(fRadius, vPosition, params);
  }

  return false;
}

bool WScriptExtensionClass_Physics::OverlapTestCapsule(WWorld* pWorld, float fRadius, float fHeight, const WTransform& transform, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes /*= WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    return pModule->OverlapTestCapsule(fRadius, fHeight, transform, params);
  }
  return false;
}

bool WScriptExtensionClass_Physics::SweepTestSphere(WVec3& out_vHitPosition, WVec3& out_vHitNormal, WGameObjectHandle& out_hHitObject, WWorld* pWorld, float fRadius, const WVec3& vStart, const WVec3& vDirection, float fDistance, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes /*= WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    if (pModule->SweepTestSphere(res, fRadius, vStart, vDirection, fDistance, params))
    {
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal = res.m_vNormal;
      out_hHitObject = res.m_hActorObject;
      return true;
    }
  }
  return false;
}

bool WScriptExtensionClass_Physics::SweepTestCapsule(WVec3& out_vHitPosition, WVec3& out_vHitNormal, WGameObjectHandle& out_hHitObject, WWorld* pWorld, float fRadius, float fHeight, const WTransform& start, const WVec3& vDirection, float fDistance, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes /*= WPhysicsShapeType::Static | WPhysicsShapeType::Dynamic*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;

    if (pModule->SweepTestCapsule(res, fRadius, fHeight, start, vDirection, fDistance, params))
    {
      out_vHitPosition = res.m_vPosition;
      out_vHitNormal = res.m_vNormal;
      out_hHitObject = res.m_hActorObject;
      return true;
    }
  }
  return false;
}

bool WScriptExtensionClass_Physics::RaycastSurfaceInteraction(WWorld* pWorld, const WVec3& vRayStart, const WVec3& vRayDirection, WUInt8 uiCollisionLayer, WBitflags<WPhysicsShapeType> shapeTypes, WStringView sFallbackSurface, const WTempHashedString& sInteraction, float fInteractionImpulse, WUInt32 uiIgnoreObjectID /*= WInvalidIndex*/)
{
  if (auto pModule = pWorld->GetModuleReadOnly<WPhysicsWorldModuleInterface>())
  {
    WPhysicsCastResult res;
    WPhysicsQueryParameters params;
    params.m_ShapeTypes = shapeTypes;
    params.m_uiCollisionLayer = uiCollisionLayer;
    params.m_uiIgnoreObjectFilterID = uiIgnoreObjectID;
    params.m_bIgnoreInitialOverlap = true;

    if (pModule->Raycast(res, vRayStart, vRayDirection, 1.0f, params))
    {
      WSurfaceResourceHandle hSurface = res.m_hSurface;
      if (!hSurface.IsValid() && !sFallbackSurface.IsEmpty())
      {
        hSurface = WResourceManager::LoadResource<WSurfaceResource>(sFallbackSurface);
      }

      if (hSurface.IsValid())
      {
        WResourceLock<WSurfaceResource> pSurf(hSurface, WResourceAcquireMode::BlockTillLoaded_NeverFail);
        if (pSurf.GetAcquireResult() == WResourceAcquireResult::Final)
        {
          return pSurf->InteractWithSurface(pWorld, {}, res.m_vPosition, res.m_vNormal, vRayDirection, sInteraction, nullptr, fInteractionImpulse);
        }
      }
    }
  }

  return false;
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Physics);
