#pragma once

#include <EditorFramework/Assets/AssetDocumentInfo.h>
#include <EditorFramework/Assets/Declarations.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <Foundation/Utilities/AssetInfoFile.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WEditorEngineConnection;
class WEditorEngineSyncObject;
class WAssetDocumentManager;
class WPlatformProfile;
class QImage;

/// Describes whether the asset document on the editor side also needs a rendering context on the engine side
enum class WAssetDocEngineConnection : WUInt8
{
  None,               ///< Use this when the document is fully self-contained and any UI is handled by Qt only. This is very common for 'data only' assets and everything that can't be visualized in 3D.
  Simple,             ///< Use this when the asset should be visualized in 3D. This requires a 'context' to be set up on the engine side that implements custom rendering. This is the most common type for anything that can be visualized in 3D, though can also be used for 2D data.
  FullObjectMirroring ///< In this mode the entire object hierarchy on the editor side is automatically synchronized over to an engine context. This is only needed for complex documents, such as scenes and prefabs.
};

/// Frequently needed asset document states, to prevent code duplication
struct WCommonAssetUiState
{
  enum Enum : WUInt32
  {
    Pause = W_BIT(0),
    Restart = W_BIT(1),
    Loop = W_BIT(2),
    SimulationSpeed = W_BIT(3),
    Grid = W_BIT(4),
    Visualizers = W_BIT(5),
  };

  Enum m_State;
  double m_fValue = 0;
};

class W_EDITORFRAMEWORK_DLL WAssetDocument : public WDocument
{
  W_ADD_DYNAMIC_REFLECTION(WAssetDocument, WDocument);

public:
  /// The thumbnail info containing the hash of the file is appended to assets.
  /// The serialized size of this class can't change since it is found by seeking to the end of the file.
  class W_EDITORFRAMEWORK_DLL ThumbnailInfo
  {
  public:
    WResult Deserialize(WStreamReader& inout_reader);
    WResult Serialize(WStreamWriter& inout_writer) const;

    /// Checks whether the stored file contains the same hash.
    bool IsThumbnailUpToDate(WUInt64 uiExpectedHash, WUInt16 uiVersion) const { return (m_uiHash == uiExpectedHash && m_uiVersion == uiVersion); }

    /// Sets the asset file hash
    void SetFileHashAndVersion(WUInt64 uiHash, WUInt16 v)
    {
      m_uiHash = uiHash;
      m_uiVersion = v;
    }

    /// Returns the serialized size of the thumbnail info.
    /// Used to seek to the end of the file and find the thumbnail info struct.
    constexpr WUInt32 GetSerializedSize() const { return 19; }

  private:
    WUInt64 m_uiHash = 0;
    WUInt16 m_uiVersion = 0;
    WUInt16 m_uiReserved = 0;
  };

  WAssetDocument(WStringView sDocumentPath, WDocumentObjectManager* pObjectManager, WAssetDocEngineConnection engineConnectionType);
  ~WAssetDocument();

  /// \name Asset Functions
  ///@{

  WAssetDocumentManager* GetAssetDocumentManager() const;
  const WAssetDocumentInfo* GetAssetDocumentInfo() const;

  WBitflags<WAssetDocumentFlags> GetAssetFlags() const;

  const WAssetDocumentTypeDescriptor* GetAssetDocumentTypeDescriptor() const
  {
    return static_cast<const WAssetDocumentTypeDescriptor*>(GetDocumentTypeDescriptor());
  }

  /// Transforms an asset.
  ///   Typically not called manually but by the curator which takes care of dependencies first.
  ///
  /// If WTransformFlags::ForceTransform is set, it will try to transform the asset, ignoring whether the transform is up to date.
  /// If WTransformFlags::TriggeredManually is set, transform produced changes will be saved back to the document.
  /// If WTransformFlags::BackgroundProcessing is set and transforming the asset would require re-saving it, nothing is done.
  WTransformStatus TransformAsset(WBitflags<WTransformFlags> transformFlags, const WPlatformProfile* pAssetProfile = nullptr);

  /// Updates the thumbnail of the asset.
  ///   Should never be called manually. Called only by the curator which takes care of dependencies first.
  WTransformStatus CreateThumbnail();

  /// Returns the RTTI type version of this asset document type. E.g. when the algorithm to transform an asset changes,
  /// Increase the RTTI version. This will ensure that assets get re-transformed, even though their settings and dependencies might not have changed.
  WUInt16 GetAssetTypeVersion() const;

