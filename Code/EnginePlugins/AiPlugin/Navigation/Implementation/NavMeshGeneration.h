#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <Foundation/Threading/TaskSystem.h>

class WNavmeshGeoWorldModuleInterface;

class WNavMeshSectorGenerationTask : public WTask
{
public:
  WAiNavMesh::SectorID m_SectorID = WInvalidIndex;
  WAiNavMesh* m_pWorldNavMesh = nullptr;
  const WNavmeshGeoWorldModuleInterface* m_pNavGeo = nullptr;

protected:
  virtual void Execute() override;
};
