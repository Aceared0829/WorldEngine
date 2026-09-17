#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/ActionManager.h>

const WActionDescriptor* WActionDescriptorHandle::GetDescriptor() const
{
  return WActionManager::GetActionDescriptor(*this);
}

WActionDescriptor::WActionDescriptor(WActionType::Enum type, WActionScope::Enum scope, const char* szName, const char* szCategoryPath,
  const char* szShortcut, CreateActionFunc createAction, DeleteActionFunc deleteAction)
  : m_Type(type)
  , m_Scope(scope)
  , m_sActionName(szName)
  , m_sCategoryPath(szCategoryPath)
  , m_sShortcut(szShortcut)
  , m_sDefaultShortcut(szShortcut)
  , m_CreateAction(createAction)
  , m_DeleteAction(deleteAction)
{
}

WAction* WActionDescriptor::CreateAction(const WActionContext& context) const
{
  W_ASSERT_DEV(!m_Handle.IsInvalidated(), "Handle invalid!");
  auto pAction = m_CreateAction(context);
  pAction->m_hDescriptorHandle = m_Handle;

  m_CreatedActions.PushBack(pAction);
  return pAction;
}

void WActionDescriptor::DeleteAction(WAction* pAction) const
{
  m_CreatedActions.RemoveAndSwap(pAction);

  if (m_DeleteAction == nullptr)
  {
    W_DEFAULT_DELETE(pAction);
  }
  else
    m_DeleteAction(pAction);
}


void WActionDescriptor::UpdateExistingActions()
{
  for (auto pAction : m_CreatedActions)
  {
    pAction->TriggerUpdate();
  }
}

void WAction::TriggerUpdate()
{
  m_StatusUpdateEvent.Broadcast(this);
}

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
