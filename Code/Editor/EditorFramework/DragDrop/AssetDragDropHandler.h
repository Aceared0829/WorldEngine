#pragma once

#include <EditorFramework/DragDrop/DragDropHandler.h>

class WDocument;

class W_EDITORFRAMEWORK_DLL WAssetDragDropHandler : public WDragDropHandler
{
  W_ADD_DYNAMIC_REFLECTION(WAssetDragDropHandler, WDragDropHandler);

public:
protected:
  bool IsAssetType(const WDragDropInfo* pInfo) const;

  WString GetAssetGuidString(const WDragDropInfo* pInfo) const;

  WUuid GetAssetGuid(const WDragDropInfo* pInfo) const { return WConversionUtils::ConvertStringToUuid(GetAssetGuidString(pInfo)); }

  WString GetAssetsDocumentTypeName(const WUuid& assetTypeGuid) const;

  bool IsSpecificAssetType(const WDragDropInfo* pInfo, const char* szType) const;

  WDocument* m_pDocument;
};
