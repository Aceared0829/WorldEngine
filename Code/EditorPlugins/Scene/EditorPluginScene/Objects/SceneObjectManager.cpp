#include <EditorPluginScene/EditorPluginScenePCH.h>

#include "Foundation/Serialization/GraphPatch.h"
#include <Core/World/GameObject.h>
#include <EditorPluginScene/Objects/SceneObjectManager.h>
#include <EditorPluginScene/Scene/Scene2Document.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneDocumentSettingsBase, 1, WRTTINoAllocator)
{
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPrefabDocumentSettings, 1, WRTTIDefaultAllocator<WPrefabDocumentSettings>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("ExposedProperties", m_ExposedProperties),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLayerDocumentSettings, 1, WRTTIDefaultAllocator<WLayerDocumentSettings>)
{
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSceneDocumentRoot, 2, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Settings", m_pSettings)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

WSceneObjectManager::WSceneObjectManager()
  : WDocumentObjectManager(WGetStaticRTTI<WSceneDocumentRoot>())
{
}

void WSceneObjectManager::GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const
{
  out_types.PushBack(WGetStaticRTTI<WGameObject>());

  WRTTI::ForEachDerivedType<WComponent>(
    [&](const WRTTI* pRtti)
    { out_types.PushBack(pRtti); },
    WRTTI::ForEachOptions::ExcludeAbstract);
}

WStatus WSceneObjectManager::InternalCanAdd(
  const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const
{
  if (IsUnderRootProperty("Children", pParent, sParentProperty))
  {
    if (pParent == nullptr)
    {
      bool bIsDerived = pRtti->IsDerivedFrom<WGameObject>();
      if (!bIsDerived)
      {
        return WStatus("Only WGameObject can be added to the root of the world!");
      }
    }
    else
    {
      // only prevent adding game objects (as children) to objects that already have a prefab component
      // do allow to attach components to objects with prefab components
      // if (pRtti->IsDerivedFrom<WGameObject>())
      //{
      //  if (pParent->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
      //  {
      //    auto children = pParent->GetChildren();
      //    for (auto pChild : children)
      //    {
      //      if (pChild->GetType()->IsDerivedFrom<WPrefabReferenceComponent>())
      //        return WStatus("Cannot add objects to a prefab node.");
      //    }
      //  }
      //}

      // in case prefab component should be the only component on a node
      // if (pRtti->IsDerivedFrom<WPrefabReferenceComponent>())
      //{
      //  if (!pParent->GetChildren().IsEmpty())
      //    return WStatus("Prefab components can only be added to empty nodes.");
      //}
    }
  }
  return WStatus(W_SUCCESS);
}

WStatus WSceneObjectManager::InternalCanMove(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const
{
  // code to disallow attaching nodes to a prefab node
  // if (pNewParent != nullptr)
  //{
  //  if (pNewParent->GetTypeAccessor().GetType()->IsDerivedFrom<WGameObject>())
  //  {
  //    auto children = pNewParent->GetChildren();
  //    for (auto pChild : children)
  //    {
  //      if (pChild->GetType()->IsDerivedFrom<WPrefabReferenceComponent>())
  //        return WStatus("Cannot move objects into a prefab node.");
  //    }
  //  }
  //}

  return WStatus(W_SUCCESS);
}

WStatus WSceneObjectManager::InternalCanSelect(const WDocumentObject* pObject) const
{
  const WRTTI* pRtti = pObject->GetTypeAccessor().GetType();

  if (pRtti == WGetStaticRTTI<WGameObject>())
    return WStatus(W_SUCCESS);

  if (pRtti == WGetStaticRTTI<WSceneLayer>())
    return WStatus(W_SUCCESS);

  if (pRtti == WGetStaticRTTI<WPrefabDocumentSettings>())
    return WStatus(W_SUCCESS);

  return WStatus(WFmt("Object of type '{0}' is not a 'WGameObject' and can't be selected.", pObject->GetTypeAccessor().GetType()->GetTypeName()));
}

namespace
{
  /// Patch class
  class WSceneDocumentSettings_1_2 : public WGraphPatch
  {
  public:
    WSceneDocumentSettings_1_2()
      : WGraphPatch("WSceneDocumentSettings", 2)
    {
    }
    virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
    {
      // Previously, WSceneDocumentSettings only contained prefab settings. As these only apply to prefab documents, we switch the old version to prefab.
      ref_context.RenameClass("WPrefabDocumentSettings", 1);
      WVersionKey bases[] = {{"WSceneDocumentSettingsBase", 1}, {"WReflectedClass", 1}};
      ref_context.ChangeBaseClass(bases);
    }
  };
  WSceneDocumentSettings_1_2 g_WSceneDocumentSettings_1_2;
} // namespace
