#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>

WPropertyAnimObjectManager::WPropertyAnimObjectManager() = default;

WPropertyAnimObjectManager::~WPropertyAnimObjectManager() = default;

WStatus WPropertyAnimObjectManager::InternalCanAdd(
  const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return WStatus(W_SUCCESS);

  if (IsTemporary(pParent, sParentProperty))
    return WStatus("The structure of the context cannot be animated.");
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectManager::InternalCanRemove(const WDocumentObject* pObject) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return WStatus(W_SUCCESS);

  if (IsTemporary(pObject))
    return WStatus("The structure of the context cannot be animated.");
  return WStatus(W_SUCCESS);
}

WStatus WPropertyAnimObjectManager::InternalCanMove(
  const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const
{
  if (m_bAllowStructureChangeOnTemporaries)
    return WStatus(W_SUCCESS);

  if (IsTemporary(pObject))
    return WStatus("The structure of the context cannot be animated.");
  return WStatus(W_SUCCESS);
}
