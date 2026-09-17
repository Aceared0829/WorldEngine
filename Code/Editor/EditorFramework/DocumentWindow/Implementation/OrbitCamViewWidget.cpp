#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

WQtOrbitCamViewWidget::WQtOrbitCamViewWidget(WQtEngineDocumentWindow* pOwnerWindow, WEngineViewConfig* pViewConfig, bool bPicking)
  : WQtEngineViewWidget(nullptr, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_pOrbitCameraContext = W_DEFAULT_NEW(WOrbitCameraContext, pOwnerWindow, this);
  m_pOrbitCameraContext->SetCamera(&m_pViewConfig->m_Camera);

  if (bPicking)
  {
    m_pSelectionContext = W_DEFAULT_NEW(WSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts.PushBack(m_pSelectionContext.Borrow());
  }

  m_InputContexts.PushBack(m_pOrbitCameraContext.Borrow());
}

WQtOrbitCamViewWidget::~WQtOrbitCamViewWidget() = default;


void WQtOrbitCamViewWidget::ConfigureFixed(const WVec3& vCenterPos, const WVec3& vHalfBoxSize, const WVec3& vCamPosition)
{
  m_pOrbitCameraContext->SetDefaultCameraFixed(vCamPosition);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = false;
}

void WQtOrbitCamViewWidget::ConfigureRelative(const WVec3& vCenterPos, const WVec3& vHalfBoxSize, const WVec3& vCamDirection, float fCamDistanceScale)
{
  m_pOrbitCameraContext->SetDefaultCameraRelative(vCamDirection, fCamDistanceScale);
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);
  m_pOrbitCameraContext->MoveCameraToDefaultPosition();
  m_bSetDefaultCamPos = true;
}

void WQtOrbitCamViewWidget::SetOrbitVolume(const WVec3& vCenterPos, const WVec3& vHalfBoxSize)
{
  m_pOrbitCameraContext->SetOrbitVolume(vCenterPos, vHalfBoxSize);

  if (m_bSetDefaultCamPos)
  {
    if (vHalfBoxSize != WVec3(0.1f))
    {
      // 0.1f is a hard-coded value for the bounding box, in case nothing is available yet
      // not pretty, but somehow we need to know when the first 'proper' bounds are available

      m_bSetDefaultCamPos = false;
      m_pOrbitCameraContext->MoveCameraToDefaultPosition();
    }
  }
}

WOrbitCameraContext* WQtOrbitCamViewWidget::GetOrbitCamera()
{
  return m_pOrbitCameraContext.Borrow();
}

void WQtOrbitCamViewWidget::SyncToEngine()
{
  if (m_pSelectionContext)
  {
    m_pSelectionContext->SetWindowConfig(WVec2I32(width(), height()));
  }

  WQtEngineViewWidget::SyncToEngine();
}
