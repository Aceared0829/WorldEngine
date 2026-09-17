#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <GameEngine/Utils/ImageDataResource.h>
#include <GameEngine/Volumes/VolumeSampler.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <Texture/Image/ImageUtils.h>

namespace
{
  W_FORCE_INLINE float ApplyValue(WProcGenBlendMode::Enum blendMode, float fInitialValue, float fNewValue)
  {
    switch (blendMode)
    {
      case WProcGenBlendMode::Add:
        return fInitialValue + fNewValue;
      case WProcGenBlendMode::Subtract:
        return fInitialValue - fNewValue;
      case WProcGenBlendMode::Multiply:
        return fInitialValue * fNewValue;
      case WProcGenBlendMode::Divide:
        return fInitialValue / fNewValue;
      case WProcGenBlendMode::Max:
        return WMath::Max(fInitialValue, fNewValue);
      case WProcGenBlendMode::Min:
        return WMath::Min(fInitialValue, fNewValue);
      case WProcGenBlendMode::Set:
        return fNewValue;
      default:
        return fInitialValue;
    }
  }
} // namespace

static_assert(sizeof(WVolumeCollection::Sphere) == 60);
static_assert(sizeof(WVolumeCollection::Box) == 80);

void WVolumeCollection::Shape::SetGlobalToLocalTransform(const WSimdMat4f& t)
{
  WSimdVec4f r0, r1, r2, r3;
  t.GetRows(r0, r1, r2, r3);

  m_GlobalToLocalTransform0 = WSimdConversion::ToVec4(r0);
  m_GlobalToLocalTransform1 = WSimdConversion::ToVec4(r1);
  m_GlobalToLocalTransform2 = WSimdConversion::ToVec4(r2);
}

