#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Tracks/ColorGradient.h>

class WColorGradientAssetData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WColorGradientAssetData, WReflectedClass);

public:
  WColorGradient m_Gradient;

  /// Fills out the WColorGradient structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the color gradient, that must be called manually.
  void FillGradientData(WColorGradient& out_result) const;
  WColor Evaluate(WInt64 iTick) const;
};

class WColorGradientAssetDocument : public WSimpleAssetDocument<WColorGradientAssetData>
{
  W_ADD_DYNAMIC_REFLECTION(WColorGradientAssetDocument, WSimpleAssetDocument<WColorGradientAssetData>);

public:
  WColorGradientAssetDocument(WStringView sDocumentPath);

  void WriteResource(WStreamWriter& inout_stream) const;

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
