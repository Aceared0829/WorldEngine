#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Pools/FencePoolDX11.h>
#include <RendererDX11/Pools/QueryPoolDX11.h>

WQueryPoolDX11::WQueryPoolDX11(WGALDeviceDX11* pDevice)
  : m_pDevice(pDevice)
  , m_TimestampPool(pDevice->GetAllocator())
  , m_OcclusionPool(pDevice->GetAllocator())
  , m_OcclusionPredicatePool(pDevice->GetAllocator())
  , m_PendingFrames(pDevice->GetAllocator())
  , m_FreeFrames(pDevice->GetAllocator())
{
}

WResult WQueryPoolDX11::Initialize()
{
  W_SUCCEED_OR_RETURN(m_TimestampPool.Initialize(m_pDevice, D3D11_QUERY_TIMESTAMP, 4096));
  W_SUCCEED_OR_RETURN(m_OcclusionPool.Initialize(m_pDevice, D3D11_QUERY_OCCLUSION, 512));
  W_SUCCEED_OR_RETURN(m_OcclusionPredicatePool.Initialize(m_pDevice, D3D11_QUERY_OCCLUSION_PREDICATE, 512));

  m_SyncTimeDiff = WTime::MakeZero();
  return W_SUCCESS;
}

void WQueryPoolDX11::DeInitialize()
{
  m_TimestampPool.DeInitialize();
  m_OcclusionPool.DeInitialize();
  m_OcclusionPredicatePool.DeInitialize();

  for (auto& perFrameData : m_FreeFrames)
  {
    W_GAL_DX11_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }
  for (auto& perFrameData : m_PendingFrames)
  {
    W_GAL_DX11_RELEASE(perFrameData.m_pDisjointTimerQuery);
  }
}

void WQueryPoolDX11::BeginFrame()
{
  WUInt64 uiCurrentFrame = m_pDevice->GetCurrentFrame();

  auto perFrameData = GetFreeFrame();
  perFrameData.m_uiFrameCounter = uiCurrentFrame;
  m_pDevice->GetDXImmediateContext()->Begin(perFrameData.m_pDisjointTimerQuery);
  perFrameData.m_fInvTicksPerSecond = -1.0f;
  m_PendingFrames.PushBack(perFrameData);

  for (PerFrameData& data : m_PendingFrames)
  {
    if (data.m_fInvTicksPerSecond != s_fInvalid)
      data.m_uiReadyFrames++;
  }

  // Clear out old frames
  while (m_PendingFrames[0].m_uiReadyFrames > s_uiRetainFrames)
  {
    PerFrameData& data = m_PendingFrames.PeekFront();
    data.m_hFence = {};
    data.m_fInvTicksPerSecond = s_fInvalid;
    data.m_uiReadyFrames = 0;
    data.m_uiFrameCounter = WUInt64(-1);

    m_FreeFrames.PushBack(data);
    m_PendingFrames.PopFront();
  }
  m_uiFirstFrameIndex = m_PendingFrames[0].m_uiFrameCounter;
}

void WQueryPoolDX11::EndFrame()
{
  {
    auto& perFrameData = m_PendingFrames.PeekBack();
    m_pDevice->GetDXImmediateContext()->End(perFrameData.m_pDisjointTimerQuery);
    perFrameData.m_hFence = m_pDevice->GetFenceQueue().GetCurrentFenceHandle();
  }

  // Get Results
  for (WUInt32 i = 0; i < m_PendingFrames.GetCount(); i++)
  {
    auto& perFrameData = m_PendingFrames[i];
    if (m_pDevice->GetFenceQueue().GetFenceResult(perFrameData.m_hFence) != WGALAsyncResult::Ready)
      break;

    if (perFrameData.m_fInvTicksPerSecond == s_fInvalid)
    {
      D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data = {};
      HRESULT res = m_pDevice->GetDXImmediateContext()->GetData(perFrameData.m_pDisjointTimerQuery, &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);
      if (res == S_OK)
      {
        if (data.Disjoint)
        {
          perFrameData.m_fInvTicksPerSecond = 0.0;
          continue;
        }

        perFrameData.m_fInvTicksPerSecond = 1.0 / (double)data.Frequency;

        if (m_bSyncTimeNeeded)
        {
          WGALTimestampHandle hTimestamp = InsertTimestamp();
          ID3D11Query* pQuery = m_TimestampPool.GetQuery(hTimestamp);
          WUInt64 uiTimestamp;
          while (m_pDevice->GetDXImmediateContext()->GetData(pQuery, &uiTimestamp, sizeof(uiTimestamp), 0) != S_OK)
          {
            WThreadUtils::YieldTimeSlice();
          }

          m_SyncTimeDiff = WTime::Now() - WTime::MakeFromSeconds(double(uiTimestamp) * perFrameData.m_fInvTicksPerSecond);
          m_bSyncTimeNeeded = false;
        }
      }
    }
  }
}


WQueryPoolDX11::PerFrameData WQueryPoolDX11::GetFreeFrame()
{
  if (!m_FreeFrames.IsEmpty())
  {
    PerFrameData data = m_FreeFrames.PeekFront();
    m_FreeFrames.PopFront();
    return data;
  }

  PerFrameData perFrameData;
  D3D11_QUERY_DESC disjointQueryDesc;
  disjointQueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
  disjointQueryDesc.MiscFlags = 0;
  HRESULT res = m_pDevice->GetDXDevice()->CreateQuery(&disjointQueryDesc, &perFrameData.m_pDisjointTimerQuery);
  W_ASSERT_DEV(SUCCEEDED(res), "Creation of native DirectX query for disjoint query has failed!");
  W_IGNORE_UNUSED(res);
  return perFrameData;
}

