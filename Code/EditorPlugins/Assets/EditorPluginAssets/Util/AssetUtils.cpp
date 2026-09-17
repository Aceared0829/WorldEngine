#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Util/AssetUtils.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WMaterialResourceSlot, WNoBase, 1, WRTTIDefaultAllocator<WMaterialResourceSlot>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new WReadOnlyAttribute()),
    W_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new WAssetBrowserAttribute("CompatibleAsset_Material"), new WRequiredAttribute()),
    W_MEMBER_PROPERTY("Highlight", m_bHighlight)->AddAttributes(new WTemporaryAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;
// clang-format on
