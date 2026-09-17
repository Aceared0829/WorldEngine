#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Tracks/CurveEditData.h>
#include <GuiFoundation/Dialogs/CurveEditDlg.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

QByteArray WQtCurveEditDlg::s_LastDialogGeometry;

WQtCurveEditDlg::WQtCurveEditDlg(WObjectAccessorBase* pObjectAccessor, const WDocumentObject* pCurveObject, QWidget* pParent, WStringView sTitle)
  : WQtDialog(pParent)
{
  m_pObjectAccessor = pObjectAccessor;
  m_pCurveObject = pCurveObject;

  setupUi(this);

  if (!sTitle.IsEmpty())
  {
    WStringBuilder tmp;
    setWindowTitle(sTitle.GetData(tmp));
  }

  WQtCurve1DEditorWidget* pEdit = CurveEditor;

  connect(pEdit, &WQtCurve1DEditorWidget::CpMovedEvent, this, &WQtCurveEditDlg::OnCpMovedEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::CpDeletedEvent, this, &WQtCurveEditDlg::OnCpDeletedEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::TangentMovedEvent, this, &WQtCurveEditDlg::OnTangentMovedEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::InsertCpEvent, this, &WQtCurveEditDlg::OnInsertCpEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::TangentLinkEvent, this, &WQtCurveEditDlg::OnTangentLinkEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::CpTangentModeEvent, this, &WQtCurveEditDlg::OnCpTangentModeEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::BeginCpChangesEvent, this, &WQtCurveEditDlg::OnBeginCpChangesEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::EndCpChangesEvent, this, &WQtCurveEditDlg::OnEndCpChangesEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::BeginOperationEvent, this, &WQtCurveEditDlg::OnBeginOperationEvent);
  connect(pEdit, &WQtCurve1DEditorWidget::EndOperationEvent, this, &WQtCurveEditDlg::OnEndOperationEvent);

  m_pShortcutUndo = new QShortcut(QKeySequence("Ctrl+Z"), this);
  m_pShortcutRedo = new QShortcut(QKeySequence("Ctrl+Y"), this);

  connect(m_pShortcutUndo, &QShortcut::activated, this, &WQtCurveEditDlg::on_actionUndo_triggered);
  connect(m_pShortcutRedo, &QShortcut::activated, this, &WQtCurveEditDlg::on_actionRedo_triggered);

  m_Curves.m_Curves.PushBack(W_DEFAULT_NEW(WSingleCurveData));

  RetrieveCurveState();

  m_uiActionsUndoBaseline = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory()->GetUndoStackSize();

  UpdateUndoRedoState();
}

void WQtCurveEditDlg::RetrieveCurveState()
{
  auto& curve = m_Curves.m_Curves.PeekBack();

  WInt32 iNumPoints = 0;
  m_pObjectAccessor->GetCountByName(m_pCurveObject, "ControlPoints", iNumPoints).AssertSuccess();
  curve->m_ControlPoints.SetCount(iNumPoints);

  WVariant v;

  // get a local representation of the curve once, so that we can update the preview more efficiently
  for (WInt32 i = 0; i < iNumPoints; ++i)
  {
    const WDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", i);

    m_pObjectAccessor->GetValueByName(pPoint, "Tick", v).AssertSuccess();
    curve->m_ControlPoints[i].m_iTick = v.ConvertTo<WInt32>();

    m_pObjectAccessor->GetValueByName(pPoint, "Value", v).AssertSuccess();
    curve->m_ControlPoints[i].m_fValue = v.ConvertTo<double>();

    m_pObjectAccessor->GetValueByName(pPoint, "LeftTangent", v).AssertSuccess();
    curve->m_ControlPoints[i].m_LeftTangent = v.ConvertTo<WVec2>();

    m_pObjectAccessor->GetValueByName(pPoint, "RightTangent", v).AssertSuccess();
    curve->m_ControlPoints[i].m_RightTangent = v.ConvertTo<WVec2>();

    m_pObjectAccessor->GetValueByName(pPoint, "Linked", v).AssertSuccess();
    curve->m_ControlPoints[i].m_bTangentsLinked = v.ConvertTo<bool>();

    m_pObjectAccessor->GetValueByName(pPoint, "LeftTangentMode", v).AssertSuccess();
    curve->m_ControlPoints[i].m_LeftTangentMode = (WCurveTangentMode::Enum)v.ConvertTo<WInt32>();

    m_pObjectAccessor->GetValueByName(pPoint, "RightTangentMode", v).AssertSuccess();
    curve->m_ControlPoints[i].m_RightTangentMode = (WCurveTangentMode::Enum)v.ConvertTo<WInt32>();
  }
}

