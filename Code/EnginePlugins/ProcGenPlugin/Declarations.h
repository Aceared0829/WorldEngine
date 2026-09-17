#pragma once

#include <ProcGenPlugin/ProcGenPluginDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Declarations.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/SimdMath/SimdTransform.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/SharedPtr.h>

using WColorGradientResourceHandle = WTypedResourceHandle<class WColorGradientResource>;
using WPrefabResourceHandle = WTypedResourceHandle<class WPrefabResource>;
using WSurfaceResourceHandle = WTypedResourceHandle<class WSurfaceResource>;

struct WProcGenBinaryOperator
{
  using StorageType = WUInt8;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,

    Default = Multiply
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcGenBinaryOperator);

struct WProcGenBlendMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Add,
    Subtract,
    Multiply,
    Divide,
    Max,
    Min,
    Set,

    Default = Multiply
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcGenBlendMode);

struct WProcVertexColorChannelMapping
{
  using StorageType = WUInt8;

  enum Enum
  {
    R,
    G,
    B,
    A,
    Black,
    White,

    Default = R
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcVertexColorChannelMapping);

struct WProcVertexColorMapping
{
  WEnum<WProcVertexColorChannelMapping> m_R = WProcVertexColorChannelMapping::R;
  WEnum<WProcVertexColorChannelMapping> m_G = WProcVertexColorChannelMapping::G;
  WEnum<WProcVertexColorChannelMapping> m_B = WProcVertexColorChannelMapping::B;
  WEnum<WProcVertexColorChannelMapping> m_A = WProcVertexColorChannelMapping::A;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcVertexColorMapping);

struct WProcPlacementMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Raycast,
    RaycastHighQuality,
    Fixed,

    Default = Raycast
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcPlacementMode);

struct WProcPlacementPattern
{
  using StorageType = WUInt8;

  enum Enum
  {
    RegularGrid,
    HexGrid,
    Natural,

    COUNT,

    Default = Natural
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcPlacementPattern);

struct WProcVolumeImageMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    ReferenceColor,
    ChannelR,
    ChannelG,
    ChannelB,
    ChannelA,

    Default = ReferenceColor
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PROCGENPLUGIN_DLL, WProcVolumeImageMode);

//////////////////////////////////////////////////////////////////////////

namespace WProcGenInternal
{
  class PlacementTile;
  class FindPlacementTilesTask;
  class PreparePlacementTask;
  class PlacementTask;
  class VertexColorTask;
  struct PlacementData;

  struct InvalidatedArea
  {
    WBoundingBox m_Box;
    WWorld* m_pWorld = nullptr;
  };

  struct Pattern
  {
    struct Point
    {
      float x;
      float y;
      float threshold;
    };

    WArrayPtr<Point> m_Points;
    float m_fSize;
  };

  struct W_PROCGENPLUGIN_DLL GraphSharedDataBase : public WRefCounted
  {
    virtual ~GraphSharedDataBase();
  };

  struct W_PROCGENPLUGIN_DLL Output : public WRefCounted
  {
    virtual ~Output();

    WHashedString m_sName;

    WSmallArray<WUInt8, 4> m_VolumeTagSetIndices;
    WSmallArray<WUInt8, 4> m_CurveIndices;
    WSharedPtr<const GraphSharedDataBase> m_pGraphSharedData;

    WUniquePtr<WExpressionByteCode> m_pByteCode;
  };

  struct PlacementOutput : public Output
  {
    float GetTileSize() const { return m_pPattern->m_fSize * m_fFootprint; }

    bool IsValid() const
    {
      return !m_ObjectsToPlace.IsEmpty() && m_pPattern != nullptr && m_fFootprint > 0.0f && m_fCullDistance > 0.0f && m_pByteCode != nullptr;
    }

    WHybridArray<WPrefabResourceHandle, 4> m_ObjectsToPlace;

    const Pattern* m_pPattern = nullptr;
    float m_fFootprint = 1.0f;

