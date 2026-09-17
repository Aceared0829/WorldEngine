#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginAssets/CustomDataAsset/CustomDataAsset.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/ApplyNativePropertyChangesContext.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/DocumentObjectMirror.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomDataAssetProperties, 1, WRTTIDefaultAllocator<WCustomDataAssetProperties>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_pType)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCustomDataAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCustomDataAssetDocument::WCustomDataAssetDocument(WStringView sDocumentPath)
  : WSimpleAssetDocument<WCustomDataAssetProperties>(sDocumentPath, WAssetDocEngineConnection::None)
{
}

WTransformStatus WCustomDataAssetDocument::InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
  const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WAbstractObjectGraph abstractObjectGraph;
  WDocumentObjectConverterWriter objectWriter(&abstractObjectGraph, GetObjectManager());

  WDocumentObject* pObject = GetPropertyObject();

  WVariant type = pObject->GetTypeAccessor().GetValue("Type");
  W_ASSERT_DEV(type.IsA<WUuid>(), "Implementation error");

  if (WDocumentObject* pDataObject = pObject->GetChild(type.Get<WUuid>()))
  {
    WAbstractObjectNode* pAbstractNode = objectWriter.AddObjectToGraph(pDataObject, "root");
  }

  WAbstractGraphBinarySerializer::Write(stream, &abstractObjectGraph);
  return WStatus(W_SUCCESS);
}

void WCustomDataAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const WDocumentObject* pObject = GetPropertyObject();

  const WUuid typeGuid = GetObjectAccessor()->GetByName<WUuid>(pObject, "Type");
  if (const WDocumentObject* pDataObject = GetObjectAccessor()->GetObject(typeGuid))
  {
    const WRTTI* pRtti = pDataObject->GetType();

    WStringBuilder tags(";");

    while (pRtti && pRtti != WGetStaticRTTI<WCustomData>())
    {
      tags.Append(pRtti->GetTypeName(), ";");

      pRtti = pRtti->GetParentType();
    }

    pInfo->m_sAssetsDocumentTags = tags;
  }
}
