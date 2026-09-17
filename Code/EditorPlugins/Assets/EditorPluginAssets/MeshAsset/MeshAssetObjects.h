#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct WPropertyMetaStateEvent;

struct WMeshPrimitive
{
  using StorageType = WInt8;

  enum Enum
  {
    File,
    Box,
    Rect,
    Cylinder,
    Cone,
    Pyramid,
    Sphere,
    HalfSphere,
    GeodesicSphere,
    Capsule,
    Torus,

    Default = File
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WMeshPrimitive);

class WMeshAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WMeshAssetProperties, WReflectedClass);

public:
  WMeshAssetProperties();
  ~WMeshAssetProperties();

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WString m_sMeshFile;
  WString m_sMeshIncludeTags;
  WString m_sMeshExcludeTags;
  float m_fUniformScaling = 1.0f;

  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  WAngle m_Angle = WAngle::MakeFromDegree(360.0f);
  WUInt16 m_uiDetail = 0;
  WUInt16 m_uiDetail2 = 0;
  bool m_bCap = true;
  bool m_bCap2 = true;

  WEnum<WMeshImportTransform> m_ImportTransform;
  WEnum<WBasisAxis> m_RightDir = WBasisAxis::NegativeX;
  WEnum<WBasisAxis> m_UpDir = WBasisAxis::PositiveY;
  bool m_bFlipForwardDir = false;
  WVec3 m_vPositionOffset = WVec3::MakeZero();

  WMeshPrimitive::Enum m_PrimitiveType = WMeshPrimitive::Default;

  bool m_bRecalculateNormals = false;
  bool m_bRecalculateTangents = true;
  bool m_bImportMaterials = true;

  bool m_bHighPrecision = false;
  WEnum<WMeshVertexColorConversion> m_VertexColorConversion;

  WHybridArray<WMaterialResourceSlot, 8> m_Slots;

  WUInt32 m_uiVertices = 0;
  WUInt32 m_uiTriangles = 0;

  bool m_bSimplifyMesh = false;
  float m_fNormalWeight = 0.5f;
  bool m_bAggressiveSimplification = false;
  WUInt8 m_uiMeshSimplification = 50;
  WUInt8 m_uiMaxSimplificationError = 5;
};
