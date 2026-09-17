
// static
template <typename T, WUInt32 Size>
void WVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(WArrayPtr<const T> a, WUInt32& inout_uiAdditionalDataSize)
{
  if (a.GetCount() > Size)
  {
    inout_uiAdditionalDataSize = WMemoryUtils::AlignSize<WUInt32>(inout_uiAdditionalDataSize, alignof(T));
    inout_uiAdditionalDataSize += a.GetCount() * sizeof(T);
  }
}

// static
template <typename T, WUInt32 Size>
void WVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(WUInt32 uiSize, WUInt32 uiAlignment, WUInt32& inout_uiAdditionalDataSize)
{
  if (uiSize > Size * sizeof(T))
  {
    inout_uiAdditionalDataSize = WMemoryUtils::AlignSize<WUInt32>(inout_uiAdditionalDataSize, uiAlignment);
    inout_uiAdditionalDataSize += uiSize;
  }
}

template <typename T, WUInt32 Size>
T* WVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::Init(WUInt8 uiCount, WUInt32 uiAlignment, WUInt8*& inout_pAdditionalData)
{
  if (uiCount <= Size)
  {
    return m_Embedded;
  }

  inout_pAdditionalData = WMemoryUtils::AlignForwards(inout_pAdditionalData, uiAlignment);
  m_Ptr = reinterpret_cast<T*>(inout_pAdditionalData);
  inout_pAdditionalData += uiCount * sizeof(T);
  return m_Ptr;
}