    WVec3 m_vMinOffset = WVec3::MakeZero();
    WVec3 m_vMaxOffset = WVec3::MakeZero();

    WAngle m_YawRotationSnap = WAngle::MakeFromRadian(0.0f);
    float m_fAlignToNormal = 1.0f;

    WVec3 m_vMinScale = WVec3(1.0f);
    WVec3 m_vMaxScale = WVec3(1.0f);

    float m_fCullDistance = 30.0f;

    WUInt32 m_uiCollisionLayer = 0;

    WColorGradientResourceHandle m_hColorGradient;

    WSurfaceResourceHandle m_hSurface;

    WEnum<WProcPlacementMode> m_Mode;
    WUInt8 m_uiNumAdditionalRays = 4;
    float m_fRaySpread = 1.0f;
  };

  struct W_PROCGENPLUGIN_DLL VertexColorOutput : public Output
  {
  };

  struct W_PROCGENPLUGIN_DLL ExpressionInputs
  {
    static WHashedString s_sPosition;
    static WHashedString s_sPositionX;
    static WHashedString s_sPositionY;
    static WHashedString s_sPositionZ;
    static WHashedString s_sNormal;
    static WHashedString s_sNormalX;
    static WHashedString s_sNormalY;
    static WHashedString s_sNormalZ;
    static WHashedString s_sColor;
    static WHashedString s_sColorR;
    static WHashedString s_sColorG;
    static WHashedString s_sColorB;
    static WHashedString s_sColorA;
    static WHashedString s_sPointIndex;
  };

  struct W_PROCGENPLUGIN_DLL ExpressionOutputs
  {
    static WHashedString s_sOutDensity;
    static WHashedString s_sOutScale;
    static WHashedString s_sOutColorIndex;
    static WHashedString s_sOutObjectIndex;

    static WHashedString s_sOutColor;
    static WHashedString s_sOutColorR;
    static WHashedString s_sOutColorG;
    static WHashedString s_sOutColorB;
    static WHashedString s_sOutColorA;
  };

  struct PlacementPoint
  {
    W_DECLARE_POD_TYPE();

    WVec3 m_vPosition;
    float m_fScale;
    WVec3 m_vNormal;
    WUInt8 m_uiColorIndex;
    WUInt8 m_uiObjectIndex;
    WUInt16 m_uiPointIndex;
  };

  struct PlacementTransform
  {
    W_DECLARE_POD_TYPE();

    WSimdTransform m_Transform;
    WColorLinear16f m_ObjectColor;
    WUInt16 m_uiPointIndex;
    WUInt8 m_uiObjectIndex;
    bool m_bHasValidColor;
    WUInt32 m_uiPadding;
  };

  struct PlacementTileDesc
  {
    WComponentHandle m_hComponent;
    WUInt32 m_uiOutputIndex;
    WInt32 m_iPosX;
    WInt32 m_iPosY;
    float m_fMinZ;
    float m_fMaxZ;
    float m_fTileSize;
    float m_fDistanceToCamera;

    bool operator==(const PlacementTileDesc& other) const
    {
      return m_hComponent == other.m_hComponent && m_uiOutputIndex == other.m_uiOutputIndex && m_iPosX == other.m_iPosX && m_iPosY == other.m_iPosY;
    }

    WBoundingBox GetBoundingBox() const
    {
      WVec2 vCenter = WVec2(m_iPosX * m_fTileSize, m_iPosY * m_fTileSize);
      WVec3 vMin = (vCenter - WVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMinZ);
      WVec3 vMax = (vCenter + WVec2(m_fTileSize * 0.5f)).GetAsVec3(m_fMaxZ);

      return WBoundingBox::MakeFromMinMax(vMin, vMax);
    }

    WHybridArray<WSimdMat4f, 8, WAlignedAllocatorWrapper> m_GlobalToLocalBoxTransforms;
  };
} // namespace WProcGenInternal
