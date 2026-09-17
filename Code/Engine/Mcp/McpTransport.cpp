#include <Mcp/McpPCH.h>

#include <Mcp/McpSocket.h>
#include <Mcp/McpTransport.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  /// A request larger than this is not a request, it is either a mistake or an attack. The largest
  /// legitimate body is an object_modify call carrying a value, which is kilobytes.
  constexpr WUInt32 uiMaxRequestSize = 16 * 1024 * 1024;

  /// How long the accept loop blocks before looking at the stop flag again. Only bounds how long Stop()
  /// takes, so it can be generous.
  constexpr WTime AcceptPollInterval = WTime::MakeFromMilliseconds(100);

  /// Finds the value of one header in the raw header block, case insensitively.
  ///
  /// The result points into \a sHeaders, so it stays valid as long as the receive buffer does. Nothing
  /// is copied into a local builder for that reason: searching the original directly also means the
  /// offset cannot drift, which it would if a lower-cased copy of a header containing non-ASCII bytes
  /// came out at a different byte length than the input.
  WStringView FindHeaderValue(WStringView sHeaders, WStringView sName)
  {
    WStringBuilder sSearch = sName;
    sSearch.Append(":");

    const char* szFound = sHeaders.FindSubString_NoCase(sSearch);

    if (szFound == nullptr)
      return {};

    const char* szStart = szFound + sSearch.GetElementCount();
    const char* szEnd = sHeaders.GetEndPointer();

    if (const char* szLineEnd = WStringView(szStart, szEnd).FindSubString("\r\n"))
    {
      szEnd = szLineEnd;
    }

    while (szStart < szEnd && (*szStart == ' ' || *szStart == '\t'))
      ++szStart;

    while (szEnd > szStart && (szEnd[-1] == ' ' || szEnd[-1] == '\t'))
      --szEnd;

    return WStringView(szStart, szEnd);
  }
} // namespace

/// Owns the listening socket and turns bytes into WMcpHttpRequest.
class WMcpTransportThread : public WThread
{
public:
  WMcpTransportThread(WMcpTransport* pOwner)
    : WThread("WMcpTransport")
    , m_pOwner(pOwner)
  {
  }

  WMcpSocket m_Listener;
  WAtomicInteger32 m_iStopRequested;

private:
  virtual WUInt32 Run() override
  {
    while (m_iStopRequested == 0)
    {
      WMcpSocket client;

      if (m_Listener.Accept(client, AcceptPollInterval).Failed())
        continue; // nothing connected within the poll interval, or we are shutting down

      HandleConnection(client);
    }

    return 0;
  }

  /// Reads one request off the connection, has it answered, writes the response and hangs up.
  ///
  /// One request per connection. The server always answers `Connection: close`, so a client that wants
  /// to make a second call opens a second connection.
  void HandleConnection(WMcpSocket& client)
  {
    WDynamicArray<WUInt8> buffer;
    buffer.Reserve(4096);

    WUInt8 chunk[4096];
    WInt32 iHeaderEnd = -1;
    WInt32 iContentLength = 0;
    WUInt32 uiBodyStart = 0;

    while (true)
    {
      // find the end of the headers, then keep reading until the whole body has arrived - a single TCP
      // read is not guaranteed to contain either
      if (iHeaderEnd < 0)
      {
        const WStringView sSoFar(reinterpret_cast<const char*>(buffer.GetData()), buffer.GetCount());
        const char* szEnd = sSoFar.FindSubString("\r\n\r\n");

        if (szEnd != nullptr)
        {
          iHeaderEnd = static_cast<WInt32>(szEnd - sSoFar.GetStartPointer());
          uiBodyStart = static_cast<WUInt32>(iHeaderEnd) + 4;

          const WStringView sHeaders(reinterpret_cast<const char*>(buffer.GetData()), static_cast<WUInt32>(iHeaderEnd));
          const WStringView sLength = FindHeaderValue(sHeaders, "Content-Length");

          if (!sLength.IsEmpty())
          {
            WInt64 iParsed = 0;
            if (WConversionUtils::StringToInt64(sLength, iParsed).Succeeded() && iParsed > 0)
            {
              iContentLength = static_cast<WInt32>(WMath::Min<WInt64>(iParsed, uiMaxRequestSize));
            }
          }
        }
      }

      if (iHeaderEnd >= 0 && buffer.GetCount() >= uiBodyStart + static_cast<WUInt32>(iContentLength))
        break;  // request is complete

      if (buffer.GetCount() > uiMaxRequestSize)
        return; // hang up on something that is not going to become a valid request

      const WInt32 iRead = client.Receive(chunk, W_ARRAY_SIZE(chunk));

      if (iRead <= 0)
        return; // peer hung up, timed out, or errored - either way there is nothing to answer

      buffer.PushBackRange(WArrayPtr<const WUInt8>(chunk, static_cast<WUInt32>(iRead)));
    }

    WMcpHttpRequest request;
    ParseRequestLine(WStringView(reinterpret_cast<const char*>(buffer.GetData()), static_cast<WUInt32>(iHeaderEnd)), request);
    request.m_sBody = WStringView(reinterpret_cast<const char*>(buffer.GetData()) + uiBodyStart, static_cast<WUInt32>(iContentLength));

    WMcpHttpResponse response;

    if (m_pOwner->DispatchAndWait(request, response).Failed())
      return; // the transport was stopped before the main thread got to this - drop the connection

    SendResponse(client, response);
  }

