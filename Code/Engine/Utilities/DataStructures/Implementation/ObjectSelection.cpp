#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/ObjectSelection.h>

WObjectSelection::WObjectSelection()
{
  m_pWorld = nullptr;
}

void WObjectSelection::SetWorld(WWorld* pWorld)
{
  W_ASSERT_DEV((m_pWorld == pWorld) || m_Objects.IsEmpty(), "The selection has to be empty to change the world.");

  m_pWorld = pWorld;
}

void WObjectSelection::RemoveDeadObjects()
{
  W_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  for (WUInt32 i = m_Objects.GetCount(); i > 0; --i)
  {
    WGameObject* pObject;
    if (!m_pWorld->TryGetObject(m_Objects[i - 1], pObject))
    {
      m_Objects.RemoveAtAndCopy(i - 1); // keep the order
    }
  }
}

void WObjectSelection::AddObject(WGameObjectHandle hObject, bool bDontAddTwice)
{
  W_IGNORE_UNUSED(bDontAddTwice);
  W_ASSERT_DEV(m_pWorld != nullptr, "The world has not been set.");

  // only insert valid objects
  WGameObject* pObject;
  if (!m_pWorld->TryGetObject(hObject, pObject))
    return;

  if (m_Objects.IndexOf(hObject) != WInvalidIndex)
    return;

  m_Objects.PushBack(hObject);
}

bool WObjectSelection::RemoveObject(WGameObjectHandle hObject)
{
  return m_Objects.RemoveAndCopy(hObject);
}

void WObjectSelection::ToggleSelection(WGameObjectHandle hObject)
{
  for (WUInt32 i = 0; i < m_Objects.GetCount(); ++i)
  {
    if (m_Objects[i] == hObject)
    {
      m_Objects.RemoveAtAndCopy(i); // keep the order
      return;
    }
  }

  // ensures invalid objects don't get added
  AddObject(hObject);
}
