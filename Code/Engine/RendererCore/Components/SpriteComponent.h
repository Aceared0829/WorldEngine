#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/World.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Time/Time.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

struct WMsgSetColor;
using WTexture2DResourceHandle = WTypedResourceHandle<class WTexture2DResource>;

struct WSpriteBlendMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Masked,
    Transparent,
    Additive,
    ShapeIcon,

    Default = Masked
  };

  static WTempHashedString GetPermutationValue(Enum blendMode);
};

W_DECLARE_REFLECTABLE_TYPE(W_RENDERERCORE_DLL, WSpriteBlendMode);

class W_RENDERERCORE_DLL WSpriteRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WSpriteRenderData, WRenderData);

public:
  void FillSortingKey();
  virtual bool CanBatch(const WRenderData& other) const override;

  WTexture2DResourceHandle m_hTexture;
  WEnum<WSpriteBlendMode> m_BlendMode;

  float m_fSize;
  float m_fMaxScreenSize;
  float m_fAspectRatio;

  WColorLinear16f m_color;

  WFloat16Vec2 m_texCoordScale;
  WFloat16Vec2 m_texCoordOffset;

  WUInt32 m_uiUniqueID;
};

using WSpriteComponentManager = WComponentManagerSimple<class WSpriteComponent, WComponentUpdateType::Always, WBlockStorageType::Compact>;

/// Renders a screen-oriented quad (billboard) with a maximum screen size.
///
/// This component is typically used to attach an icon to a game object.
/// The sprite becomes smaller the farther away it is, but when you come closer, its screen size gets clamped to a fixed maximum.
///
/// It can also be used to render simple projectiles.
///
/// If you want to render a glow effect for a lightsource, use the WLensFlareComponent instead.
class W_RENDERERCORE_DLL WSpriteComponent : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WSpriteComponent, WRenderComponent, WSpriteComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

public:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, WMsgUpdateLocalBounds& ref_msg) override;

  //////////////////////////////////////////////////////////////////////////
  // WSpriteComponent

public:
  WSpriteComponent();
  ~WSpriteComponent();

  void Update();

  void SetTexture(const WTexture2DResourceHandle& hTexture); // [ property ]
  const WTexture2DResourceHandle& GetTexture() const;        // [ property ]

  void SetColor(WColor color);                               // [ property ]
  WColor GetColor() const;                                   // [ property ]

  /// Sets the size of the sprite in world-space units. This determines how large the sprite will be at certain distances.
  void SetSize(float fSize); // [ property ]
  float GetSize() const;     // [ property ]

  /// Sets the maximum screen-space size in pixels. Once a sprite is close enough to have reached this size, it will not grow larger.
  void SetMaxScreenSize(float fSize);                             // [ property ]
  float GetMaxScreenSize() const;                                 // [ property ]

private:
  void OnMsgSetColor(WMsgSetColor& ref_msg);                     // [ msg handler ]
  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg);         // [ msg handler ]
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const; // [ msg handler ]

  WTexture2DResourceHandle m_hTexture;
  WColor m_Color = WColor::White;

  WEnum<WSpriteBlendMode> m_BlendMode;
  WEnum<WOnComponentFinishedAction> m_OnFinishedAction = WOnComponentFinishedAction::Default;
  bool m_bUseMaxScreenSize = true;
  bool m_bIsAnimated = false;

  float m_fSize = 1.0f;
  float m_fMaxScreenSize = 64.0f;
  float m_fAspectRatio = 1.0f;

  float m_fFramerate = 24.0f;

  WUInt32 m_uiLoops = 0;

  WTime m_TimeSinceStart;
  WUInt32 m_uiCurrentLoop = 0;

  WUInt8 m_uiColumns = 1;
  WUInt8 m_uiRows = 1;
};
