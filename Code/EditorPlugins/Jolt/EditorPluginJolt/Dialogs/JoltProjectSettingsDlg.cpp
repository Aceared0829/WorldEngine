#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QCheckBox>
#include <QInputDialog>

void UpdateCollisionLayerDynamicEnumValues();
void UpdateWeightCategoryDynamicEnumValues();
void UpdateImpulseTypeDynamicEnumValues();

WQtJoltProjectSettingsDlg::WQtJoltProjectSettingsDlg(const WVariant& startup, QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  ButtonRemoveLayer->setEnabled(false);
  ButtonRenameLayer->setEnabled(false);

  ButtonRenameImpulse->setEnabled(false);
  ButtonRemoveImpulse->setEnabled(false);

  ButtonRenameCategory->setEnabled(false);
  ButtonRemoveCategory->setEnabled(false);

  EnsureConfigFileExists();

  Load().IgnoreResult();
  SetupFilterTable();
  SetupWeightTable();
  SetupImpulseTable();

  Tabs->setCurrentIndex(0);

  if (startup.IsValid())
  {
    const WString sStartup = startup.ConvertTo<WString>();
    if (sStartup == "CollisionLayers")
    {
      Tabs->setCurrentIndex(0);
    }
    if (sStartup == "WeightCategories")
    {
      Tabs->setCurrentIndex(1);
    }
    if (sStartup == "ImpulseTypes")
    {
      Tabs->setCurrentIndex(2);
    }
  }
}

void WQtJoltProjectSettingsDlg::EnsureConfigFileExists()
{
  EnsureFilterConfigFileExists();
  EnsureWeightsConfigFileExists();
  EnsureImpulseConfigFileExists();
}

void WQtJoltProjectSettingsDlg::EnsureFilterConfigFileExists()
{
  if (WFileSystem::ExistsFile(WCollisionFilterConfig::s_sConfigFile))
    return;

  WCollisionFilterConfig cfg;

  cfg.SetGroupName(0, "Default");
  cfg.SetGroupName(1, "Transparent");
  cfg.SetGroupName(2, "Debris");
  cfg.SetGroupName(3, "Water");
  cfg.SetGroupName(4, "Foliage");
  cfg.SetGroupName(5, "AI");
  cfg.SetGroupName(6, "Player");
  cfg.SetGroupName(7, "Visibility Raycast");
  cfg.SetGroupName(8, "Interaction Raycast");

  cfg.EnableCollision(0, 0);
  cfg.EnableCollision(0, 1);
  cfg.EnableCollision(0, 2);
  cfg.EnableCollision(0, 3, false);
  cfg.EnableCollision(0, 4, false);
  cfg.EnableCollision(0, 5);
  cfg.EnableCollision(0, 6);
  cfg.EnableCollision(0, 7);
  cfg.EnableCollision(0, 8);

  cfg.EnableCollision(1, 1);
  cfg.EnableCollision(1, 2);
  cfg.EnableCollision(1, 3, false);
  cfg.EnableCollision(1, 4, false);
  cfg.EnableCollision(1, 5);
  cfg.EnableCollision(1, 6);
  cfg.EnableCollision(1, 7, false);
  cfg.EnableCollision(1, 8);

  cfg.EnableCollision(2, 2, false);
  cfg.EnableCollision(2, 3, false);
  cfg.EnableCollision(2, 4, false);
  cfg.EnableCollision(2, 5, false);
  cfg.EnableCollision(2, 6, false);
  cfg.EnableCollision(2, 7, false);
  cfg.EnableCollision(2, 8, false);

  cfg.EnableCollision(3, 3, false);
  cfg.EnableCollision(3, 4, false);
  cfg.EnableCollision(3, 5, false);
  cfg.EnableCollision(3, 6, false);
  cfg.EnableCollision(3, 7, false);
  cfg.EnableCollision(3, 8);

  cfg.EnableCollision(4, 4, false);
  cfg.EnableCollision(4, 5, false);
  cfg.EnableCollision(4, 6, false);
  cfg.EnableCollision(4, 7);
  cfg.EnableCollision(4, 8, false);

  cfg.EnableCollision(5, 5);
  cfg.EnableCollision(5, 6);
  cfg.EnableCollision(5, 7);
  cfg.EnableCollision(5, 8);

  cfg.EnableCollision(6, 6);
  cfg.EnableCollision(6, 7);
  cfg.EnableCollision(6, 8);

  cfg.EnableCollision(7, 7, false);
  cfg.EnableCollision(7, 8, false);

  cfg.EnableCollision(8, 8, false);

  cfg.Save().IgnoreResult();
}

