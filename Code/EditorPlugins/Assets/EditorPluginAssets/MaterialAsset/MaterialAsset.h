#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <EditorPluginAssets/VisualShader/VisualShaderNodeManager.h>

class WMaterialAssetDocument;
struct WPropertyMetaStateEvent;
struct WEditorAppEvent;

struct WMaterialShaderMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    BaseMaterial,
    File,
    Custom,

    Default = BaseMaterial
  };
};

struct WMaterialVisualShaderEvent
{
  enum Type
  {
    TransformFailed,
    TransformSucceeded,
    VisualShaderNotUsed,
  };

  Type m_Type;
  WString m_sTransformError;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WMaterialShaderMode);

struct WMaterialAssetPreview
{
  using StorageType = WUInt8;

  enum Enum
  {
    Ball,
    Sphere,
    Box,
    Plane,

    Default = Ball
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WMaterialAssetPreview);

class WMaterialAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialAssetProperties, WReflectedClass);

public:
  WMaterialAssetProperties() = default;

  void SetBaseMaterial(const char* szBaseMaterial);
  const char* GetBaseMaterial() const;

  void SetSurface(const char* szSurface) { m_sSurface = szSurface; }
  const char* GetSurface() const { return m_sSurface; }

  void SetShader(const char* szShader);
  const char* GetShader() const;
  void SetShaderProperties(WReflectedClass* pProperties);
  WReflectedClass* GetShaderProperties() const;
  void SetShaderMode(WEnum<WMaterialShaderMode> mode);
  WEnum<WMaterialShaderMode> GetShaderMode() const { return m_ShaderMode; }

  void SetDocument(WMaterialAssetDocument* pDocument);
  void UpdateShader(bool bForce = false);

  void DeleteProperties();
  void CreateProperties(const char* szShaderPath);

  void SaveOldValues();
  void LoadOldValues();

  WString ResolveRelativeShaderPath() const;
  WString GetAutoGenShaderPathAbs() const;

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

public:
  WString m_sBaseMaterial;
  WString m_sSurface;
  WString m_sShader;
  WString m_sAssetFilterTags;

  WMap<WString, WVariant> m_CachedProperties;
  WMaterialAssetDocument* m_pDocument = nullptr;
  WEnum<WMaterialShaderMode> m_ShaderMode;
};

class WMaterialAssetDocument : public WSimpleAssetDocument<WMaterialAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WMaterialAssetDocument, WSimpleAssetDocument<WMaterialAssetProperties>);

public:
  WMaterialAssetDocument(WStringView sDocumentPath);
  ~WMaterialAssetDocument();

  WDocumentObject* GetShaderPropertyObject();
  const WDocumentObject* GetShaderPropertyObject() const;

  void SetBaseMaterial(const char* szBaseMaterial);

  WStatus WriteMaterialAsset(WStreamWriter& inout_stream, const WPlatformProfile* pAssetProfile, bool bEmbedLowResData) const;

  /// Will make sure that the visual shader is rebuilt.
  /// Typically called during asset transformation, but can be triggered manually to enforce getting visual shader node changes in.
  WStatus RecreateVisualShaderFile(const WAssetFileHeader& assetHeader);

  /// If shader compilation failed this will modify the output shader file such that transforming it again, will trigger a full
  /// regeneration Otherwise the AssetCurator would early out
  void TagVisualShaderFileInvalid(const WPlatformProfile* pAssetProfile, const char* szError);

  /// Deletes all Visual Shader nodes that are not connected to the output
  void RemoveDisconnectedNodes();

  static WUuid GetLitBaseMaterial();
  static WUuid GetLitAlphaTestBaseMaterial();
  static WUuid GetNeutralNormalMap();

  virtual void GetSupportedMimeTypesForPasting(WDynamicArray<WString>& out_mimeTypes) const override;
  virtual bool CopySelectedObjects(WAbstractObjectGraph& out_objectGraph, WStringBuilder& out_sMimeType) const override;
  virtual bool Paste(const WArrayPtr<PasteInfo>& info, const WAbstractObjectGraph& objectGraph, bool bAllowPickedPosition, WStringView sMimeType) override;

  WEvent<const WMaterialVisualShaderEvent&> m_VisualShaderEvents;
  WEnum<WMaterialAssetPreview> m_PreviewModel;

protected:
  WUuid GetSeedFromBaseMaterial(const WAbstractObjectGraph* pBaseGraph);
  static WUuid GetMaterialNodeGuid(const WAbstractObjectGraph& graph);
  virtual void UpdatePrefabObject(WDocumentObject* pObject, const WUuid& PrefabAsset, const WUuid& PrefabSeed, WStringView sBasePrefab) override;
  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;

  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo) override;

  virtual void InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const override;
  virtual void AttachMetaDataBeforeSaving(WAbstractObjectGraph& graph) const override;
  virtual void RestoreMetaDataAfterLoading(const WAbstractObjectGraph& graph, bool bUndoable) override;

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;

  void InvalidateCachedShader();
  void EditorEventHandler(const WEditorAppEvent& e);

private:
  WStringBuilder m_sCheckPermutations;
  static WUuid s_LitBaseMaterial;
  static WUuid s_LitAlphaTextBaseMaterial;
  static WUuid s_NeutralNormalMap;
};

class WMaterialObjectManager : public WVisualShaderNodeManager
{
};
