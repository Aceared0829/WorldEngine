#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/Util/AssetUtils.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class WMeshResourceDescriptor;
class WAssetInfoFile;

namespace WModelImporter2
{
  class Importer;
  enum class TextureSemantic : WInt8;
} // namespace WModelImporter2

namespace WMeshImportUtils
{
  W_EDITORPLUGINASSETS_DLL WString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, WStringView sTexturePath, WModelImporter2::TextureSemantic hint, bool bTextureClamp, const WModelImporter2::Importer* pImporter);

  W_EDITORPLUGINASSETS_DLL void SetMeshAssetMaterialSlots(WDynamicArray<WMaterialResourceSlot>& inout_materialSlots, const WModelImporter2::Importer* pImporter);
  W_EDITORPLUGINASSETS_DLL void CopyMeshAssetMaterialSlotToResource(WMeshResourceDescriptor& ref_desc, const WArrayPtr<WMaterialResourceSlot>& materialSlots);
  W_EDITORPLUGINASSETS_DLL void ImportMeshAssetMaterials(WDynamicArray<WMaterialResourceSlot>& inout_materialSlots, WStringView sDocumentDirectory, const WModelImporter2::Importer* pImporter);

  /// Records vertex and triangle counts, sub-mesh count and the bounding box of a transformed mesh.
  ///
  /// Call this after the descriptor has been saved, because saving computes the bounds if they were not set explicitly.
  W_EDITORPLUGINASSETS_DLL void RecordMeshTransformInfo(WAssetInfoFile& ref_info, const WMeshResourceDescriptor& desc);

  /// Records the names of the meshes that the source file contains.
  W_EDITORPLUGINASSETS_DLL void RecordAvailableMeshes(WAssetInfoFile& ref_info, const WModelImporter2::Importer* pImporter);

  /// Extracts external buffer file dependencies from a glTF file and adds them to the transform dependencies set.
  ///
  /// Parses the glTF JSON to find all external buffer files referenced in the "buffers" array.
  /// Skips embedded data URIs and only processes external file references.
  /// All referenced buffer files are converted to data directory relative paths before being added.
  ///
  /// \param sMeshFile Data directory relative path to the glTF file
  /// \param inout_dependencies Set to add the buffer file dependencies to
  W_EDITORPLUGINASSETS_DLL void AddGltfBufferDependencies(WStringView sMeshFile, WSet<WString>& inout_dependencies);
} // namespace WMeshImportUtils
