#pragma once

#include <Core/Graphics/Spline.h>
#include <Core/World/World.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

using WImageDataResourceHandle = WTypedResourceHandle<class WImageDataResource>;

class W_PROCGENPLUGIN_DLL WVolumeCollection : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WVolumeCollection, WReflectedClass);

public:
  WVolumeCollection();
  ~WVolumeCollection();

  struct ShapeType
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Sphere,
      Box,
      Image,
      Spline,

      Default = Sphere
    };
  };

  struct Shape
  {
    WVec4 m_GlobalToLocalTransform0;
    WVec4 m_GlobalToLocalTransform1;
    WVec4 m_GlobalToLocalTransform2;
    WEnum<ShapeType> m_Type;
    WEnum<WProcGenBlendMode> m_BlendMode;
    WFloat16 m_fValue;
    WUInt32 m_uiSortingKey;

    W_ALWAYS_INLINE bool operator<(const Shape& other) const { return m_uiSortingKey < other.m_uiSortingKey; }

    void SetGlobalToLocalTransform(const WSimdMat4f& t);
    WSimdMat4f GetGlobalToLocalTransform() const;
  };

  struct Sphere : public Shape
  {
    W_DECLARE_POD_TYPE();

    float m_fFadeOut;
  };

  struct Box : public Shape
  {
    W_DECLARE_POD_TYPE();

    WVec3 m_vPositiveFadeOut;
    WVec3 m_vNegativeFadeOut;
  };

  struct Image : public Box
  {
    WImageDataResourceHandle m_hImage;
    const WColor* m_pPixelData = nullptr;
    WUInt32 m_uiImageWidth = 0;
    WUInt32 m_uiImageHeight = 0;
  };

  struct Spline : public Shape
  {
    WSpline m_Spline;
    WBoundingBox m_BoundingBox;
    float m_fInvRadius;
    float m_fFadeOut;
    float m_fMaxError;
  };

  bool IsEmpty() { return m_SortedShapes.IsEmpty(); }

  float EvaluateAtGlobalPosition(const WSimdVec4f& vPosition, float fInitialValue, WProcVolumeImageMode::Enum imgMode, const WColor& refColor) const;

  static void ExtractVolumesInBox(const WWorld& world, const WBoundingBox& box, WSpatialData::Category spatialCategory, const WTagSet& includeTags, WVolumeCollection& out_collection, const WRTTI* pComponentBaseType = nullptr);

  void AddSphere(const WSimdTransform& transform, float fRadius, WEnum<WProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff);

  void AddBox(const WSimdTransform& transform, const WVec3& vExtents, WEnum<WProcGenBlendMode> blendMode, float fSortOrder, float fValue, const WVec3& vPositiveFalloff, const WVec3& vNegativeFalloff, const WImageDataResourceHandle& hImage = {});

  void AddSpline(const WSimdTransform& transform, const WSpline& spline, float fRadius, WEnum<WProcGenBlendMode> blendMode, float fSortOrder, float fValue, float fFalloff);

private:
  WLinearAllocator<WAllocatorTrackingMode::Basics> m_Allocator;

  WDynamicArray<const Shape*> m_SortedShapes;
};

struct W_PROCGENPLUGIN_DLL WMsgExtractVolumes : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgExtractVolumes, WMessage);

  WVolumeCollection* m_pCollection = nullptr;
};
