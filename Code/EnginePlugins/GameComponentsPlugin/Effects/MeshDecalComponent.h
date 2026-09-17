#pragma once

#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <RendererCore/Declarations.h>

struct WMsgExtractRenderData;

struct W_GAMECOMPONENTS_DLL WMeshDecalDescription
{
  WUInt16 m_uiIndex = 0;
  WTexture2DResourceHandle m_hBaseColorTexture;

  WResult Serialize(WStreamWriter& inout_stream) const;
  WResult Deserialize(WStreamReader& inout_stream);
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMECOMPONENTS_DLL, WMeshDecalDescription);

////////////////////////////////////////////////////////////////////////////

using WMeshDecalComponentManager = WComponentManager<class WMeshDecalComponent, WBlockStorageType::Compact>;

/// A component that takes a couple of decal textures, picks a random one for each slot,
/// adds them to runtime decal atlas and sends a custom data message with the corresponding decal indices.
///
/// The decals in this case are not regular projected decals, but rather mesh decals aka floaters.
/// This requires a special shader to be used which uses the custom data in the instance data to map the UV coordinates to the decal atlas.
/// See "Data/Samples/Testing Chambers/Materials/MeshDecalMaterial.WShader" for an example shader.
class W_GAMECOMPONENTS_DLL WMeshDecalComponent final : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WMeshDecalComponent, WComponent, WMeshDecalComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  void OnMsgExtractRenderData(WMsgExtractRenderData& msg) const;

private:
  WUInt32 Decals_GetCount() const;
  const WMeshDecalDescription& Decals_Get(WUInt32 uiIndex) const;
  void Decals_Set(WUInt32 uiIndex, const WMeshDecalDescription& desc);
  void Decals_Insert(WUInt32 uiIndex, const WMeshDecalDescription& desc);
  void Decals_Remove(WUInt32 uiIndex);

  void UpdateDecals();
  void DeleteDecals();

  WSmallArray<WMeshDecalDescription, 2> m_DecalDescs;
  WSmallArray<WDecalId, 2> m_DecalIds;
};
