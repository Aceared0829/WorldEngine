#pragma once

#include <Core/Collection/CollectionResource.h>
#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <RendererCore/Components/BlackboardComponent.h>
#include <RendererCore/Utils/BlackboardTemplateResource.h>

struct WBlackboardTemplateAssetObject : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WBlackboardTemplateAssetObject, WReflectedClass);

  WDynamicArray<WString> m_BaseTemplates;
  WDynamicArray<WBlackboardEntry> m_Entries;
};

class WBlackboardTemplateAssetDocument : public WSimpleAssetDocument<WBlackboardTemplateAssetObject>
{
  W_ADD_DYNAMIC_REFLECTION(WBlackboardTemplateAssetDocument, WSimpleAssetDocument<WBlackboardTemplateAssetObject>);

public:
  WBlackboardTemplateAssetDocument(WStringView sDocumentPath);

  WStatus WriteAsset(WStreamWriter& inout_stream, const WPlatformProfile* pAssetProfile) const;

protected:
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& inout_stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) override;

  WStatus RetrieveState(const WBlackboardTemplateAssetObject* pProp, WBlackboardTemplateResourceDescriptor& inout_Desc) const;
};
