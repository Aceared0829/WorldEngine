#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Utils/RingBufferTracker.h>

WRingBufferTracker::WRingBufferTracker(WUInt32 uiAlignment, WUInt32 uiTotalSize)
  : m_uiAlignment(uiAlignment)
  , m_uiTotalSize(uiTotalSize)
  , m_uiFree(uiTotalSize)
{
  W_ASSERT_DEBUG(WMath::IsPowerOf2(m_uiAlignment), "Non-power of two alignment not supported");
}

WResult WRingBufferTracker::CanAllocate(WUInt32 uiSize) const
{
  uiSize = WMemoryUtils::AlignSize(uiSize, m_uiAlignment);
  if (m_uiFree < uiSize)
    return W_FAILURE;

  if (m_uiCurrentOffset + uiSize > m_uiTotalSize)
  {
    const WUInt32 uiSkip = m_uiTotalSize - m_uiCurrentOffset;
    if (m_uiFree - uiSkip < uiSize)
      return W_FAILURE;
  }
  return W_SUCCESS;
}

WResult WRingBufferTracker::Allocate(WUInt32 uiSize, WUInt64 uiCurrentFrame, WUInt32& out_uiAllocatedOffset)
{
  uiSize = WMemoryUtils::AlignSize(uiSize, m_uiAlignment);

  FrameData* pData = nullptr;
  // New frame data blocks need to be created in these cases:
  // 1: There is no block yet.
  // 2: The last block is from a previous frame.
  // 3: The block was already submitted. For this, we explicitly don't remove the s_FrameDataSubmitted flag before comparing frames.
  // 4: We wrapped around in the last allocation, i.e. m_uiCurrentOffset is before the last blocks starting point.
  if (m_FrameData.IsEmpty() || m_FrameData.PeekBack().m_uiFrame != uiCurrentFrame || m_uiCurrentOffset < m_FrameData.PeekBack().m_uiStartOffset)
  {
    pData = &m_FrameData.ExpandAndGetRef();
    pData->m_uiFrame = uiCurrentFrame;
    pData->m_uiStartOffset = m_uiCurrentOffset;
  }
  else
  {
    pData = &m_FrameData.PeekBack();
  }

  if (m_uiFree < uiSize)
    return W_FAILURE;

  if (m_uiCurrentOffset + uiSize > m_uiTotalSize)
  {
    const WUInt32 uiSkip = m_uiTotalSize - m_uiCurrentOffset;
    if (m_uiFree - uiSkip < uiSize)
      return W_FAILURE;

    pData->m_uiSize += uiSkip;
    m_uiFree -= uiSkip;
    // Creating a new frame data when going over the buffer boundary makes it easier to later upload the data.
    pData = &m_FrameData.ExpandAndGetRef();
    pData->m_uiFrame = uiCurrentFrame;
    m_uiCurrentOffset = 0;
  }

  out_uiAllocatedOffset = m_uiCurrentOffset;
  pData->m_uiSize += uiSize;
  m_uiFree -= uiSize;
  m_uiCurrentOffset = (m_uiCurrentOffset + uiSize) % m_uiTotalSize;

  return W_SUCCESS;
}

void WRingBufferTracker::Free(WUInt64 uiUpToFrame)
{
  while (!m_FrameData.IsEmpty() && (m_FrameData[0].m_uiFrame & ~s_FrameDataSubmitted) <= uiUpToFrame)
  {
    m_uiFree += m_FrameData[0].m_uiSize;
    m_FrameData.RemoveAtAndCopy(0);
  }
}

WResult WRingBufferTracker::SubmitFrame(WUInt64 uiFrame, WDynamicArray<FrameData>& out_frameData)
{
  out_frameData.Clear();
  for (WUInt32 i = 0; i < m_FrameData.GetCount(); i++)
  {
    if (m_FrameData[i].m_uiFrame == uiFrame)
    {
      FrameData& data = out_frameData.ExpandAndGetRef();
      data = m_FrameData[i];
      m_FrameData[i].m_uiFrame |= s_FrameDataSubmitted;
    }
  }
  return out_frameData.IsEmpty() ? W_FAILURE : W_SUCCESS;
}