WQtCurveEditDlg::~WQtCurveEditDlg()
{
  s_LastDialogGeometry = saveGeometry();
}

void WQtCurveEditDlg::SetCurveColor(const WColor& color)
{
  m_Curves.m_Curves.PeekBack()->m_CurveColor = color;
}

void WQtCurveEditDlg::SetCurveExtents(double fLower, bool bLowerFixed, double fUpper, bool bUpperFixed)
{
  m_fLowerExtents = fLower;
  m_fUpperExtents = fUpper;
  m_bLowerFixed = bLowerFixed;
  m_bUpperFixed = bUpperFixed;
}

void WQtCurveEditDlg::SetCurveRanges(double fLower, double fUpper)
{
  m_fLowerRange = fLower;
  m_fUpperRange = fUpper;
}

void WQtCurveEditDlg::reject()
{
  // ignore
}

void WQtCurveEditDlg::accept()
{
  // ignore
}

void WQtCurveEditDlg::cancel()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();
  cmd.Undo(cmd.GetUndoStackSize() - m_uiActionsUndoBaseline).AssertSuccess();

  QDialog::reject();
}

void WQtCurveEditDlg::UpdatePreview()
{
  WQtCurve1DEditorWidget* pEdit = CurveEditor;
  pEdit->SetCurveExtents(m_fLowerExtents, m_fUpperExtents, m_bLowerFixed, m_bUpperFixed);
  pEdit->SetCurveRanges(m_fLowerRange, m_fUpperRange);
  pEdit->SetCurves(m_Curves);
}

void WQtCurveEditDlg::closeEvent(QCloseEvent*)
{
  cancel();
}

void WQtCurveEditDlg::showEvent(QShowEvent* e)
{
  QDialog::showEvent(e);

  UpdatePreview();
}

void WQtCurveEditDlg::OnCpMovedEvent(WUInt32 curveIdx, WUInt32 cpIdx, WInt64 iTickX, double newPosY)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (cp.m_iTick != iTickX || cp.m_fValue != newPosY)
    {
      cp.m_iTick = iTickX;
      cp.m_fValue = newPosY;
    }
  }

  // update the actual object
  {
    const WDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValueByName(pPoint, "Tick", iTickX).AssertSuccess();
    m_pObjectAccessor->SetValueByName(pPoint, "Value", newPosY).AssertSuccess();
  }
}

void WQtCurveEditDlg::OnCpDeletedEvent(WUInt32 curveIdx, WUInt32 cpIdx)
{
  // update the local representation
  {
    m_Curves.m_Curves[curveIdx]->m_ControlPoints.RemoveAtAndCopy(cpIdx);
  }

  // update the actual object
  {
    const WDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);
    m_pObjectAccessor->RemoveObject(pPoint).AssertSuccess();
  }
}

void WQtCurveEditDlg::OnTangentMovedEvent(WUInt32 curveIdx, WUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangent.Set(newPosX, newPosY);
    else
      cp.m_LeftTangent.Set(newPosX, newPosY);
  }

  // update the actual object
  {
    const WDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValueByName(pPoint, "RightTangent", WVec2(newPosX, newPosY)).AssertSuccess();
    else
      m_pObjectAccessor->SetValueByName(pPoint, "LeftTangent", WVec2(newPosX, newPosY)).AssertSuccess();
  }
}

void WQtCurveEditDlg::OnInsertCpEvent(WUInt32 curveIdx, WInt64 tickX, double value)
{
  // update the local representation
  {
    WCurveControlPointData cp;
    cp.m_iTick = tickX;
    cp.m_fValue = value;

    m_Curves.m_Curves[curveIdx]->m_ControlPoints.PushBack(cp);
  }

  // update the actual object
  {
    WUuid guid;
    m_pObjectAccessor->AddObjectByName(m_pCurveObject, "ControlPoints", -1, WGetStaticRTTI<WCurveControlPointData>(), guid).AssertSuccess();

    const WDocumentObject* pPoint = m_pObjectAccessor->GetObject(guid);

    m_pObjectAccessor->SetValueByName(pPoint, "Tick", tickX).AssertSuccess();
    m_pObjectAccessor->SetValueByName(pPoint, "Value", value).AssertSuccess();
  }

  // Record which point was inserted (will be at the end of the array)
  m_iInsertedCurveIdx = curveIdx;
  m_uiInsertedPointIdx = m_Curves.m_Curves[curveIdx]->m_ControlPoints.GetCount() - 1;
}