void WQtJoltProjectSettingsDlg::SetupFilterTable()
{
  WQtScopedBlockSignals s1(FilterTable);
  WQtScopedUpdatesDisabled s2(FilterTable);

  const WUInt32 uiLayers = m_Config.GetNumNamedGroups();

  FilterTable->setRowCount(uiLayers);
  FilterTable->setColumnCount(uiLayers);
  FilterTable->horizontalHeader()->setHighlightSections(false);

  QStringList headers;
  WStringBuilder tmp;

  for (WUInt32 r = 0; r < uiLayers; ++r)
  {
    m_IndexRemap[r] = m_Config.GetNamedGroupIndex(r);

    headers.push_back(QString::fromUtf8(m_Config.GetGroupName(m_IndexRemap[r]).GetData(tmp)));
  }

  FilterTable->setVerticalHeaderLabels(headers);
  FilterTable->setHorizontalHeaderLabels(headers);

  for (WUInt32 r = 0; r < uiLayers; ++r)
  {
    for (WUInt32 c = 0; c < uiLayers; ++c)
    {
      QCheckBox* pCheck = new QCheckBox();
      pCheck->setText(QString());
      pCheck->setChecked(m_Config.IsCollisionEnabled(m_IndexRemap[r], m_IndexRemap[c]));
      pCheck->setProperty("column", c);
      pCheck->setProperty("row", r);
      connect(pCheck, &QCheckBox::clicked, this, &WQtJoltProjectSettingsDlg::onCheckBoxClicked);

      QWidget* pWidget = new QWidget();
      QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
      pLayout->addWidget(pCheck);
      pLayout->setAlignment(Qt::AlignCenter);
      pLayout->setContentsMargins(0, 0, 0, 0);
      pWidget->setLayout(pLayout);

      FilterTable->setCellWidget(r, c, pWidget);
    }
  }
}

WResult WQtJoltProjectSettingsDlg::Save()
{
  if (m_Config.Save().Failed())
  {
    WStringBuilder sError;
    sError.SetFormat("Failed to save the Collision Layer file\n'{0}'", WCollisionFilterConfig::s_sConfigFile);

    WQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return W_FAILURE;
  }

  UpdateCollisionLayerDynamicEnumValues();

  if (m_WeightConfig.Save().Failed())
  {
    WStringBuilder sError;
    sError.SetFormat("Failed to save the Weight Categories file\n'{0}'", WWeightCategoryConfig::s_sConfigFile);

    WQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return W_FAILURE;
  }

  UpdateWeightCategoryDynamicEnumValues();

  if (m_ImpulseConfig.Save().Failed())
  {
    WStringBuilder sError;
    sError.SetFormat("Failed to save the Force Categories file\n'{0}'", WImpulseTypeConfig::s_sConfigFile);

    WQtUiServices::GetSingleton()->MessageBoxWarning(sError);

    return W_FAILURE;
  }

  UpdateImpulseTypeDynamicEnumValues();

  return W_SUCCESS;
}

WResult WQtJoltProjectSettingsDlg::Load()
{
  W_SUCCEED_OR_RETURN(m_Config.Load());
  W_SUCCEED_OR_RETURN(m_WeightConfig.Load());
  W_SUCCEED_OR_RETURN(m_ImpulseConfig.Load());

  m_ConfigReset = m_Config;
  m_WeightConfigReset = m_WeightConfig;
  m_ImpulseConfigReset = m_ImpulseConfig;
  return W_SUCCESS;
}

