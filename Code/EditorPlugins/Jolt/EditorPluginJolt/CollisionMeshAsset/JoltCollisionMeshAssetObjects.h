#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/Declarations.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>

struct WPropertyMetaStateEvent;

struct WJoltSurfaceResourceSlot
{
  WString m_sLabel;
  WString m_sResource;
  bool m_bExclude = false;
};

struct WJoltCollisionMeshType
{
  using StorageType = WInt8;

  enum Enum
  {
    ConvexHull,
    TriangleMesh,
    Cylinder,

    Default = TriangleMesh
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WJoltCollisionMeshType);

struct WJoltConvexCollisionMeshType
{
  using StorageType = WInt8;

  enum Enum
  {
    ConvexHull,
    Cylinder,
    ConvexDecomposition,
    ConvexHullGroup,

    Default = ConvexHull
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WJoltConvexCollisionMeshType);

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WJoltSurfaceResourceSlot);

class WJoltCollisionMeshAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WJoltCollisionMeshAssetProperties, WReflectedClass);

public:
  WJoltCollisionMeshAssetProperties();
  ~WJoltCollisionMeshAssetProperties();

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WString m_sMeshFile;
  WString m_sMeshIncludeTags;
  WString m_sMeshExcludeTags;
  float m_fUniformScaling = 1.0f;
  WString m_sConvexMeshSurface;

  WEnum<WMeshImportTransform> m_ImportTransform;
  WEnum<WBasisAxis> m_RightDir = WBasisAxis::NegativeX;
  WEnum<WBasisAxis> m_UpDir = WBasisAxis::PositiveY;
  bool m_bFlipForwardDir = false;
  WVec3 m_vPositionOffset = WVec3::MakeZero();
  bool m_bIsConvexMesh = false;
  WEnum<WJoltConvexCollisionMeshType> m_ConvexMeshType;
  WUInt16 m_uiMaxConvexPieces = 2;

  // Cylinder
  float m_fRadius = 0.5f;
  float m_fRadius2 = 0.5f;
  float m_fHeight = 1.0f;
  WUInt8 m_uiDetail = 1;

  WHybridArray<WJoltSurfaceResourceSlot, 8> m_Slots;

  WUInt32 m_uiVertices = 0;
  WUInt32 m_uiTriangles = 0;

  bool m_bSimplifyMesh = false;
  float m_fNormalWeight = 0.5f;
  bool m_bAggressiveSimplification = false;
  WUInt8 m_uiMeshSimplification = 50;
  WUInt8 m_uiMaxSimplificationError = 10;
};
