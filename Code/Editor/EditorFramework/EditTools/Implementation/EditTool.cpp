#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WGameObjectEditTool, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WGameObjectEditTool::WGameObjectEditTool() = default;

void WGameObjectEditTool::ConfigureTool(
  WGameObjectDocument* pDocument, WQtGameObjectDocumentWindow* pWindow, WGameObjectGizmoInterface* pInterface)
{
  m_pDocument = pDocument;
  m_pWindow = pWindow;
  m_pInterface = pInterface;

  OnConfigured();
}

void WGameObjectEditTool::SetActive(bool bActive)
{
  if (m_bIsActive == bActive)
    return;

  m_bIsActive = bActive;
  OnActiveChanged(m_bIsActive);

  if (!m_bIsActive)
  {
    m_pWindow->SetPermanentStatusBarMsg("");
  }
}