  /// Values that InternalTransformAsset() may fill out, to record facts only known after the transform. \see WAssetInfoFile
  ///
  /// Cleared before each output is generated. Leaving it empty means that no file is written, which is the case for most
  /// asset types. An asset whose output is produced by an external tool may instead have that tool write the file.
  WAssetInfoFile& GetTransformInfo() { return m_TransformInfo; }
  const WAssetInfoFile& GetTransformInfo() const { return m_TransformInfo; }

  ///@}
  /// \name IPC Functions
  ///@{

  enum class EngineStatus
  {
    Unsupported,  ///< This document does not have engine IPC.
    Disconnected, ///< Engine process crashed or not started yet.
    Initializing, ///< Document is being initialized on the engine process side.
    Loaded,       ///< Any message sent after this state is reached will work on a fully loaded document.
  };

  /// Returns the current state of the engine process side of this document.
  EngineStatus GetEngineStatus() const { return m_EngineStatus; }
  /// Waits for GetEngineStatus to return Loaded or returns a failure reason.
  WStatus WaitForEngineStatusLoaded() const;

  /// Passed into WEngineProcessDocumentContext::Initialize on the engine process side. Allows the document to provide additional data to the engine process during context creation.
  virtual WVariant GetCreateEngineMetaData() const { return WVariant(); }

  /// Sends a message to the corresponding WEngineProcessDocumentContext on the engine process.
  bool SendMessageToEngine(WEditorEngineDocumentMsg* pMessage) const;

  /// Handles all messages received from the corresponding WEngineProcessDocumentContext on the engine process.
  virtual void HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg);

  struct AssetUsage
  {
    WString m_sObjectName;
    WUuid m_ObjectGuid;
  };

  /// Finds all usages of the given asset in this document and appends them to out_usages. The default implementation does nothing, override this if your document can reference other assets.
  virtual void FindAssetUsages(WStringView sAssetToFind, WDynamicArray<AssetUsage>& out_usages, WUInt32 uiMaxResults) const {}

  /// Returns the WEditorEngineConnection for this document.
  WEditorEngineConnection* GetEditorEngineConnection() const { return m_pEngineConnection; }

  /// Registers a sync object for this document. It will be mirrored to the WEngineProcessDocumentContext on the engine process.
  void AddSyncObject(WEditorEngineSyncObject* pSync) const;

  /// Removes a previously registered sync object. It will be removed on the engine process side.
  void RemoveSyncObject(WEditorEngineSyncObject* pSync) const;

  /// Returns the sync object registered under the given guid.
  WEditorEngineSyncObject* FindSyncObject(const WUuid& guid) const;

  /// Returns the first sync object registered with the given type.
  WEditorEngineSyncObject* FindSyncObject(const WRTTI* pType) const;

  /// Sends messages to sync all sync objects to the engine process side.
  void SyncObjectsToEngine() const;

  /// Sends a message that the document has been opened or closed. Resends all document data.
  ///
  /// Calling this will always clear the existing document on the engine side and reset the state to the editor state.
  void SendDocumentOpenMessage(bool bOpen);


  ///@}

  WEvent<const WEditorEngineDocumentMsg*> m_ProcessMessageEvent;

