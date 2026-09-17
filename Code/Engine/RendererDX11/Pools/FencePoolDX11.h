#pragma once

#include <RendererDX11/RendererDX11DLL.h>

#include <Foundation/Time/Timestamp.h>

struct ID3D11Query;
class WGALDeviceDX11;


class W_RENDERERDX11_DLL WFencePoolDX11
{
public:
  static void Initialize(WGALDeviceDX11* pDevice);
  static void DeInitialize();

  static ID3D11Query* RequestFence();
  static void ReclaimFence(ID3D11Query*& ref_pFence);

  static void InsertFence(ID3D11Query* pFence);
  static WEnum<WGALAsyncResult> GetFenceResult(ID3D11Query* pFence, WTime timeout = WTime::MakeZero());

private:
  static WHybridArray<ID3D11Query*, 4> s_Fences;
  static WGALDeviceDX11* s_pDevice;
};


class W_RENDERERDX11_DLL WFenceQueueDX11
{
public:
  WFenceQueueDX11(WAllocator* pAllocator);
  ~WFenceQueueDX11();

  WGALFenceHandle GetCurrentFenceHandle();
  WGALFenceHandle SubmitCurrentFence();
  WEnum<WGALAsyncResult> GetFenceResult(WGALFenceHandle hFence, WTime timeout = WTime::MakeZero());

private:
  void FlushReadyFences();
  WEnum<WGALAsyncResult> WaitForNextFence(WTime timeout = WTime::MakeZero());

private:
  struct PendingFence
  {
    ID3D11Query* m_pFence = nullptr;
    WGALFenceHandle m_hFence = {};
  };
  WDeque<PendingFence> m_PendingFences;
  WUInt64 m_uiCurrentFenceCounter = 1;
  WUInt64 m_uiReachedFenceCounter = 0;
};
