

const void* WGALShaderByteCode::GetByteCode() const
{
  if (m_ByteCode.IsEmpty())
    return nullptr;

  return &m_ByteCode[0];
}

WUInt32 WGALShaderByteCode::GetSize() const
{
  return m_ByteCode.GetCount();
}

bool WGALShaderByteCode::IsValid() const
{
  return !m_ByteCode.IsEmpty();
}