WGALTimestampHandle WQueryPoolDX11::InsertTimestamp()
{
  WGALTimestampHandle hTimestamp = m_TimestampPool.CreateQuery();
  ID3D11Query* pDXQuery = m_TimestampPool.GetQuery(hTimestamp);
  m_pDevice->GetDXImmediateContext()->End(pDXQuery);
  return hTimestamp;
}

WEnum<WGALAsyncResult> WQueryPoolDX11::GetTimestampResult(WGALTimestampHandle hTimestamp, WTime& out_result)
{
  out_result = WTime();
  if (hTimestamp.m_Generation < m_uiFirstFrameIndex)
  {
    // expired
    return WGALAsyncResult::Expired;
  }

  const WUInt32 uiFrameIndex = static_cast<WUInt32>(hTimestamp.m_Generation - m_uiFirstFrameIndex);
  PerFrameData& pPerFrameData = m_PendingFrames[uiFrameIndex];
  // Check whether frequency and sync timer are already available for the frame of the timestamp
  if (pPerFrameData.m_fInvTicksPerSecond == s_fInvalid)
    return WGALAsyncResult::Pending;

  WUInt64 uiTimestamp;
  WEnum<WGALAsyncResult> res = m_TimestampPool.GetResult(hTimestamp, uiTimestamp);
  if (res != WGALAsyncResult::Ready)
    return res;

  if (pPerFrameData.m_fInvTicksPerSecond == 0.0)
  {
    out_result = WTime::MakeZero();
    return WGALAsyncResult::Expired;
  }
  else
  {
    out_result = WTime::MakeFromSeconds(double(uiTimestamp) * pPerFrameData.m_fInvTicksPerSecond) + m_SyncTimeDiff;
    return WGALAsyncResult::Ready;
  }
}


WGALPoolHandle WQueryPoolDX11::BeginOcclusionQuery(WEnum<WGALQueryType> type)
{
  WGALPoolHandle hPool;
  ID3D11Query* pQuery = nullptr;
  if (type == WGALQueryType::NumSamplesPassed)
  {
    hPool = m_OcclusionPool.CreateQuery();
    pQuery = m_OcclusionPool.GetQuery(hPool);
  }
  else if (type == WGALQueryType::AnySamplesPassed)
  {
    hPool = m_OcclusionPredicatePool.CreateQuery();
    pQuery = m_OcclusionPredicatePool.GetQuery(hPool);
    hPool.m_InstanceIndex |= s_uiPredicateFlag;
  }
  m_pDevice->GetDXImmediateContext()->Begin(pQuery);

  return hPool;
}


void WQueryPoolDX11::EndOcclusionQuery(WGALPoolHandle hPool)
{
  ID3D11Query* pQuery = nullptr;
  bool bPredicate = (hPool.m_InstanceIndex & s_uiPredicateFlag) != 0;
  hPool.m_InstanceIndex &= ~s_uiPredicateFlag;
  if (bPredicate)
  {
    pQuery = m_OcclusionPredicatePool.GetQuery(hPool);
  }
  else
  {
    pQuery = m_OcclusionPool.GetQuery(hPool);
  }
  m_pDevice->GetDXImmediateContext()->End(pQuery);
}


WEnum<WGALAsyncResult> WQueryPoolDX11::GetOcclusionQueryResult(WGALPoolHandle hPool, WUInt64& out_uiQueryResult)
{
  out_uiQueryResult = 0;
  bool bPredicate = (hPool.m_InstanceIndex & s_uiPredicateFlag) != 0;
  hPool.m_InstanceIndex &= ~s_uiPredicateFlag;
  if (bPredicate)
  {
    WUInt32 uiTemp;
    WEnum<WGALAsyncResult> res = m_OcclusionPredicatePool.GetResult(hPool, uiTemp);
    out_uiQueryResult = uiTemp;
    return res;
  }
  else
  {
    return m_OcclusionPool.GetResult(hPool, out_uiQueryResult);
  }
}

WQueryPoolDX11::Pool::Pool(WAllocator* pAllocator)
  : m_Queries(pAllocator)
{
}

WResult WQueryPoolDX11::Pool::Initialize(WGALDeviceDX11* pDevice, D3D11_QUERY queryType, WUInt32 uiCount)
{
  m_pDevice = pDevice;

  D3D11_QUERY_DESC timerQueryDesc;
  timerQueryDesc.Query = queryType;
  timerQueryDesc.MiscFlags = 0;

  m_Queries.SetCountUninitialized(uiCount);
  for (WUInt32 i = 0; i < m_Queries.GetCount(); ++i)
  {
    if (FAILED(pDevice->GetDXDevice()->CreateQuery(&timerQueryDesc, &m_Queries[i])))
    {
      WLog::Error("Creation of native DirectX query for timestamp has failed!");
      return W_FAILURE;
    }
  }
  return W_SUCCESS;
}

void WQueryPoolDX11::Pool::DeInitialize()
{
  for (auto& timestamp : m_Queries)
  {
    W_GAL_DX11_RELEASE(timestamp);
  }
  m_Queries.Clear();
}

WGALPoolHandle WQueryPoolDX11::Pool::CreateQuery()
{
  WUInt32 uiIndex = m_uiNextTimestamp;
  m_uiNextTimestamp = (m_uiNextTimestamp + 1) % m_Queries.GetCount();
  WGALTimestampHandle hTimestamp = {uiIndex, m_pDevice->GetCurrentFrame()};
  return hTimestamp;
}

ID3D11Query* WQueryPoolDX11::Pool::GetQuery(WGALPoolHandle hPool)
{
  if (hPool.m_InstanceIndex < m_Queries.GetCount())
  {
    return m_Queries[static_cast<WUInt32>(hPool.m_InstanceIndex)];
  }

  return nullptr;
}
