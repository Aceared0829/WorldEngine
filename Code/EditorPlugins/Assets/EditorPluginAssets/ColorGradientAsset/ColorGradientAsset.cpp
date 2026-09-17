#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <Core/Curves/ColorGradientResource.h>
#include <EditorPluginAssets/ColorGradientAsset/ColorGradientAsset.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WColorGradientAssetData, 3, WRTTIDefaultAllocator<WColorGradientAssetData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Gradient", m_Gradient),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WColorGradientAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WColorGradientAssetDocument::WColorGradientAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WColorGradientAssetData>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

void WColorGradientAssetDocument::WriteResource(WStreamWriter& inout_stream) const
{
  const WColorGradientAssetData* pProp = GetProperties();

  WColorGradientResourceDescriptor desc;
  pProp->FillGradientData(desc.m_Gradient);

  desc.Save(inout_stream);
}

void WColorGradientAssetData::FillGradientData(WColorGradient& out_result) const
{
  out_result = m_Gradient;
}

WColor WColorGradientAssetData::Evaluate(WInt64 iTick) const
{
  WColorGradient temp = m_Gradient;

  WColor color;
  temp.Evaluate(WColorGradient::TickToTime(iTick), color);
  return color;
}

WTransformStatus WColorGradientAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WriteResource(stream);
  return WStatus(W_SUCCESS);
}

WTransformStatus WColorGradientAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  const WColorGradientAssetData* pProp = GetProperties();

  WImageHeader imgHeader;
  imgHeader.SetWidth(256);
  imgHeader.SetHeight(256);
  imgHeader.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);
  WImage img;
  img.ResetAndAlloc(imgHeader);

  WColorGradient gradient;
  pProp->FillGradientData(gradient);

  double fMin, fMax;
  gradient.GetExtents(fMin, fMax);
  const double range = fMax - fMin;
  const double div = 1.0 / (img.GetWidth() - 1);
  const double factor = range * div;

  for (WUInt32 x = 0; x < img.GetWidth(); ++x)
  {
    const double pos = fMin + x * factor;

    WColorGammaUB color;
    gradient.EvaluateColor(pos, color);

    WUInt8 alpha;
    gradient.EvaluateAlpha(pos, alpha);
    const WColorLinearUB alphaColor = WColorLinearUB(alpha, alpha, alpha, 255);

    const float fAlphaFactor = WMath::ColorByteToFloat(alpha);
    WColor colorWithAlpha = color;
    colorWithAlpha.r *= fAlphaFactor;
    colorWithAlpha.g *= fAlphaFactor;
    colorWithAlpha.b *= fAlphaFactor;

    const WColorGammaUB colWithAlpha = colorWithAlpha;

    for (WUInt32 y = 0; y < img.GetHeight() / 4; ++y)
    {
      WColorGammaUB* pixel = img.GetPixelPointer<WColorGammaUB>(0, 0, 0, x, y);
      *pixel = alphaColor;
    }

    for (WUInt32 y = img.GetHeight() / 4; y < img.GetHeight() / 2; ++y)
    {
      WColorGammaUB* pixel = img.GetPixelPointer<WColorGammaUB>(0, 0, 0, x, y);
      *pixel = colWithAlpha;
    }

    for (WUInt32 y = img.GetHeight() / 2; y < img.GetHeight(); ++y)
    {
      WColorGammaUB* pixel = img.GetPixelPointer<WColorGammaUB>(0, 0, 0, x, y);
      *pixel = color;
    }
  }

  return SaveThumbnail(img, ThumbnailInfo);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WColorGradientAssetDataPatch_1_2 : public WGraphPatch
{
public:
  WColorGradientAssetDataPatch_1_2()
    : WGraphPatch("WColorGradientAssetData", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Color CPs", "ColorCPs");
    pNode->RenameProperty("Alpha CPs", "AlphaCPs");
    pNode->RenameProperty("Intensity CPs", "IntensityCPs");
  }
};

WColorGradientAssetDataPatch_1_2 g_WColorGradientAssetDataPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class WColorControlPoint_1_2 : public WGraphPatch
{
public:
  WColorControlPoint_1_2()
    : WGraphPatch("WColorControlPoint", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", WColorGradient::SnapTimeToTick(fTime));
    }
  }
};

WColorControlPoint_1_2 g_WColorControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class WAlphaControlPoint_1_2 : public WGraphPatch
{
public:
  WAlphaControlPoint_1_2()
    : WGraphPatch("WAlphaControlPoint", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", WColorGradient::SnapTimeToTick(fTime));
    }
  }
};

WAlphaControlPoint_1_2 g_WAlphaControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

class WIntensityControlPoint_1_2 : public WGraphPatch
{
public:
  WIntensityControlPoint_1_2()
    : WGraphPatch("WIntensityControlPoint", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Position");
    if (pPoint && pPoint->m_Value.IsA<float>())
    {
      const float fTime = pPoint->m_Value.Get<float>();
      pNode->AddProperty("Tick", WColorGradient::SnapTimeToTick(fTime));
    }
  }
};

