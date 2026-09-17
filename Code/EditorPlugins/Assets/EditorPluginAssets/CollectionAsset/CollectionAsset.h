#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>

class WCollectionAssetEntry : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCollectionAssetEntry, WReflectedClass);

public:
  WString m_sLookupName;
  WString m_sRedirectionAsset;
};

class WCollectionAssetData : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCollectionAssetData, WReflectedClass);

public:
  WDynamicArray<WCollectionAssetEntry> m_Entries;
};

class WCollectionAssetDocument : public WSimpleAssetDocument<WCollectionAssetData>
{
  W_ADD_DYNAMIC_REFLECTION(WCollectionAssetDocument, WSimpleAssetDocument<WCollectionAssetData>);

public:
  WCollectionAssetDocument(WStringView sDocumentPath);

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;
};
