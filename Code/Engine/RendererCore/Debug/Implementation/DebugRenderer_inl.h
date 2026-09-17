
W_ALWAYS_INLINE WDebugRendererLine::WDebugRendererLine() = default;

W_ALWAYS_INLINE WDebugRendererLine::WDebugRendererLine(const WVec3& vStart, const WVec3& vEnd)
  : m_start(vStart)
  , m_end(vEnd)
{
}

W_ALWAYS_INLINE WDebugRendererLine::WDebugRendererLine(const WVec3& vStart, const WVec3& vEnd, const WColor& color)
  : m_start(vStart)
  , m_end(vEnd)
  , m_startColor(color)
  , m_endColor(color)
{
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WDebugRendererTriangle::WDebugRendererTriangle() = default;

W_ALWAYS_INLINE WDebugRendererTriangle::WDebugRendererTriangle(const WVec3& v0, const WVec3& v1, const WVec3& v2)

{
  m_position[0] = v0;
  m_position[1] = v1;
  m_position[2] = v2;
}
