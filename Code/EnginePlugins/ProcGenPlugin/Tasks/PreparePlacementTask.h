#pragma once

#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

namespace WProcGenInternal
{
  class PreparePlacementTask final : public WTask
  {
  public:
    PreparePlacementTask(PlacementData* pData, const char* szName);
    ~PreparePlacementTask();

    void Clear() {}

  private:
    friend class PlacementTile;

    PlacementData* m_pData = nullptr;

    virtual void Execute() override;
  };
} // namespace WProcGenInternal
