#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

class WPhysicsWorldModuleInterface;
class WVolumeCollection;

namespace WProcGenInternal
{
  class PlacementTask final : public WTask
  {
  public:
    PlacementTask(PlacementData* pData, const char* szName);
    ~PlacementTask();

    void Clear();

    WArrayPtr<const PlacementPoint> GetInputPoints() const { return m_InputPoints; }
    WArrayPtr<const PlacementTransform> GetOutputTransforms() const { return m_OutputTransforms; }

  private:
    virtual void Execute() override;

    void FindPlacementPoints();
    void ExecuteVM();

    WProcessingStream MakeInputStream(const WHashedString& sName, WUInt32 uiOffset, WProcessingStream::DataType dataType = WProcessingStream::DataType::Float)
    {
      return WProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), dataType, sizeof(PlacementPoint));
    }

    WProcessingStream MakeOutputStream(const WHashedString& sName, WUInt32 uiOffset, WProcessingStream::DataType dataType = WProcessingStream::DataType::Float)
    {
      return WProcessingStream(sName, m_InputPoints.GetByteArrayPtr().GetSubArray(uiOffset), dataType, sizeof(PlacementPoint));
    }

    PlacementData* m_pData = nullptr;

    WDynamicArray<PlacementPoint, WAlignedAllocatorWrapper> m_InputPoints;
    WDynamicArray<PlacementTransform, WAlignedAllocatorWrapper> m_OutputTransforms;
    WDynamicArray<float> m_Density;
    WDynamicArray<WUInt32> m_ValidPoints;

    WExpressionVM m_VM;
  };
} // namespace WProcGenInternal
