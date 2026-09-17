#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <EditorPluginAssets/Curve1DAsset/Curve1DAsset.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCurve1DAssetDocument, 3, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WCurve1DAssetDocument::WCurve1DAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WCurveGroupData>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

WCurve1DAssetDocument::~WCurve1DAssetDocument() = default;

void WCurve1DAssetDocument::FillCurve(WUInt32 uiCurveIdx, WCurve1D& out_result) const
{
  const WCurveGroupData* pProp = static_cast<const WCurveGroupData*>(GetProperties());
  pProp->ConvertToRuntimeData(uiCurveIdx, out_result);
}

WUInt32 WCurve1DAssetDocument::GetCurveCount() const
{
  const WCurveGroupData* pProp = GetProperties();
  return pProp->m_Curves.GetCount();
}

void WCurve1DAssetDocument::WriteResource(WStreamWriter& inout_stream) const
{
  const WCurveGroupData* pProp = GetProperties();

  WCurve1DResourceDescriptor desc;
  desc.m_Curves.SetCount(pProp->m_Curves.GetCount());

  for (WUInt32 i = 0; i < pProp->m_Curves.GetCount(); ++i)
  {
    FillCurve(i, desc.m_Curves[i]);
    desc.m_Curves[i].SortControlPoints();
  }

  desc.Save(inout_stream);
}

WTransformStatus WCurve1DAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WriteResource(stream);
  return WStatus(W_SUCCESS);
}

WTransformStatus WCurve1DAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const WCurveGroupData* pProp = GetProperties();

  QImage qimg(WThumbnailSize, WThumbnailSize, QImage::Format_RGBA8888);
  qimg.fill(QColor(50, 50, 50));

  QPainter p(&qimg);
  QPainter* painter = &p;
  painter->setBrush(Qt::NoBrush);
  painter->setRenderHint(QPainter::Antialiasing);

  if (!pProp->m_Curves.IsEmpty())
  {

    double fExtentsMin, fExtentsMax;
    double fExtremesMin, fExtremesMax;

    for (WUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      WCurve1D curve;
      FillCurve(curveIdx, curve);

      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      double fMin, fMax;
      curve.QueryExtents(fMin, fMax);

      double fMin2, fMax2;
      curve.QueryExtremeValues(fMin2, fMax2);

      if (curveIdx == 0)
      {
        fExtentsMin = fMin;
        fExtentsMax = fMax;
        fExtremesMin = fMin2;
        fExtremesMax = fMax2;
      }
      else
      {
        fExtentsMin = WMath::Min(fExtentsMin, fMin);
        fExtentsMax = WMath::Max(fExtentsMax, fMax);
        fExtremesMin = WMath::Min(fExtremesMin, fMin2);
        fExtremesMax = WMath::Max(fExtremesMax, fMax2);
      }
    }

    const float range = fExtentsMax - fExtentsMin;
    const float div = 1.0f / (qimg.width() - 1);
    const float factor = range * div;

    const float lowValue = (fExtremesMin > 0) ? 0.0f : fExtremesMin;
    const float highValue = (fExtremesMax < 0) ? 0.0f : fExtremesMax;

    const float range2 = highValue - lowValue;

    for (WUInt32 curveIdx = 0; curveIdx < pProp->m_Curves.GetCount(); ++curveIdx)
    {
      QPainterPath path;

      WCurve1D curve;
      FillCurve(curveIdx, curve);
      curve.SortControlPoints();
      curve.CreateLinearApproximation();

      const QColor curColor = WToQtColor(pProp->m_Curves[curveIdx]->m_CurveColor);
      QPen pen(curColor, 8.0f);
      painter->setPen(pen);

      for (WUInt32 x = 0; x < (WUInt32)qimg.width(); ++x)
      {
        const float pos = fExtentsMin + x * factor;
        const float value = 1.0f - (curve.Evaluate(pos) - lowValue) / range2;

        const WUInt32 y = WMath::Clamp<WUInt32>(qimg.height() * value, 0, qimg.height() - 1);

        if (x == 0)
          path.moveTo(x, y);
        else
          path.lineTo(x, y);
      }

      painter->drawPath(path);
    }
  }

  return SaveThumbnail(qimg, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WCurve1DControlPointPatch_1_2 : public WGraphPatch
{
public:
  WCurve1DControlPointPatch_1_2()
    : WGraphPatch("WCurve1DControlPoint", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Left Tangent", "LeftTangent");
    pNode->RenameProperty("Right Tangent", "RightTangent");
  }
};

WCurve1DControlPointPatch_1_2 g_WCurve1DControlPointPatch_1_2;


class WCurve1DDataPatch_1_2 : public WGraphPatch
{
public:
  WCurve1DDataPatch_1_2()
    : WGraphPatch("WCurve1DData", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override { pNode->RenameProperty("Control Points", "ControlPoints"); }
};

WCurve1DDataPatch_1_2 g_WCurve1DDataPatch_1_2;