  /// Splits "POST /mcp HTTP/1.1" into the two parts we care about.
  ///
  /// Done by hand rather than with WStringBuilder::Split(), whose result views would point into a
  /// builder that goes out of scope here.
  static void ParseRequestLine(WStringView sHeaders, WMcpHttpRequest& ref_request)
  {
    const char* szStart = sHeaders.GetStartPointer();
    const char* szEnd = sHeaders.GetEndPointer();

    if (const char* szLineEnd = sHeaders.FindSubString("\r\n"))
    {
      szEnd = szLineEnd;
    }

    const char* szFirstSpace = szStart;
    while (szFirstSpace < szEnd && *szFirstSpace != ' ')
      ++szFirstSpace;

    ref_request.m_sMethod = WStringView(szStart, szFirstSpace);

    if (szFirstSpace >= szEnd)
      return;

    const char* szPathStart = szFirstSpace + 1;
    const char* szSecondSpace = szPathStart;
    while (szSecondSpace < szEnd && *szSecondSpace != ' ')
      ++szSecondSpace;

    ref_request.m_sPath = WStringView(szPathStart, szSecondSpace);
  }

  static void SendResponse(WMcpSocket& client, const WMcpHttpResponse& response)
  {
    WStringBuilder sHeader;
    sHeader.SetFormat("HTTP/1.1 {}\r\n"
                      "Content-Type: application/json\r\n"
                      "Content-Length: {}\r\n"
                      "Connection: close\r\n"
                      "\r\n",
      response.m_sStatus, response.m_sBody.GetElementCount());

    if (client.SendAll(sHeader.GetData(), sHeader.GetElementCount()).Failed())
      return;

    if (!response.m_sBody.IsEmpty())
    {
      client.SendAll(response.m_sBody.GetData(), response.m_sBody.GetElementCount()).IgnoreResult();
    }
  }

  WMcpTransport* m_pOwner = nullptr;
};

WMcpTransport::WMcpTransport() = default;

WMcpTransport::~WMcpTransport()
{
  Stop();
}

WResult WMcpTransport::Start(WUInt16 uiPort, RequestHandler handler)
{
  Stop();

  WUniquePtr<WMcpTransportThread> pThread = W_DEFAULT_NEW(WMcpTransportThread, this);

  if (pThread->m_Listener.Listen(uiPort).Failed())
    return W_FAILURE;

  m_uiPort = pThread->m_Listener.GetPort();
  m_Handler = handler;
  m_bShutdown = false;

  m_pThread = std::move(pThread);
  m_pThread->Start();

  return W_SUCCESS;
}

void WMcpTransport::Stop()
{
  if (m_pThread == nullptr)
    return;

  {
    W_LOCK(m_Mutex);
    m_bShutdown = true;
  }

  // release a transport thread that is waiting for a response the main thread is never going to give it
  m_ResponseSignal.RaiseSignal();

  m_pThread->m_iStopRequested.Increment();
  m_pThread->Join();
  m_pThread->m_Listener.Close();
  m_pThread = nullptr;

  m_uiPort = 0;
  m_Handler = {};

  {
    W_LOCK(m_Mutex);
    m_pPendingRequest = nullptr;
    m_pPendingResponse = nullptr;
    m_bShutdown = false;
  }
}

bool WMcpTransport::HasPendingRequest() const
{
  W_LOCK(m_Mutex);
  return m_pPendingRequest != nullptr && !m_bRequestAnswered;
}

WResult WMcpTransport::DispatchAndWait(const WMcpHttpRequest& request, WMcpHttpResponse& out_response)
{
  {
    W_LOCK(m_Mutex);

    if (m_bShutdown)
      return W_FAILURE;

    m_pPendingRequest = &request;
    m_pPendingResponse = &out_response;
    m_bRequestAnswered = false;
  }

  // The main thread may take arbitrarily long over this - a project export or a C++ build runs for
  // minutes - so there is deliberately no timeout here. A client that gives up simply closes its end.
  while (true)
  {
    m_ResponseSignal.WaitForSignal();

    W_LOCK(m_Mutex);

    if (m_bShutdown)
    {
      m_pPendingRequest = nullptr;
      m_pPendingResponse = nullptr;
      return W_FAILURE;
    }

    if (m_bRequestAnswered)
    {
      m_pPendingRequest = nullptr;
      m_pPendingResponse = nullptr;
      return W_SUCCESS;
    }
  }
}

void WMcpTransport::ProcessPendingRequests()
{
  const WMcpHttpRequest* pRequest = nullptr;
  WMcpHttpResponse* pResponse = nullptr;

  {
    W_LOCK(m_Mutex);

    if (m_pPendingRequest == nullptr || m_bRequestAnswered)
      return;

    pRequest = m_pPendingRequest;
    pResponse = m_pPendingResponse;
  }

  // Outside the lock: this runs the tool, which may take minutes, and holding the mutex for that would
  // block the transport thread out of Stop() as well.
  if (m_Handler.IsValid())
  {
    pResponse->m_bDeferred = false;
    m_Handler(*pRequest, *pResponse);

    // The handler wants to be asked again rather than answered. Leaving the request pending is all that
    // takes: the transport thread is already blocked on the signal, and the client on its socket.
    if (pResponse->m_bDeferred)
      return;
  }
  else
  {
    pResponse->m_sStatus = "503 Service Unavailable";
    pResponse->m_sBody.Clear();
  }

  {
    W_LOCK(m_Mutex);
    m_bRequestAnswered = true;
  }

  m_ResponseSignal.RaiseSignal();
}
