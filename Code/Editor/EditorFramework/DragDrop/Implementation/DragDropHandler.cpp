#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDragDropHandler, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WDragDropHandler* WDragDropHandler::s_pActiveDnD = nullptr;

WDragDropHandler::WDragDropHandler() = default;


WDragDropHandler* WDragDropHandler::FindDragDropHandler(const WDragDropInfo* pInfo)
{
  float fBestValue = 0.0f;
  WDragDropHandler* pBestDnD = nullptr;

  WRTTI::ForEachDerivedType<WDragDropHandler>(
    [&](const WRTTI* pRtti)
    {
      WDragDropHandler* pDnD = pRtti->GetAllocator()->Allocate<WDragDropHandler>();

      const float fValue = pDnD->CanHandle(pInfo);
      if (fValue > fBestValue)
      {
        if (pBestDnD != nullptr)
        {
          pBestDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pBestDnD);
        }

        fBestValue = fValue;
        pBestDnD = pDnD;
      }
      else
      {
        pDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(pDnD);
      }
    },
    WRTTI::ForEachOptions::ExcludeNonAllocatable);

  return pBestDnD;
}

bool WDragDropHandler::BeginDragDropOperation(const WDragDropInfo* pInfo, WDragDropConfig* pConfigToFillOut)
{
  W_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  WDragDropHandler* pHandler = FindDragDropHandler(pInfo);

  if (pHandler != nullptr)
  {
    if (pConfigToFillOut != nullptr)
      pHandler->RequestConfiguration(pConfigToFillOut);

    s_pActiveDnD = pHandler;
    s_pActiveDnD->OnDragBegin(pInfo);
    return true;
  }

  return false;
}

void WDragDropHandler::UpdateDragDropOperation(const WDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragUpdate(pInfo);
}

void WDragDropHandler::FinishDragDrop(const WDragDropInfo* pInfo)
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDrop(pInfo);

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

void WDragDropHandler::CancelDragDrop()
{
  if (s_pActiveDnD == nullptr)
    return;

  s_pActiveDnD->OnDragCancel();

  s_pActiveDnD->GetDynamicRTTI()->GetAllocator()->Deallocate(s_pActiveDnD);
  s_pActiveDnD = nullptr;
}

bool WDragDropHandler::CanDropOnly(const WDragDropInfo* pInfo)
{
  W_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  WDragDropHandler* pHandler = FindDragDropHandler(pInfo);

  if (pHandler != nullptr)
  {
    pHandler->GetDynamicRTTI()->GetAllocator()->Deallocate(pHandler);
    return true;
  }

  return false;
}

bool WDragDropHandler::DropOnly(const WDragDropInfo* pInfo)
{
  W_ASSERT_DEV(s_pActiveDnD == nullptr, "A drag & drop handler is already active");

  if (BeginDragDropOperation(pInfo))
  {
    FinishDragDrop(pInfo);
    return true;
  }

  return false;
}
