#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>

class WGeometry;
struct WJoltMeshDesc;

class WJoltCollisionMeshAssetDocument : public WSimpleAssetDocument<WJoltCollisionMeshAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WJoltCollisionMeshAssetDocument, WSimpleAssetDocument<WJoltCollisionMeshAssetProperties>);

public:
  WJoltCollisionMeshAssetDocument(WStringView sDocumentPath, bool bConvexMesh);

protected:
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  WStatus CreateMeshFromFile(WJoltMeshDesc& outMesh);
  WStatus CreateMeshFromGeom(WGeometry& geom, WJoltMeshDesc& outMesh);
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  bool m_bIsConvexMesh = false;

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////


class WJoltCollisionMeshAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WJoltCollisionMeshAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WJoltCollisionMeshAssetDocumentGenerator();
  ~WJoltCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WJoltCollisionMeshAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};

class WJoltConvexCollisionMeshAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WJoltConvexCollisionMeshAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WJoltConvexCollisionMeshAssetDocumentGenerator();
  ~WJoltConvexCollisionMeshAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WJoltConvexCollisionMeshAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
