#pragma once

#include <Core/CoreDLL.h>

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

class WWorld;
class WUuid;

/// Event data for surface resource lifecycle notifications.
struct WSurfaceResourceEvent
{
  enum class Type
  {
    Created,
    Destroyed
  };

  Type m_Type;
  WSurfaceResource* m_pSurface = nullptr;
};

/// Resource representing a physics surface with material properties and interaction behaviors.
///
/// Defines how objects interact with a surface through collision responses, sound effects,
/// particle effects, and other configurable behaviors. Supports inheritance from base surfaces
/// and provides integration with physics engines through material pointers.
class W_CORE_DLL WSurfaceResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WSurfaceResource, WResource);
  W_RESOURCE_DECLARE_COMMON_CODE(WSurfaceResource);
  W_RESOURCE_DECLARE_CREATEABLE(WSurfaceResource, WSurfaceResourceDescriptor);

public:
  WSurfaceResource();
  ~WSurfaceResource();

  const WSurfaceResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

  static WEvent<const WSurfaceResourceEvent&, WMutex> s_Events;

  void* m_pPhysicsMaterialPhysX = nullptr;
  void* m_pPhysicsMaterialJolt = nullptr;

  /// Spawns the prefab that was defined for the given interaction at the given position and using the configured orientation.
  /// Returns false, if the interaction type was not defined in this surface or any of its base surfaces
  bool InteractWithSurface(WWorld* pWorld, WGameObjectHandle hObject, const WVec3& vPosition, const WVec3& vSurfaceNormal, const WVec3& vIncomingDirection, const WTempHashedString& sInteraction, const WUInt16* pOverrideTeamID, float fImpulseSqr = 0.0f) const;

  bool IsBasedOn(const WSurfaceResource* pThisOrBaseSurface) const;

  bool IsBasedOn(const WSurfaceResourceHandle hThisOrBaseSurface) const;

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  static const WSurfaceInteraction* FindInteraction(const WSurfaceResource* pCurSurf, WUInt64 uiHash, float fImpulseSqr, float& out_fImpulseParamValue);

  WSurfaceResourceDescriptor m_Descriptor;

  struct SurfInt
  {
    WUInt64 m_uiInteractionTypeHash = 0;
    const WSurfaceInteraction* m_pInteraction;
  };

  WDynamicArray<SurfInt> m_Interactions;
};