WSimdMat4f WVolumeCollection::Shape::GetGlobalToLocalTransform() const
{
  WSimdMat4f m;
  m.SetRows(WSimdConversion::ToVec4(m_GlobalToLocalTransform0), WSimdConversion::ToVec4(m_GlobalToLocalTransform1),
    WSimdConversion::ToVec4(m_GlobalToLocalTransform2), WSimdVec4f(0, 0, 0, 1));

  return m;
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVolumeCollection, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WVolumeCollection::WVolumeCollection()
  : m_Allocator("VolumeCollection", WFoundation::GetAlignedAllocator(), 4 * 1024)
  , m_SortedShapes(&m_Allocator)
{
}

WVolumeCollection::~WVolumeCollection() = default;

float WVolumeCollection::EvaluateAtGlobalPosition(const WSimdVec4f& vPosition, float fInitialValue, WProcVolumeImageMode::Enum imgMode, const WColor& refColor) const
{
  float fValue = fInitialValue;

  for (auto pShape : m_SortedShapes)
  {
    if (pShape->m_Type == ShapeType::Sphere)
    {
      auto& sphere = *static_cast<const Sphere*>(pShape);
      const WSimdVec4f localPos = sphere.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const float distSquared = localPos.GetLengthSquared<3>();
      if (distSquared <= 1.0f)
      {
        const float fNewValue = ApplyValue(sphere.m_BlendMode, fValue, sphere.m_fValue);
        const float fAlpha = WMath::Saturate(WMath::Sqrt(distSquared) * sphere.m_fFadeOut - sphere.m_fFadeOut);
        fValue = WMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Box)
    {
      auto& box = *static_cast<const Box*>(pShape);
      const WSimdVec4f localPos = box.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const WSimdVec4f absLocalPos = localPos.Abs();
      if ((absLocalPos <= WSimdVec4f(1.0f)).AllSet<3>())
      {
        const WSimdVec4f fadeOut = WSimdVec4f::Select(localPos > WSimdVec4f::MakeZero(), WSimdConversion::ToVec3(box.m_vPositiveFadeOut), WSimdConversion::ToVec3(box.m_vNegativeFadeOut));
        WSimdVec4f vAlpha = absLocalPos.CompMul(fadeOut) - fadeOut;
        vAlpha = vAlpha.CompMin(WSimdVec4f(1.0f)).CompMax(WSimdVec4f::MakeZero());
        const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();

        const float fNewValue = ApplyValue(box.m_BlendMode, fValue, box.m_fValue);
        fValue = WMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else if (pShape->m_Type == ShapeType::Image)
    {
      auto& image = *static_cast<const Image*>(pShape);

      const WSimdVec4f localPos = image.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const WSimdVec4f absLocalPos = localPos.Abs();

      if ((absLocalPos <= WSimdVec4f(1.0f)).AllSet<3>() && image.m_pPixelData != nullptr)
      {
        WVec2 uv;
        uv.x = static_cast<float>(localPos.x()) * 0.5f + 0.5f;
        uv.y = static_cast<float>(localPos.y()) * 0.5f + 0.5f;

        const WColor col = WImageUtils::NearestSample(image.m_pPixelData, image.m_uiImageWidth, image.m_uiImageHeight, WImageAddressMode::Clamp, uv);

        float fValueToUse = image.m_fValue;
        W_IGNORE_UNUSED(fValueToUse);

        switch (imgMode)
        {
          case WProcVolumeImageMode::ReferenceColor:
            fValueToUse = image.m_fValue;
            break;
          case WProcVolumeImageMode::ChannelR:
            fValueToUse = image.m_fValue * col.r;
            break;
          case WProcVolumeImageMode::ChannelG:
            fValueToUse = image.m_fValue * col.g;
            break;
          case WProcVolumeImageMode::ChannelB:
            fValueToUse = image.m_fValue * col.b;
            break;
          case WProcVolumeImageMode::ChannelA:
            fValueToUse = image.m_fValue * col.a;
            break;
        }

        if (imgMode != WProcVolumeImageMode::ReferenceColor || col.IsEqualRGBA(refColor, 0.1f))
        {
          const WSimdVec4f fadeOut = WSimdVec4f::Select(localPos > WSimdVec4f::MakeZero(), WSimdConversion::ToVec3(image.m_vPositiveFadeOut), WSimdConversion::ToVec3(image.m_vNegativeFadeOut));
          WSimdVec4f vAlpha = absLocalPos.CompMul(fadeOut) - fadeOut;
          vAlpha = vAlpha.CompMin(WSimdVec4f(1.0f)).CompMax(WSimdVec4f::MakeZero());
          const float fAlpha = vAlpha.x() * vAlpha.y() * vAlpha.z();

          const float fNewValue = ApplyValue(image.m_BlendMode, fValue, image.m_fValue);
          fValue = WMath::Lerp(fValue, fNewValue, fAlpha);
        }
      }
    }
    else if (pShape->m_Type == ShapeType::Spline)
    {
      auto& spline = *static_cast<const Spline*>(pShape);
      const WSimdVec4f localPos = spline.GetGlobalToLocalTransform().TransformPosition(vPosition);
      const WSimdBBox localBox = WSimdConversion::ToBBox(spline.m_BoundingBox);
      if (!localBox.Contains(localPos))
        continue;

      float fT = 0.0f;
      float fDistanceSquared = 0.0f;
      spline.m_Spline.FindClosestPoint(localPos, fT, fDistanceSquared, spline.m_fMaxError);
      const WSimdMat4f t = spline.m_Spline.EvaluateTransform(fT).GetAsMat4().GetInverse();
      const float fNormalizedDistance = float(t.TransformPosition(localPos).GetLength<3>()) * spline.m_fInvRadius;
      if (fNormalizedDistance <= 1.0f)
      {
        const float fNewValue = ApplyValue(spline.m_BlendMode, fValue, spline.m_fValue);
        const float fAlpha = WMath::Saturate(fNormalizedDistance * spline.m_fFadeOut - spline.m_fFadeOut);
        fValue = WMath::Lerp(fValue, fNewValue, fAlpha);
      }
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }

  return fValue;
}

// static
void WVolumeCollection::ExtractVolumesInBox(const WWorld& world, const WBoundingBox& box, WSpatialData::Category spatialCategory,
  const WTagSet& includeTags, WVolumeCollection& out_collection, const WRTTI* pComponentBaseType)
{
  WMsgExtractVolumes msg;
  msg.m_pCollection = &out_collection;

  WSpatialSystem::QueryParams queryParams;
  queryParams.m_uiCategoryBitmask = spatialCategory.GetBitmask();
  queryParams.m_pIncludeTags = &includeTags;

  world.GetSpatialSystem()->FindObjectsInBox(box, queryParams,
    [&](WGameObject* pObject)
    {
      if (pComponentBaseType != nullptr)
      {
        WTempHybridArray<const WComponent*, 8> components;
        pObject->TryGetComponentsOfBaseType(pComponentBaseType, components);

        for (auto pComponent : components)
        {
          pComponent->SendMessage(msg);
        }
      }
      else
      {
        pObject->SendMessage(msg);
      }

      return WVisitorExecution::Continue;
    });

  struct Sorter
  {
    bool Less(const Shape* lhs, const Shape* rhs) const
    {
      return *lhs < *rhs;
    }
  };

  out_collection.m_SortedShapes.Sort(Sorter());
}

void WVolumeCollection::AddSphere(const WSimdTransform& transform, float fRadius, WEnum<WProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff)
{
  WSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale *= fRadius;

  Sphere* pSphere = W_NEW(&m_Allocator, Sphere);
  pSphere->SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  pSphere->m_Type = ShapeType::Sphere;
  pSphere->m_BlendMode = blendMode;
  pSphere->m_fValue = fValue;
  pSphere->m_uiSortingKey = WVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  pSphere->m_fFadeOut = -1.0f / WMath::Max(fFalloff, 0.0001f);

  m_SortedShapes.PushBack(pSphere);
}

void WVolumeCollection::AddBox(const WSimdTransform& transform, const WVec3& vExtents, WEnum<WProcGenBlendMode> blendMode, float fSortOrder, float fValue, const WVec3& vPositiveFalloff, const WVec3& vNegativeFalloff, const WImageDataResourceHandle& hImage)
{
  const bool bHasImage = hImage.IsValid();

  WSimdTransform scaledTransform = transform;
  scaledTransform.m_Scale = scaledTransform.m_Scale.CompMul(WSimdConversion::ToVec3(vExtents)) * 0.5f;

  Box* pBox = nullptr;
  if (bHasImage)
  {
    Image* pImage = W_NEW(&m_Allocator, Image);
    pImage->m_Type = ShapeType::Image;
    pImage->m_hImage = hImage;

    WResourceLock<WImageDataResource> pImageData(hImage, WResourceAcquireMode::BlockTillLoaded);
    auto& image = pImageData->GetDescriptor().m_Image;
    pImage->m_pPixelData = image.GetPixelPointer<WColor>();
    pImage->m_uiImageWidth = image.GetWidth();
    pImage->m_uiImageHeight = image.GetHeight();

    pBox = pImage;
  }
  else
  {
    pBox = W_NEW(&m_Allocator, Box);
    pBox->m_Type = ShapeType::Box;
  }

  pBox->SetGlobalToLocalTransform(scaledTransform.GetAsMat4().GetInverse());
  pBox->m_BlendMode = blendMode;
  pBox->m_fValue = fValue;
  pBox->m_uiSortingKey = WVolumeSampler::ComputeSortingKey(fSortOrder, scaledTransform.GetMaxScale());
  pBox->m_vPositiveFadeOut = WVec3(-1.0f).CompDiv(vPositiveFalloff.CompMax(WVec3(0.0001f)));
  pBox->m_vNegativeFadeOut = WVec3(-1.0f).CompDiv(vNegativeFalloff.CompMax(WVec3(0.0001f)));

  m_SortedShapes.PushBack(pBox);
}

void WVolumeCollection::AddSpline(const WSimdTransform& transform, const WSpline& spline, float fRadius, WEnum<WProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff)
{
  Spline* pSpline = W_NEW(&m_Allocator, Spline);
  pSpline->SetGlobalToLocalTransform(transform.GetAsMat4().GetInverse());
  pSpline->m_Type = ShapeType::Spline;
  pSpline->m_BlendMode = blendMode;
  pSpline->m_fValue = fValue;
  pSpline->m_uiSortingKey = WVolumeSampler::ComputeSortingKey(fSortOrder, transform.GetMaxScale());

  WSimdBBoxSphere bounds;
  spline.CalculateBounds(bounds).IgnoreResult();
  WSimdBBox box = bounds.GetBox();
  box.m_Min -= WSimdVec4f(fRadius);
  box.m_Max += WSimdVec4f(fRadius);

  pSpline->m_Spline = spline;
  pSpline->m_BoundingBox = WSimdConversion::ToBBox(box);
  pSpline->m_fInvRadius = 1.0f / WMath::Max(fRadius, 0.0001f);
  pSpline->m_fFadeOut = -1.0f / WMath::Max(fFalloff, 0.0001f);
  pSpline->m_fMaxError = WMath::Max(fRadius / 20.0f, 0.1f);

  m_SortedShapes.PushBack(pSpline);
}

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgExtractVolumes);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgExtractVolumes, 1, WRTTIDefaultAllocator<WMsgExtractVolumes>)
W_END_DYNAMIC_REFLECTED_TYPE;


W_STATICLINK_FILE(ProcGenPlugin, ProcGenPlugin_Components_Implementation_VolumeCollection);
