#pragma once

W_ALWAYS_INLINE void WSimdMat4d::Transpose()
{
  WMath::Swap(m_col0.m_v.y, m_col1.m_v.x);
  WMath::Swap(m_col0.m_v.z, m_col2.m_v.x);
  WMath::Swap(m_col0.m_v.w, m_col3.m_v.x);
  WMath::Swap(m_col1.m_v.z, m_col2.m_v.y);
  WMath::Swap(m_col1.m_v.w, m_col3.m_v.y);
  WMath::Swap(m_col2.m_v.w, m_col3.m_v.z);
}