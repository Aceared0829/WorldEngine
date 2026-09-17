#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GameEngine/Physics/ImpulseType.h>
#include <GameEngine/Physics/WeightCategory.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <memory>

class WJoltMaterial;
struct WSurfaceResourceEvent;
class WJoltDebugRenderer;
class WWorld;

namespace JPH
{
  class JobSystem;
}

class W_JOLTPLUGIN_DLL WJoltCore
{
public:
  static JPH::JobSystem* GetJoltJobSystem();
  static const WJoltMaterial* GetDefaultMaterial() { return s_pDefaultMaterial; }

  static void DebugDraw(WWorld* pWorld);

#ifdef JPH_DEBUG_RENDERER
  static std::unique_ptr<WJoltDebugRenderer> s_pDebugRenderer;
#endif

  static const WCollisionFilterConfig& GetCollisionFilterConfig();
  static const WWeightCategoryConfig& GetWeightCategoryConfig();
  static const WImpulseTypeConfig& GetImpulseTypeConfig();

  static void ReloadConfigs();

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(Jolt, JoltPlugin);

  static void Startup();
  static void Shutdown();

  static void SurfaceResourceEventHandler(const WSurfaceResourceEvent& e);

  static void* JoltMalloc(size_t inSize);
  static void JoltFree(void* inBlock);
  static void* JoltReallocate(void* inBlock, size_t inOldSize, size_t inNewSize);
  static void* JoltAlignedMalloc(size_t inSize, size_t inAlignment);
  static void JoltAlignedFree(void* inBlock);

  static void LoadCollisionFilters();
  static void LoadWeightCategories();
  static void LoadImpulseTypes();

  static WJoltMaterial* s_pDefaultMaterial;

  static WUniquePtr<JPH::JobSystem> s_pJobSystemEZ;
  static std::unique_ptr<JPH::JobSystem> s_pJobSystemJolt;

  static WUniquePtr<WProxyAllocator> s_pAllocator;
  static WUniquePtr<WProxyAllocator> s_pAllocatorAligned;

  static WUniquePtr<WCollisionFilterConfig> s_pCollisionFilterConfig;
  static WUniquePtr<WWeightCategoryConfig> s_pWeightCategoryConfig;
  static WUniquePtr<WImpulseTypeConfig> s_pImpulseTypeConfig;
};
