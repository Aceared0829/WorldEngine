#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Prefabs.h>

#include <Core/Prefabs/PrefabResource.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WScriptExtensionClass_Prefabs, WNoBase, 1, WRTTINoAllocator)
{
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SpawnPrefab, In, "World", In, "Prefab", In, "GlobalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new WFunctionArgumentAttributes(1, new WAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new WFunctionArgumentAttributes(3, new WDefaultValueAttribute(WVariant(WInvalidIndex))),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute(true)),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(true))),

    W_SCRIPT_FUNCTION_PROPERTY(SpawnPrefabAsChild, In, "World", In, "Prefab", In, "Parent", In, "LocalTransform", In, "UniqueID", In, "SetCreatedByPrefab", In, "SetHideShapeIcon")->AddAttributes(
      new WFunctionArgumentAttributes(1, new WAssetBrowserAttribute("CompatibleAsset_Prefab")),
      new WFunctionArgumentAttributes(4, new WDefaultValueAttribute(WVariant(WInvalidIndex))),
      new WFunctionArgumentAttributes(5, new WDefaultValueAttribute(true)),
      new WFunctionArgumentAttributes(6, new WDefaultValueAttribute(true))),
  }
  W_END_FUNCTIONS;
  W_BEGIN_ATTRIBUTES
  {
    new WScriptExtensionAttribute("Prefabs"),
  }
  W_END_ATTRIBUTES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on

void SpawnPrefabHelper(WWorld& ref_world, WStringView sPrefab, WGameObjectHandle hParent, const WTransform& transform, WUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon, WVariantArray& out_rootObjects)
{
  WPrefabResourceHandle hPrefab = WResourceManager::LoadResource<WPrefabResource>(sPrefab);

  WResourceLock<WPrefabResource> pPrefab(hPrefab, WResourceAcquireMode::BlockTillLoaded_NeverFail);

  if (pPrefab.GetAcquireResult() != WResourceAcquireResult::Final)
    return;

  WTempHybridArray<WGameObject*, 8> createdRootObjects;
  WTempHybridArray<WGameObject*, 8> createdChildObjects;

  WPrefabInstantiationOptions opt;
  opt.m_hParent = hParent;
  opt.m_pCreatedRootObjectsOut = &createdRootObjects;
  opt.m_pCreatedChildObjectsOut = &createdChildObjects;

  pPrefab->InstantiatePrefab(ref_world, transform, opt);

  auto FixupObject = [&](WGameObject* pObject)
  {
    if (uiUniqueID != WInvalidIndex)
    {
      for (auto pComponent : pObject->GetComponents())
      {
        pComponent->SetUniqueID(uiUniqueID);
      }
    }

    if (bSetCreatedByPrefab)
      pObject->SetCreatedByPrefab();

    if (bSetHideShapeIcon)
      pObject->SetHideShapeIcon();
  };

  for (auto pObject : createdRootObjects)
  {
    FixupObject(pObject);
    out_rootObjects.PushBack(pObject->GetHandle());
  }

  for (auto pObject : createdChildObjects)
  {
    FixupObject(pObject);
  }
}

WVariantArray WScriptExtensionClass_Prefabs::SpawnPrefab(WWorld* pWorld, WStringView sPrefab, const WTransform& globalTransform, WUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  WVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, WGameObjectHandle(), globalTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}

WVariantArray WScriptExtensionClass_Prefabs::SpawnPrefabAsChild(WWorld* pWorld, WStringView sPrefab, WGameObject* pParent, const WTransform& localTransform, WUInt32 uiUniqueID, bool bSetCreatedByPrefab, bool bSetHideShapeIcon)
{
  if (pWorld == nullptr || sPrefab.IsEmpty())
    return {};

  WVariantArray rootObjects;
  SpawnPrefabHelper(*pWorld, sPrefab, pParent != nullptr ? pParent->GetHandle() : WGameObjectHandle(), localTransform, uiUniqueID, bSetCreatedByPrefab, bSetHideShapeIcon, rootObjects);
  return rootObjects;
}


W_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Prefabs);