WIntensityControlPoint_1_2 g_WIntensityControlPoint_1_2;

//////////////////////////////////////////////////////////////////////////

// Patch to rename old control point types to new standalone types
class WColorControlPoint_2_3 : public WGraphPatch
{
public:
  WColorControlPoint_2_3()
    : WGraphPatch("WColorControlPoint", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WColorGradientColorCP");
  }
};

WColorControlPoint_2_3 g_WColorControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class WAlphaControlPoint_2_3 : public WGraphPatch
{
public:
  WAlphaControlPoint_2_3()
    : WGraphPatch("WAlphaControlPoint", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WColorGradientAlphaCP");
  }
};

WAlphaControlPoint_2_3 g_WAlphaControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class WIntensityControlPoint_2_3 : public WGraphPatch
{
public:
  WIntensityControlPoint_2_3()
    : WGraphPatch("WIntensityControlPoint", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("WColorGradientIntensityCP");
  }
};

WIntensityControlPoint_2_3 g_WIntensityControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class WColorGradientAssetDataPatch_2_3 : public WGraphPatch
{
public:
  WColorGradientAssetDataPatch_2_3()
    : WGraphPatch("WColorGradientAssetData", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    // Create new Gradient node
    WUuid gradientGuid = WUuid::MakeUuid();
    auto* pGradientNode = pGraph->AddNode(gradientGuid, "WColorGradient", 1);

    // Migrate ColorCPs
    auto* pColorCPs = pNode->FindProperty("ColorCPs");
    if (pColorCPs && pColorCPs->m_Value.IsA<WVariantArray>())
    {
      const WVariantArray& oldCPs = pColorCPs->m_Value.Get<WVariantArray>();
      WVariantArray newCPs;

      for (const auto& cpGuid : oldCPs)
      {
        auto* pOldCP = pGraph->GetNode(cpGuid.Get<WUuid>());
        if (pOldCP)
        {
          WUuid newCPGuid = WUuid::MakeUuid();
          auto* pNewCP = pGraph->AddNode(newCPGuid, "WColorGradientColorCP", 1);

          pNewCP->AddProperty("Tick", pOldCP->FindProperty("Tick")->m_Value);
          pNewCP->AddProperty("Red", pOldCP->FindProperty("Red")->m_Value);
          pNewCP->AddProperty("Green", pOldCP->FindProperty("Green")->m_Value);
          pNewCP->AddProperty("Blue", pOldCP->FindProperty("Blue")->m_Value);

          newCPs.PushBack(newCPGuid);
        }
      }
      pGradientNode->AddProperty("ColorCPs", newCPs);
    }

    // Migrate AlphaCPs
    auto* pAlphaCPs = pNode->FindProperty("AlphaCPs");
    if (pAlphaCPs && pAlphaCPs->m_Value.IsA<WVariantArray>())
    {
      const WVariantArray& oldCPs = pAlphaCPs->m_Value.Get<WVariantArray>();
      WVariantArray newCPs;

      for (const auto& cpGuid : oldCPs)
      {
        auto* pOldCP = pGraph->GetNode(cpGuid.Get<WUuid>());
        if (pOldCP)
        {
          WUuid newCPGuid = WUuid::MakeUuid();
          auto* pNewCP = pGraph->AddNode(newCPGuid, "WColorGradientAlphaCP", 1);

          pNewCP->AddProperty("Tick", pOldCP->FindProperty("Tick")->m_Value);
          pNewCP->AddProperty("Alpha", pOldCP->FindProperty("Alpha")->m_Value);

          newCPs.PushBack(newCPGuid);
        }
      }
      pGradientNode->AddProperty("AlphaCPs", newCPs);
    }

    // Migrate IntensityCPs
    auto* pIntensityCPs = pNode->FindProperty("IntensityCPs");
    if (pIntensityCPs && pIntensityCPs->m_Value.IsA<WVariantArray>())
    {
      const WVariantArray& oldCPs = pIntensityCPs->m_Value.Get<WVariantArray>();
      WVariantArray newCPs;

      for (const auto& cpGuid : oldCPs)
      {
        auto* pOldCP = pGraph->GetNode(cpGuid.Get<WUuid>());
        if (pOldCP)
        {
          WUuid newCPGuid = WUuid::MakeUuid();
          auto* pNewCP = pGraph->AddNode(newCPGuid, "WColorGradientIntensityCP", 1);

          pNewCP->AddProperty("Tick", pOldCP->FindProperty("Tick")->m_Value);
          pNewCP->AddProperty("Intensity", pOldCP->FindProperty("Intensity")->m_Value);

          newCPs.PushBack(newCPGuid);
        }
      }
      pGradientNode->AddProperty("IntensityCPs", newCPs);
    }

    // Replace old structure with new
    pNode->RemoveProperty("ColorCPs");
    pNode->RemoveProperty("AlphaCPs");
    pNode->RemoveProperty("IntensityCPs");
    pNode->AddProperty("Gradient", gradientGuid);
  }
};

WColorGradientAssetDataPatch_2_3 g_WColorGradientAssetDataPatch_2_3;
