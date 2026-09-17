

WUInt32 WGALBuffer::GetSize() const
{
  return m_Description.m_uiTotalSize;
}

WGALBufferRange WGALBuffer::ClampRange(WGALBufferRange range) const
{
  const WUInt32 uiBufferSize = GetDescription().m_uiTotalSize;
  W_ASSERT_DEBUG(range.m_uiByteOffset < uiBufferSize, "Invalid WGALBufferRange: Buffer offset {} is out of bounds of the buffer size {}", range.m_uiByteOffset, uiBufferSize);
  if (range.m_uiByteCount == W_GAL_WHOLE_SIZE)
  {
    range.m_uiByteCount = uiBufferSize - range.m_uiByteOffset;
  }
  W_ASSERT_DEBUG(range.m_uiByteOffset + range.m_uiByteCount <= uiBufferSize, "Invalid WGALBufferRange: Buffer offset {} + byte count {} is bigger than buffer size {}", range.m_uiByteOffset, range.m_uiByteCount, uiBufferSize);
  return range;
}
