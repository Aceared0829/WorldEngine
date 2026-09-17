#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>

/// Tracks memory in a ring buffer on a frame by frame basis.
/// The intended usage pattern is that for frame N, multiple allocations are called. This is followed by a call to SubmitFrame for frame N (optional) which allows the user to handle to memory ranges that were modified in this frame. Eventually at a later point in time Free is called for frame N to allow reuse of the memory.
/// Note that the assumption is that N is always increasing and that free is called for each frame in order.
class W_RENDERERFOUNDATION_DLL WRingBufferTracker
{
public:
  // Used to tag the frameData as submitted to make sure we create a new FrameData bucket and don't upload the same range twice.
  constexpr static WUInt64 s_FrameDataSubmitted = W_BIT(63);

  // A memory range starting at m_uiStartOffset of size m_uiSize that was modified in a frame.
  struct FrameData
  {
    WUInt64 m_uiFrame = 0;       ///< The frame in which the memory was allocated.
    WUInt32 m_uiStartOffset = 0; ///< The start offset of the memory that was modified in this frame.
    WUInt32 m_uiSize = 0;        ///< The size of the memory modification.
  };

public:
  /// Crates a new tracker.
  /// \param uiAlignment All allocations will be aligned to this. Must be power of two.
  /// \param uiTotalSize The size of the memory.
  WRingBufferTracker(WUInt32 uiAlignment, WUInt32 uiTotalSize); // [tested]

  // Checks whether a contiguous memory block of the given size can be allocated.
  /// \param uiSize Size requested.
  /// \return Returns W_FAILURE if not enough memory remains to fulfil the allocation.
  WResult CanAllocate(WUInt32 uiSize) const; // [tested]

  /// Allocates memory of the given size.
  /// \param uiSize Size requested.
  /// \param uiCurrentFrame The current frame for which this allocation is tracked.
  /// \param out_uiAllocatedOffset Will be filled by the start offset of the allocation inside the memory if successful.
  /// \return Returns W_FAILURE if not enough memory remains to fulfil the allocation.
  WResult Allocate(WUInt32 uiSize, WUInt64 uiCurrentFrame, WUInt32& out_uiAllocatedOffset); // [tested]

  /// Makes all memory that was used up to uiUpToFrame available again for allocations.
  /// \param uiUpToFrame All frames below this and this frame itself are safe to be reused.
  void Free(WUInt64 uiUpToFrame); // [tested]

  /// Retrieves the dirty memory ranges of this frame.
  /// Note that this can be anything between zero and two. In case that the end of the buffer is reached and wraps around within a frame, the end part and the part at the start are split into two FrameData objects. Can be called multiple times per frame. Each dirty range will only be given out once.
  /// \param uiFrame Frame index to submit dirty ranges.
  /// \param out_frameData Will receive the FrameData objects containing the dirty ranges of the memory.
  /// \return Returns W_FAILURE if no allocations were made in uiFrame.
  WResult SubmitFrame(WUInt64 uiFrame, WDynamicArray<FrameData>& out_frameData);    // [tested]

  W_ALWAYS_INLINE WUInt32 GetTotalMemory() const { return m_uiTotalSize; }           // [tested]
  W_ALWAYS_INLINE WUInt32 GetUsedMemory() const { return m_uiTotalSize - m_uiFree; } // [tested]
  W_ALWAYS_INLINE WUInt32 GetFreeMemory() const { return m_uiFree; }                 // [tested]

private:
  WUInt32 m_uiAlignment = 0;
  WUInt32 m_uiTotalSize = 0;
  WUInt32 m_uiFree = 0;
  WUInt32 m_uiCurrentOffset = 0;
  WHybridArray<FrameData, 4> m_FrameData;
};
