#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>

class WMeshResourceDescriptor;
class WGeometry;
class WMaterialAssetDocument;

namespace WModelImporter2
{
  class Importer;
}

class WMeshAssetDocument : public WSimpleAssetDocument<WMeshAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WMeshAssetDocument, WSimpleAssetDocument<WMeshAssetProperties>);

public:
  WMeshAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  void CreateMeshFromGeom(WMeshAssetProperties* pProp, WMeshResourceDescriptor& desc);

  WTransformStatus CreateMeshFromFile(WMeshAssetProperties* pProp, WMeshResourceDescriptor& desc, bool bAllowMaterialImport);

  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};

//////////////////////////////////////////////////////////////////////////

class WMeshAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WMeshAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WMeshAssetDocumentGenerator();
  ~WMeshAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WMeshAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Meshes"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;

protected:
  WMeshAssetDocumentGenerator(bool bAnimMesh);
  virtual WStatus ConfigureMeshDocument(WStringView sInputFile, WStringView sOutFile, WModelImporter2::Importer* pImporter, WArrayPtr<WMaterialResourceSlot> materials, WDynamicArray<WDocument*>& out_generatedDocuments);

  bool m_bAnimatedMesh = false;
  bool m_bShowImportDlg = true;
  static bool s_bReuseSkeleton;
  static bool s_bImportAllClips;
  static bool s_bUseSharedMaterials;
  static bool s_bCreateMaterials;
  static bool s_bAddLODs;
  static WUInt8 s_uiNumLODs;
  static WUuid s_SharedSkeleton;
};

class WAnimatedMeshAssetDocumentGenerator : public WMeshAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WAnimatedMeshAssetDocumentGenerator, WMeshAssetDocumentGenerator);

public:
  WAnimatedMeshAssetDocumentGenerator();
  ~WAnimatedMeshAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WAnimatedMeshAsset"; }

protected:
  virtual WStatus ConfigureMeshDocument(WStringView sInputFile, WStringView sOutFile, WModelImporter2::Importer* pImporter, WArrayPtr<WMaterialResourceSlot> materials, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
