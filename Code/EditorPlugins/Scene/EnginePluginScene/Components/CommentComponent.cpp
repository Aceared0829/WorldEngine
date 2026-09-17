#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <EnginePluginScene/Components/CommentComponent.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WCommentComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("Comment", GetComment, SetComment),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Editing"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCommentComponent::WCommentComponent() = default;
WCommentComponent::~WCommentComponent() = default;

void WCommentComponent::SetComment(const char* szText)
{
  m_sComment.Assign(szText);
}

const char* WCommentComponent::GetComment() const
{
  return m_sComment.GetString();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneExportModifier_RemoveCommentComponents, 1, WRTTIDefaultAllocator<WSceneExportModifier_RemoveCommentComponents>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WSceneExportModifier_RemoveCommentComponents::ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport)
{
  W_LOCK(ref_world.GetWriteMarker());

  if (WCommentComponentManager* pMan = ref_world.GetComponentManager<WCommentComponentManager>())
  {
    for (auto it = pMan->GetComponents(); it.IsValid(); it.Next())
    {
      pMan->DeleteComponent(it);
    }
  }
}
