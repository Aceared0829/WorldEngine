#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Types/Status.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <ToolsFoundation/Document/DocumentManager.h>

struct WSubAsset;
class WPlatformProfile;

class W_EDITORFRAMEWORK_DLL WAssetDocumentManager : public WDocumentManager
{
  W_ADD_DYNAMIC_REFLECTION(WAssetDocumentManager, WDocumentManager);

public:
  WAssetDocumentManager();
  ~WAssetDocumentManager();

  /// Opens the asset file and reads the "Header" into the given WAssetDocumentInfo.
  virtual WStatus ReadAssetDocumentInfo(WUniquePtr<WAssetDocumentInfo>& out_pInfo, WStreamReader& inout_stream) const;
  virtual void FillOutSubAssetList(const WAssetDocumentInfo& assetInfo, WDynamicArray<WSubAssetData>& out_subAssets) const {}

  /// If this asset type has additional output files that need to be generated (like a texture atlas that combines outputs from multiple assets)
  /// this function should make sure those files are all generated and return the list of relative file paths (from the data directory root).
  virtual WStatus GetAdditionalOutputs(WDynamicArray<WString>& ref_files) { return WStatus(W_SUCCESS); }

  // WDocumentManager overrides:
public:
  virtual WStatus CloneDocument(WStringView sPath, WStringView sClonePath, WUuid& inout_cloneGuid) override;

  /// \name Asset Profile Functions
  ///@{
public:
  /// Called by the WAssetCurator when the active asset profile changes to re-compute m_uiAssetProfileHash.
  void ComputeAssetProfileHash(const WPlatformProfile* pAssetProfile);

  /// Returns the hash that was previously computed through ComputeAssetProfileHash().
  W_ALWAYS_INLINE WUInt64 GetAssetProfileHash() const { return m_uiAssetProfileHash; }

  /// Returns pAssetProfile, or if that is null, WAssetCurator::GetSingleton()->GetActiveAssetProfile().
  static const WPlatformProfile* DetermineFinalTargetProfile(const WPlatformProfile* pAssetProfile);

private:
  virtual WUInt64 ComputeAssetProfileHashImpl(const WPlatformProfile* pAssetProfile) const;

  // The hash that is combined with the asset document hash to determine whether the document output is up to date.
  // This hash needs to be computed in ComputeAssetProfileHash() and should reflect all important settings from the givne asset profile that
  // affect the asset output for this manager.
  // However, if GeneratesProfileSpecificAssets() return false, the hash must be zero, as then all outputs must be identical in all
  // profiles.
  WUInt64 m_uiAssetProfileHash = 0;

  ///@}
  /// \name Thumbnail Functions
  ///@{
public:
  /// Returns the absolute path to the thumbnail that belongs to the given document.
  virtual WString GenerateResourceThumbnailPath(WStringView sDocumentPath, WStringView sSubAssetName = WStringView());
  virtual bool IsThumbnailUpToDate(WStringView sDocumentPath, WStringView sSubAssetName, WUInt64 uiThumbnailHash, WUInt32 uiTypeVersion);

  ///@}
  /// \name Output Functions
  ///@{

  virtual void AddEntriesToAssetTable(WStringView sDataDirectory, const WPlatformProfile* pAssetProfile, WDelegate<void(WStringView sGuid, WStringView sPath, WStringView sType)> addEntry) const;
  virtual WString GetAssetTableEntry(const WSubAsset* pSubAsset, WStringView sDataDirectory, const WPlatformProfile* pAssetProfile) const;

  /// Calls GetRelativeOutputFileName and prepends [DataDir]/AssetCache/ .
  WString GetAbsoluteOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile = nullptr) const;

  /// Relative to 'AssetCache' folder.
  virtual WString GetRelativeOutputFileName(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sDataDirectory, WStringView sDocumentPath, WStringView sOutputTag, const WPlatformProfile* pAssetProfile = nullptr) const;

  /// Should return the document type of the given sOutputTag.
  virtual WStringView GetOutputDocumentType(const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sOutputTag, const WPlatformProfile* pAssetProfile = nullptr) const { return pTypeDesc->m_sDocumentTypeName; }

  virtual bool GeneratesProfileSpecificAssets() const = 0;

  /// Reads the info that the last transform recorded for the given asset output. \see WAssetInfoFile
  ///
  /// Fails if there is no such file, which is the normal case for most asset types, or if it is stale. Treat a failure
  /// as 'no information available' rather than as an error.
  WResult ReadAssetInfoFile(WAssetInfoFile& out_info, const WAssetDocumentTypeDescriptor* pTypeDesc, WStringView sDocumentPath, WUInt64 uiHash, WStringView sOutputTag = {}, const WPlatformProfile* pAssetProfile = nullptr) const;

  /// Override this to append the values from an WAssetInfoFile that are worth showing at a glance, e.g. in a tooltip.
  virtual void AppendAssetInfoSummary(WStringBuilder& ref_sOut, const WAssetInfoFile& info, WStringView sLinePrefix = "\n"_wsv) const {}

  bool IsOutputUpToDate(WStringView sDocumentPath, const WDynamicArray<WString>& outputs, WUInt64 uiHash, const WAssetDocumentTypeDescriptor* pTypeDescriptor);
  virtual bool IsOutputUpToDate(WStringView sDocumentPath, WStringView sOutputTag, WUInt64 uiHash, const WAssetDocumentTypeDescriptor* pTypeDescriptor);

  /// Describes how likely it is that a generated file is 'corrupted', due to dependency issues and such.
  /// For example a prefab may not work correctly, if it was written with a very different C++ plugin state, but this can't be detected later.
  /// Whereas a texture always produces exactly the same output and is thus perfectly reliable.
  /// This is used to clear asset caches selectively, and keep things that are unlikely to be in a broken state.
  enum OutputReliability : WUInt8
  {
    Unknown = 0,
    Good = 1,
    Perfect = 2,
  };

  /// \see OutputReliability
  virtual OutputReliability GetAssetTypeOutputReliability() const { return OutputReliability::Unknown; }

  ///@}


  /// Called by the editor to try to open a document for the matching picking result
  virtual WResult OpenPickedDocument(const WDocumentObject* pPickedComponent, WUInt32 uiPartIndex) { return W_FAILURE; }

  static WResult TryOpenAssetDocument(const char* szPathOrGuid);

  /// In case this manager deals with types that need to be force transformed on scene export, it can add the asset type names to this list.
  /// This is only needed for assets that have such special dependencies for their transform step, that the regular dependency tracking doesn't work for them.
  /// Currently the only known case are Collection assets, because they have to manually go through the Package dependencies transitively, which means
  /// that the asset curator can't know when they need to be updated.
  virtual void GetAssetTypesRequiringTransformForSceneExport(WSet<WTempHashedString>& inout_assetTypes) {};

protected:
  static bool IsResourceUpToDate(const char* szResourceFile, WUInt64 uiHash, WUInt16 uiTypeVersion);
  static void GenerateOutputFilename(WStringBuilder& inout_sRelativeDocumentPath, const WPlatformProfile* pAssetProfile, const char* szExtension, bool bPlatformSpecific);
};
