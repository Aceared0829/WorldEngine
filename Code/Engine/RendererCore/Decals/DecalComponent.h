#pragma once

#include <Foundation/Math/Color16f.h>
#include <Foundation/Types/VarianceTypes.h>
#include <RendererCore/Components/RenderComponent.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Pipeline/RenderData.h>

class WAbstractObjectNode;
struct WMsgComponentInternalTrigger;
struct WMsgOnlyApplyToObject;
struct WMsgSetColor;

using WDecalComponentManager = WComponentManager<class WDecalComponent, WBlockStorageType::Compact>;

class W_RENDERERCORE_DLL WDecalRenderData : public WRenderData
{
  W_ADD_DYNAMIC_REFLECTION(WDecalRenderData, WRenderData);

public:
  WFloat16Vec4 m_qGlobalRotation;
  WFloat16Vec3 m_vGlobalScale;

  WUInt32 m_uiApplyOnlyToId;
  WUInt32 m_uiFlags;
  WUInt32 m_uiAngleFadeParams;

  WColorLinearUB m_BaseColor;
  WColorLinear16f m_EmissiveColor;

  WUInt32 m_uiBaseColorAtlasScale;
  WUInt32 m_uiBaseColorAtlasOffset;

  WUInt32 m_uiNormalAtlasScale;
  WUInt32 m_uiNormalAtlasOffset;

  WUInt32 m_uiORMAtlasScale;
  WUInt32 m_uiORMAtlasOffset;
};

