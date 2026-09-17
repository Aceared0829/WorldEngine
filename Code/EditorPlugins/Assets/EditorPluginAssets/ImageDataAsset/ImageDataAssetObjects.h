#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

class WImageDataAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WImageDataAssetProperties, WReflectedClass);

public:
  WString m_sInputFile;

  // TODO: more WImageData options
  // * maximum resolution
  // * 1, 2, 3, 4 channels
  // * compression: lossy (jpg), lossless (png), uncompressed
  // * HDR data ?
  // * combine from multiple images (channel mapping)
};
