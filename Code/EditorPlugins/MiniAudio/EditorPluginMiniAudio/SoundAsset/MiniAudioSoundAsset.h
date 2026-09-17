#pragma once

#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WMiniAudioSoundAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioSoundAssetProperties, WReflectedClass);

public:
  WMiniAudioSoundAssetProperties() = default;

  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  WString m_sGroup;
  bool m_bLoop = false;
  float m_fMinVolume = 1.0f;
  float m_fMaxVolume = 1.0f;
  float m_fMinPitch = 1.0f;
  float m_fMaxPitch = 1.0f;
  bool m_bSpatialize = true;
  float m_fMinDistance = 0.1f;
  float m_fMaxDistance = 10.0f;
  float m_fRolloff = 1.0f;
  float m_fDopplerFactor = 0.0f;
  WDynamicArray<WString> m_SoundFiles;
  // disable pitch
  // no global pitch
  // looping
  // fade out duration ?
  // fully decode / stream
};

class WMiniAudioSoundAssetDocument : public WSimpleAssetDocument<WMiniAudioSoundAssetProperties>
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioSoundAssetDocument, WSimpleAssetDocument<WMiniAudioSoundAssetProperties>);

public:
  WMiniAudioSoundAssetDocument(WStringView sDocumentPath);

protected:
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const override;
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
};

//////////////////////////////////////////////////////////////////////////


class WMiniAudioSoundAssetDocumentGenerator : public WAssetDocumentGenerator
{
  W_ADD_DYNAMIC_REFLECTION(WMiniAudioSoundAssetDocumentGenerator, WAssetDocumentGenerator);

public:
  WMiniAudioSoundAssetDocumentGenerator();
  ~WMiniAudioSoundAssetDocumentGenerator();

  virtual void GetImportModes(WStringView sAbsInputFile, WDynamicArray<WAssetDocumentGenerator::ImportMode>& out_modes) const override;
  virtual WStringView GetDocumentExtension() const override { return "WMiniAudioSoundAsset"; }
  virtual WStringView GetGeneratorGroup() const override { return "Sounds"; }
  virtual WStatus Generate(WStringView sInputFileAbs, WStringView sMode, WDynamicArray<WDocument*>& out_generatedDocuments) override;
};
