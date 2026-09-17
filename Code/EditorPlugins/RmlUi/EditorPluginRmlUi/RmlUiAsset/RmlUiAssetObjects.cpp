#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRmlUiAssetProperties, 1, WRTTIDefaultAllocator<WRmlUiAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RmlFile", m_sRmlFile)->AddAttributes(new WFileBrowserAttribute("Select Rml file", "*.rml", {}, "RmlUI", WDependencyFlags::Package | WDependencyFlags::Thumbnail | WDependencyFlags::Transform), new WRequiredAttribute()),
    W_ENUM_MEMBER_PROPERTY("ScaleMode", WRmlUiScaleMode, m_ScaleMode),
    W_MEMBER_PROPERTY("ReferenceResolution", m_ReferenceResolution)->AddAttributes(new WDefaultValueAttribute(WVec2U32(1920, 1080))),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WRmlUiAssetProperties::WRmlUiAssetProperties() = default;
WRmlUiAssetProperties::~WRmlUiAssetProperties() = default;