static void AddWeightCfg(WWeightCategoryConfig& ref_cfg, WStringView sName, float fMass, WStringView sDesc)
{
  WUInt8 idx = ref_cfg.GetFreeKey();
  auto& e = ref_cfg.m_Categories[idx];
  e.m_sName.Assign(sName);
  e.m_fMass = fMass;
  e.m_sDescription = sDesc;
}

void WQtJoltProjectSettingsDlg::EnsureWeightsConfigFileExists()
{
  if (WFileSystem::ExistsFile(WWeightCategoryConfig::s_sConfigFile))
    return;

  WWeightCategoryConfig cfg;
  AddWeightCfg(cfg, "Barrel", 35.0f, "");
  AddWeightCfg(cfg, "Car", 500.0f, "");
  AddWeightCfg(cfg, "Chair", 10.0f, "");
  AddWeightCfg(cfg, "Crate - Large", 100.0f, "larger than 1 meter");
  AddWeightCfg(cfg, "Crate - Medium", 40.0f, "up to 1 meter");
  AddWeightCfg(cfg, "Crate - Small", 10.0f, "smaller than 0.5 meters");
  AddWeightCfg(cfg, "Creature - Small", 10.0f, "small animals, rats, birds");
  AddWeightCfg(cfg, "Creature - Medium", 70.0f, "Humanoids, regular monsters");
  AddWeightCfg(cfg, "Creature - Large", 150.0f, "Large monsters");
  AddWeightCfg(cfg, "Debris", 1.0f, "tiny objects, cans, garbage");
  AddWeightCfg(cfg, "Decoration - Small", 2.0f, "picture frames, plates, mugs");
  AddWeightCfg(cfg, "Decoration - Medium", 5.0f, "paintings, desk lamps, regular vases");
  AddWeightCfg(cfg, "Decoration - Large", 8.0f, "ceiling lamps, large vases");
  AddWeightCfg(cfg, "Furniture - Large", 80.0f, "shelves, tables, large cupboards");
  AddWeightCfg(cfg, "Furniture - Medium", 25.0f, "sideboards, cupboards, small tables");
  AddWeightCfg(cfg, "Furniture - Small", 10.0f, "stools, bedside tables");
  AddWeightCfg(cfg, "Truck", 1000.0f, "trucks, trains, containers, large machinery");

  cfg.Save().IgnoreResult();
}

static void AddForceCfg(WImpulseTypeConfig& ref_cfg, WStringView sName, float fForce, WStringView sDesc)
{
  WUInt8 idx = ref_cfg.GetFreeKey();
  auto& e = ref_cfg.m_Types[idx];
  e.m_sName.Assign(sName);
  e.m_fDefaultValue = fForce;
  e.m_sDescription = sDesc;
}

void WQtJoltProjectSettingsDlg::EnsureImpulseConfigFileExists()
{
  if (WFileSystem::ExistsFile(WImpulseTypeConfig::s_sConfigFile))
    return;

  WImpulseTypeConfig cfg;
  AddForceCfg(cfg, "Projectile - Light", 10.0f, "");
  AddForceCfg(cfg, "Projectile - Medium", 40.0f, "");
  AddForceCfg(cfg, "Projectile - Heavy", 150.0f, "");

  AddForceCfg(cfg, "Explosion - Small", 150.0f, "");
  AddForceCfg(cfg, "Explosion - Medium", 250.0f, "");
  AddForceCfg(cfg, "Explosion - Large", 500.0f, "");

  AddForceCfg(cfg, "Throw Object", 250.0f, "");

  cfg.Save().IgnoreResult();
}

void WQtJoltProjectSettingsDlg::onCheckBoxClicked(bool checked)
{
  QCheckBox* pCheck = qobject_cast<QCheckBox*>(sender());

  const WInt32 c = pCheck->property("column").toInt();
  const WInt32 r = pCheck->property("row").toInt();

  m_Config.EnableCollision(m_IndexRemap[c], m_IndexRemap[r], pCheck->isChecked());

  if (r != c)
  {
    QCheckBox* pCheck2 = qobject_cast<QCheckBox*>(FilterTable->cellWidget(c, r)->layout()->itemAt(0)->widget());
    pCheck2->setChecked(pCheck->isChecked());
  }
}

void WQtJoltProjectSettingsDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
{
  if (pButton == DefaultButtons->button(QDialogButtonBox::Ok))
  {
    if (Save().Failed())
      return;

    accept();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Reset))
  {
    m_Config = m_ConfigReset;
    m_WeightConfig = m_WeightConfigReset;
    m_ImpulseConfig = m_ImpulseConfigReset;
    SetupFilterTable();
    SetupWeightTable();
    SetupImpulseTable();
    return;
  }
}

void WQtJoltProjectSettingsDlg::on_ButtonAddLayer_clicked()
{
  const WUInt32 uiNewIdx = m_Config.FindUnnamedGroup();

  if (uiNewIdx == WInvalidIndex)
  {
    WQtUiServices::GetSingleton()->MessageBoxInformation("The maximum number of collision layers has been reached.");
    return;
  }

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Add Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) != WInvalidIndex)
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("A Collision Layer with the given name already exists.");
      continue;
    }

    m_Config.SetGroupName(uiNewIdx, result.toUtf8().data());
    break;
  }

  SetupFilterTable();
}

void WQtJoltProjectSettingsDlg::on_ButtonRemoveLayer_clicked()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  if (WQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected Collision Layer?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::No)
    return;

  const int iRow = sel[0].row();

  m_Config.SetGroupName(m_IndexRemap[iRow], "");

  SetupFilterTable();

  FilterTable->clearSelection();
}

void WQtJoltProjectSettingsDlg::on_ButtonRenameLayer_clicked()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const int iGroupIdx = m_IndexRemap[sel[0].row()];
  const WString sOldName = m_Config.GetGroupName(iGroupIdx);

  m_Config.SetGroupName(iGroupIdx, "");

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Rename Layer"), QStringLiteral("Name:"), QLineEdit::Normal, QString::fromUtf8(sOldName.GetData()), &ok);

    if (!ok)
    {
      m_Config.SetGroupName(iGroupIdx, sOldName);
      return;
    }

    if (m_Config.GetFilterGroupByName(result.toUtf8().data()) != WInvalidIndex)
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("A collision layer with that name already exists.");
      continue;
    }

    m_Config.SetGroupName(iGroupIdx, result.toUtf8().data());
    SetupFilterTable();

    return;
  }
}

void WQtJoltProjectSettingsDlg::on_FilterTable_itemSelectionChanged()
{
  const auto sel = FilterTable->selectionModel()->selectedRows();
  ButtonRemoveLayer->setEnabled(!sel.isEmpty());
  ButtonRenameLayer->setEnabled(!sel.isEmpty());
}

