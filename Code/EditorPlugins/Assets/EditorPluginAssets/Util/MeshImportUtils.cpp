#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/Util/MeshImportUtils.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <Foundation/Utilities/Progress.h>
#include <ModelImporter2/Importer/Importer.h>
#include <RendererCore/Meshes/MeshResourceDescriptor.h>

namespace WMeshImportUtils
{
  void FillFileFilter(WDynamicArray<WString>& out_list, WStringView sSeparated)
  {
    sSeparated.Split(false, out_list, ";", "*", ".");
  }

  static WStringView TextureSemanticToString(WModelImporter2::TextureSemantic semantic)
  {
    switch (semantic)
    {
      case WModelImporter2::TextureSemantic::DiffuseMap:
        return "diffuse";
      case WModelImporter2::TextureSemantic::DiffuseAlphaMap:
        return "diffuse+alpha";
      case WModelImporter2::TextureSemantic::OcclusionMap:
        return "occlusion";
      case WModelImporter2::TextureSemantic::RoughnessMap:
        return "roughness";
      case WModelImporter2::TextureSemantic::MetallicMap:
        return "metallic";
      case WModelImporter2::TextureSemantic::OrmMap:
        return "ORM";
      case WModelImporter2::TextureSemantic::DisplacementMap:
        return "displacement";
      case WModelImporter2::TextureSemantic::NormalMap:
        return "normal";
      case WModelImporter2::TextureSemantic::EmissiveMap:
        return "emissive";
      default:
        return "unknown";
    }
  }

  WString ImportOrResolveTexture(const char* szImportSourceFolder, const char* szImportTargetFolder, WStringView sTexturePath, WModelImporter2::TextureSemantic hint, bool bTextureClamp, const WModelImporter2::Importer* pImporter)
  {
    if (!WUnicodeUtils::IsValidUtf8(sTexturePath.GetStartPointer(), sTexturePath.GetEndPointer()))
    {
      WLog::Error("Texture to resolve is not a valid UTF-8 string.");
      return WString();
    }

    WTempHybridArray<WString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, WFileBrowserAttribute::ImagesLdrAndHdr);

    WStringBuilder sFinalTextureName;
    WPathUtils::MakeValidFilename(sTexturePath.GetFileName(), '_', sFinalTextureName);

    WStringBuilder relTexturePath = szImportSourceFolder;
    relTexturePath.AppendPath(sFinalTextureName);