protected:
  void EngineConnectionEventHandler(const WEditorEngineProcessConnection::Event& e);

  /// \name Hash Functions
  ///@{

  /// Computes the hash from all document objects
  WUInt64 GetDocumentHash() const;

  /// Computes the hash for one document object and combines it with the given hash
  void GetChildHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const;

  /// Computes the hash for transform relevant meta data of the given document object and combines it with the given hash.
  virtual void InternalGetMetaDataHash(const WDocumentObject* pObject, WUInt64& inout_uiHash) const {}

  ///@}
  /// \name Reimplemented Base Functions
  ///@{

  /// Overrides the base function to call UpdateAssetDocumentInfo() to update the settings hash
  virtual WTaskGroupID InternalSaveDocument(AfterSaveCallback callback) override;

  /// Implements auto transform on save
  virtual void InternalAfterSaveDocument() override;

  virtual void InitializeAfterLoading(bool bFirstTimeCreation) override;
  virtual void InitializeAfterLoadingAndSaving() override;

  ///@}
  /// \name Asset Functions
  ///@{

  /// Override this to add custom data (e.g. additional file dependencies) to the info struct.
  ///
  /// \note ALWAYS call the base function! It automatically fills out references that it can determine.
  ///       In most cases that is already sufficient.
  virtual void UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const;

  /// Override this and write the transformed file for the given szOutputTag into the given stream.
  ///
  /// The stream already contains the WAssetFileHeader. This is the function to prefer when the asset can be written
  /// directly from the editor process. AssetHeader is already written to the stream, but provided as reference.
  ///
  /// \param stream Data stream to write the asset to.
  /// \param szOutputTag Either empty for the default output or matches one of the tags defined in WAssetDocumentInfo::m_Outputs.
  /// \param szPlatform Platform for which is the output is to be created. Default is 'Default'.
  /// \param AssetHeader Header already written to the stream, provided for reference.
  /// \param transformFlags flags that affect the transform process, see WTransformFlags.
  virtual WTransformStatus InternalTransformAsset(WStreamWriter& stream, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags) = 0;

  /// Only override this function, if the transformed file for the given szOutputTag must be written from another process.
  ///
  /// szTargetFile is where the transformed asset should be written to. The overriding function must ensure to first
  /// write \a AssetHeader to the file, to make it a valid asset file or provide a custom WAssetDocumentManager::IsOutputUpToDate function.
  /// See WTransformFlags for definition of transform flags.
  virtual WTransformStatus InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile,
    const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags);

  WStatus RemoteExport(const WAssetFileHeader& header, const char* szOutputTarget) const;

  ///@}
  /// \name Thumbnail Functions
  ///@{

  /// Override this function to generate a thumbnail. Only called if GetAssetFlags returns WAssetDocumentFlags::SupportsThumbnail.
  virtual WTransformStatus InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo);

  /// Returns the full path to the jpg file in which the thumbnail for this asset is supposed to be
  WString GetThumbnailFilePath(WStringView sSubAssetName = WStringView()) const;

  /// Should be called after manually changing the thumbnail, such that the system will reload it
  void InvalidateAssetThumbnail(WStringView sSubAssetName = WStringView()) const;

  /// Requests the engine side to render a thumbnail, will call SaveThumbnail on success.
  WStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, WArrayPtr<WStringView> viewExclusionTags /*= WStringView("SkyLight")*/) const;
  WStatus RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo) const
  {
    WStringView defVal("SkyLight");
    return RemoteCreateThumbnail(thumbnailInfo, {&defVal, 1});
  }

  /// Saves the given image as the new thumbnail for the asset
  WStatus SaveThumbnail(const WImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// Saves the given image as the new thumbnail for the asset
  WStatus SaveThumbnail(const QImage& img, const ThumbnailInfo& thumbnailInfo) const;

  /// Appends an asset header containing the thumbnail hash to the file. Each thumbnail is appended by it to check up-to-date state.
  void AppendThumbnailInfo(WStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const;

  ///@}
  /// \name Common Asset States
  ///@{

public:
  /// Override this to handle a change to a common asset state differently.
  ///
  /// By default an on-off flag for every state is tracked, but nothing else.
  /// Also this automatically broadcasts the m_CommonAssetUiChangeEvent event.
  virtual void SetCommonAssetUiState(WCommonAssetUiState::Enum state, double value);

  /// Override this to return custom values for a common asset state.
  virtual double GetCommonAssetUiState(WCommonAssetUiState::Enum state) const;

  /// Used to broadcast state change events for common asset states.
  WEvent<const WCommonAssetUiState&> m_CommonAssetUiChangeEvent;

protected:
  WUInt32 m_uiCommonAssetStateFlags = 0;

  ///@}

protected:
  /// Adds all prefab dependencies to the WAssetDocumentInfo object. Called automatically by UpdateAssetDocumentInfo()
  void AddPrefabDependencies(const WDocumentObject* pObject, WAssetDocumentInfo* pInfo) const;

  /// Crawls through all asset properties of pObject and adds all string properties that have a WAssetBrowserAttribute as a dependency to
  /// pInfo. Automatically called by UpdateAssetDocumentInfo()
  void AddReferences(const WDocumentObject* pObject, WAssetDocumentInfo* pInfo, bool bInsidePrefab) const;

protected:
  WUniquePtr<WIPCObjectMirrorEditor> m_pMirror;

  virtual WDocumentInfo* CreateDocumentInfo() override;

  WTransformStatus DoTransformAsset(const WPlatformProfile* pAssetProfile, WBitflags<WTransformFlags> transformFlags);

  EngineStatus m_EngineStatus;
  WAssetDocEngineConnection m_EngineConnectionType = WAssetDocEngineConnection::None;

  WEditorEngineConnection* m_pEngineConnection;

  mutable WHashTable<WUuid, WEditorEngineSyncObject*> m_AllSyncObjects;
  mutable WDeque<WEditorEngineSyncObject*> m_SyncObjects;

  mutable WHybridArray<WUuid, 32> m_DeletedObjects;

  WAssetInfoFile m_TransformInfo;
};
