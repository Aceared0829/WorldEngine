#pragma once

W_ALWAYS_INLINE void WSimdMat4d::Transpose()
{
#if W_SSE_LEVEL >= W_SSE_AVX
    _MM_TRANSPOSE4_PD_AVX(
    m_col0.m_v, 
    m_col1.m_v, 
    m_col2.m_v, 
    m_col3.m_v);
#else
  _MM_TRANSPOSE4_PD(
    m_col0.m_v.xy, m_col0.m_v.zw, 
    m_col1.m_v.xy, m_col1.m_v.zw, 
    m_col2.m_v.xy, m_col2.m_v.zw, 
    m_col3.m_v.xy, m_col3.m_v.zw);
#endif

}