void WQtCurveEditDlg::OnTangentLinkEvent(WUInt32 curveIdx, WUInt32 cpIdx, bool bLink)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];
    cp.m_bTangentsLinked = bLink;
  }

  // update the actual object
  {
    const WDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    m_pObjectAccessor->SetValueByName(pPoint, "Linked", bLink).AssertSuccess();
  }
}

void WQtCurveEditDlg::OnCpTangentModeEvent(WUInt32 curveIdx, WUInt32 cpIdx, bool rightTangent, int mode)
{
  // update the local representation
  {
    auto& cp = m_Curves.m_Curves[curveIdx]->m_ControlPoints[cpIdx];

    if (rightTangent)
      cp.m_RightTangentMode = (WCurveTangentMode::Enum)mode;
    else
      cp.m_LeftTangentMode = (WCurveTangentMode::Enum)mode;
  }

  // update the actual object
  {
    const WDocumentObject* pPoint = m_pObjectAccessor->GetChildObjectByName(m_pCurveObject, "ControlPoints", cpIdx);

    if (rightTangent)
      m_pObjectAccessor->SetValueByName(pPoint, "RightTangentMode", mode).AssertSuccess();
    else
      m_pObjectAccessor->SetValueByName(pPoint, "LeftTangentMode", mode).AssertSuccess();
  }
}

void WQtCurveEditDlg::OnBeginCpChangesEvent(QString name)
{
  m_pObjectAccessor->StartTransaction(name.toUtf8().data());
}

void WQtCurveEditDlg::OnEndCpChangesEvent()
{
  m_pObjectAccessor->FinishTransaction();

  UpdatePreview();
  UpdateUndoRedoState();

  // If a control point was inserted, select it now that the operation is complete
  if (m_iInsertedCurveIdx >= 0)
  {
    WSelectedCurveCP sel;
    sel.m_uiCurve = static_cast<WUInt16>(m_iInsertedCurveIdx);
    sel.m_uiPoint = static_cast<WUInt16>(m_uiInsertedPointIdx);

    CurveEditor->CurveEdit->ClearSelection();
    CurveEditor->CurveEdit->SetSelection(sel);

    m_iInsertedCurveIdx = -1;
  }
}

void WQtCurveEditDlg::OnBeginOperationEvent(QString name)
{
  m_pObjectAccessor->BeginTemporaryCommands(name.toUtf8().data());
}

void WQtCurveEditDlg::OnEndOperationEvent(bool commit)
{
  if (commit)
    m_pObjectAccessor->FinishTemporaryCommands();
  else
    m_pObjectAccessor->CancelTemporaryCommands();

  UpdatePreview();
  UpdateUndoRedoState();
}

void WQtCurveEditDlg::on_actionUndo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanUndo() && cmd.GetUndoStackSize() > m_uiActionsUndoBaseline)
  {
    cmd.Undo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
    UpdateUndoRedoState();
  }
}

void WQtCurveEditDlg::on_actionRedo_triggered()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  if (cmd.CanRedo())
  {
    cmd.Redo().IgnoreResult();

    RetrieveCurveState();
    UpdatePreview();
    UpdateUndoRedoState();
  }
}

void WQtCurveEditDlg::on_ButtonOk_clicked()
{
  QDialog::accept();
}

void WQtCurveEditDlg::on_ButtonCancel_clicked()
{
  cancel();
}

void WQtCurveEditDlg::on_ButtonUndo_clicked()
{
  on_actionUndo_triggered();
}

void WQtCurveEditDlg::on_ButtonRedo_clicked()
{
  on_actionRedo_triggered();
}

void WQtCurveEditDlg::UpdateUndoRedoState()
{
  auto& cmd = *m_pObjectAccessor->GetObjectManager()->GetDocument()->GetCommandHistory();

  const bool canUndo = cmd.CanUndo() && cmd.GetUndoStackSize() > m_uiActionsUndoBaseline;
  const bool canRedo = cmd.CanRedo();

  ButtonUndo->setEnabled(canUndo);
  ButtonRedo->setEnabled(canRedo);
}
