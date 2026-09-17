#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/DataTransfer.h>

bool WDataTransfer::s_bInitialized = false;
WSet<WDataTransfer*> WDataTransfer::s_AllTransfers;

WDataTransferObject::WDataTransferObject(WDataTransfer& ref_belongsTo, WStringView sObjectName, WStringView sMimeType, WStringView sFileExtension)
  : m_BelongsTo(ref_belongsTo)
{
  m_bHasBeenTransferred = false;

  m_Msg.SetMessageID('TRAN', 'DATA');
  m_Msg.GetWriter() << ref_belongsTo.m_sDataName;
  m_Msg.GetWriter() << sObjectName;
  m_Msg.GetWriter() << sMimeType;
  m_Msg.GetWriter() << sFileExtension;
}

WDataTransferObject::~WDataTransferObject()
{
  W_ASSERT_DEV(m_bHasBeenTransferred, "The data transfer object has never been transmitted.");
}

void WDataTransferObject::Transmit()
{
  W_ASSERT_DEV(!m_bHasBeenTransferred, "The data transfer object has been transmitted already.");

  if (m_bHasBeenTransferred)
    return;

  m_bHasBeenTransferred = true;

  m_BelongsTo.Transfer(*this);
}

WDataTransfer::WDataTransfer()
{
  m_bTransferRequested = false;
  m_bEnabled = false;
}

WDataTransfer::~WDataTransfer()
{
  DisableDataTransfer();
}

void WDataTransfer::SendStatus()
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  WTelemetryMessage msg;
  msg.GetWriter() << m_sDataName;

  if (m_bEnabled)
  {
    msg.SetMessageID('TRAN', 'ENBL');
  }
  else
  {
    msg.SetMessageID('TRAN', 'DSBL');
  }

  WTelemetry::Broadcast(WTelemetry::Reliable, msg);
}

void WDataTransfer::DisableDataTransfer()
{
  if (!m_bEnabled)
    return;

  WDataTransfer::s_AllTransfers.Remove(this);

  m_bEnabled = false;
  SendStatus();

  m_bTransferRequested = false;
  m_sDataName.Clear();
}

void WDataTransfer::EnableDataTransfer(WStringView sDataName)
{
  if (m_bEnabled && m_sDataName == sDataName)
    return;

  DisableDataTransfer();

  Initialize();

  WDataTransfer::s_AllTransfers.Insert(this);

  m_sDataName = sDataName;

  W_ASSERT_DEV(!m_sDataName.IsEmpty(), "The name for the data transfer must not be empty.");

  m_bEnabled = true;
  SendStatus();
}

void WDataTransfer::RequestDataTransfer()
{
  if (!m_bEnabled)
  {
    m_bTransferRequested = false;
    return;
  }

  WLog::Dev("Data Transfer Request: {0}", m_sDataName);

  m_bTransferRequested = true;

  OnTransferRequest();
}

bool WDataTransfer::IsTransferRequested(bool bReset)
{
  const bool bRes = m_bTransferRequested;

  if (bReset)
    m_bTransferRequested = false;

  return bRes;
}

void WDataTransfer::Transfer(WDataTransferObject& Object)
{
  if (!m_bEnabled)
    return;

  WTelemetry::Broadcast(WTelemetry::Reliable, Object.m_Msg);
}

void WDataTransfer::Initialize()
{
  if (s_bInitialized)
    return;

  s_bInitialized = true;

  WTelemetry::AddEventHandler(TelemetryEventsHandler);
  WTelemetry::AcceptMessagesForSystem('DTRA', true, TelemetryMessage, nullptr);
}

void WDataTransfer::TelemetryMessage(void* pPassThrough)
{
  W_IGNORE_UNUSED(pPassThrough);

  WTelemetryMessage Msg;

  while (WTelemetry::RetrieveMessage('DTRA', Msg) == W_SUCCESS)
  {
    if (Msg.GetMessageID() == ' REQ')
    {
      WStringBuilder sName;
      Msg.GetReader() >> sName;

      WLog::Dev("Requested data transfer '{0}'", sName);

      for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key()->m_sDataName == sName)
        {
          it.Key()->RequestDataTransfer();
          break;
        }
      }
    }
  }
}

void WDataTransfer::TelemetryEventsHandler(const WTelemetry::TelemetryEventData& e)
{
  if (!WTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
    case WTelemetry::TelemetryEventData::ConnectedToClient:
      SendAllDataTransfers();
      break;

    default:
      break;
  }
}

void WDataTransfer::SendAllDataTransfers()
{
  WTelemetryMessage msg;
  msg.SetMessageID('TRAN', ' CLR');
  WTelemetry::Broadcast(WTelemetry::Reliable, msg);

  for (auto it = s_AllTransfers.GetIterator(); it.IsValid(); ++it)
  {
    it.Key()->SendStatus();
  }
}
