WInt32 WImageFilterWeights::GetFirstSourceSampleIndex(WUInt32 uiDstSampleIndex) const
{
  WSimdFloat dstSampleInSourceSpace = (WSimdFloat(uiDstSampleIndex) + WSimdFloat(0.5f)) * m_fDestToSourceScale;

  return WInt32(WMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline WArrayPtr<const float> WImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
