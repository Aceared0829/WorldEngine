#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Texture/Image/ImageConversion.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WAssetDocument::WAssetDocument(WStringView sDocumentPath, WDocumentObjectManager* pObjectManager, WAssetDocEngineConnection engineConnectionType)
  : WDocument(sDocumentPath, pObjectManager)
{
  m_EngineConnectionType = engineConnectionType;
  m_EngineStatus = (m_EngineConnectionType != WAssetDocEngineConnection::None) ? EngineStatus::Disconnected : EngineStatus::Unsupported;
  m_pEngineConnection = nullptr;
  m_uiCommonAssetStateFlags = WCommonAssetUiState::Grid | WCommonAssetUiState::Loop | WCommonAssetUiState::Visualizers;

  if (m_EngineConnectionType != WAssetDocEngineConnection::None)
  {
    WEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(WMakeDelegate(&WAssetDocument::EngineConnectionEventHandler, this));
  }
}

WAssetDocument::~WAssetDocument()
{
  m_pMirror->DeInit();

  if (m_EngineConnectionType != WAssetDocEngineConnection::None)
  {
    WEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(WMakeDelegate(&WAssetDocument::EngineConnectionEventHandler, this));

    if (m_pEngineConnection)
    {
      WEditorEngineProcessConnection::GetSingleton()->DestroyEngineConnection(this);
    }
  }
}

void WAssetDocument::SetCommonAssetUiState(WCommonAssetUiState::Enum state, double value)
{
  if (value == 0)
  {
    m_uiCommonAssetStateFlags &= ~((WUInt32)state);
  }
  else
  {
    m_uiCommonAssetStateFlags |= (WUInt32)state;
  }

  WCommonAssetUiState e;
  e.m_State = state;
  e.m_fValue = value;

  m_CommonAssetUiChangeEvent.Broadcast(e);
}

double WAssetDocument::GetCommonAssetUiState(WCommonAssetUiState::Enum state) const
{
  return (m_uiCommonAssetStateFlags & (WUInt32)state) != 0 ? 1.0f : 0.0f;
}

WAssetDocumentManager* WAssetDocument::GetAssetDocumentManager() const
{
  return static_cast<WAssetDocumentManager*>(GetDocumentManager());
}

const WAssetDocumentInfo* WAssetDocument::GetAssetDocumentInfo() const
{
  return static_cast<WAssetDocumentInfo*>(m_pDocumentInfo);
}

WBitflags<WAssetDocumentFlags> WAssetDocument::GetAssetFlags() const
{
  return GetAssetDocumentTypeDescriptor()->m_AssetDocumentFlags;
}

WDocumentInfo* WAssetDocument::CreateDocumentInfo()
{
  return W_DEFAULT_NEW(WAssetDocumentInfo);
}

WTaskGroupID WAssetDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  WAssetDocumentInfo* pInfo = static_cast<WAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_TransformDependencies.Clear();
  pInfo->m_ThumbnailDependencies.Clear();
  pInfo->m_PackageDependencies.Clear();
  pInfo->m_Outputs.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetsDocumentTypeName.Assign(GetDocumentTypeName());
  pInfo->ClearMetaData();
  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_TransformDependencies.Remove(WString());
  pInfo->m_ThumbnailDependencies.Remove(WString());
  pInfo->m_PackageDependencies.Remove(WString());

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  // Dependencies must be either an asset GUID or a path that other machines can resolve as well,
  // so a data directory relative path or a ':rootname/...' path. An absolute path only works on this machine.
  auto CheckForAbsolutePaths = [this](const WSet<WString>& deps, const char* szWhich)
  {
    for (const WString& sDep : deps)
    {
      W_ASSERT_DEV(!WPathUtils::IsAbsolutePath(sDep), "The {} of asset '{}' contain the absolute path '{}'. Asset dependencies must not be absolute paths.", szWhich, GetDocumentPath(), sDep);
    }
  };

  CheckForAbsolutePaths(pInfo->m_TransformDependencies, "transform dependencies");
  CheckForAbsolutePaths(pInfo->m_ThumbnailDependencies, "thumbnail dependencies");
  CheckForAbsolutePaths(pInfo->m_PackageDependencies, "package dependencies");