void WQtJoltProjectSettingsDlg::on_ImpulsesTable_itemSelectionChanged()
{
  OverridesTable->clear();

  const QModelIndexList sel = ImpulsesTable->selectionModel()->selectedRows();
  if (sel.isEmpty())
  {
    ButtonRenameImpulse->setEnabled(false);
    ButtonRemoveImpulse->setEnabled(false);
    return;
  }

  ButtonRenameImpulse->setEnabled(true);
  ButtonRemoveImpulse->setEnabled(true);

  const WHashedString sImpulse = m_RowToImpulse[sel[0].row()];
  const WUInt8 uiImpulseKey = m_ImpulseConfig.FindByName(sImpulse);
  if (uiImpulseKey == WImpulseTypeConfig::InvalidKey)
    return;

  const auto& type = m_ImpulseConfig.m_Types[uiImpulseKey];

  OverridesTable->setColumnCount(2);
  OverridesTable->setHorizontalHeaderLabels({"Mass Category", "Override Impulse"});
  OverridesTable->setRowCount(m_WeightConfig.m_Categories.GetCount());

  for (WUInt32 idx = 0; idx < m_WeightConfig.m_Categories.GetCount(); ++idx)
  {
    const WUInt8 uiWeightKey = m_WeightConfig.m_Categories.GetKey(idx);
    const auto& weight = m_WeightConfig.m_Categories.GetValue(idx);

    OverridesTable->setItem(idx, 0, new QTableWidgetItem(WMakeQString(weight.m_sName)));

    const bool bOverride = type.m_WeightOverrides.Contains(uiWeightKey);

    QCheckBox* pCheck = new QCheckBox("Override");
    pCheck->setChecked(bOverride);
    pCheck->setProperty("ImpulseKey", uiImpulseKey);
    pCheck->setProperty("WeightKey", uiWeightKey);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
    connect(pCheck, &QCheckBox::checkStateChanged, this, &WQtJoltProjectSettingsDlg::onImpulseOverrideChecked);
#else
    W_WARNING_PUSH()
    W_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    W_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
    connect(pCheck, &QCheckBox::stateChanged, this, &WQtJoltProjectSettingsDlg::onImpulseOverrideChecked);
    W_WARNING_POP()
#endif

    WQtDoubleSpinBox* pNumber = new WQtDoubleSpinBox(nullptr);
    pNumber->setProperty("ImpulseKey", uiImpulseKey);
    pNumber->setProperty("WeightKey", uiWeightKey);
    pNumber->setMinimum(0);
    pNumber->setMaximum(10000);
    pNumber->setDecimals(1);
    pNumber->setEnabled(bOverride);
    connect(pNumber, &WQtDoubleSpinBox::valueChanged, this, &WQtJoltProjectSettingsDlg::onImpulseOverrideValue);

    if (bOverride)
    {
      pNumber->setValue(type.m_WeightOverrides.GetValue(type.m_WeightOverrides.Find(uiWeightKey)));
    }

    QWidget* pWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
    pLayout->addWidget(pCheck);
    pLayout->addWidget(pNumber);
    pLayout->setAlignment(Qt::AlignCenter);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pWidget->setLayout(pLayout);

    OverridesTable->setCellWidget(idx, 1, pWidget);
  }

  OverridesTable->resizeColumnToContents(0);
}

void WQtJoltProjectSettingsDlg::on_WeightsTable_itemSelectionChanged()
{
  const QModelIndexList sel = WeightsTable->selectionModel()->selectedRows();
  if (sel.isEmpty())
  {
    ButtonRenameCategory->setEnabled(false);
    ButtonRemoveCategory->setEnabled(false);
    return;
  }

  ButtonRenameCategory->setEnabled(true);
  ButtonRemoveCategory->setEnabled(true);
}

void WQtJoltProjectSettingsDlg::on_ButtonAddCategory_clicked()
{
  WHashedString name;

  const WUInt8 uiFreeKey = m_WeightConfig.GetFreeKey();
  if (uiFreeKey == WWeightCategoryConfig::InvalidKey)
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning("You managed to create too many categories.");
    return;
  }

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Add Weight Category"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (m_WeightConfig.FindByName(WTempHashedString(result.toUtf8().data())) != WWeightCategoryConfig::InvalidKey)
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("A weight category with that name already exists.");
      continue;
    }

    m_WeightConfig.m_Categories[uiFreeKey].m_sName.Assign(result.toUtf8().data());
    break;
  }

  SetupWeightTable();
}

void WQtJoltProjectSettingsDlg::on_ButtonRemoveCategory_clicked()
{
  const auto sel = WeightsTable->selectionModel()->selectedIndexes();

  if (sel.isEmpty())
    return;

  if (WQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected category?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::No)
    return;

  const WUInt32 uiRow = sel[0].row();

  const WUInt8 idx = m_WeightConfig.FindByName(m_RowToWeight[uiRow]);
  m_WeightConfig.m_Categories.RemoveAndCopy(idx);

  SetupWeightTable();
}

