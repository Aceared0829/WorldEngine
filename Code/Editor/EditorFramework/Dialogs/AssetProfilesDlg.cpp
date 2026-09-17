#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Dialogs/AssetProfilesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Platform/PlatformDesc.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class WAssetProfilesObjectManager : public WDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override { out_types.PushBack(WGetStaticRTTI<WPlatformProfile>()); }
};

class WAssetProfilesDocument : public WDocument
{
  W_ADD_DYNAMIC_REFLECTION(WAssetProfilesDocument, WDocument);

public:
  WAssetProfilesDocument(WStringView sDocumentPath)
    : WDocument(sDocumentPath, W_DEFAULT_NEW(WAssetProfilesObjectManager))
  {
  }

public:
  virtual WDocumentInfo* CreateDocumentInfo() override { return W_DEFAULT_NEW(WDocumentInfo); }
};

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAssetProfilesDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

class WQtAssetConfigAdapter : public WQtNameableAdapter
{
public:
  WQtAssetConfigAdapter(const WQtAssetProfilesDlg* pDialog, const WDocumentObjectManager* pTree, const WRTTI* pType)
    : WQtNameableAdapter(pTree, pType, "", "Name")
  {
    m_pDialog = pDialog;
  }

  virtual QVariant data(const WDocumentObject* pObject, int iRow, int iColumn, int iRole) const override
  {
    if (iColumn == 0)
    {
      if (iRole == Qt::DecorationRole)
      {
        const WString sTargetPlatform = pObject->GetTypeAccessor().GetValue("TargetPlatform").ConvertTo<WString>();

        const WStringBuilder sIconName(":Platforms/Icons/Platform", sTargetPlatform, ".svg");

        return WQtUiServices::GetSingleton()->GetCachedIconResource(sIconName);
      }

      if (iRole == Qt::DisplayRole)
      {
        QString name = WQtNameableAdapter::data(pObject, iRow, iColumn, iRole).toString();

        if (iRow == WAssetCurator::GetSingleton()->GetActiveAssetProfileIndex())
        {
          name += " (active)";
        }
        else if (iRow == m_pDialog->m_uiActiveConfig)
        {
          name += " (switch to)";
        }

        return name;
      }
    }

    return WQtNameableAdapter::data(pObject, iRow, iColumn, iRole);
  }

private:
  const WQtAssetProfilesDlg* m_pDialog = nullptr;
};

WQtAssetProfilesDlg::WQtAssetProfilesDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  // do not allow to delete or rename the first item
  DeleteButton->setEnabled(false);
  RenameButton->setEnabled(false);

  {
    auto& platEnum = WDynamicStringEnum::CreateDynamicEnum("TargetPlatformNames");
    platEnum.Clear();

    for (auto pDesc = WPlatformDesc::GetFirstInstance(); pDesc != nullptr; pDesc = pDesc->GetNextInstance())
    {
      platEnum.AddValidValue(pDesc->GetName(), true);
    }
  }

  m_pDocument = W_DEFAULT_NEW(WAssetProfilesDocument, "<none>");
  m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WQtAssetProfilesDlg::SelectionEventHandler, this));

  std::unique_ptr<WQtDocumentTreeModel> pModel(new WQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new WQtDummyAdapter(m_pDocument->GetObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "Children"));
  pModel->AddAdapter(new WQtAssetConfigAdapter(this, m_pDocument->GetObjectManager(), WPlatformProfile::GetStaticRTTI()));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->SetAllowDragDrop(false);
  Tree->SetAllowDeleteObjects(false);
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  connect(Tree, &QTreeView::doubleClicked, this, &WQtAssetProfilesDlg::OnItemDoubleClicked);

  AllAssetProfilesToObject();

  Properties->SetDocument(m_pDocument);

  auto& rootChildArray = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();

  if (!rootChildArray.IsEmpty())
  {
    m_pDocument->GetSelectionManager()->SetSelection(rootChildArray[WAssetCurator::GetSingleton()->GetActiveAssetProfileIndex()]);
  }
}

WQtAssetProfilesDlg::~WQtAssetProfilesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  W_DEFAULT_DELETE(m_pDocument);
}

WUuid WQtAssetProfilesDlg::NativeToObject(WPlatformProfile* pProfile)
{
  const WRTTI* pType = pProfile->GetDynamicRTTI();
  // Write properties to graph.
  WAbstractObjectGraph graph;
  WRttiConverterContext context;
  WRttiConverterWriter conv(&graph, &context, true, true);

  const WUuid guid = WUuid::MakeUuid();
  context.RegisterObject(guid, pType, pProfile);
  WAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pProfile, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  WDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  WDocumentObjectConverterReader objectConverter(&graph, m_pDocument->GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void WQtAssetProfilesDlg::ObjectToNative(WUuid objectGuid, WPlatformProfile* pProfile)
{
  WDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(objectGuid);
  const WRTTI* pType = pObject->GetTypeAccessor().GetType();

  // Write object to graph.
  WAbstractObjectGraph graph;
  auto filter = [](const WDocumentObject*, const WAbstractProperty* pProp) -> bool
  {
    if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
      return false;
    return true;
  };
  WDocumentObjectConverterWriter objectConverter(&graph, m_pDocument->GetObjectManager(), filter);
  WAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pObject, "root");

  // Read from graph and write to native object.
  WRttiConverterContext context;
  WRttiConverterReader conv(&graph, &context);

  conv.ApplyPropertiesToObject(pNode, pType, pProfile);
}


void WQtAssetProfilesDlg::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  const auto& selection = m_pDocument->GetSelectionManager()->GetSelection();

  const bool bAllowModification = !selection.IsEmpty() && (selection[0] != m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  DeleteButton->setEnabled(bAllowModification);
  RenameButton->setEnabled(bAllowModification);
}

void WQtAssetProfilesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();

  // SaveAssetProfileRuntimeConfig
  for (WUInt32 i = 0; i < WAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    WStringBuilder sProfileRuntimeDataFile;

    WPlatformProfile* pProfile = WAssetCurator::GetSingleton()->GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".WProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
  }

  accept();

  WAssetCurator::GetSingleton()->SaveAssetProfiles().IgnoreResult();
}

