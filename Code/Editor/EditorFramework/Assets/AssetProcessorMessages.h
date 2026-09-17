#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/Assets/Declarations.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/LogEntry.h>

class W_EDITORFRAMEWORK_DLL WProcessAssetMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WProcessAssetMsg, WProcessMessage);

public:
  WUuid m_AssetGuid;
  WUInt64 m_AssetHash = 0;
  WUInt64 m_ThumbHash = 0;
  WUInt64 m_PackageHash = 0;
  WString m_sAssetPath;
  WString m_sPlatform;
  WDynamicArray<WString> m_DepRefHull;
};

class W_EDITORFRAMEWORK_DLL WProcessAssetResponseMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WProcessAssetResponseMsg, WProcessMessage);

public:
  WTransformStatus m_Status;
  mutable WDynamicArray<WLogEntry> m_LogEntries;

  // If the fields below are used, the WEditorProcessor detected a hash missmatch between the editor and its own state. It will send over its own state so the editor can detect differences. See WEditorProcessorProcess::HandleHashMissmatch.
  mutable WMap<WString, WUInt64> m_MissmatchTransformDependencies; ///< Hashes of all transform dependencies.
  mutable WMap<WString, WUInt64> m_MissmatchThumbnailDependencies; ///< Hashes of all thumbnail dependencies.
  WUInt64 m_uiMissmatchAssetHash = 0;                                ///< Transform hash observed by the WEditorProcessor.
  WUInt64 m_uiMissmatchThumbHash = 0;                                ///< Thumbnail hash observed by the WEditorProcessor.
  WTime m_StartedProcessing;
  WTime m_StartedTransform;
  WTime m_FinishedProcessing;
};

class W_EDITORFRAMEWORK_DLL WFreeAllResourcesMsg : public WProcessMessage
{
  W_ADD_DYNAMIC_REFLECTION(WFreeAllResourcesMsg, WProcessMessage);
};
