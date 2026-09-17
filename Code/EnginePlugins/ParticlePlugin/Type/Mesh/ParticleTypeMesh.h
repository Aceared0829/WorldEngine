#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererFoundation/RendererFoundationDLL.h>

using WMeshResourceHandle = WTypedResourceHandle<class WMeshResource>;
using WMaterialResourceHandle = WTypedResourceHandle<class WMaterialResource>;

/// Factory for creating mesh particle types.
class W_PARTICLEPLUGIN_DLL WParticleTypeMeshFactory final : public WParticleTypeFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeMeshFactory, WParticleTypeFactory);

public:
  virtual const WRTTI* GetTypeType() const override;
  virtual void CopyTypeProperties(WParticleType* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WHashedString m_sMesh;
  WHashedString m_sMaterial;
  float m_fScale = 1.0f;
  WHashedString m_sTintColorParameter;

  WMeshResourceHandle m_hMesh;
  WMaterialResourceHandle m_hMaterial;
};

/// Renders particles as instanced 3D meshes.
///
/// Each particle renders a full 3D mesh at its position, oriented according to
/// its rotation axis. Materials can be overridden globally or use the mesh's
/// default materials. Uses instanced rendering for performance when many particles
/// share the same mesh.
class W_PARTICLEPLUGIN_DLL WParticleTypeMesh final : public WParticleType
{
  W_ADD_DYNAMIC_REFLECTION(WParticleTypeMesh, WParticleType);

public:
  WParticleTypeMesh();
  ~WParticleTypeMesh();

  virtual void CreateRequiredStreams() override;

  WMeshResourceHandle m_hMesh;
  mutable WMaterialResourceHandle m_hMaterial;
  float m_fScale = 1.0f;
  WTempHashedString m_sTintColorParameter;

  virtual void ExtractTypeRenderData(WMsgExtractRenderData& ref_msg, const WTransform& instanceTransform) const override;

protected:
  friend class WParticleTypeMeshFactory;

  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
  virtual void Process(WUInt64 uiNumElements) override {}

  /// Queries and caches mesh and material information from resources.
  bool QueryMeshAndMaterialInfo() const;

  void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) override;

  WRenderDataManager* m_pRenderDataManager = nullptr;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamSize = nullptr;
  WProcessingStream* m_pStreamColor = nullptr;
  WProcessingStream* m_pStreamRotationSpeed = nullptr;
  WProcessingStream* m_pStreamRotationOffset = nullptr;
  WProcessingStream* m_pStreamAxis = nullptr;
  WProcessingStream* m_pStreamVariation = nullptr;

  mutable bool m_bRenderDataCached = false;
  mutable WRenderData::Category m_RenderCategory;
  mutable WInstanceDataOffset m_InstanceDataOffset;
  mutable WUInt8 m_uiNumSubMeshes = 0;
  mutable WDynamicArray<WMaterialResourceHandle> m_CachedSubMeshMaterials;
  bool m_bMaterialOverride = false;
};
