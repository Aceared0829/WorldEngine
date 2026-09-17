#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WImageDataAssetProperties, 1, WRTTIDefaultAllocator<WImageDataAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Input", m_sInputFile)->AddAttributes(new WFileBrowserAttribute("Select Image", WFileBrowserAttribute::ImagesLdrAndHdr), new WRequiredAttribute())
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
