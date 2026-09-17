#pragma once

#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

struct WPropertyMetaStateEvent;

class WAnimatedMeshAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAnimatedMeshAssetProperties, WReflectedClass);

public:
  WAnimatedMeshAssetProperties();
  ~WAnimatedMeshAssetProperties();

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WString m_sMeshFile;
  WString m_sMeshIncludeTags;
  WString m_sMeshExcludeTags;
  WString m_sDefaultSkeleton;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTangents = true;
  bool m_bNormalizeWeights = false;
  bool m_bImportMaterials = true;

  bool m_bHighPrecision = false;
  WEnum<WMeshVertexColorConversion> m_VertexColorConversion;

  WHybridArray<WMaterialResourceSlot, 8> m_Slots;

  bool m_bSimplifyMesh = false;
  float m_fNormalWeight = 0.5f;
  bool m_bAggressiveSimplification = false;
  WUInt8 m_uiMeshSimplification = 50;
  WUInt8 m_uiMaxSimplificationError = 5;
};