/// Projects a decal texture onto geometry within a box volume.
///
/// This is used to add dirt, scratches, signs and other surface imperfections to geometry.
/// The component uses a box shape to define the position and volume and projection direction.
/// This can be set up in a level to add detail, but it can also be used by dynamic effects such as bullet hits,
/// to visualize the impact. To add variety a prefab may use different textures and vary in size.
class W_RENDERERCORE_DLL WDecalComponent final : public WRenderComponent
{
  W_DECLARE_COMPONENT_TYPE(WDecalComponent, WRenderComponent, WDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnActivated() override;

  //////////////////////////////////////////////////////////////////////////
  // WRenderComponent

protected:
  virtual WResult GetLocalBounds(WBoundingBoxSphere& bounds, bool& bAlwaysVisible, WMsgUpdateLocalBounds& msg) override;
  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;


  //////////////////////////////////////////////////////////////////////////
  // WDecalComponent

public:
  WDecalComponent();
  ~WDecalComponent();

  /// Sets the extents of the box inside which to project the decal.
  void SetExtents(const WVec3& value); // [ property ]
  const WVec3& GetExtents() const;     // [ property ]

  /// The size variance defines how much the size may randomly deviate, such that the decals look different.
  void SetSizeVariance(float fVariance); // [ property ]
  float GetSizeVariance() const;         // [ property ]

  /// An additional tint color for the decal.
  void SetColor(WColorGammaUB color); // [ property ]
  WColorGammaUB GetColor() const;     // [ property ]

  /// An additional emissive color to make the decal glow.
  void SetEmissiveColor(WColor color); // [ property ]
  WColor GetEmissiveColor() const;     // [ property ]

  /// At which angle between the decal orientation and the surface it is projected onto, to start fading the decal out.
  void SetInnerFadeAngle(WAngle fadeAngle); // [ property ]
  WAngle GetInnerFadeAngle() const;         // [ property ]

  /// At which angle between the decal orientation and the surface it is projected onto, to fully fade out the decal.
  void SetOuterFadeAngle(WAngle fadeAngle); // [ property ]
  WAngle GetOuterFadeAngle() const;         // [ property ]

  /// If multiple decals are in the same location, this allows to tweak which one is rendered on top.
  void SetSortOrder(float fOrder); // [ property ]
  float GetSortOrder() const;      // [ property ]

  /// Whether the decal projection should use a kind of three-way texture mapping to wrap the image around curved geometry.
  void SetWrapAround(bool bWrapAround);         // [ property ]
  bool GetWrapAround() const;                   // [ property ]

  void SetMapNormalToGeometry(bool bMapNormal); // [ property ]
  bool GetMapNormalToGeometry() const;          // [ property ]

  /// Sets the decal resource to use. If more than one is set, a random one will be chosen.
  ///
  /// Indices that are written to will be created on-demand.
  void SetDecal(WUInt32 uiIndex, const WDecalResourceHandle& hResource); // [ property ]
  const WDecalResourceHandle& GetDecal(WUInt32 uiIndex) const;           // [ property ]

  /// Selects which variation of the decal texture to display.
  ///
  /// Only has an effect if the decal asset declares that its texture contains a grid of variations.
  /// Zero means that a variation is chosen randomly, based on the owner object's stable random seed.
  /// Otherwise the value is a one-based index into the grid, values that exceed the number of
  /// available variations wrap around.
  void SetVariation(WInt8 iVariation); // [ property ]
  WInt8 GetVariation() const;          // [ property ]

  /// If non-zero, the decal fades out after this time and then vanishes.
  WVarianceTypeTime m_FadeOutDelay; // [ property ]

  /// How much time the fade out takes.
  WTime m_FadeOutDuration; // [ property ]

  /// If fade-out is used, the decal may delete itself afterwards.
  WEnum<WOnComponentFinishedAction> m_OnFinishedAction; // [ property ]

  /// Sets the cardinal axis into which the decal projection should be.
  void SetProjectionAxis(WEnum<WBasisAxis> projectionAxis); // [ property ]
  WEnum<WBasisAxis> GetProjectionAxis() const;              // [ property ]

  /// If set, the decal only appears on the given object.
  ///
  /// This is typically used to limit the decal to a single dynamic object, such that damage decals don't project
  /// onto static geometry and other objects.
  void SetApplyOnlyTo(WGameObjectHandle hObject);
  WGameObjectHandle GetApplyOnlyTo() const;

  // TODO: Using WStringView for the array accessors doesn't work (currently)

  WUInt32 DecalFile_GetCount() const;                     // [ property ]
  WString DecalFile_Get(WUInt32 uiIndex) const;          // [ property ]
  void DecalFile_Set(WUInt32 uiIndex, WString sFile);    // [ property ]
  void DecalFile_Insert(WUInt32 uiIndex, WString sFile); // [ property ]
  void DecalFile_Remove(WUInt32 uiIndex);                 // [ property ]


protected:
  void SetApplyToRef(const char* szReference); // [ property ]
  void UpdateApplyTo();

  void OnTriggered(WMsgComponentInternalTrigger& msg);
  void OnMsgDeleteGameObject(WMsgDeleteGameObject& msg);
  void OnMsgOnlyApplyToObject(WMsgOnlyApplyToObject& msg);
  void OnMsgSetColor(WMsgSetColor& ref_msg);

  WVec3 m_vExtents = WVec3(1.0f);
  float m_fSizeVariance = 0;
  WColorGammaUB m_Color = WColor::White;
  WColor m_EmissiveColor = WColor::Black;
  WAngle m_InnerFadeAngle = WAngle::MakeFromDegree(50.0f);
  WAngle m_OuterFadeAngle = WAngle::MakeFromDegree(80.0f);
  float m_fSortOrder = 0;
  bool m_bWrapAround = false;
  bool m_bMapNormalToGeometry = false;
  WUInt8 m_uiRandomDecalIdx = 0xFF;
  WInt8 m_iVariation = 0;
  WUInt8 m_uiRandomVariationIdx = 0;
  WEnum<WBasisAxis> m_ProjectionAxis;
  WHybridArray<WDecalResourceHandle, 1> m_Decals;

  WGameObjectHandle m_hApplyOnlyToObject;
  WUInt32 m_uiApplyOnlyToId = 0;

  WTime m_StartFadeOutTime;
  WUInt32 m_uiInternalSortKey = 0;

private:
  const char* DummyGetter() const { return nullptr; }
};
