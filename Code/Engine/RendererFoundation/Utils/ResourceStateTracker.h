#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Delegate.h>

#include <RendererFoundation/Descriptors/Enumerations.h>

class WGALDevice;
struct WGALDeviceEvent;

template <>
struct WHashHelper<WGALTextureHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WGALTextureHandle value)
  {
    return WHashHelper<WGALTextureHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  W_ALWAYS_INLINE static bool Equal(WGALTextureHandle a, WGALTextureHandle b)
  {
    return a == b;
  }
};

template <>
struct WHashHelper<WGALBufferHandle>
{
  W_ALWAYS_INLINE static WUInt32 Hash(WGALBufferHandle value)
  {
    return WHashHelper<WGALBufferHandle::IdType::StorageType>::Hash(value.GetInternalID().m_Data);
  }

  W_ALWAYS_INLINE static bool Equal(WGALBufferHandle a, WGALBufferHandle b)
  {
    return a == b;
  }
};

/// Tracks texture and buffer states and emits barriers through callbacks when synchronization is required.
///
/// For resources that are first seen without an explicit initial state, the tracker starts from the resource default state (WGAL*CreationDescription::GetDefaultState) with stage Auto.
///
/// Barrier emission is callback-based: callbacks are only invoked when a barrier is needed. A barrier is required when the state changes, or when the previous state contains WGALResourceState::UnorderedAccess (UAV write-after-write).
/// States cannot be tracked across frames and before the frame end all resources must be reverted to their default state via RevertTextureState / RevertBufferState.
class W_RENDERERFOUNDATION_DLL WGALResourceStateTracker
{
public:
  struct SubResourceState
  {
    W_DECLARE_POD_TYPE();
    WBitflags<WGALResourceState> m_State;
    WBitflags<WGALShaderStageFlags> m_Stages;
  };

  struct TextureState
  {
    TextureState();
    WGALTextureRange m_FullRange; ///< Range covering the entire texture, filled on first occurrence.
    /// If size == 1, the single entry represents the entire resource (compressed).
    /// If size > 1, each entry corresponds to a sub-resource at index:
    /// uiMipLevel + uiLayer * m_FullRange.m_uiMipLevels
    WHybridArray<SubResourceState, 1> m_SubResourceStates;
  };

public:
  explicit WGALResourceStateTracker(WGALDevice* pDevice);
  ~WGALResourceStateTracker();

  WGALResourceStateTracker(const WGALResourceStateTracker&) = delete;
  WGALResourceStateTracker& operator=(const WGALResourceStateTracker&) = delete;

  void Clear();

  /// \name Initial State
  ///@{

  /// Overrides the tracked state for a texture.
  ///
  /// Resets tracking to a single compressed entry that represents the full resource and clears tracked stage flags.
  void SetInitialTextureState(WGALTextureHandle hTexture, WBitflags<WGALResourceState> state, WBitflags<WGALShaderStageFlags> stages = WGALShaderStageFlags::Auto);

  /// Overrides the tracked state for a buffer and clears tracked stage flags.
  void SetInitialBufferState(WGALBufferHandle hBuffer, WBitflags<WGALResourceState> state, WBitflags<WGALShaderStageFlags> stages = WGALShaderStageFlags::Auto);

  ///@}
  /// \name State Transitions
  ///@{

  /// Transitions a texture range to the given state.
  ///
  /// If range uses W_GAL_ALL_* sentinels they are resolved against the full texture range. The callback is invoked once per emitted barrier.
  void ChangeState(WGALTextureHandle hTexture,
    WGALTextureRange range,
    WBitflags<WGALResourceState> newState,
    WBitflags<WGALShaderStageFlags> stage,
    const WDelegate<void(const WGALTextureBarrier&)>& barrierCallback);

  /// Transitions a buffer to the given state.
  ///
  /// The callback is invoked only when a barrier is required.
  void ChangeState(WGALBufferHandle hBuffer,
    WBitflags<WGALResourceState> newState,
    WBitflags<WGALShaderStageFlags> stage,
    const WDelegate<void(const WGALBufferBarrier&)>& barrierCallback);

  /// Transitions all tracked textures back to their default state (WGALTextureCreationDescription::GetDefaultState).
  ///
  /// Emits full-resource barriers for compressed texture entries and per-subresource barriers for expanded entries.
  void RevertTextureState(const WDelegate<void(const WGALTextureBarrier&)>& barrierCallback);

  /// Transitions all tracked buffers back to their default state (WGALBufferCreationDescription::GetDefaultState).
  void RevertBufferState(const WDelegate<void(const WGALBufferBarrier&)>& barrierCallback);

  ///@}
  /// \name State Verification
  ///@{

  const SubResourceState* GetBufferState(WGALBufferHandle hBuffer) const;
  const TextureState* GetTextureState(WGALTextureHandle hTexture) const;
  static bool IsTextureBarrierNeeded(const SubResourceState& oldState, const SubResourceState& newState, bool bForceUAVBarrier = true);
  static bool IsBufferBarrierNeeded(const SubResourceState& oldState, const SubResourceState& newState, bool bForceUAVBarrier = true);
  static bool AreStagesCovered(WBitflags<WGALShaderStageFlags> coveredStages, WBitflags<WGALShaderStageFlags> requiredStages);

  ///@}
private:
  void GALDeviceEventHandler(const WGALDeviceEvent& e);
  void ResolveProxyTexture(WGALTextureHandle& ref_hTexture,
    WGALTextureRange& ref_range) const;
  TextureState& GetOrCreateTextureState(WGALTextureHandle hTexture);
  void ExpandTextureState(TextureState& state);
  SubResourceState& GetOrCreateBufferState(WGALBufferHandle hBuffer);

private:
  WGALDevice* m_pDevice = nullptr;
  WEventSubscriptionID m_GALDeviceEventSubscriptionID = 0;
  WHashTable<WGALTextureHandle, TextureState> m_TextureStates;
  WHashTable<WGALBufferHandle, SubResourceState> m_BufferStates;
};
