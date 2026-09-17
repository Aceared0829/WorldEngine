#pragma once

#include <RmlUiPlugin/Resources/RmlUiResource.h>

class WRmlUiAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WRmlUiAssetProperties, WReflectedClass);

public:
  WRmlUiAssetProperties();
  ~WRmlUiAssetProperties();

  WString m_sRmlFile;
  WEnum<WRmlUiScaleMode> m_ScaleMode;
  WVec2U32 m_ReferenceResolution;
};