void WQtAssetProfilesDlg::on_ButtonCancel_clicked()
{
  m_uiActiveConfig = WAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();
  reject();
}

void WQtAssetProfilesDlg::OnItemDoubleClicked(QModelIndex idx)
{
  if (m_uiActiveConfig == idx.row())
    return;

  const QModelIndex oldIdx = Tree->model()->index(m_uiActiveConfig, 0);

  m_uiActiveConfig = idx.row();

  QVector<int> roles;
  roles.push_back(Qt::DisplayRole);
  Tree->model()->dataChanged(idx, idx, roles);
  Tree->model()->dataChanged(oldIdx, oldIdx, roles);
}

bool WQtAssetProfilesDlg::CheckProfileNameUniqueness(const char* szName)
{
  if (WStringUtils::IsNullOrEmpty(szName))
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("Empty strings are not allowed as profile names.");
    return false;
  }

  if (!WStringUtils::IsValidIdentifierName(szName))
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("Profile names may only contain characters, digits and underscores.");
    return false;
  }

  const auto& objects = m_pDocument->GetObjectManager()->GetRootObject()->GetChildren();
  for (const WDocumentObject* pObject : objects)
  {
    if (pObject->GetTypeAccessor().GetValue("Name").ConvertTo<WString>().IsEqual_NoCase(szName))
    {
      WQtUiServices::GetSingleton()->MessageBoxInformation("A profile with this name already exists.");
      return false;
    }
  }

  return true;
}

bool WQtAssetProfilesDlg::DetermineNewProfileName(QWidget* parent, WString& result)
{
  while (true)
  {
    bool ok = false;
    result = QInputDialog::getText(parent, "Profile Name", "New Name:", QLineEdit::Normal, "", &ok).toUtf8().data();

    if (!ok)
      return false;

    if (CheckProfileNameUniqueness(result))
      return true;
  }
}

void WQtAssetProfilesDlg::on_AddButton_clicked()
{
  WString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  WPlatformProfile profile;
  profile.SetConfigName(sProfileName);
  profile.AddMissingConfigs();

  auto& binding = m_ProfileBindings[NativeToObject(&profile)];
  binding.m_pProfile = nullptr;
  binding.m_State = Binding::State::Added;

  // select the new profile
  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren().PeekBack());
}

void WQtAssetProfilesDlg::on_DeleteButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to delete the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  if (WQtUiServices::GetSingleton()->MessageBoxQuestion(WFmt("Delete the selected profile?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
    return;

  m_ProfileBindings[sel[0]->GetGuid()].m_State = Binding::State::Deleted;

  m_pDocument->GetCommandHistory()->StartTransaction("Delete Profile");

  WRemoveObjectCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();

  m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void WQtAssetProfilesDlg::on_RenameButton_clicked()
{
  const auto& sel = m_pDocument->GetSelectionManager()->GetSelection();
  if (sel.IsEmpty())
    return;

  // do not allow to rename the first object
  if (sel[0] == m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0])
    return;

  WString sProfileName;
  if (!DetermineNewProfileName(this, sProfileName))
    return;

  m_pDocument->GetCommandHistory()->StartTransaction("Rename Profile");

  WSetObjectPropertyCommand cmd;
  cmd.m_Object = sel[0]->GetGuid();
  cmd.m_sProperty = "Name";
  cmd.m_NewValue = sProfileName;

  m_pDocument->GetCommandHistory()->AddCommand(cmd).AssertSuccess();

  m_pDocument->GetCommandHistory()->FinishTransaction();
}

void WQtAssetProfilesDlg::on_SwitchToButton_clicked()
{
  const auto& sel = Tree->selectionModel()->selectedRows();
  if (sel.isEmpty())
    return;

  OnItemDoubleClicked(sel[0]);
}

void WQtAssetProfilesDlg::AllAssetProfilesToObject()
{
  m_uiActiveConfig = WAssetCurator::GetSingleton()->GetActiveAssetProfileIndex();

  m_ProfileBindings.Clear();

  for (WUInt32 i = 0; i < WAssetCurator::GetSingleton()->GetNumAssetProfiles(); ++i)
  {
    auto* pProfile = WAssetCurator::GetSingleton()->GetAssetProfile(i);

    m_ProfileBindings[NativeToObject(pProfile)].m_pProfile = pProfile;
  }
}

void WQtAssetProfilesDlg::PropertyChangedEventHandler(const WDocumentObjectPropertyEvent& e)
{
  const WUuid guid = e.m_pObject->GetGuid();
  W_ASSERT_DEV(m_ProfileBindings.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_ProfileBindings[guid].m_pProfile);
}

void WQtAssetProfilesDlg::ApplyAllChanges()
{
  for (auto it = m_ProfileBindings.GetIterator(); it.IsValid(); ++it)
  {
    const auto& binding = it.Value();

    WPlatformProfile* pProfile = binding.m_pProfile;

    if (binding.m_State == Binding::State::Deleted)
    {
      WAssetCurator::GetSingleton()->DeleteAssetProfile(pProfile).IgnoreResult();
      continue;
    }

    if (binding.m_State == Binding::State::Added)
    {
      // create a new profile object and synchronize the state directly into that
      pProfile = WAssetCurator::GetSingleton()->CreateAssetProfile();
    }

    ObjectToNative(it.Key(), pProfile);
  }
}