template <typename T, WUInt32 Size>
WResult WVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::ReadFromStream(WUInt8& out_uiCount, WStreamReader& inout_stream, WUInt8*& inout_pAdditionalData)
{
  WUInt16 uiCount = 0;
  inout_stream >> uiCount;

  if (uiCount > WMath::MaxValue<WUInt8>())
  {
    return W_FAILURE;
  }
  out_uiCount = static_cast<WUInt8>(uiCount);

  T* pTargetPtr = Init(out_uiCount, alignof(T), inout_pAdditionalData);
  const WUInt64 uiNumBytesToRead = uiCount * sizeof(T);
  if (inout_stream.ReadBytes(pTargetPtr, uiNumBytesToRead) != uiNumBytesToRead)
    return W_FAILURE;

  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE WUInt32 WVisualScriptGraphDescription::Node::GetExecutionIndex(WUInt32 uiSlot) const
{
  if (uiSlot < m_NumExecutionIndices)
  {
    return m_NumExecutionIndices <= W_ARRAY_SIZE(m_ExecutionIndices.m_Embedded) ? m_ExecutionIndices.m_Embedded[uiSlot] : m_ExecutionIndices.m_Ptr[uiSlot];
  }

  return WInvalidIndex;
}

W_ALWAYS_INLINE WVisualScriptGraphDescription::DataOffset WVisualScriptGraphDescription::Node::GetInputDataOffset(WUInt32 uiSlot) const
{
  if (uiSlot < m_NumInputDataOffsets)
  {
    return m_NumInputDataOffsets <= W_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded[uiSlot] : m_InputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

W_ALWAYS_INLINE WVisualScriptGraphDescription::DataOffset WVisualScriptGraphDescription::Node::GetOutputDataOffset(WUInt32 uiSlot) const
{
  if (uiSlot < m_NumOutputDataOffsets)
  {
    return m_NumOutputDataOffsets <= W_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded[uiSlot] : m_OutputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

W_ALWAYS_INLINE WVisualScriptGraphDescription::DataOffset* WVisualScriptGraphDescription::Node::GetInputDataOffsets()
{
  return m_NumInputDataOffsets <= W_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded : m_InputDataOffsets.m_Ptr;
}

W_ALWAYS_INLINE WVisualScriptGraphDescription::DataOffset* WVisualScriptGraphDescription::Node::GetOutputDataOffsets()
{
  return m_NumOutputDataOffsets <= W_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded : m_OutputDataOffsets.m_Ptr;
}

// static
template <typename T>
W_ALWAYS_INLINE constexpr WUInt32 WVisualScriptGraphDescription::Node::GetUserDataAlignment()
{
  // Ensures at least 8 byte alignment, thus making it compatible between 32 and 64 bit platforms.
  return WMath::Max<WUInt32>(alignof(T), 8u);
}

template <typename T>
W_ALWAYS_INLINE const T& WVisualScriptGraphDescription::Node::GetUserData() const
{
  W_ASSERT_DEBUG(m_UserDataByteSize >= sizeof(T), "Invalid data");
  return *reinterpret_cast<const T*>(m_UserDataByteSize <= sizeof(m_UserData.m_Embedded) ? m_UserData.m_Embedded : m_UserData.m_Ptr);
}

template <typename T>
T& WVisualScriptGraphDescription::Node::InitUserData(WUInt8*& inout_pAdditionalData, WUInt32 uiByteSize /*= sizeof(T)*/, WUInt32 uiAlignment /*= GetUserDataAlignment<T>()*/)
{
  m_UserDataByteSize = uiByteSize;
  const WUInt32 uiUserDataCount = uiByteSize / sizeof(WUInt32);
  W_ASSERT_DEBUG(uiUserDataCount <= WMath::MaxValue<WUInt8>(), "User data is too big");
  auto pUserData = m_UserData.Init(static_cast<WUInt8>(uiUserDataCount), uiAlignment, inout_pAdditionalData);
  W_CHECK_ALIGNMENT(pUserData, uiAlignment);
  return *reinterpret_cast<T*>(pUserData);
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE const WVisualScriptGraphDescription::Node* WVisualScriptGraphDescription::GetNode(WUInt32 uiIndex) const
{
  return uiIndex < m_Nodes.GetCount() ? &m_Nodes.GetPtr()[uiIndex] : nullptr;
}

W_ALWAYS_INLINE bool WVisualScriptGraphDescription::IsCoroutine() const
{
  auto entryNodeType = GetNode(0)->m_Type;
  return entryNodeType == WVisualScriptNodeDescription::Type::EntryCall_Coroutine || entryNodeType == WVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
}

W_ALWAYS_INLINE const WSharedPtr<const WVisualScriptDataDescription>& WVisualScriptGraphDescription::GetLocalDataDesc() const
{
  return m_pLocalDataDesc;
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
W_FORCE_INLINE const T& WVisualScriptExecutionContext::GetData(DataOffset dataOffset) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetData<T>(dataOffset);
}

template <typename T>
W_FORCE_INLINE T& WVisualScriptExecutionContext::GetWritableData(DataOffset dataOffset)
{
  W_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Can't write to constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetWritableData<T>(dataOffset);
}

template <typename T>
W_FORCE_INLINE void WVisualScriptExecutionContext::SetData(DataOffset dataOffset, const T& value)
{
  W_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetData<T>(dataOffset, value);
}

W_FORCE_INLINE WTypedPointer WVisualScriptExecutionContext::GetPointerData(DataOffset dataOffset)
{
  W_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetPointerData(dataOffset, m_uiExecutionCounter);
}

template <typename T>
W_FORCE_INLINE void WVisualScriptExecutionContext::SetPointerData(DataOffset dataOffset, T ptr, const WRTTI* pType)
{
  W_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  m_DataStorage[dataOffset.m_uiSource]->SetPointerData(dataOffset, ptr, pType, m_uiExecutionCounter);
}

W_FORCE_INLINE WVariant WVisualScriptExecutionContext::GetDataAsVariant(DataOffset dataOffset, const WRTTI* pExpectedType) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetDataAsVariant(dataOffset, pExpectedType, m_uiExecutionCounter);
}

W_FORCE_INLINE void WVisualScriptExecutionContext::SetDataFromVariant(DataOffset dataOffset, const WVariant& value)
{
  W_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetDataFromVariant(dataOffset, value, m_uiExecutionCounter);
}

W_ALWAYS_INLINE void WVisualScriptExecutionContext::SetCurrentCoroutine(WScriptCoroutine* pCoroutine)
{
  m_pCurrentCoroutine = pCoroutine;
}

inline WTime WVisualScriptExecutionContext::GetDeltaTimeSinceLastExecution()
{
  W_ASSERT_DEBUG(m_pDesc->IsCoroutine(), "Delta time is only valid for coroutines");
  return m_DeltaTimeSinceLastExecution;
}