void WQtJoltProjectSettingsDlg::on_ButtonRenameCategory_clicked()
{
  const auto sel = WeightsTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const WHashedString sOldCatName = m_RowToWeight[sel[0].row()];
  const WUInt8 uiCatIdx = m_WeightConfig.FindByName(sOldCatName);

  if (uiCatIdx == WWeightCategoryConfig::InvalidKey)
    return;

  m_WeightConfig.m_Categories[uiCatIdx].m_sName.Assign("-tmp-");

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Rename Category"), QStringLiteral("Name:"), QLineEdit::Normal, QString::fromUtf8(sOldCatName.GetString().GetData()), &ok);

    if (!ok)
    {
      m_WeightConfig.m_Categories[uiCatIdx].m_sName = sOldCatName;
      return;
    }

    if (m_WeightConfig.FindByName(WTempHashedString(result.toUtf8().data())) != WWeightCategoryConfig::InvalidKey)
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("A weight category with that name already exists.");
      continue;
    }

    m_WeightConfig.m_Categories[uiCatIdx].m_sName.Assign(result.toUtf8().data());
    SetupWeightTable();
    return;
  }
}

void WQtJoltProjectSettingsDlg::on_ButtonAddImpulse_clicked()
{
  WHashedString name;

  const WUInt8 uiFreeKey = m_ImpulseConfig.GetFreeKey();
  if (uiFreeKey == WImpulseTypeConfig::InvalidKey)
  {
    WQtUiServices::GetSingleton()->MessageBoxWarning("You managed to create too many impulse types.");
    return;
  }

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Add Impulse Type"), QStringLiteral("Name:"), QLineEdit::Normal, QString(), &ok);

    if (!ok)
      return;

    if (m_ImpulseConfig.FindByName(WTempHashedString(result.toUtf8().data())) != WImpulseTypeConfig::InvalidKey)
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("An impulse type with that name already exists.");
      continue;
    }

    m_ImpulseConfig.m_Types[uiFreeKey].m_sName.Assign(result.toUtf8().data());
    break;
  }

  SetupImpulseTable();
}