#endif

  return WDocument::InternalSaveDocument(callback);
}

void WAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();
  WAssetCurator::GetSingleton()->NotifyOfFileChange(GetDocumentPath());
  WAssetCurator::GetSingleton()->MainThreadTick(false);

  if (flags.IsAnySet(WAssetDocumentFlags::AutoTransformOnSave))
  {
    // If we request an engine connection but the mirror is not set up yet we are still
    // creating the document and TransformAsset will most likely fail.
    if (m_EngineConnectionType == WAssetDocEngineConnection::None || m_pEngineConnection)
    {
      WUuid docGuid = GetGuid();

      WSharedPtr<WDelegateTask<void>> pTask = W_DEFAULT_NEW(WDelegateTask<void>, "TransformAfterSaveDocument", WTaskNesting::Never, [docGuid]()
        {
          WDocument* pDoc = WDocumentManager::GetDocumentByGuid(docGuid);
          if (pDoc == nullptr)
            return;

          /// \todo Should only be done for platform agnostic assets
          WTransformStatus ret = WAssetCurator::GetSingleton()->TransformAsset(docGuid, WTransformFlags::TriggeredManually);

          if (ret.Failed())
          {
            WLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, pDoc->GetDocumentPath());
          }
          else
          {
            WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
          }
          //
        });

      pTask->ConfigureTask("TransformAfterSaveDocument", WTaskNesting::Maybe);
      WTaskSystem::StartSingleTask(pTask, WTaskPriority::ThisFrameMainThread);
    }
  }
}

void WAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = W_DEFAULT_NEW(WIPCObjectMirrorEditor);
}

void WAssetDocument::InitializeAfterLoadingAndSaving()
{
  if (m_EngineConnectionType != WAssetDocEngineConnection::None)
  {
    m_pEngineConnection = WEditorEngineProcessConnection::GetSingleton()->CreateEngineConnection(this);
    m_EngineStatus = EngineStatus::Initializing;

    if (m_EngineConnectionType == WAssetDocEngineConnection::FullObjectMirroring)
    {
      m_pMirror->SetIPC(m_pEngineConnection);
      m_pMirror->InitSender(GetObjectManager());
    }
  }
}

void WAssetDocument::AddPrefabDependencies(const WDocumentObject* pObject, WAssetDocumentInfo* pInfo) const
{
  {
    const WDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      WStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(WConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }


  const WTempHybridArray<WDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;
    AddPrefabDependencies(pChild, pInfo);
  }
}


