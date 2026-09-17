
W_ALWAYS_INLINE const WExpressionByteCode::StorageType* WExpressionByteCode::GetByteCodeStart() const
{
  return m_pByteCode;
}

W_ALWAYS_INLINE const WExpressionByteCode::StorageType* WExpressionByteCode::GetByteCodeEnd() const
{
  return m_pByteCode + m_uiByteCodeCount;
}

W_ALWAYS_INLINE WArrayPtr<const WExpressionByteCode::StorageType> WExpressionByteCode::GetByteCode() const
{
  return WMakeArrayPtr(m_pByteCode, m_uiByteCodeCount);
}

W_ALWAYS_INLINE WUInt32 WExpressionByteCode::GetNumInstructions() const
{
  return m_uiNumInstructions;
}

W_ALWAYS_INLINE WUInt32 WExpressionByteCode::GetNumTempRegisters() const
{
  return m_uiNumTempRegisters;
}

W_ALWAYS_INLINE WArrayPtr<const WExpression::StreamDesc> WExpressionByteCode::GetInputs() const
{
  return WMakeArrayPtr(m_pInputs, m_uiNumInputs);
}

W_ALWAYS_INLINE WArrayPtr<const WExpression::StreamDesc> WExpressionByteCode::GetOutputs() const
{
  return WMakeArrayPtr(m_pOutputs, m_uiNumOutputs);
}

W_ALWAYS_INLINE WArrayPtr<const WExpression::FunctionDesc> WExpressionByteCode::GetFunctions() const
{
  return WMakeArrayPtr(m_pFunctions, m_uiNumFunctions);
}

// static
W_ALWAYS_INLINE WExpressionByteCode::OpCode::Enum WExpressionByteCode::GetOpCode(const StorageType*& ref_pByteCode)
{
  WUInt32 uiOpCode = *ref_pByteCode;
  ++ref_pByteCode;
  return static_cast<OpCode::Enum>((uiOpCode >= 0 && uiOpCode < OpCode::Count) ? uiOpCode : 0);
}

// static
W_ALWAYS_INLINE WUInt32 WExpressionByteCode::GetRegisterIndex(const StorageType*& ref_pByteCode)
{
  WUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
W_ALWAYS_INLINE WExpression::Register WExpressionByteCode::GetConstant(const StorageType*& ref_pByteCode)
{
  WExpression::Register r;
  r.i = WSimdVec4i(*ref_pByteCode);
  ++ref_pByteCode;
  return r;
}

// static
W_ALWAYS_INLINE WUInt32 WExpressionByteCode::GetFunctionIndex(const StorageType*& ref_pByteCode)
{
  WUInt32 uiIndex = *ref_pByteCode;
  ++ref_pByteCode;
  return uiIndex;
}

// static
W_ALWAYS_INLINE WUInt32 WExpressionByteCode::GetFunctionArgCount(const StorageType*& ref_pByteCode)
{
  WUInt32 uiArgCount = *ref_pByteCode;
  ++ref_pByteCode;
  return uiArgCount;
}
