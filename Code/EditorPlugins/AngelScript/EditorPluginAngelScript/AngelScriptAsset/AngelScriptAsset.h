#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <ToolsFoundation/VisualGraph/VisualGraphObjectManager.h>

class WAngelScriptAssetDocument;
struct WPropertyMetaStateEvent;

struct WAngelScriptCodeMode
{
  using StorageType = WUInt8;

  enum Enum : StorageType
  {
    Inline,
    FromFile,

    Default = Inline
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_EDITORPLUGINANGELSCRIPT_DLL, WAngelScriptCodeMode);

class WAngelScriptParameter : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAngelScriptParameter, WReflectedClass);

public:
  bool m_bExpose = false;
  WString m_sDeclaration;
  WString m_sName;
  WVariant m_DefaultValue;
};

class WAngelScriptAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAngelScriptAssetProperties, WReflectedClass);

  WEnum<WAngelScriptCodeMode> m_CodeMode;
  WString m_sScriptFile;
  WString m_sClassName;
  WString m_sCode;

  WDynamicArray<WAngelScriptParameter> m_Parameters;
  WDynamicArray<WString> m_Dependencies;
};

class WAngelScriptAssetDocument : public WSimpleAssetDocument<WAngelScriptAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WAngelScriptAssetDocument, WSimpleAssetDocument<WAngelScriptAssetProperties>);

public:
  WAngelScriptAssetDocument(WStringView sDocumentPath);

  void OpenExternalEditor();
  void SyncExposedParameters();

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

protected:
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  void SyncInfos();

  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
};
