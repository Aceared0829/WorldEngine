#include <EditorPluginScene/EditorPluginScenePCH.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WFakeRopeComponentPatch_2_3 : public WGraphPatch
{
public:
  WFakeRopeComponentPatch_2_3()
    : WGraphPatch("WFakeRopeComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

WFakeRopeComponentPatch_2_3 g_WFakeRopeComponentPatch_2_3;
