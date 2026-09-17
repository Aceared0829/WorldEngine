#pragma once

#include <ModelImporter2/ModelImporterDLL.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>

class WLogInterface;
class WProgress;
class WEditableSkeleton;
class WMeshResourceDescriptor;
struct WAnimationClipResourceDescriptor;

namespace WModelImporter2
{
  enum AdditiveReference
  {
    FirstKeyFrame,
    LastKeyFrame,
  };

  struct ImportOptions
  {
    WString m_sSourceFile;

    bool m_bImportSkinningData = false;
    bool m_bRecomputeNormals = false;
    bool m_bRecomputeTangents = false;
    bool m_bNormalizeWeights = false;
    WEnum<WMeshVertexColorConversion> m_MeshVertexColorConversion = WMeshVertexColorConversion::Default;
    bool m_bHighPrecision = false;
    WMat3 m_RootTransform = WMat3::MakeIdentity();
    /// Translation that is applied to the mesh vertices after m_RootTransform.
    /// Not applied to skinned meshes, animations or skeletons.
    WVec3 m_vRootPosition = WVec3::MakeZero();

    // if non-empty, only import meshes whose names start or end with any of these strings
    WDynamicArray<WString> m_MeshIncludeTags;
    // if non-empty, do not import meshes whose names start or end with any of these strings (unless already explicitly included)
    WDynamicArray<WString> m_MeshExcludeTags;

    WMeshResourceDescriptor* m_pMeshOutput = nullptr;

    WEditableSkeleton* m_pSkeletonOutput = nullptr;

    bool m_bAdditiveAnimation = false;
    AdditiveReference m_AdditiveReference = AdditiveReference::FirstKeyFrame;

    WString m_sAnimationToImport; // empty = first in file; "name" = only anim with that name
    WAnimationClipResourceDescriptor* m_pAnimationOutput = nullptr;
    WUInt32 m_uiFirstAnimKeyframe = 0;
    WUInt32 m_uiNumAnimKeyframes = 0;

    WUInt8 m_uiMeshSimplification = 0;
    WUInt8 m_uiMaxSimplificationError = 5;
    float m_fNormalWeight = 0.5f;
    bool m_bAggressiveSimplification = false;

    // Adjustments to deal with bad data:
    float m_fAnimationPositionScale = 1.0f;
  };

  enum class PropertySemantic : WInt8
  {
    Unknown = 0,

    DiffuseColor,
    RoughnessValue,
    MetallicValue,
    EmissiveColor,
    TwosidedValue,
  };

  enum class TextureSemantic : WInt8
  {
    Unknown = 0,

    DiffuseMap,
    DiffuseAlphaMap,
    OcclusionMap,
    RoughnessMap,
    MetallicMap,
    OrmMap,
    DisplacementMap,
    NormalMap,
    EmissiveMap,
  };

  struct W_MODELIMPORTER2_DLL OutputTexture
  {
    WString m_sFilename;
    WString m_sFileFormatExtension;
    WConstByteArrayPtr m_RawData;

    void GenerateFileName(WStringBuilder& out_sName) const;
  };

  struct W_MODELIMPORTER2_DLL OutputMaterial
  {
    WString m_sName;

    WInt32 m_iReferencedByMesh = -1;                     // if -1, no sub-mesh in the output actually references this
    WMap<TextureSemantic, WString> m_TextureReferences; // semantic -> path
    WMap<PropertySemantic, WVariant> m_Properties;      // semantic -> value
  };

  class W_MODELIMPORTER2_DLL Importer
  {
  public:
    Importer();
    virtual ~Importer();

    WResult Import(const ImportOptions& options, WLogInterface* pLogInterface = nullptr, WProgress* pProgress = nullptr);
    const ImportOptions& GetImportOptions() const { return m_Options; }

    WMap<WString, OutputTexture> m_OutputTextures; // path -> additional data
    WDeque<OutputMaterial> m_OutputMaterials;
    WDynamicArray<WString> m_OutputAnimationNames;
    WDynamicArray<WString> m_OutputMeshNames;

  protected:
    virtual WResult DoImport() = 0;

    ImportOptions m_Options;
    WProgress* m_pProgress = nullptr;
  };

} // namespace WModelImporter2
