#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/Document/Document.h>

class W_EDITORFRAMEWORK_DLL WAssetDocumentInfo final : public WDocumentInfo
{
  W_ADD_DYNAMIC_REFLECTION(WAssetDocumentInfo, WDocumentInfo);

public:
  WAssetDocumentInfo();
  virtual ~WAssetDocumentInfo();
  WAssetDocumentInfo(WAssetDocumentInfo&& rhs);
  void operator=(WAssetDocumentInfo&& rhs);
  /// Creates a clone without meta data.
  void CreateShallowClone(WAssetDocumentInfo& out_docInfo) const;
  void ClearMetaData();

  WUInt64 m_uiSettingsHash;                    ///< Current hash over all settings in the document, used to check resulting resource for being up-to-date in combination with dependency hashes.

  WSet<WString> m_TransformDependencies;      ///< [Data dir relative path or GUID] Files that are required to generate the asset, ie. if one changes, the asset needs to be recreated
  WSet<WString> m_ThumbnailDependencies;      ///< [Data dir relative path or GUID] Files that are used to generate the thumbnail.
  WSet<WString> m_PackageDependencies;        ///< [Data dir relative path or GUID] Files that are needed at runtime and should be packaged with the game.

  WSet<WString> m_Outputs;                    ///< Additional output this asset produces besides the default one. These are tags like VISUAL_SHADER that are resolved
                                                ///< by the WAssetDocumentManager into paths.
  WHashedString m_sAssetsDocumentTypeName;
  WString m_sAssetsDocumentTags;
  WDynamicArray<WReflectedClass*> m_MetaInfo; ///< Holds arbitrary objects that store meta-data for the asset document. Mainly used for exposed parameters, but can be any reflected
                                                ///< type. This array takes ownership of all objects and deallocates them on shutdown.

  const char* GetAssetsDocumentTypeName() const;
  void SetAssetsDocumentTypeName(const char* szSz);

  const WString& GetAssetsDocumentTags() const;
  void SetAssetsDocumentTags(const WString& sTags);

  /// Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  const WReflectedClass* GetMetaInfo(const WRTTI* pType) const;

  /// Returns an object from m_MetaInfo of the given base type, or nullptr if none exists
  template <typename T>
  const T* GetMetaInfo() const
  {
    return static_cast<const T*>(GetMetaInfo(WGetStaticRTTI<T>()));
  }

private:
  WAssetDocumentInfo(const WAssetDocumentInfo&);
  void operator=(const WAssetDocumentInfo&) = delete;
};
