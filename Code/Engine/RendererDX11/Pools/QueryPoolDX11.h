#pragma once

#include <RendererDX11/RendererDX11DLL.h>
#include <d3d11.h>

struct ID3D11Query;
class WGALDeviceDX11;

/// Pool for GPU queries.
class W_RENDERERDX11_DLL WQueryPoolDX11
{
public:
  WQueryPoolDX11(WGALDeviceDX11* pDevice);

  /// Initializes the pool.
  WResult Initialize();
  void DeInitialize();

  void BeginFrame();
  void EndFrame();

  WGALTimestampHandle InsertTimestamp();

  /// Retrieves the timestamp value if it is available.
  /// \param hTimestamp The target timestamp to resolve.
  /// \param result The time of the timestamp. If this is empty on success the timestamp has expired.
  /// \return Returns false if the result is not available yet.
  WEnum<WGALAsyncResult> GetTimestampResult(WGALTimestampHandle hTimestamp, WTime& out_result);

  WGALPoolHandle BeginOcclusionQuery(WEnum<WGALQueryType> type);
  void EndOcclusionQuery(WGALPoolHandle hPool);
  WEnum<WGALAsyncResult> GetOcclusionQueryResult(WGALPoolHandle hPool, WUInt64& out_uiQueryResult);

private:
  static constexpr WUInt32 s_uiRetainFrames = 4;
  static constexpr WUInt64 s_uiPredicateFlag = W_BIT(19);
  static constexpr double s_fInvalid = -1.0;

  struct PerFrameData
  {
    WGALFenceHandle m_hFence;
    ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = s_fInvalid;
    WUInt32 m_uiReadyFrames = 0; ///< How many frames ago the m_pDisjointTimerQuery result was ready. Used to retain data for s_uiRetainFrames.
    WUInt64 m_uiFrameCounter = WUInt64(-1);
  };

private:
  PerFrameData GetFreeFrame();

private:
  WGALDeviceDX11* m_pDevice = nullptr;

  struct Pool
  {
    Pool(WAllocator* pAllocator);

    WResult Initialize(WGALDeviceDX11* pDevice, D3D11_QUERY queryType, WUInt32 uiCount);
    void DeInitialize();

    WGALPoolHandle CreateQuery();
    ID3D11Query* GetQuery(WGALPoolHandle hPool);
    template <typename T>
    WEnum<WGALAsyncResult> GetResult(WGALPoolHandle hPool, T& out_uiResult)
    {
      ID3D11Query* pQuery = GetQuery(hPool);
      HRESULT res = m_pDevice->GetDXImmediateContext()->GetData(pQuery, &out_uiResult, sizeof(out_uiResult), D3D11_ASYNC_GETDATA_DONOTFLUSH);
      if (res == S_FALSE)
      {
        return WGALAsyncResult::Pending;
      }
      else if (res == S_OK)
      {
        return WGALAsyncResult::Ready;
      }
      else
      {
        return WGALAsyncResult::Expired;
      }
    }

    // #TODO_DX11 Replace ring buffer with proper pool like in Vulkan to prevent buffer overrun.
    WDynamicArray<ID3D11Query*, WLocalAllocatorWrapper> m_Queries;
    WUInt32 m_uiNextTimestamp = 0;
    WGALDeviceDX11* m_pDevice = nullptr;
  };

  // Pools
  Pool m_TimestampPool;
  Pool m_OcclusionPool;
  Pool m_OcclusionPredicatePool;

  // Disjoint timer and frame meta data needed for timestamps
  WDeque<PerFrameData> m_PendingFrames;
  WDeque<PerFrameData> m_FreeFrames;
  WUInt64 m_uiFirstFrameIndex = 0;

  WTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};
