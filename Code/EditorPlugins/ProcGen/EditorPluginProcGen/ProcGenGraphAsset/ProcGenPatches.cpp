#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

/// Migrates ProcGen nodes from using RenderPipeline pin types to ProcGen-specific pin types.
///
/// This patch processes all ProcGen node types and updates their pin property types:
/// - WRenderPipelineNodeInputPin -> WProcGenNodeInputPin
/// - WRenderPipelineNodeOutputPin -> WProcGenNodeOutputPin
class WProcGenPinTypePatch_1_2 : public WGraphPatch
{
public:
  WProcGenPinTypePatch_1_2()
    : WGraphPatch("", 2, PatchType::GraphPatch)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Iterate through all nodes in the graph
    for (auto it = pGraph->GetAllNodes().GetIterator(); it.IsValid(); ++it)
    {
      WAbstractObjectNode* pCurrentNode = it.Value();
      const WStringView sType = pCurrentNode->GetType();

      // Only process ProcGen node types
      if (sType.StartsWith("WProcGen"))
      {
        // Iterate through all properties of this node
        auto& properties = pCurrentNode->GetProperties();
        for (WUInt32 i = 0; i < properties.GetCount(); ++i)
        {
          auto& prop = properties[i];

          // Check if this property is a struct with a RenderPipeline pin type
          if (prop.m_Value.IsA<WUuid>())
          {
            WUuid propertyObjectGuid = prop.m_Value.Get<WUuid>();
            WAbstractObjectNode* pPropertyNode = pGraph->GetNode(propertyObjectGuid);

            if (pPropertyNode)
            {
              const WStringView sPropertyType = pPropertyNode->GetType();

              // Rename RenderPipeline pin types to ProcGen pin types
              if (sPropertyType == "WRenderPipelineNodeInputPin")
              {
                pPropertyNode->SetType("WProcGenNodeInputPin");
              }
              else if (sPropertyType == "WRenderPipelineNodeOutputPin")
              {
                pPropertyNode->SetType("WProcGenNodeOutputPin");
              }
              else if (sPropertyType == "WRenderPipelineNodePin")
              {
                pPropertyNode->SetType("WProcGenNodePin");
              }
            }
          }
        }
      }
    }
  }
};

WProcGenPinTypePatch_1_2 g_WProcGenPinTypePatch_1_2;
