#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/Progress.h>

static WProgress* s_pGlobal = nullptr;

WProgress::WProgress() = default;

WProgress::~WProgress()
{
  if (s_pGlobal == this)
  {
    s_pGlobal = nullptr;
  }
}

float WProgress::GetCompletion() const
{
  return m_fCurrentCompletion;
}

void WProgress::SetCompletion(float fCompletion)
{
  W_ASSERT_DEV(fCompletion >= 0.0f && fCompletion <= 1.0f, "Completion value {0} is out of valid range", fCompletion);

  m_fCurrentCompletion = fCompletion;

  if (fCompletion > m_fLastReportedCompletion + 0.001f)
  {
    m_fLastReportedCompletion = fCompletion;

    WProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = WProgressEvent::Type::ProgressChanged;

    m_Events.Broadcast(e, 1);
  }
}

void WProgress::Reset()
{
  m_fCurrentCompletion = 0.0f;
  m_fLastReportedCompletion = 0.0f;

  WProgressEvent e;
  e.m_pProgressbar = this;
  e.m_Type = WProgressEvent::Type::ProgressChanged;

  m_Events.Broadcast(e, 1);
}

void WProgress::SetActiveRange(WProgressRange* pRange)
{
  if (m_pActiveRange == nullptr && pRange != nullptr)
  {
    m_fLastReportedCompletion = 0.0;
    m_fCurrentCompletion = 0.0;
    m_bCancelClicked = false;
    m_bEnableCancel = pRange->m_bAllowCancel;

    WProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = WProgressEvent::Type::ProgressStarted;

    m_Events.Broadcast(e);
  }

  if (m_pActiveRange != nullptr && pRange == nullptr)
  {
    WProgressEvent e;
    e.m_pProgressbar = this;
    e.m_Type = WProgressEvent::Type::ProgressEnded;

    m_Events.Broadcast(e);
  }

  m_pActiveRange = pRange;
  m_pRootRange = m_pActiveRange;

  while (m_pRootRange && m_pRootRange->m_pParentRange)
  {
    m_pRootRange = m_pRootRange->m_pParentRange;
  }
}

WStringView WProgress::GetMainDisplayText() const
{
  if (m_pRootRange == nullptr)
    return {};

  return m_pRootRange->m_sDisplayText;
}

WStringView WProgress::GetStepDisplayText() const
{
  if (m_pRootRange == nullptr)
    return {};

  return m_pRootRange->m_sStepDisplayText;
}

void WProgress::UserClickedCancel()
{
  if (m_bCancelClicked)
    return;

  m_bCancelClicked = true;

  WProgressEvent e;
  e.m_Type = WProgressEvent::Type::CancelClicked;
  e.m_pProgressbar = this;

  m_Events.Broadcast(e, 1);
}

bool WProgress::WasCanceled() const
{
  return m_bCancelClicked;
}

bool WProgress::AllowUserCancel() const
{
  return m_bEnableCancel;
}

WProgress* WProgress::GetGlobalProgressbar()
{
  if (!s_pGlobal)
  {
    static WProgress s_Global;
    return &s_Global;
  }

  return s_pGlobal;
}

void WProgress::SetGlobalProgressbar(WProgress* pProgress)
{
  s_pGlobal = pProgress;
}

//////////////////////////////////////////////////////////////////////////

WProgressRange::WProgressRange(WStringView sDisplayText, WUInt32 uiSteps, bool bAllowCancel, WProgress* pProgressbar /*= nullptr*/)
{
  W_ASSERT_DEV(uiSteps > 0, "Every progress range must have at least one step to complete");

  m_iCurrentStep = -1;
  m_fWeightedCompletion = -1.0;
  m_fSummedWeight = (double)uiSteps;

  Init(sDisplayText, bAllowCancel, pProgressbar);
}

WProgressRange::WProgressRange(WStringView sDisplayText, bool bAllowCancel, WProgress* pProgressbar /*= nullptr*/)
{
  Init(sDisplayText, bAllowCancel, pProgressbar);
}

