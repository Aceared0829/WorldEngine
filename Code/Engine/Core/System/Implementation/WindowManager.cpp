#include <Core/CorePCH.h>

#include <Core/GameApplication/WindowOutputTargetBase.h>
#include <Core/System/Window.h>
#include <Core/System/WindowManager.h>
#include <Foundation/Configuration/Startup.h>

W_IMPLEMENT_SINGLETON(WWindowManager);

//////////////////////////////////////////////////////////////////////////

static WUniquePtr<WWindowManager> s_pWindowManager;

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Core, WWindowManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pWindowManager = W_DEFAULT_NEW(WWindowManager);
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    s_pWindowManager.Clear();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    if (s_pWindowManager)
    {
      s_pWindowManager->CloseAll(nullptr);
    }
  }

W_END_SUBSYSTEM_DECLARATION;

// clang-format on

//////////////////////////////////////////////////////////////////////////

WWindowManager::WWindowManager()
  : m_SingletonRegistrar(this)
{
}

WWindowManager::~WWindowManager()
{
  CloseAll(nullptr);
}

void WWindowManager::Update()
{
  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    it.Value()->m_pWindow->ProcessWindowMessages();
  }
}

void WWindowManager::Close(WRegisteredWndHandle hWindow)
{
  WUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  Data* pData = pDataPtr->Borrow();
  W_ASSERT_DEV(pData != nullptr, "Invalid window data");

  if (pData->m_OnDestroy.IsValid())
  {
    pData->m_OnDestroy(hWindow);
  }

  // The window output target has a dependency to the window, e.g. the swapchain renders to it.
  // Explicitly destroy it first to ensure correct destruction order.

  if (pData->m_pOutputTarget)
  {
    pData->m_pOutputTarget.Clear();
  }

  if (pData->m_pWindow)
  {
    pData->m_pWindow.Clear();
  }

  m_Data.Remove(hWindow.GetInternalID());
}

void WWindowManager::CloseAll(const void* pCreatedBy)
{
  WDynamicArray<WRegisteredWndHandle> toClose;

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      toClose.PushBack(WRegisteredWndHandle(it.Id()));
    }
  }

  for (const WRegisteredWndHandle& hWindow : toClose)
  {
    Close(hWindow);
  }
}

bool WWindowManager::IsValid(WRegisteredWndHandle hWindow) const
{
  return m_Data.Contains(hWindow.GetInternalID());
}

void WWindowManager::GetRegistered(WDynamicArray<WRegisteredWndHandle>& out_windowHandles, const void* pCreatedBy /*= nullptr*/)
{
  out_windowHandles.Clear();

  for (auto it = m_Data.GetIterator(); it.IsValid(); ++it)
  {
    if (pCreatedBy == nullptr || it.Value()->m_pCreatedBy == pCreatedBy)
    {
      out_windowHandles.PushBack(WRegisteredWndHandle(it.Id()));
    }
  }
}

WRegisteredWndHandle WWindowManager::Register(WStringView sName, const void* pCreatedBy, WUniquePtr<WWindowBase>&& pWindow)
{
  W_ASSERT_ALWAYS(pCreatedBy != nullptr, "pCreatedBy is invalid");
  W_ASSERT_ALWAYS(pWindow != nullptr, "pWindow is invalid");

  WUniquePtr<Data> pData = W_DEFAULT_NEW(Data);
  pData->m_sName = sName;
  pData->m_pCreatedBy = pCreatedBy;
  pData->m_pWindow = std::move(pWindow);

  return WRegisteredWndHandle(m_Data.Insert(std::move(pData)));
}

void WWindowManager::SetOutputTarget(WRegisteredWndHandle hWindow, WUniquePtr<WWindowOutputTargetBase>&& pOutputTarget)
{
  WUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_pOutputTarget = std::move(pOutputTarget);
}

void WWindowManager::SetDestroyCallback(WRegisteredWndHandle hWindow, WWindowDestroyFunc onDestroyCallback)
{
  WUniquePtr<Data>* pDataPtr = nullptr;
  if (!m_Data.TryGetValue(hWindow.GetInternalID(), pDataPtr))
    return;

  (*pDataPtr)->m_OnDestroy = onDestroyCallback;
}

WStringView WWindowManager::GetName(WRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return WStringView();

  return m_Data[hWindow.GetInternalID()]->m_sName;
}

WWindowBase* WWindowManager::GetWindow(WRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pWindow.Borrow();
}

WWindowOutputTargetBase* WWindowManager::GetOutputTarget(WRegisteredWndHandle hWindow) const
{
  if (!m_Data.Contains(hWindow.GetInternalID()))
    return nullptr;

  return m_Data[hWindow.GetInternalID()]->m_pOutputTarget.Borrow();
}


W_STATICLINK_FILE(Core, Core_System_Implementation_WindowManager);