void WQtJoltProjectSettingsDlg::on_ButtonRemoveImpulse_clicked()
{
  const auto sel = ImpulsesTable->selectionModel()->selectedIndexes();

  if (sel.isEmpty())
    return;

  if (WQtUiServices::GetSingleton()->MessageBoxQuestion("Remove selected impulse type?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No, QMessageBox::StandardButton::Yes) == QMessageBox::StandardButton::No)
    return;

  const WUInt32 uiRow = sel[0].row();

  const WUInt8 idx = m_ImpulseConfig.FindByName(m_RowToImpulse[uiRow]);
  m_ImpulseConfig.m_Types.RemoveAndCopy(idx);

  SetupImpulseTable();
}

void WQtJoltProjectSettingsDlg::on_ButtonRenameImpulse_clicked()
{
  const auto sel = ImpulsesTable->selectionModel()->selectedRows();

  if (sel.isEmpty())
    return;

  const WHashedString sOldCatName = m_RowToImpulse[sel[0].row()];
  const WUInt8 uiCatIdx = m_ImpulseConfig.FindByName(sOldCatName);

  if (uiCatIdx == WImpulseTypeConfig::InvalidKey)
    return;

  m_ImpulseConfig.m_Types[uiCatIdx].m_sName.Assign("-tmp-");

  while (true)
  {
    bool ok;
    QString result = QInputDialog::getText(this, QStringLiteral("Rename Impulse Type"), QStringLiteral("Name:"), QLineEdit::Normal, QString::fromUtf8(sOldCatName.GetString().GetData()), &ok);

    if (!ok)
    {
      m_ImpulseConfig.m_Types[uiCatIdx].m_sName = sOldCatName;
      return;
    }

    if (m_ImpulseConfig.FindByName(WTempHashedString(result.toUtf8().data())) != WImpulseTypeConfig::InvalidKey)
    {
      WQtUiServices::GetSingleton()->MessageBoxWarning("An impulse type with that name already exists.");
      continue;
    }

    m_ImpulseConfig.m_Types[uiCatIdx].m_sName.Assign(result.toUtf8().data());
    SetupImpulseTable();
    return;
  }
}

void WQtJoltProjectSettingsDlg::SetupWeightTable()
{
  WQtScopedBlockSignals s1(WeightsTable);
  WQtScopedUpdatesDisabled s2(WeightsTable);

  WMap<WString, WWeightCategory, WCompareString_NoCase> sorted;

  m_WeightConfig.m_Categories.Sort();
  const WUInt32 uiRows = m_WeightConfig.m_Categories.GetCount();

  for (WUInt32 r = 0; r < uiRows; ++r)
  {
    const auto& cat = m_WeightConfig.m_Categories.GetPair(r);
    sorted[cat.value.m_sName.GetString()] = cat.value;
  }

  WeightsTable->clear();
  WeightsTable->setColumnCount(3);
  WeightsTable->setHorizontalHeaderLabels({"Name", "Mass", "Description"});
  WeightsTable->setRowCount(uiRows);

  m_RowToWeight.SetCount(uiRows);

  WUInt32 uiRow = 0;
  for (const auto cat : sorted)
  {
    m_RowToWeight[uiRow].Assign(cat.Key());

    WeightsTable->setItem(uiRow, 0, new QTableWidgetItem(WMakeQString(cat.Key())));

    WQtDoubleSpinBox* pNumber = new WQtDoubleSpinBox(nullptr);
    pNumber->setMinimum(1.0);
    pNumber->setMaximum(1000);
    pNumber->setDecimals(1);
    pNumber->setValue(cat.Value().m_fMass);

    pNumber->setProperty("category", WMakeQString(cat.Key()));
    connect(pNumber, &WQtDoubleSpinBox::valueChanged, this, &WQtJoltProjectSettingsDlg::onWeightChanged);

    QWidget* pWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
    pLayout->addWidget(pNumber);
    pLayout->setAlignment(Qt::AlignCenter);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pWidget->setLayout(pLayout);

    WeightsTable->setCellWidget(uiRow, 1, pWidget);

    QLineEdit* pDesc = new QLineEdit();
    pDesc->setText(WMakeQString(cat.Value().m_sDescription));
    pDesc->setProperty("category", WMakeQString(cat.Key()));
    connect(pDesc, &QLineEdit::textChanged, this, &WQtJoltProjectSettingsDlg::onWeightDescChanged);

    WeightsTable->setCellWidget(uiRow, 2, pDesc);

    ++uiRow;
  }

  WeightsTable->resizeColumnToContents(1);
}

void WQtJoltProjectSettingsDlg::SetupImpulseTable()
{
  OverridesTable->clear();

  WQtScopedBlockSignals s1(ImpulsesTable);
  WQtScopedUpdatesDisabled s2(ImpulsesTable);

  WMap<WString, WImpulseType, WCompareString_NoCase> sorted;

  m_ImpulseConfig.m_Types.Sort();
  const WUInt32 uiRows = m_ImpulseConfig.m_Types.GetCount();

  for (WUInt32 r = 0; r < uiRows; ++r)
  {
    const auto& cat = m_ImpulseConfig.m_Types.GetPair(r);
    sorted[cat.value.m_sName.GetString()] = cat.value;
  }

  ImpulsesTable->clear();
  ImpulsesTable->setColumnCount(3);
  ImpulsesTable->setHorizontalHeaderLabels({"Name", "Impulse", "Description"});
  ImpulsesTable->setRowCount(uiRows);

  m_RowToImpulse.SetCount(uiRows);

  WUInt32 uiRow = 0;
  for (const auto cat : sorted)
  {
    m_RowToImpulse[uiRow].Assign(cat.Key());

    ImpulsesTable->setItem(uiRow, 0, new QTableWidgetItem(WMakeQString(cat.Key())));

    WQtDoubleSpinBox* pNumber = new WQtDoubleSpinBox(nullptr);
    pNumber->setMinimum(0);
    pNumber->setMaximum(10000);
    pNumber->setDecimals(1);
    pNumber->setValue(cat.Value().m_fDefaultValue);

    pNumber->setProperty("impulse", WMakeQString(cat.Key()));
    connect(pNumber, &WQtDoubleSpinBox::valueChanged, this, &WQtJoltProjectSettingsDlg::onImpulseChanged);

    QWidget* pWidget = new QWidget();
    QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
    pLayout->addWidget(pNumber);
    pLayout->setAlignment(Qt::AlignCenter);
    pLayout->setContentsMargins(0, 0, 0, 0);
    pWidget->setLayout(pLayout);

    ImpulsesTable->setCellWidget(uiRow, 1, pWidget);

    QLineEdit* pDesc = new QLineEdit();
    pDesc->setText(WMakeQString(cat.Value().m_sDescription));
    pDesc->setProperty("impulse", WMakeQString(cat.Key()));
    connect(pDesc, &QLineEdit::textChanged, this, &WQtJoltProjectSettingsDlg::onImpulseDescChanged);

    ImpulsesTable->setCellWidget(uiRow, 2, pDesc);

    ++uiRow;
  }

  ImpulsesTable->resizeColumnToContents(1);
}

void WQtJoltProjectSettingsDlg::onWeightChanged(double value)
{
  WQtDoubleSpinBox* pNumber = qobject_cast<WQtDoubleSpinBox*>(sender());

  const WString sCat = pNumber->property("category").toString().toUtf8().data();
  const WUInt8 idx = m_WeightConfig.FindByName(WTempHashedString(sCat));

  m_WeightConfig.m_Categories[idx].m_fMass = static_cast<float>(value);
}

void WQtJoltProjectSettingsDlg::onWeightDescChanged(const QString& txt)
{
  QLineEdit* pEdit = qobject_cast<QLineEdit*>(sender());

  const WString sCat = pEdit->property("category").toString().toUtf8().data();
  const WUInt8 idx = m_WeightConfig.FindByName(WTempHashedString(sCat));

  m_WeightConfig.m_Categories[idx].m_sDescription = txt.toUtf8().data();
}

void WQtJoltProjectSettingsDlg::onImpulseChanged(double value)
{
  WQtDoubleSpinBox* pNumber = qobject_cast<WQtDoubleSpinBox*>(sender());

  const WString sCat = pNumber->property("impulse").toString().toUtf8().data();
  const WUInt8 idx = m_ImpulseConfig.FindByName(WTempHashedString(sCat));

  m_ImpulseConfig.m_Types[idx].m_fDefaultValue = static_cast<float>(value);
}

void WQtJoltProjectSettingsDlg::onImpulseDescChanged(const QString& txt)
{
  QLineEdit* pEdit = qobject_cast<QLineEdit*>(sender());

  const WString sCat = pEdit->property("impulse").toString().toUtf8().data();
  const WUInt8 idx = m_ImpulseConfig.FindByName(WTempHashedString(sCat));

  m_ImpulseConfig.m_Types[idx].m_sDescription = txt.toUtf8().data();
}

void WQtJoltProjectSettingsDlg::onImpulseOverrideChecked(int)
{
  QCheckBox* pCheck = qobject_cast<QCheckBox*>(sender());

  const WUInt32 uiImpulseKey = pCheck->property("ImpulseKey").toUInt();
  const WUInt32 uiWeightKey = pCheck->property("WeightKey").toUInt();

  const bool bOverride = pCheck->isChecked();

  if (!bOverride)
  {
    m_ImpulseConfig.m_Types[uiImpulseKey].m_WeightOverrides.RemoveAndCopy(uiWeightKey);
  }

  QLayout* pLayout = pCheck->parentWidget()->layout();
  for (int i = 0; i < pLayout->count(); ++i)
  {
    if (WQtDoubleSpinBox* pSpin = qobject_cast<WQtDoubleSpinBox*>(pLayout->itemAt(i)->widget()))
    {
      pSpin->setEnabled(bOverride);

      if (bOverride)
      {
        m_ImpulseConfig.m_Types[uiImpulseKey].m_WeightOverrides[uiWeightKey] = (float)pSpin->value();
      }
    }
  }
}

void WQtJoltProjectSettingsDlg::onImpulseOverrideValue(double fValue)
{
  const WUInt32 uiImpulseKey = sender()->property("ImpulseKey").toUInt();
  const WUInt32 uiWeightKey = sender()->property("WeightKey").toUInt();

  m_ImpulseConfig.m_Types[uiImpulseKey].m_WeightOverrides[uiWeightKey] = (float)fValue;
}