void WProgressRange::Init(WStringView sDisplayText, bool bAllowCancel, WProgress* pProgressbar)
{
  if (pProgressbar == nullptr)
    m_pProgressbar = WProgress::GetGlobalProgressbar();
  else
    m_pProgressbar = pProgressbar;

  W_ASSERT_DEV(m_pProgressbar != nullptr, "No global progress-bar context available.");

  m_bAllowCancel = bAllowCancel;
  m_sDisplayText = sDisplayText;

  m_pParentRange = m_pProgressbar->m_pActiveRange;

  if (m_pParentRange == nullptr)
  {
    m_fPercentageBase = 0.0;
    m_fPercentageRange = 1.0;
  }
  else
  {
    m_pParentRange->ComputeCurStepBaseAndRange(m_fPercentageBase, m_fPercentageRange);
  }

  m_pProgressbar->SetActiveRange(this);
}

WProgressRange::~WProgressRange()
{
  m_pProgressbar->SetCompletion((float)(m_fPercentageBase + m_fPercentageRange));
  m_pProgressbar->SetActiveRange(m_pParentRange);
}

WProgress* WProgressRange::GetProgressbar() const
{
  return m_pProgressbar;
}

void WProgressRange::SetStepWeighting(WUInt32 uiStep, float fWeight)
{
  W_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_fSummedWeight -= GetStepWeight(uiStep);
  m_fSummedWeight += fWeight;
  m_StepWeights[uiStep] = fWeight;
}

float WProgressRange::GetStepWeight(WUInt32 uiStep) const
{
  const float* pOldWeight = m_StepWeights.GetValue(uiStep);
  return pOldWeight != nullptr ? *pOldWeight : 1.0f;
}

void WProgressRange::ComputeCurStepBaseAndRange(double& out_base, double& out_range)
{
  const double internalBase = WMath::Max(m_fWeightedCompletion, 0.0) / m_fSummedWeight;
  const double internalRange = GetStepWeight(WMath::Max(m_iCurrentStep, 0)) / m_fSummedWeight;

  out_range = internalRange * m_fPercentageRange;
  out_base = m_fPercentageBase + (internalBase * m_fPercentageRange);

  W_ASSERT_DEBUG(out_base <= 1.0f, "Invalid range");
  W_ASSERT_DEBUG(out_range <= 1.0f, "Invalid range");
  W_ASSERT_DEBUG(out_base + out_range <= 1.0f, "Invalid range");
}

bool WProgressRange::BeginNextStep(WStringView sStepDisplayText, WUInt32 uiNumSteps)
{
  W_ASSERT_DEV(m_fSummedWeight > 0.0, "This function is only supported if ProgressRange was initialized with steps");

  m_sStepDisplayText = sStepDisplayText;

  for (WUInt32 i = 0; i < uiNumSteps; ++i)
  {
    m_fWeightedCompletion += GetStepWeight(m_iCurrentStep + i);
  }
  m_iCurrentStep += uiNumSteps;

  const double internalCompletion = m_fWeightedCompletion / m_fSummedWeight;
  const double finalCompletion = m_fPercentageBase + internalCompletion * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool WProgressRange::SetCompletion(double fCompletionFactor)
{
  W_ASSERT_DEV(m_fSummedWeight == 0.0, "This function is only supported if ProgressRange was initialized without steps");

  const double finalCompletion = m_fPercentageBase + fCompletionFactor * m_fPercentageRange;

  m_pProgressbar->SetCompletion((float)finalCompletion);

  return !m_pProgressbar->WasCanceled();
}

bool WProgressRange::WasCanceled() const
{
  if (!m_pProgressbar->m_bCancelClicked)
    return false;

  const WProgressRange* pCur = this;

  // if there is any action in the stack above, that cannot be canceled
  // all sub actions should be fully executed, even if they could be canceled
  while (pCur)
  {
    if (!pCur->m_bAllowCancel)
      return false;

    pCur = pCur->m_pParentRange;
  }

  return true;
}