void WAssetDocument::AddReferences(const WDocumentObject* pObject, WAssetDocumentInfo* pInfo, bool bInsidePrefab) const
{
  {
    const WDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      bInsidePrefab = true;
      WStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(WConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
      pInfo->m_ThumbnailDependencies.Insert(WConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }

  const WRTTI* pType = pObject->GetTypeAccessor().GetType();
  WTempHybridArray<const WAbstractProperty*, 32> Properties;
  pType->GetAllProperties(Properties);
  for (auto pProp : Properties)
  {
    if (pProp->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    WBitflags<WDependencyFlags> depFlags;

    if (auto pAttr = pProp->GetAttributeByType<WAssetBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    if (auto pAttr = pProp->GetAttributeByType<WFileBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    const auto propVarType = pProp->GetSpecificType()->GetVariantType();
    if (propVarType != WVariantType::String && propVarType != WVariantType::StringView)
      continue;

    // add all strings that are marked as asset references or file references
    if (depFlags != 0)
    {
      switch (pProp->GetCategory())
      {
        case WPropertyCategory::Member:
        {
          if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
          {
            if (bInsidePrefab)
            {
              WTempHybridArray<WPropertySelection, 1> selection;
              selection.PushBack({pObject, WVariant()});
              WDefaultObjectState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr());
              if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultValue(pProp))
                continue;
            }

            const WVariant& value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

            if (depFlags.IsSet(WDependencyFlags::Transform))
              pInfo->m_TransformDependencies.Insert(value.Get<WString>());

            if (depFlags.IsSet(WDependencyFlags::Thumbnail))
              pInfo->m_ThumbnailDependencies.Insert(value.Get<WString>());

            if (depFlags.IsSet(WDependencyFlags::Package))
              pInfo->m_PackageDependencies.Insert(value.Get<WString>());
          }
        }
        break;

        case WPropertyCategory::Array:
        case WPropertyCategory::Set:
        {
          if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
          {
            const WInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

            if (bInsidePrefab)
            {
              WTempHybridArray<WPropertySelection, 1> selection;
              selection.PushBack({pObject, WVariant()});
              WDefaultContainerState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (WInt32 i = 0; i < iCount; ++i)
              {
                WVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(i))
                {
                  continue;
                }
                if (depFlags.IsSet(WDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(value.Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(value.Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(value.Get<WString>());
              }
            }
            else
            {
              for (WInt32 i = 0; i < iCount; ++i)
              {
                WVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);

                if (depFlags.IsSet(WDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(value.Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(value.Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(value.Get<WString>());
              }
            }
          }
        }
        break;

        case WPropertyCategory::Map:
          // #TODO Search for exposed params that reference assets.
          if (pProp->GetFlags().IsSet(WPropertyFlags::StandardType))
          {
            WVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());
            const WVariantDictionary& varDict = value.Get<WVariantDictionary>();
            if (bInsidePrefab)
            {
              WTempHybridArray<WPropertySelection, 1> selection;
              selection.PushBack({pObject, WVariant()});
              WDefaultContainerState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (auto it : varDict)
              {
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(it.Key()))
                {
                  continue;
                }

                if (depFlags.IsSet(WDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(it.Value().Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(it.Value().Get<WString>());
              }
            }
            else
            {
              for (auto it : varDict)
              {
                if (depFlags.IsSet(WDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(it.Value().Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<WString>());

                if (depFlags.IsSet(WDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(it.Value().Get<WString>());
              }
            }
          }
          break;

        default:
          break;
      }
    }
  }

  const WTempHybridArray<WDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;

    AddReferences(pChild, pInfo, bInsidePrefab);
  }
}

void WAssetDocument::UpdateAssetDocumentInfo(WAssetDocumentInfo* pInfo) const
{
  const WDocumentObject* pRoot = GetObjectManager()->GetRootObject();

  AddPrefabDependencies(pRoot, pInfo);
  AddReferences(pRoot, pInfo, false);
}

void WAssetDocument::EngineConnectionEventHandler(const WEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == WEditorEngineProcessConnection::Event::Type::ProcessCrashed)
  {
    m_EngineStatus = EngineStatus::Disconnected;
  }
  else if (e.m_Type == WEditorEngineProcessConnection::Event::Type::ProcessStarted)
  {
    m_EngineStatus = EngineStatus::Initializing;
  }
}

WUInt64 WAssetDocument::GetDocumentHash() const
{
  WUInt64 uiHash = WHashingUtils::xxHash64(&m_pDocumentInfo->m_DocumentID, sizeof(WUuid));
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<WTemporaryAttribute>() != nullptr)
      continue;
    GetChildHash(pChild, uiHash);
    InternalGetMetaDataHash(pChild, uiHash);
  }

  // Gather used types, sort by name to make it stable and hash their data
  WSet<const WRTTI*> types;
  WToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
  WDynamicArray<const WRTTI*> typesSorted;
  typesSorted.Reserve(types.GetCount());
  for (const WRTTI* pType : types)
  {
    typesSorted.PushBack(pType);
  }

  typesSorted.Sort([](const WRTTI* a, const WRTTI* b)
    { return a->GetTypeName().Compare(b->GetTypeName()) < 0; });

  for (const WRTTI* pType : typesSorted)
  {
    uiHash = WHashingUtils::xxHash64(pType->GetTypeName().GetStartPointer(), pType->GetTypeName().GetElementCount(), uiHash);
    const WUInt32 uiType = pType->GetTypeVersion();
    uiHash = WHashingUtils::xxHash64(&uiType, sizeof(uiType), uiHash);
  }
  return uiHash;
}

void WAssetDocument::GetChildHash(const WDocumentObject* pObject, WUInt64& uiHash) const
{
  pObject->ComputeObjectHash(uiHash);

  for (auto pChild : pObject->GetChildren())
  {
    GetChildHash(pChild, uiHash);
  }
}

WTransformStatus WAssetDocument::DoTransformAsset(const WPlatformProfile* pAssetProfile0 /*= nullptr*/, WBitflags<WTransformFlags> transformFlags)
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(WAssetDocumentFlags::DisableTransform))
    return WStatus("Asset transform has been disabled on this asset");

  if (GetUnknownObjectTypeInstances() > 0)
  {
    return WStatus("Asset contains unknown object types. Please open the document and fix the errors.");
  }

  if (!GetLoadingErrors().IsEmpty())
  {
    return WStatus("Asset had loading errors. Please open the document and fix the errors.");
  }

  const WPlatformProfile* pAssetProfile = WAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);

  WUInt64 uiHash = 0;
  WUInt64 uiThumbHash = 0;
  WUInt64 uiPackageHash = 0;
  WAssetInfo::TransformState state = WAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), pAssetProfile, GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash, uiPackageHash);

  if (state == WAssetInfo::TransformState::UpToDate && !transformFlags.IsSet(WTransformFlags::ForceTransform))
    return WStatus(W_SUCCESS);

  if (uiHash == 0)
    return WStatus("Computing the hash for this asset or any dependency failed");

  // Write resource
  {
    WAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());
    const auto& outputs = GetAssetDocumentInfo()->m_Outputs;

    auto GenerateOutput = [this, pAssetProfile, &AssetHeader, transformFlags](const char* szOutputTag) -> WTransformStatus
    {
      const WString sTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), szOutputTag, pAssetProfile);

      m_TransformInfo.Clear();

      WTransformStatus ret = InternalTransformAsset(sTargetFile, szOutputTag, pAssetProfile, AssetHeader, transformFlags);

      const WStringBuilder sInfoFile = WAssetInfoFile::GetInfoFilePathForOutput(sTargetFile);

      // if writing failed, make sure the output file does not exist
      if (ret.Failed())
      {
        WFileSystem::DeleteFile(sTargetFile);
        WOSFile::DeleteFile(sInfoFile).IgnoreResult();
      }
      else if (!m_TransformInfo.IsEmpty())
      {
        // An empty map is left alone rather than deleting the file, because some asset types have the
        // external tool that generates the output write this file directly (e.g. TexConv).
        if (m_TransformInfo.WriteToFile(sInfoFile, AssetHeader).Failed())
        {
          WLog::Warning("Failed to write asset info file '{}'", sInfoFile);
        }
      }

      m_TransformInfo.Clear();

      WAssetCurator::GetSingleton()->NotifyOfFileChange(sTargetFile);
      return ret;
    };

    WTransformStatus res;
    for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
    {
      res = GenerateOutput(it.Key());
      if (res.Failed())
        return res;
    }

    res = GenerateOutput("");
    if (res.Failed())
      return res;

    WAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
}

WTransformStatus WAssetDocument::TransformAsset(WBitflags<WTransformFlags> transformFlags, const WPlatformProfile* pAssetProfile)
{
  W_PROFILE_SCOPE("TransformAsset");

  if (!transformFlags.IsSet(WTransformFlags::ForceTransform))
  {
    W_SUCCEED_OR_RETURN(SaveDocument());

    const auto assetFlags = GetAssetFlags();

    if (assetFlags.IsSet(WAssetDocumentFlags::DisableTransform) || (assetFlags.IsSet(WAssetDocumentFlags::OnlyTransformManually) && !transformFlags.IsSet(WTransformFlags::TriggeredManually)))
    {
      return WStatus(W_SUCCESS);
    }
  }

  const WTransformStatus res = DoTransformAsset(pAssetProfile, transformFlags);

  if (transformFlags.IsSet(WTransformFlags::TriggeredManually))
  {
    SaveDocument().LogFailure();
    WAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
  }

  return res;
}

WTransformStatus WAssetDocument::CreateThumbnail()
{
  WUInt64 uiHash = 0;
  WUInt64 uiThumbHash = 0;
  WUInt64 uiPackageHash = 0;

  WAssetInfo::TransformState state = WAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), WAssetCurator::GetSingleton()->GetActiveAssetProfile(), GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash, uiPackageHash);

  if (state == WAssetInfo::TransformState::UpToDate)
    return WStatus(W_SUCCESS);

  if (uiHash == 0)
    return WStatus("Computing the hash for this asset or any dependency failed");

  if (state == WAssetInfo::NeedsThumbnail)
  {
    ThumbnailInfo ThumbnailInfo;
    ThumbnailInfo.SetFileHashAndVersion(uiThumbHash, GetAssetTypeVersion());
    WTransformStatus res = InternalCreateThumbnail(ThumbnailInfo);

    InvalidateAssetThumbnail();
    WAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
  return WTransformStatus(WFmt("Asset state is {}", state));
}

WTransformStatus WAssetDocument::InternalTransformAsset(const char* szTargetFile, WStringView sOutputTag, const WPlatformProfile* pAssetProfile, const WAssetFileHeader& AssetHeader, WBitflags<WTransformFlags> transformFlags)
{
  WDeferredFileWriter file;
  file.SetOutput(szTargetFile);

  if (AssetHeader.Write(file) == W_FAILURE)
  {
    file.Discard();
    return WTransformStatus("Failed to write asset header");
  }

  WTransformStatus res = InternalTransformAsset(file, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  if (res.m_Result != WTransformResult::Success)
  {
    // We do not want to overwrite the old output file if we failed to transform the asset.
    file.Discard();
    return res;
  }


  if (file.Close().Failed())
  {
    WLog::Error("Could not open file for writing: '{0}'", szTargetFile);
    return WStatus("Opening the asset output file failed");
  }

  return WStatus(W_SUCCESS);
}

WString WAssetDocument::GetThumbnailFilePath(WStringView sSubAssetName /*= WStringView()*/) const
{
  return GetAssetDocumentManager()->GenerateResourceThumbnailPath(GetDocumentPath(), sSubAssetName);
}

void WAssetDocument::InvalidateAssetThumbnail(WStringView sSubAssetName /*= WStringView()*/) const
{
  const WString sResourceFile = GetThumbnailFilePath(sSubAssetName);
  WAssetCurator::GetSingleton()->NotifyOfFileChange(sResourceFile);
  WQtImageCache::GetSingleton()->InvalidateCache(sResourceFile);
}

WStatus WAssetDocument::SaveThumbnail(const WImage& img, const ThumbnailInfo& thumbnailInfo) const
{
  WImage converted;

  // make sure the thumbnail is in a format that Qt understands

  /// \todo A conversion to B8G8R8X8_UNORM currently fails

  if (WImageConversion::Convert(img, converted, WImageFormat::R8G8B8A8_UNORM).Failed())
  {
    const WStringBuilder sResourceFile = GetThumbnailFilePath();

    WLog::Error("Could not convert asset thumbnail to target format: '{0}'", sResourceFile);
    return WStatus(WFmt("Could not convert asset thumbnail to target format: '{0}'", sResourceFile));
  }

  QImage qimg(converted.GetPixelPointer<WUInt8>(), converted.GetWidth(), converted.GetHeight(), QImage::Format_RGBA8888);

  return SaveThumbnail(qimg, thumbnailInfo);
}

WStatus WAssetDocument::SaveThumbnail(const QImage& qimg0, const ThumbnailInfo& thumbnailInfo) const
{
  const WStringBuilder sResourceFile = GetThumbnailFilePath();
  W_LOG_BLOCK("Save Asset Thumbnail", sResourceFile.GetData());

  QImage qimg = qimg0;

  if (qimg.width() == qimg.height())
  {
    // if necessary scale the image to the proper size
    if (qimg.width() != WThumbnailSize)
      qimg = qimg.scaled(WThumbnailSize, WThumbnailSize, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
  }
  else
  {
    // center the image in a square canvas

    // scale the longer edge to WThumbnailSize
    if (qimg.width() > qimg.height())
      qimg = qimg.scaledToWidth(WThumbnailSize, Qt::TransformationMode::SmoothTransformation);
    else
      qimg = qimg.scaledToHeight(WThumbnailSize, Qt::TransformationMode::SmoothTransformation);

    // create a black canvas
    QImage img2(WThumbnailSize, WThumbnailSize, QImage::Format_RGBA8888);
    img2.fill(Qt::GlobalColor::black);

    QPoint destPos = QPoint((WThumbnailSize - qimg.width()) / 2, (WThumbnailSize - qimg.height()) / 2);

    // paint the smaller image such that it ends up centered
    QPainter painter(&img2);
    painter.drawImage(destPos, qimg);
    painter.end();

    qimg = img2;
  }

  // make sure the directory exists, Qt will not create sub-folders
  const WStringBuilder sDir = sResourceFile.GetFileDirectory();
  W_SUCCEED_OR_RETURN(WOSFile::CreateDirectoryStructure(sDir));

  // save to JPEG
  if (!qimg.save(QString::fromUtf8(sResourceFile.GetData()), nullptr, 90))
  {
    WLog::Error("Could not save asset thumbnail: '{0}'", sResourceFile);
    return WStatus(WFmt("Could not save asset thumbnail: '{0}'", sResourceFile));
  }

  AppendThumbnailInfo(sResourceFile, thumbnailInfo);
  InvalidateAssetThumbnail();

  return WStatus(W_SUCCESS);
}

void WAssetDocument::AppendThumbnailInfo(WStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const
{
  WContiguousMemoryStreamStorage storage;
  {
    WFileReader reader;
    if (reader.Open(sThumbnailFile).Failed())
    {
      return;
    }
    storage.ReadAll(reader);
  }

  WDeferredFileWriter writer;
  writer.SetOutput(sThumbnailFile);
  writer.WriteBytes(storage.GetData(), storage.GetStorageSize64()).IgnoreResult();

  thumbnailInfo.Serialize(writer).IgnoreResult();

  if (writer.Close().Failed())
  {
    WLog::Error("Could not open file for writing: '{0}'", sThumbnailFile);
  }
}

WStatus WAssetDocument::RemoteExport(const WAssetFileHeader& header, const char* szOutputTarget) const
{
  WProgressRange range("Exporting Asset", 2, false);

  WLog::Info("Exporting {0} to \"{1}\"", GetDocumentTypeName(), szOutputTarget);

  W_SUCCEED_OR_RETURN(WaitForEngineStatusLoaded());

  range.BeginNextStep(szOutputTarget);

  WExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  WStatus status(W_SUCCESS);
  WProcessCommunicationChannel::WaitForMessageCallback callback = [&status](WProcessMessage* pMsg) -> bool
  {
    WExportDocumentMsgToEditor* pMsg2 = WDynamicCast<WExportDocumentMsgToEditor*>(pMsg);

    if (!pMsg2->m_bOutputSuccess)
      status = WStatus(pMsg2->m_sFailureMsg.GetView());

    return true;
  };

  if (WEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), WExportDocumentMsgToEditor::GetStaticRTTI(), WTime::MakeFromSeconds(60), &callback).Failed())
  {
    return WStatus(WFmt("Remote exporting {0} to \"{1}\" timed out.", GetDocumentTypeName(), msg.m_sOutputFile));
  }
  else
  {
    if (status.Failed())
    {
      return status;
    }

    WLog::Success("{0} \"{1}\" has been exported.", GetDocumentTypeName(), msg.m_sOutputFile);

    ShowDocumentStatus(WFmt("{0} exported successfully", GetDocumentTypeName()));

    return WStatus(W_SUCCESS);
  }
}

WTransformStatus WAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo)
{
  W_ASSERT_NOT_IMPLEMENTED;
  return WStatus("Not implemented");
}

WStatus WAssetDocument::RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, WArrayPtr<WStringView> viewExclusionTags) const
{
  WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  WLog::Info("Create {0} thumbnail for \"{1}\"", GetDocumentTypeName(), GetDocumentPath());

  if (GetEngineStatus() == WAssetDocument::EngineStatus::Disconnected)
  {
    return WStatus(WFmt("Create {0} thumbnail for \"{1}\" failed, engine not started or crashed.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == WAssetDocument::EngineStatus::Initializing)
  {
    if (WEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), WDocumentOpenResponseMsgToEditor::GetStaticRTTI(), WTime::MakeFromSeconds(10)).Failed())
    {
      return WStatus(WFmt("Create {0} thumbnail for \"{1}\" failed, document initialization timed out.", GetDocumentTypeName(), GetDocumentPath()));
    }
    W_ASSERT_DEV(GetEngineStatus() == WAssetDocument::EngineStatus::Loaded, "After receiving WDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  SyncObjectsToEngine();
  WCreateThumbnailMsgToEngine msg;
  msg.m_uiWidth = WThumbnailSize;
  msg.m_uiHeight = WThumbnailSize;
  for (const WStringView& tag : viewExclusionTags)
  {
    msg.m_ViewExcludeTags.PushBack(tag);
  }
  GetEditorEngineConnection()->SendMessage(&msg);

  WDataBuffer data;
  WProcessCommunicationChannel::WaitForMessageCallback callback = [&data](WProcessMessage* pMsg) -> bool
  {
    WCreateThumbnailMsgToEditor* pThumbnailMsg = WDynamicCast<WCreateThumbnailMsgToEditor*>(pMsg);
    data = pThumbnailMsg->m_ThumbnailData;
    return true;
  };

  if (WEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), WCreateThumbnailMsgToEditor::GetStaticRTTI(), WTime::MakeFromSeconds(60), &callback).Failed())
  {
    return WStatus(WFmt("Create {0} thumbnail for \"{1}\" failed timed out.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return WStatus(WFmt("Thumbnail generation for {0} failed, thumbnail data is empty.", GetDocumentTypeName()));
    }

    WImageHeader imgHeader;
    imgHeader.SetImageFormat(WImageFormat::R8G8B8A8_UNORM);
    imgHeader.SetWidth(msg.m_uiWidth);
    imgHeader.SetHeight(msg.m_uiHeight);

    WImage image;
    image.ResetAndAlloc(imgHeader);
    W_ASSERT_DEV(data.GetCount() == imgHeader.ComputeDataSize(), "Thumbnail WImage has different size than data buffer!");
    WMemoryUtils::Copy(image.GetPixelPointer<WUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, thumbnailInfo).LogFailure();

    WLog::Success("{0} thumbnail for \"{1}\" has been exported.", GetDocumentTypeName(), GetDocumentPath());

    ShowDocumentStatus(WFmt("{0} thumbnail created successfully", GetDocumentTypeName()));

    return WStatus(W_SUCCESS);
  }
}

WUInt16 WAssetDocument::GetAssetTypeVersion() const
{
  return (WUInt16)GetDynamicRTTI()->GetTypeVersion();
}


WStatus WAssetDocument::WaitForEngineStatusLoaded() const
{
  if (GetEngineStatus() == WAssetDocument::EngineStatus::Disconnected)
  {
    return WStatus(WFmt("Loading {0} document '{1}' failed, engine not started or crashed.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == WAssetDocument::EngineStatus::Initializing)
  {
    if (WEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), WDocumentOpenResponseMsgToEditor::GetStaticRTTI(), {}).Failed())
    {
      return WStatus(WFmt("Loading {0} document '{1}' failed, document initialization timed out or engine crashed.", GetDocumentTypeName(), GetDocumentPath()));
    }
    W_ASSERT_DEV(GetEngineStatus() == WAssetDocument::EngineStatus::Loaded, "After receiving WDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }
  return WStatus(W_SUCCESS);
}

bool WAssetDocument::SendMessageToEngine(WEditorEngineDocumentMsg* pMessage /*= false*/) const
{
  return GetEditorEngineConnection()->SendMessage(pMessage);
}

void WAssetDocument::HandleEngineMessage(const WEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<WDocumentOpenResponseMsgToEditor>())
  {
    if (m_EngineConnectionType == WAssetDocEngineConnection::FullObjectMirroring)
    {
      // make sure the engine clears the document first
      WDocumentClearMsgToEngine msgClear;
      msgClear.m_DocumentGuid = GetGuid();
      SendMessageToEngine(&msgClear);

      m_pMirror->SendDocument();
    }
    m_EngineStatus = EngineStatus::Loaded;
    // make sure all sync objects are 'modified' so that they will get resent as well
    for (auto* pObject : m_SyncObjects)
    {
      pObject->SetModified();
    }
  }

  m_ProcessMessageEvent.Broadcast(pMsg);
}

void WAssetDocument::AddSyncObject(WEditorEngineSyncObject* pSync) const
{
  pSync->Configure(GetGuid(), [this](WEditorEngineSyncObject* pSync)
    { RemoveSyncObject(pSync); });

  m_SyncObjects.PushBack(pSync);
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void WAssetDocument::RemoveSyncObject(WEditorEngineSyncObject* pSync) const
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveAndSwap(pSync);
}

WEditorEngineSyncObject* WAssetDocument::FindSyncObject(const WUuid& guid) const
{
  WEditorEngineSyncObject* pSync = nullptr;
  m_AllSyncObjects.TryGetValue(guid, pSync);
  return pSync;
}

WEditorEngineSyncObject* WAssetDocument::FindSyncObject(const WRTTI* pType) const
{
  for (WEditorEngineSyncObject* pSync : m_SyncObjects)
  {
    if (pSync->GetDynamicRTTI() == pType)
    {
      return pSync;
    }
  }
  return nullptr;
}

void WAssetDocument::SyncObjectsToEngine() const
{
  // Tell the engine which sync objects have been removed recently
  {
    for (const auto& guid : m_DeletedObjects)
    {
      WEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = guid;
      SendMessageToEngine(&msg);
    }

    m_DeletedObjects.Clear();
  }

  for (auto* pObject : m_SyncObjects)
  {
    if (!pObject->GetModified())
      continue;

    WEditorEngineSyncObjectMsg msg;
    msg.m_ObjectGuid = pObject->m_SyncObjectGuid;
    msg.m_sObjectType = pObject->GetDynamicRTTI()->GetTypeName();

    WContiguousMemoryStreamStorage storage;
    WMemoryStreamWriter writer(&storage);
    WMemoryStreamReader reader(&storage);

    WReflectionSerializer::WriteObjectToBinary(writer, pObject->GetDynamicRTTI(), pObject);
    msg.m_ObjectData = WArrayPtr<const WUInt8>(storage.GetData(), storage.GetStorageSize32());

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

void WAssetDocument::SendDocumentOpenMessage(bool bOpen)
{
  W_PROFILE_SCOPE("SendDocumentOpenMessage");

  // it is important to have up-to-date lookup tables in the engine process, because document contexts might try to
  // load resources, and if the file redirection does not happen correctly, derived resource types may not be created as they should
  WAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  m_EngineStatus = EngineStatus::Initializing;

  WDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = GetDocumentTypeDescriptor()->m_sDocumentTypeName;
  m.m_DocumentMetaData = GetCreateEngineMetaData();

  if (!WEditorEngineProcessConnection::GetSingleton()->SendMessage(&m))
  {
    WLog::Error("Failed to send DocumentOpenMessage");
  }
}

namespace
{
  static const char* szThumbnailInfoTag = "WThumb";
}

WResult WAssetDocument::ThumbnailInfo::Deserialize(WStreamReader& inout_reader)
{
  char tag[8] = {0};

  if (inout_reader.ReadBytes(tag, 7) != 7)
    return W_FAILURE;

  if (!WStringUtils::IsEqual(tag, szThumbnailInfoTag))
  {
    return W_FAILURE;
  }

  inout_reader >> m_uiHash;
  inout_reader >> m_uiVersion;
  inout_reader >> m_uiReserved;

  return W_SUCCESS;
}

WResult WAssetDocument::ThumbnailInfo::Serialize(WStreamWriter& inout_writer) const
{
  W_SUCCEED_OR_RETURN(inout_writer.WriteBytes(szThumbnailInfoTag, 7));

  inout_writer << m_uiHash;
  inout_writer << m_uiVersion;
  inout_writer << m_uiReserved;

  return W_SUCCESS;
}
