#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetImportDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>

enum Columns
{
  Method,
  InputFile,
  ENUM_COUNT
};

WQtAssetImportDlg::WQtAssetImportDlg(QWidget* pParent, WDynamicArray<WAssetDocumentGenerator::ImportGroupOptions>& ref_allImports)
  : WQtDialog(pParent)
  , m_AllImports(ref_allImports)
{
  setupUi(this);

  QStringList headers;
  headers.push_back(QString::fromUtf8("Import Method"));
  headers.push_back(QString::fromUtf8("Input File"));

  QTableWidget* table = AssetTable;
  table->setColumnCount(headers.size());
  table->setRowCount(m_AllImports.GetCount());
  table->setHorizontalHeaderLabels(headers);

  {
    WQtScopedBlockSignals _1(table);

    for (WUInt32 i = 0; i < m_AllImports.GetCount(); ++i)
    {
      InitRow(i);
    }
  }

  table->horizontalHeader()->setSectionResizeMode(Columns::Method, QHeaderView::ResizeMode::ResizeToContents);
  table->horizontalHeader()->setSectionResizeMode(Columns::InputFile, QHeaderView::ResizeMode::Stretch);
}

WQtAssetImportDlg::~WQtAssetImportDlg() = default;

void WQtAssetImportDlg::InitRow(WUInt32 uiRow)
{
  QTableWidget* table = AssetTable;
  const auto& data2 = m_AllImports[uiRow];

  table->setItem(uiRow, Columns::InputFile, new QTableWidgetItem(data2.m_sInputFileRelative.GetData()));
  table->item(uiRow, Columns::InputFile)->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  QComboBox* pCombo = new QComboBox();
  table->setCellWidget(uiRow, Columns::Method, pCombo);

  pCombo->addItem(WQtUiServices::GetSingleton()->GetCachedIconResource(":/GuiFoundation/Icons/NoEntry.svg"), "No Import");
  for (const auto& option : data2.m_ImportOptions)
  {
    QIcon icon = WQtUiServices::GetSingleton()->GetCachedIconResource(option.m_sIcon);
    pCombo->addItem(icon, WMakeQString(WTranslate(option.m_sName)));

    // The lookup returns the key unchanged when there is no tooltip for it, so an untranslated mode
    // would otherwise get its own name as the tooltip.
    const WStringView sTooltip = WTranslateTooltip(option.m_sName);
    if (!sTooltip.IsEmpty() && sTooltip != option.m_sName)
    {
      pCombo->setItemData(pCombo->count() - 1, WMakeQString(sTooltip), Qt::ToolTipRole);
    }
  }

  pCombo->setProperty("row", uiRow);
  pCombo->setCurrentIndex(data2.m_iSelectedOption + 1);
  connect(pCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SelectedOptionChanged(int)));
}

void WQtAssetImportDlg::SelectedOptionChanged(int index)
{
  QComboBox* pCombo = qobject_cast<QComboBox*>(sender());
  const WUInt32 uiRow = pCombo->property("row").toInt();

  m_AllImports[uiRow].m_iSelectedOption = index - 1;
}

void WQtAssetImportDlg::on_ButtonImport_clicked()
{
  W_LOG_BLOCK("Importing Assets");

  for (auto& data : m_AllImports)
  {
    if (data.m_iSelectedOption < 0)
      continue;

    W_LOG_BLOCK("Asset Import", data.m_sInputFileRelative);

    const auto& option = data.m_ImportOptions[data.m_iSelectedOption];

    option.m_pGenerator->Import(data.m_sInputFileAbsolute, option.m_sName, false).LogFailure();
  }

  accept();
}
