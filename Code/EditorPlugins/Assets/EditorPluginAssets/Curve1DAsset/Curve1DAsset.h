#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <Foundation/Tracks/CurveEditData.h>

class WCurve1D;

class WCurve1DAssetDocument : public WSimpleAssetDocument<WCurveGroupData>
{
  W_ADD_DYNAMIC_REFLECTION(WCurve1DAssetDocument, WSimpleAssetDocument<WCurveGroupData>);

public:
  WCurve1DAssetDocument(WStringView sDocumentPath);
  ~WCurve1DAssetDocument();

  /// Fills out the WCurve1D structure with an exact copy of the data in the asset.
  /// Does NOT yet sort the control points, so before evaluating the curve, that must be called manually.
  void FillCurve(WUInt32 uiCurveIdx, WCurve1D& out_result) const;

  WUInt32 GetCurveCount() const;

  void WriteResource(WStreamWriter& inout_stream) const;

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;
};
