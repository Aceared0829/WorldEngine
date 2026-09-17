#pragma once

#include <Core/Physics/SurfaceResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WSurfaceAssetDocument : public WSimpleAssetDocument<WSurfaceResourceDescriptor>
{
  W_ADD_DYNAMIC_REFLECTION(WSurfaceAssetDocument, WSimpleAssetDocument<WSurfaceResourceDescriptor>);

public:
  WSurfaceAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
};
