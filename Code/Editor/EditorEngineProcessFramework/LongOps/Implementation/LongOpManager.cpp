#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

void WLongOpManager::Startup(WProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(WMakeDelegate(&WLongOpManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void WLongOpManager::Shutdown()
{
  W_LOCK(m_Mutex);

  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}