    if (auto itTex = pImporter->m_OutputTextures.Find(sTexturePath); itTex.IsValid())
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        WLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        return WString();
      }

      itTex.Value().GenerateFileName(sFinalTextureName);

      WStringBuilder sEmbededFile;
      sEmbededFile = szImportTargetFolder;
      sEmbededFile.AppendPath(sFinalTextureName);

      relTexturePath = sEmbededFile;
    }

    WStringBuilder newAssetPathAbs = szImportTargetFolder;
    newAssetPathAbs.AppendPath(sFinalTextureName);
    newAssetPathAbs.ChangeFileExtension("WTextureAsset");

    if (auto textureAssetInfo = WAssetCurator::GetSingleton()->FindSubAsset(newAssetPathAbs))
    {
      // Try to resolve.

      WStringBuilder guidString;
      return WConversionUtils::ToString(textureAssetInfo->m_Data.m_Guid, guidString);
    }
    else
    {
      // Import otherwise

      WTextureAssetDocument* textureDocument = WDynamicCast<WTextureAssetDocument*>(WQtEditorApp::GetSingleton()->CreateDocument(newAssetPathAbs, WDocumentFlags::None));
      if (!textureDocument)
      {
        WLog::Error("Failed to create new texture asset '{0}'", sTexturePath);
        return sFinalTextureName;
      }

      WObjectAccessorBase* pAccessor = textureDocument->GetObjectAccessor();
      pAccessor->StartTransaction("Import Texture");
      WDocumentObject* pTextureAsset = textureDocument->GetPropertyObject();

      WStringBuilder sOriginalReference = szImportSourceFolder;
      sOriginalReference.AppendPath(sTexturePath);
      sOriginalReference.MakeCleanPath();

      if (WAssetCurator::GetSingleton()->FindBestMatchForFile(relTexturePath, allowedExtensions).Failed())
      {
        relTexturePath = sOriginalReference;
        WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(relTexturePath);

        WLog::Warning("Could not find the {} texture '{}' referenced by the mesh, in any of the supported image formats. The texture asset is created pointing at '{}', which does not exist, so it has to be corrected manually.", TextureSemanticToString(hint), sTexturePath, relTexturePath);
      }

      pAccessor->SetValueByName(pTextureAsset, "Input1", relTexturePath.GetData()).LogFailure();

      WEnum<WTexture2DChannelMappingEnum> channelMapping;

      // Try to map usage.
      WEnum<WTexConvUsage> usage;
      switch (hint)
      {
        case WModelImporter2::TextureSemantic::DiffuseMap:
          usage = WTexConvUsage::Color;
          break;

        case WModelImporter2::TextureSemantic::DiffuseAlphaMap:
          usage = WTexConvUsage::Color;
          channelMapping = WTexture2DChannelMappingEnum::RGBA1;
          break;
        case WModelImporter2::TextureSemantic::OcclusionMap: // Making wild guesses here.
        case WModelImporter2::TextureSemantic::EmissiveMap:
          usage = WTexConvUsage::Color;
          break;

        case WModelImporter2::TextureSemantic::RoughnessMap:
        case WModelImporter2::TextureSemantic::MetallicMap:
          channelMapping = WTexture2DChannelMappingEnum::R1;
          usage = WTexConvUsage::Linear;
          break;

        case WModelImporter2::TextureSemantic::OrmMap:
          channelMapping = WTexture2DChannelMappingEnum::RGB1;
          usage = WTexConvUsage::Linear;
          break;

        case WModelImporter2::TextureSemantic::NormalMap:
          usage = WTexConvUsage::NormalMap;
          break;

        case WModelImporter2::TextureSemantic::DisplacementMap:
          usage = WTexConvUsage::Linear;
          channelMapping = WTexture2DChannelMappingEnum::R1;
          break;

        default:
          usage = WTexConvUsage::Auto;
      }

      pAccessor->SetValueByName(pTextureAsset, "Usage", usage.GetValue()).LogFailure();
      pAccessor->SetValueByName(pTextureAsset, "ChannelMapping", channelMapping.GetValue()).LogFailure();

      if (bTextureClamp)
      {
        pAccessor->SetValueByName(pTextureAsset, "AddressModeU", (int)WImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValueByName(pTextureAsset, "AddressModeV", (int)WImageAddressMode::Clamp).LogFailure();
        pAccessor->SetValueByName(pTextureAsset, "AddressModeW", (int)WImageAddressMode::Clamp).LogFailure();
      }

      pAccessor->FinishTransaction();
      textureDocument->SaveDocument().LogFailure();

      WStringBuilder guid;
      WConversionUtils::ToString(textureDocument->GetGuid(), guid);
      textureDocument->GetDocumentManager()->CloseDocument(textureDocument);

      WLog::Success("Imported texture: '{}'", newAssetPathAbs);

      return guid;
    }
  };

  void SetMeshAssetMaterialSlots(WDynamicArray<WMaterialResourceSlot>& inout_materialSlots, const WModelImporter2::Importer* pImporter)
  {
    const auto& opt = pImporter->GetImportOptions();

    const WUInt32 uiNumSubmeshes = opt.m_pMeshOutput->GetSubMeshes().GetCount();

    inout_materialSlots.SetCount(uiNumSubmeshes);

    for (const auto& material : pImporter->m_OutputMaterials)
    {
      if (material.m_iReferencedByMesh < 0)
        continue;

      inout_materialSlots[material.m_iReferencedByMesh].m_sLabel = material.m_sName;
    }
  }

  void CopyMeshAssetMaterialSlotToResource(WMeshResourceDescriptor& ref_desc, const WArrayPtr<WMaterialResourceSlot>& materialSlots)
  {
    for (WUInt32 i = 0; i < materialSlots.GetCount(); ++i)
    {
      ref_desc.SetMaterial(i, materialSlots[i].m_sResource);
    }
  }

  void RecordMeshTransformInfo(WAssetInfoFile& ref_info, const WMeshResourceDescriptor& desc)
  {
    const auto& mbd = desc.MeshBufferDesc();

    ref_info.SetValue(WAssetInfoFile::Keys::NumVertices, mbd.GetVertexCount());
    ref_info.SetValue(WAssetInfoFile::Keys::NumTriangles, mbd.GetPrimitiveCount());
    ref_info.SetValue(WAssetInfoFile::Keys::NumSubMeshes, desc.GetSubMeshes().GetCount());

    const WBoundingBoxSphere& bounds = desc.GetBounds();

    if (bounds.IsValid())
    {
      ref_info.SetValue(WAssetInfoFile::Keys::BoundsCenter, bounds.m_vCenter);
      ref_info.SetValue(WAssetInfoFile::Keys::BoundsHalfExtents, bounds.m_vBoxHalfExtents);
      ref_info.SetValue(WAssetInfoFile::Keys::BoundsRadius, bounds.m_fSphereRadius);
    }
  }

  void RecordAvailableMeshes(WAssetInfoFile& ref_info, const WModelImporter2::Importer* pImporter)
  {
    if (pImporter == nullptr || pImporter->m_OutputMeshNames.IsEmpty())
      return;

    WVariantArray meshNames;
    meshNames.Reserve(pImporter->m_OutputMeshNames.GetCount());

    for (const auto& sName : pImporter->m_OutputMeshNames)
    {
      meshNames.PushBack(WVariant(sName));
    }

    ref_info.SetValue(WAssetInfoFile::Keys::AvailableMeshes, WVariant(meshNames));
  }

  static void ImportMeshAssetMaterialProperties(WMaterialAssetDocument* pMaterialDoc, const WModelImporter2::OutputMaterial& material, const char* szImportSourceFolder, const char* szImportTargetFolder, const WModelImporter2::Importer* pImporter)
  {
    WStringBuilder materialName = WPathUtils::GetFileName(pMaterialDoc->GetDocumentPath());

    W_LOG_BLOCK("Apply Material Settings", materialName.GetData());

    WObjectAccessorBase* pAccessor = pMaterialDoc->GetObjectAccessor();
    pAccessor->StartTransaction("Apply Material Settings");
    WDocumentObject* pMaterialAsset = pMaterialDoc->GetPropertyObject();

    WStringBuilder tmp;

    // Set base material.
    WStatus res = pAccessor->SetValueByName(pMaterialAsset, "BaseMaterial", WConversionUtils::ToString(WMaterialAssetDocument::GetLitBaseMaterial(), tmp).GetData());
    res.LogFailure();
    if (res.Failed())
    {
      pAccessor->CancelTransaction();
      return;
    }

    // From now on we're setting shader properties.
    WDocumentObject* pMaterialProperties = pMaterialDoc->GetShaderPropertyObject();

    WVariant propertyValue;

    WString textureAo, textureRoughness, textureMetallic;
    material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::OcclusionMap, textureAo);
    material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::RoughnessMap, textureRoughness);
    material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::MetallicMap, textureMetallic);

    const bool bHasOrmTexture = !textureRoughness.IsEmpty() && ((textureAo == textureRoughness) || (textureMetallic == textureRoughness));


    // Set base texture.
    {
      WString textureDiffuse;

      if (material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::DiffuseMap, textureDiffuse))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseBaseTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "BaseTexture", WVariant(WMeshImportUtils::ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureDiffuse, WModelImporter2::TextureSemantic::DiffuseMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseBaseTexture", false).LogFailure();
      }
    }

    // Set Normal Texture / Roughness Texture
    {
      WString textureNormal;

      if (!material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::NormalMap, textureNormal))
      {
        // Due to the lack of options in stuff like obj files, people stuff normals into the bump slot.
        material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::DisplacementMap, textureNormal);
      }

      if (!textureNormal.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseNormalTexture", true).LogFailure();

        pAccessor->SetValueByName(pMaterialProperties, "NormalTexture", WVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureNormal, WModelImporter2::TextureSemantic::NormalMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "NormalTexture", WConversionUtils::ToString(WMaterialAssetDocument::GetNeutralNormalMap(), tmp).GetData()).LogFailure();
      }
    }

    if (!bHasOrmTexture)
    {
      if (!textureRoughness.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseRoughnessTexture", true).LogFailure();

        pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", WVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, WModelImporter2::TextureSemantic::RoughnessMap, false, pImporter))).LogFailure();
      }
      else
      {
        pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", "White.color").LogFailure();
      }
    }

    // Set metallic texture
    if (!bHasOrmTexture)
    {
      if (!textureMetallic.IsEmpty())
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseMetallicTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "MetallicTexture", WVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureMetallic, WModelImporter2::TextureSemantic::MetallicMap, false, pImporter))).LogFailure();
      }
    }

    // Set emissive texture
    {
      WString textureEmissive;

      if (material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::EmissiveMap, textureEmissive))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseEmissiveTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "EmissiveTexture", WVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureEmissive, WModelImporter2::TextureSemantic::EmissiveMap, false, pImporter))).LogFailure();
      }
    }

    // Set AO texture
    if (!bHasOrmTexture)
    {
      WString textureAo;

      if (material.m_TextureReferences.TryGetValue(WModelImporter2::TextureSemantic::OcclusionMap, textureAo))
      {
        pAccessor->SetValueByName(pMaterialProperties, "UseOcclusionTexture", true).LogFailure();
        pAccessor->SetValueByName(pMaterialProperties, "OcclusionTexture", WVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureAo, WModelImporter2::TextureSemantic::OcclusionMap, false, pImporter))).LogFailure();
      }
    }

    // TODO: ambient occlusion texture

    // Set base color property
    if (material.m_Properties.TryGetValue(WModelImporter2::PropertySemantic::DiffuseColor, propertyValue) && propertyValue.IsA<WColor>())
    {
      pAccessor->SetValueByName(pMaterialProperties, "BaseColor", propertyValue).LogFailure();
    }

    // Set emissive color property
    if (material.m_Properties.TryGetValue(WModelImporter2::PropertySemantic::EmissiveColor, propertyValue) && propertyValue.IsA<WColor>())
    {
      pAccessor->SetValueByName(pMaterialProperties, "EmissiveColor", propertyValue).LogFailure();
    }

    // Set two-sided property
    if (material.m_Properties.TryGetValue(WModelImporter2::PropertySemantic::TwosidedValue, propertyValue) && propertyValue.IsNumber())
    {
      // Do NOT set this. A lot of assets from Blender have this set incorrectly and it is not a good idea to set this automatically.
      // Force the user to change this when needed.
      // pAccessor->SetValueByName(pMaterialProperties, "TWO_SIDED", propertyValue.ConvertTo<bool>()).LogFailure();
    }

    // Set metallic property
    if (material.m_Properties.TryGetValue(WModelImporter2::PropertySemantic::MetallicValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value >= 1.0f)
        value = 1.0f;
      else
        value = 0.0f;

      pAccessor->SetValueByName(pMaterialProperties, "MetallicValue", value).LogFailure();
    }

    // Set roughness property
    if (material.m_Properties.TryGetValue(WModelImporter2::PropertySemantic::RoughnessValue, propertyValue) && propertyValue.IsNumber())
    {
      float value = propertyValue.ConvertTo<float>();

      // probably in 0-255 range
      if (value > 1.0f)
        value /= 255.0f;

      value = WMath::Clamp(value, 0.0f, 1.0f);
      value = WMath::Lerp(0.4f, 1.0f, value);

      // the extracted roughness value is really just a guess to get started

      pAccessor->SetValueByName(pMaterialProperties, "RoughnessValue", value).LogFailure();
    }

    // Set ORM Texture
    if (bHasOrmTexture)
    {
      pAccessor->SetValueByName(pMaterialProperties, "UseOrmTexture", true).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseOcclusionTexture", false).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseRoughnessTexture", false).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "UseMetallicTexture", false).LogFailure();

      pAccessor->SetValueByName(pMaterialProperties, "MetallicTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "OcclusionTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "RoughnessTexture", "").LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "RoughnessValue", 1.0f).LogFailure();
      pAccessor->SetValueByName(pMaterialProperties, "MetallicValue", 0.0f).LogFailure();

      pAccessor->SetValueByName(pMaterialProperties, "OrmTexture", WVariant(ImportOrResolveTexture(szImportSourceFolder, szImportTargetFolder, textureRoughness, WModelImporter2::TextureSemantic::OrmMap, false, pImporter))).LogFailure();
    }

    // Todo:
    // * Shading Mode
    // * Mask Threshold

    pAccessor->FinishTransaction();
  }

  static bool SearchForFile(WStringView sTargetFolder, WStringView sFilename, WStringBuilder& out_sFullpath)
  {
    WFileSystemIterator it;
    for (it.StartSearch(sTargetFolder, WFileSystemIteratorFlags::ReportFilesRecursive); it.IsValid(); it.Next())
    {
      if (it.GetStats().m_sName == sFilename)
      {
        it.GetStats().GetFullPath(out_sFullpath);
        return true;
      }
    }

    return false;
  }

  void ImportMeshAssetMaterials(WDynamicArray<WMaterialResourceSlot>& inout_materialSlots, WStringView sDocumentDirectory, const WModelImporter2::Importer* pImporter)
  {
    W_PROFILE_SCOPE("ImportMeshAssetMaterials");

    WStringBuilder targetDirectory = sDocumentDirectory;

    if (targetDirectory.HasAnyExtension())
    {
      targetDirectory.RemoveFileExtension();
      targetDirectory.Append("_data/");
    }

    const WStringBuilder sourceDirectory = WPathUtils::GetFileDirectory(pImporter->GetImportOptions().m_sSourceFile);

    WStringBuilder tmp, fullName;
    WStringBuilder newResourcePathAbs;

    const WUInt32 uiNumSubmeshes = inout_materialSlots.GetCount();

    if (uiNumSubmeshes == 0)
      return;

    WProgressRange range("Importing Materials", uiNumSubmeshes, false);

    WHashTable<const WModelImporter2::OutputMaterial*, WString> importMatToGuid;

    WTempHybridArray<WDocument*, 32> pendingSaveTasks;

    auto WaitForPendingTasks = [&pendingSaveTasks]()
    {
      W_PROFILE_SCOPE("WaitForPendingTasks");
      for (WDocument* pDoc : pendingSaveTasks)
      {
        pDoc->GetDocumentManager()->CloseDocument(pDoc);
      }
      pendingSaveTasks.Clear();
    };

    WTempHybridArray<WString, 16> allowedExtensions;
    FillFileFilter(allowedExtensions, WFileBrowserAttribute::ImagesLdrAndHdr);

    for (const auto& itTex : pImporter->m_OutputTextures)
    {
      if (itTex.Value().m_RawData.IsEmpty() || !allowedExtensions.Contains(itTex.Value().m_sFileFormatExtension))
      {
        WLog::Error("Mesh uses embedded texture of unsupported type ('{}').", itTex.Value().m_sFileFormatExtension);
        continue;
      }

      WStringBuilder sFinalTextureName;
      itTex.Value().GenerateFileName(sFinalTextureName);

      WStringBuilder sEmbededFile;
      sEmbededFile = targetDirectory;
      sEmbededFile.AppendPath(sFinalTextureName);

      WDeferredFileWriter out;
      out.SetOutput(sEmbededFile, true);
      out.WriteBytes(itTex.Value().m_RawData.GetPtr(), itTex.Value().m_RawData.GetCount()).AssertSuccess();
      out.Close().IgnoreResult();
    }

    for (const auto& impMaterial : pImporter->m_OutputMaterials)
    {
      if (impMaterial.m_iReferencedByMesh < 0)
        continue;

      const WUInt32 subMeshIdx = impMaterial.m_iReferencedByMesh;

      range.BeginNextStep(impMaterial.m_sName);

      // Didn't find currently set resource, create new imported material.
      if (!WAssetCurator::GetSingleton()->FindSubAsset(inout_materialSlots[subMeshIdx].m_sResource))
      {
        // Check first if we already imported this material.
        if (importMatToGuid.TryGetValue(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource))
          continue;

        // search for the file recursively
        fullName.Set(impMaterial.m_sName, ".WMaterialAsset");
        if (SearchForFile(targetDirectory, fullName, newResourcePathAbs))
        {
          // Does the generated path already exist? Use it.
          if (const auto assetInfo = WAssetCurator::GetSingleton()->FindSubAsset(newResourcePathAbs))
          {
            inout_materialSlots[subMeshIdx].m_sResource = WConversionUtils::ToString(assetInfo->m_Data.m_Guid, tmp);
            continue;
          }
        }

        // Put the new asset in the data folder.
        newResourcePathAbs = targetDirectory;
        newResourcePathAbs.AppendPath(impMaterial.m_sName);
        newResourcePathAbs.Append(".WMaterialAsset");

        WMaterialAssetDocument* pMaterialDoc = WDynamicCast<WMaterialAssetDocument*>(WQtEditorApp::GetSingleton()->CreateDocument(newResourcePathAbs, WDocumentFlags::AsyncSave));
        if (!pMaterialDoc)
        {
          WLog::Error("Failed to create new material '{0}'", impMaterial.m_sName);
          continue;
        }

        ImportMeshAssetMaterialProperties(pMaterialDoc, impMaterial, sourceDirectory, targetDirectory, pImporter);
        inout_materialSlots[subMeshIdx].m_sResource = WConversionUtils::ToString(pMaterialDoc->GetGuid(), tmp);

        pMaterialDoc->SaveDocumentAsync({});
        pendingSaveTasks.PushBack(pMaterialDoc);

        // we have to flush because materials create worlds in the engine process and there is a world limit of 64
        if (pendingSaveTasks.GetCount() >= 16)
          WaitForPendingTasks();

        WLog::Success("Imported material: '{}'", newResourcePathAbs);
      }

      // If we have a material now, fill the mapping.
      // It is important to do this even for "old"/known materials since a mesh might have gotten a new slot that points to the same
      // material as previous slots.
      if (!inout_materialSlots[subMeshIdx].m_sResource.IsEmpty())
      {
        importMatToGuid.Insert(&impMaterial, inout_materialSlots[subMeshIdx].m_sResource);
      }
    }

    WaitForPendingTasks();
  }

  void AddGltfBufferDependencies(WStringView sMeshFile, WSet<WString>& inout_dependencies)
  {
    if (!sMeshFile.HasExtension("gltf"))
      return;

    WStringBuilder sAbsFilePath = sMeshFile;

    if (!WQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilePath))
      return;

    WFileReader file;
    if (file.Open(sAbsFilePath).Failed())
      return;

    WJSONReader json;
    if (json.Parse(file).Failed())
      return;

    const WVariantDictionary& root = json.GetTopLevelObject();
    const WVariant* pBuffers = root.GetValue("buffers");
    if (pBuffers == nullptr || !pBuffers->IsA<WVariantArray>())
      return;

    const WVariantArray& buffers = pBuffers->Get<WVariantArray>();
    WStringBuilder sBufferPath;
    WStringBuilder sBufferDir = sAbsFilePath.GetFileDirectory();

    for (const WVariant& buffer : buffers)
    {
      if (!buffer.IsA<WVariantDictionary>())
        continue;

      const WVariantDictionary& bufferDict = buffer.Get<WVariantDictionary>();
      const WVariant* pUri = bufferDict.GetValue("uri");
      if (pUri == nullptr || !pUri->IsA<WString>())
        continue;

      const WString& sUri = pUri->Get<WString>();

      // Skip data URIs (embedded binary data)
      if (sUri.StartsWith("data:"))
        continue;

      // Construct absolute path to the buffer file
      sBufferPath = sBufferDir;
      sBufferPath.AppendPath(sUri);
      sBufferPath.MakeCleanPath();

      if (!WOSFile::ExistsFile(sBufferPath))
        continue;

      // Convert to data directory relative path
      if (WQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sBufferPath))
      {
        inout_dependencies.Insert(sBufferPath);
      }
    }
  }

} // namespace WMeshImportUtils
