#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Threading/TaskSystem.h>
#include <ProcGenPlugin/Declarations.h>

struct WMeshBufferResourceDescriptor;
class WVolumeCollection;

namespace WProcGenInternal
{
  class VertexColorTask final : public WTask
  {
  public:
    struct InputVertex
    {
      W_DECLARE_POD_TYPE();

      WVec3 m_vPosition;
      WVec3 m_vNormal;
      WColor m_Color;
      WUInt32 m_uiIndex;
    };

    VertexColorTask();
    ~VertexColorTask();

    void Prepare(const WWorld& world, const WMeshBufferResourceDescriptor& desc, const WTransform& transform, const WBoundingBox& bbox, WArrayPtr<WSharedPtr<const VertexColorOutput>> outputs, WArrayPtr<WProcVertexColorMapping> outputMappings, WArrayPtr<WColorLinearUB> outputVertexColors);

  private:
    virtual void Execute() override;

    WHybridArray<WSharedPtr<const VertexColorOutput>, 2> m_Outputs;
    WHybridArray<WProcVertexColorMapping, 2> m_OutputMappings;

    WDynamicArray<InputVertex> m_InputVertices;

    WDynamicArray<WColor> m_TempData;
    WArrayPtr<WColorLinearUB> m_OutputVertexColors;

    WDeque<WVolumeCollection> m_VolumeCollections;
    WExpression::GlobalData m_GlobalData;

    WExpressionVM m_VM;
  };
} // namespace WProcGenInternal
