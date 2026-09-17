#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <d3d11.h>

WHybridArray<ID3D11Query*, 4> WFencePoolDX11::s_Fences;
WGALDeviceDX11* WFencePoolDX11::s_pDevice;


void WFencePoolDX11::Initialize(WGALDeviceDX11* pDevice)
{
  s_pDevice = pDevice;
}

void WFencePoolDX11::DeInitialize()
{
  for (ID3D11Query* pQuery : s_Fences)
  {
    W_GAL_DX11_RELEASE(pQuery);
  }
  s_Fences.Clear();
  s_Fences.Compact();

  s_pDevice = nullptr;
}

ID3D11Query* WFencePoolDX11::RequestFence()
{
  W_ASSERT_DEBUG(s_pDevice, "WFencePoolDX11::Initialize not called");
  if (!s_Fences.IsEmpty())
  {
    ID3D11Query* pFence = s_Fences.PeekBack();
    s_Fences.PopBack();
    return pFence;
  }
  else
  {
    ID3D11Query* pFence;
    D3D11_QUERY_DESC QueryDesc;
    QueryDesc.Query = D3D11_QUERY_EVENT;
    QueryDesc.MiscFlags = 0;
    HRESULT res = s_pDevice->GetDXDevice()->CreateQuery(&QueryDesc, &pFence);
    if (!SUCCEEDED(res))
    {
      W_REPORT_FAILURE("Failed to create fence: {}", WArgErrorCode(res));
    }
    return pFence;
  }
}

void WFencePoolDX11::ReclaimFence(ID3D11Query*& ref_pFence)
{
  if (ref_pFence)
  {
    W_ASSERT_DEBUG(s_pDevice, "WFencePoolDX11::Initialize not called");
    s_Fences.PushBack(ref_pFence);
  }
  ref_pFence = nullptr;
}

void WFencePoolDX11::InsertFence(ID3D11Query* pFence)
{
  s_pDevice->GetDXImmediateContext()->End(pFence);
}

WEnum<WGALAsyncResult> WFencePoolDX11::GetFenceResult(ID3D11Query* pFence, WTime timeout)
{
  const WTime start = WTime::Now();

  do
  {
    BOOL data = FALSE;
    if (s_pDevice->GetDXImmediateContext()->GetData(pFence, &data, sizeof(data), 0) == S_OK)
    {
      W_ASSERT_DEV(data != FALSE, "Implementation error");
      return WGALAsyncResult::Ready;
    }
    WThreadUtils::YieldTimeSlice();
  } while ((WTime::Now() - start) < timeout);

  return WGALAsyncResult::Pending;
}


WFenceQueueDX11::WFenceQueueDX11(WAllocator* pAllocator)
  : m_PendingFences(pAllocator)
{
}

WFenceQueueDX11::~WFenceQueueDX11()
{
  while (!m_PendingFences.IsEmpty())
  {
    WaitForNextFence(WTime::MakeFromHours(1));
  }
}

WGALFenceHandle WFenceQueueDX11::GetCurrentFenceHandle()
{
  return m_uiCurrentFenceCounter;
}

WGALFenceHandle WFenceQueueDX11::SubmitCurrentFence()
{
  FlushReadyFences();
  ID3D11Query* pFence = WFencePoolDX11::RequestFence();
  WFencePoolDX11::InsertFence(pFence);

  m_PendingFences.PushBack({pFence, m_uiCurrentFenceCounter});
  WGALFenceHandle hCurrent = m_uiCurrentFenceCounter;
  m_uiCurrentFenceCounter++;
  return hCurrent;
}

void WFenceQueueDX11::FlushReadyFences()
{
  while (!m_PendingFences.IsEmpty())
  {
    if (WaitForNextFence() == WGALAsyncResult::Pending)
      return;
  }
}

WEnum<WGALAsyncResult> WFenceQueueDX11::GetFenceResult(WGALFenceHandle hFence, WTime timeout /*= WTime::MakeZero()*/)
{
  if (hFence <= m_uiReachedFenceCounter)
    return WGALAsyncResult::Ready;

  W_ASSERT_DEBUG(hFence <= m_uiCurrentFenceCounter, "Invalid fence handle");

  while (!m_PendingFences.IsEmpty() && m_PendingFences[0].m_hFence <= hFence)
  {
    const WTime start = WTime::Now();
    WEnum<WGALAsyncResult> res = WaitForNextFence(timeout);
    if (res == WGALAsyncResult::Pending)
      return WGALAsyncResult::Pending;

    const WTime end = WTime::Now();
    timeout -= (end - start);
  }

  return hFence <= m_uiReachedFenceCounter ? WGALAsyncResult::Ready : WGALAsyncResult::Pending;
}

WEnum<WGALAsyncResult> WFenceQueueDX11::WaitForNextFence(WTime timeout /*= WTime::MakeZero()*/)
{
  WEnum<WGALAsyncResult> fenceStatus = WFencePoolDX11::GetFenceResult(m_PendingFences[0].m_pFence, timeout);
  if (fenceStatus == WGALAsyncResult::Ready)
  {
    m_uiReachedFenceCounter = m_PendingFences[0].m_hFence;
    WFencePoolDX11::ReclaimFence(m_PendingFences[0].m_pFence);
    m_PendingFences.PopFront();
    return fenceStatus;
  }

  return fenceStatus;
}
