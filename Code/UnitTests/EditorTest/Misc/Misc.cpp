#include <EditorTest/EditorTestPCH.h>

#include "EditorFramework/Assets/AssetBrowserWidget.moc.h"
#include "Misc.h"
#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Strings/StringConversion.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <RendererCore/Components/SkyBoxComponent.h>
#include <RendererCore/Textures/TextureCubeResource.h>

static WEditorTestMisc s_EditorTestMisc;

const char* WEditorTestMisc::GetTestName() const
{
  return "Misc Tests";
}

void WEditorTestMisc::SetupSubTests()
{
  AddSubTest("GameObject References", SubTests::GameObjectReferences);
  AddSubTest("Default Values", SubTests::DefaultValues);
  AddSubTest("Asset Browser Model", SubTests::AssetBrowerModel);
}

WResult WEditorTestMisc::InitializeTest()
{
  if (SUPER::InitializeTest().Failed())
    return W_FAILURE;

  if (SUPER::OpenProject("Data/UnitTests/EditorTest").Failed())
    return W_FAILURE;

  if (WStatus res = WAssetCurator::GetSingleton()->TransformAllAssets(); res.Failed())
  {
    WLog::Error("Asset transform failed: {}", res.GetMessageString());
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WResult WEditorTestMisc::DeInitializeTest()
{
  if (SUPER::DeInitializeTest().Failed())
    return W_FAILURE;

  WMemoryTracker::DumpMemoryLeaks();

  return W_SUCCESS;
}

WTestAppRun WEditorTestMisc::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  switch (iIdentifier)
  {
    case SubTests::GameObjectReferences:
      return GameObjectReferencesTest();

    case SubTests::DefaultValues:
      return DefaultValuesTest();

    case SubTests::AssetBrowerModel:
      return AssetBrowerModelTest();
  }

  // const auto& allDesc = WDocumentManager::GetAllDocumentDescriptors();
  // for (auto* pDesc : allDesc)
  //{
  //  if (pDesc->m_bCanCreate)
  //  {
  //    WStringBuilder sName = m_sProjectPath;
  //    sName.AppendPath(pDesc->m_sDocumentTypeName);
  //    sName.ChangeFileExtension(pDesc->m_sFileExtension);
  //    WDocument* pDoc = m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::RequestWindow);
  //    W_TEST_BOOL(pDoc);
  //    ProcessEvents();
  //  }
  //}
  //// Make sure the engine process did not crash after creating every kind of document.
  // W_TEST_BOOL(!WEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed());

  ////TODO: Newly created assets actually do not transform cleanly.
  // if (false)
  //{
  //  WAssetCurator::GetSingleton()->TransformAllAssets();

  //  WUInt32 uiNumAssets;
  //  WTempHybridArray<WUInt32, WAssetInfo::TransformState::COUNT> sections;
  //  WAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  //  W_TEST_INT(sections[WAssetInfo::TransformState::TransformError], 0);
  //  W_TEST_INT(sections[WAssetInfo::TransformState::MissingDependency], 0);
  //  W_TEST_INT(sections[WAssetInfo::TransformState::MissingReference], 0);
  //}
  return WTestAppRun::Quit;
}

WResult WEditorTestMisc::InitializeSubTest(WInt32 iIdentifier)
{
  return W_SUCCESS;
}

WResult WEditorTestMisc::DeInitializeSubTest(WInt32 iIdentifier)
{
  WDocumentManager::CloseAllDocuments();
  return W_SUCCESS;
}

WTestAppRun WEditorTestMisc::GameObjectReferencesTest()
{
  m_pDocument = SUPER::OpenDocument("Scenes/GameObjectReferences.WScene");

  if (!W_TEST_BOOL(m_pDocument != nullptr))
    return WTestAppRun::Quit;

  W_ANALYSIS_ASSUME(m_pDocument != nullptr);
  WAssetCurator::GetSingleton()->TransformAsset(m_pDocument->GetGuid(), WTransformFlags::Default);

  WQtEngineDocumentWindow* pWindow = qobject_cast<WQtEngineDocumentWindow*>(WQtDocumentWindow::FindWindowByDocument(m_pDocument));

  if (!W_TEST_BOOL(pWindow != nullptr))
    return WTestAppRun::Quit;

  W_ANALYSIS_ASSUME(pWindow != nullptr);
  auto viewWidgets = pWindow->GetViewWidgets();

  if (!W_TEST_BOOL(!viewWidgets.IsEmpty()))
    return WTestAppRun::Quit;

  WQtEngineViewWidget::InteractionContext ctxt;
  ctxt.m_pLastHoveredViewWidget = viewWidgets[0];
  WQtEngineViewWidget::SetInteractionContext(ctxt);

  viewWidgets[0]->m_pViewConfig->m_RenderMode = WViewRenderMode::Default;
  viewWidgets[0]->m_pViewConfig->m_Perspective = WSceneViewPerspective::Perspective;
  viewWidgets[0]->m_pViewConfig->ApplyPerspectiveSetting(90.0f);

  ExecuteDocumentAction("Scene.Camera.JumpTo.0", m_pDocument, true);

  for (int i = 0; i < 10; ++i)
  {
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    ProcessEvents();
  }

  W_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

  W_TEST_LINE_IMAGE(1, 100);

  // Move everything to the layer and repeat the test.
  WScene2Document* pScene = WDynamicCast<WScene2Document*>(m_pDocument);
  WTempHybridArray<WUuid, 2> layerGuids;
  pScene->GetAllLayers(layerGuids);
  W_TEST_INT(layerGuids.GetCount(), 2);
  WUuid layerGuid = layerGuids[0] == pScene->GetGuid() ? layerGuids[1] : layerGuids[0];

  auto pAccessor = pScene->GetObjectAccessor();
  auto pRoot = pScene->GetObjectManager()->GetRootObject();
  WTempHybridArray<WVariant, 16> values;
  pAccessor->GetValuesByName(pRoot, "Children", values).AssertSuccess();

  WDeque<const WDocumentObject*> assets;
  for (auto& value : values)
  {
    assets.PushBack(pAccessor->GetObject(value.Get<WUuid>()));
  }
  WDeque<const WDocumentObject*> newObjects;
  MoveObjectsToLayer(pScene, assets, layerGuid, newObjects);

  W_TEST_BOOL(CaptureImage(pWindow, "GoRef").Succeeded());

  W_TEST_LINE_IMAGE(1, 100);

  return WTestAppRun::Quit;
}

bool CheckDefaultValue(const WAbstractProperty* pProperty, const WDefaultValueAttribute* pAttrib)
{
  WVariant defaultValue = pAttrib->GetValue();

  const bool isValueType = WReflectionUtils::IsValueType(pProperty);
  const WVariantType::Enum type = pProperty->GetFlags().IsSet(WPropertyFlags::Pointer) || (pProperty->GetFlags().IsSet(WPropertyFlags::Class) && !isValueType) ? WVariantType::Uuid : pProperty->GetSpecificType()->GetVariantType();

  switch (pProperty->GetCategory())
  {
    case WPropertyCategory::Member:
    {
      if (isValueType)
      {
        if (pProperty->GetSpecificType() == WGetStaticRTTI<WVariant>())
          return true;
        return pAttrib->GetValue().CanConvertTo(type);
      }
      else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags))
      {
        return pAttrib->GetValue().CanConvertTo(WVariantType::Int64);
      }
      else // Class
      {
        return false;
      }
    }
    break;
    case WPropertyCategory::Array:
    case WPropertyCategory::Set:
    {
      if (!isValueType)
        return true;

      if (!pAttrib->GetValue().IsA<WVariantArray>())
        return false;

      const auto& defaultArray = pAttrib->GetValue().Get<WVariantArray>();
      for (WUInt32 i = 0; i < defaultArray.GetCount(); ++i)
      {
        const WVariant& defaultSubValue = defaultArray[i];
        if (pProperty->GetSpecificType() == WGetStaticRTTI<WVariant>())
          continue;
        if (!defaultSubValue.CanConvertTo(type))
          return false;
      }
      return true;
    }
    break;
    case WPropertyCategory::Map:
    {
      if (isValueType)
        return true;

      if (pAttrib->GetValue().IsA<WVariantDictionary>())
        return false;

      const auto& defaultDict = pAttrib->GetValue().Get<WVariantDictionary>();
      for (auto it = defaultDict.GetIterator(); it.IsValid(); ++it)
      {
        const WVariant& defaultSubValue = it.Value();
        if (pProperty->GetSpecificType() == WGetStaticRTTI<WVariant>())
          continue;
        if (!defaultSubValue.CanConvertTo(type))
          return false;
      }
      return true;
    }
    break;
    default:
      break;
  }
  return true;
}

