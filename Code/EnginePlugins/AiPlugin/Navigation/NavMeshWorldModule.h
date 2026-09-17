#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/Implementation/NavMeshGeneration.h>
#include <Core/World/WorldModule.h>

class WAiNavMesh;
class dtNavMesh;

/// This world module keeps track of all the configured navmeshes (for different character types)
/// and makes sure to build their sectors in the background.
///
/// Through this you can get access to one of the available navmeshes.
/// Additionally, it also provides access to the different path search filters.
class W_AIPLUGIN_DLL WAiNavMeshWorldModule final : public WWorldModule
{
  W_DECLARE_WORLD_MODULE();
  W_ADD_DYNAMIC_REFLECTION(WAiNavMeshWorldModule, WWorldModule);

public:
  WAiNavMeshWorldModule(WWorld* pWorld);
  ~WAiNavMeshWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  WAiNavMesh* GetNavMesh(WStringView sName);
  const WAiNavMesh* GetNavMesh(WStringView sName) const;

  const dtQueryFilter& GetPathSearchFilter(WStringView sName) const;

  const WAiNavigationConfig& GetConfig() const { return m_Config; }

private:
  void Update(const UpdateContext& ctxt);

  WMap<WString, WAiNavMesh*> m_WorldNavMeshes;

  // TODO: this is a hacky solution to delay the navmesh generation until after Physics has been set up.
  WUInt32 m_uiUpdateDelay = 10;
  WTaskGroupID m_GenerateSectorTaskID;
  WSharedPtr<WNavMeshSectorGenerationTask> m_pGenerateSectorTask;

  WAiNavigationConfig m_Config;

  WMap<WString, dtQueryFilter> m_PathSearchFilters;
};

/* TODO:

Navmesh Generation
==================

* fix navmesh on hills
* collision group filtering
* invalidate sectors, re-generate
* Invalidate path searches after sector changes
* sector usage tracking
* unload unused sectors

Path Search
===========

* callback for touched sectors
* on-demand sector generation ???
* use max edge-length + poly flags for 'dynamic' obstacles

Steering
========

* movement with ineratia
* decoupled position and rotation
* avoid dynamic obstacles

*/