WTestAppRun WEditorTestMisc::DefaultValuesTest()
{
  WRTTI::ForEachType([&](const WRTTI* pRtti)
    {
      WArrayPtr<const WAbstractProperty* const> props = pRtti->GetProperties();
      for (const WAbstractProperty* pProperty : props)
      {
        const WDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<WDefaultValueAttribute>();
        if (!pAttrib)
          return;

        if (!CheckDefaultValue(pProperty, pAttrib))
        {
          WLog::Error("Invalid default value property! Type: {}, Property: {}, PropertyType: {}, DefaultValueType: {}", pRtti->GetTypeName(), pProperty->GetPropertyName(), pProperty->GetSpecificType()->GetTypeName(), pAttrib->GetValue().GetReflectedType()->GetTypeName());
        }
      } });

  return WTestAppRun::Quit;
}

WTestAppRun WEditorTestMisc::AssetBrowerModelTest()
{
  WQtAssetBrowserWidget* pDialog = new WQtAssetBrowserWidget(QApplication::activeWindow());
  pDialog->SetMode(WQtAssetBrowserWidget::Mode::Browser);
  pDialog->show();
  ProcessEvents();
  WQtAssetBrowserFilter* pFilter = pDialog->GetAssetBrowserFilter();
  WQtAssetBrowserModel* pModel = pDialog->GetAssetBrowserModel();
  pFilter->SetPathFilter("EditorTest/Meshes");
  pFilter->SetShowNonImportableFiles(false);
  pFilter->SetShowFiles(true);
  ProcessEvents();

  // Importable asset found:
  {
    W_TEST_INT(pModel->rowCount(), 1);
    QModelIndex index = pModel->index(0, 0);
    QString relativePath = pModel->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString();
    W_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/Cube.obj");
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)pModel->data(index, WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    W_TEST_INT(itemType.GetValue(), WAssetBrowserItemFlags::File);
    const bool bImportable = index.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();
    W_TEST_BOOL(bImportable);
  }

  // Import (manually) into mesh.WMeshAsset
  WUuid guid;
  {
    WStringBuilder sName = m_sProjectPath;
    sName.AppendPath("Meshes", "mesh.WMeshAsset");
    WAssetDocument* pDoc = static_cast<WAssetDocument*>(m_pApplication->m_pEditorApp->CreateDocument(sName, WDocumentFlags::Default));
    WDocumentObject* pMeshAsset = pDoc->GetObjectManager()->GetRootObject()->GetChildren()[0];
    WObjectAccessorBase* pAcc = pDoc->GetObjectAccessor();
    pAcc->StartTransaction("Edit Mesh");
    W_TEST_BOOL(pAcc->SetValueByName(pMeshAsset, "MeshFile", "Meshes/Cube.obj").Succeeded());
    pAcc->FinishTransaction();
    W_TEST_STATUS(pDoc->SaveDocument());
    guid = pDoc->GetGuid();
    pDoc->GetDocumentManager()->CloseDocument(pDoc);
  }

  // Wait for changes
  for (WUInt32 i = 0; i < 10; ++i)
  {
    ProcessEvents();
    if (pModel->rowCount() == 2)
    {
      QModelIndex index2 = pModel->index(1, 0);
      WUuid guid2 = pModel->data(index2, WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
      if (guid2 == guid)
        break;
    }
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
  }
  // Check new asset
  if (W_TEST_INT(pModel->rowCount(), 2))
  {
    QModelIndex index2 = pModel->index(1, 0);
    WUuid guid2 = index2.data(WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
    W_TEST_BOOL(guid2 == guid);
    QString relativePath = index2.data(WQtAssetBrowserModel::UserRoles::RelativePath).toString();
    W_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/mesh.WMeshAsset");
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index2.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    W_TEST_BOOL(itemType == (WAssetBrowserItemFlags::File | WAssetBrowserItemFlags::Asset));
    const bool bImportable = index2.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();
    W_TEST_BOOL(!bImportable);
  }

  // Rename Cobe.obj to zzz.obj, moving it to the back of the list and making the asset invalid
  {
    QModelIndex index = pModel->index(0, 0);
    QString absolutePath = index.data(WQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

    W_TEST_BOOL(pModel->setData(index, QString("zzz"), Qt::EditRole));
  }

  // Wait for changes
  for (WUInt32 i = 0; i < 10; ++i)
  {
    ProcessEvents();
    if (pModel->rowCount() == 2)
    {
      QModelIndex index2 = pModel->index(1, 0);
      QString sName = index2.data(Qt::EditRole).toString();

      if (sName.toUtf8().data() == "zzz"_wsv)
        break;
    }
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
  }

  // Check renamed file
  {
    QModelIndex index = pModel->index(1, 0);
    QString relativePath = pModel->data(index, WQtAssetBrowserModel::UserRoles::RelativePath).toString();

    W_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/zzz.obj");
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)pModel->data(index, WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    W_TEST_INT(itemType.GetValue(), WAssetBrowserItemFlags::File);
    const bool bImportable = index.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();
    W_TEST_BOOL(bImportable);
  }

  // Check asset
  {
    QModelIndex index = pModel->index(0, 0);
    WUuid guid2 = index.data(WQtAssetBrowserModel::UserRoles::AssetGuid).value<WUuid>();
    W_TEST_BOOL(guid2 == guid);
    QString relativePath = index.data(WQtAssetBrowserModel::UserRoles::RelativePath).toString();
    W_TEST_STRING(relativePath.toUtf8().data(), "EditorTest/Meshes/mesh.WMeshAsset");
    const WBitflags<WAssetBrowserItemFlags> itemType = (WAssetBrowserItemFlags::Enum)index.data(WQtAssetBrowserModel::UserRoles::ItemFlags).toInt();
    W_TEST_BOOL(itemType == (WAssetBrowserItemFlags::File | WAssetBrowserItemFlags::Asset));
    const bool bImportable = index.data(WQtAssetBrowserModel::UserRoles::Importable).toBool();
    W_TEST_BOOL(!bImportable);
  }

  // Wait for transform state of asset to change
  WAssetInfo::TransformState state = WAssetInfo::Unknown;
  for (WUInt32 i = 0; i < 10; ++i)
  {
    ProcessEvents();

    QModelIndex index = pModel->index(0, 0);
    state = (WAssetInfo::TransformState)index.data(WQtAssetBrowserModel::UserRoles::TransformState).toInt();
    if (state == WAssetInfo::TransformState::MissingTransformDependency)
      break;

    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
  }
  W_TEST_BOOL(state == WAssetInfo::TransformState::MissingTransformDependency);
  pDialog->hide();
  delete pDialog;
  ProcessEvents();
  return WTestAppRun::Quit;
}
