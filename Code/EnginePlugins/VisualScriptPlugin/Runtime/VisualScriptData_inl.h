
W_FORCE_INLINE void WVisualScriptDataDescription::CheckOffset(DataOffset dataOffset, const WRTTI* pType) const
{
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  auto givenDataType = dataOffset.GetType();
  auto& offsetAndCount = m_PerTypeInfo[givenDataType];
  W_ASSERT_DEBUG(offsetAndCount.m_uiCount > 0, "Invalid data offset");
  const WUInt32 uiLastOffset = offsetAndCount.m_uiStartOffset + (offsetAndCount.m_uiCount - 1) * WVisualScriptDataType::GetStorageSize(givenDataType);
  W_ASSERT_DEBUG(dataOffset.m_uiByteOffset >= offsetAndCount.m_uiStartOffset && dataOffset.m_uiByteOffset <= uiLastOffset, "Invalid data offset");

  if (pType != nullptr)
  {
    auto expectedDataType = WVisualScriptDataType::FromRtti(pType);
    W_ASSERT_DEBUG(expectedDataType == givenDataType, "Data type mismatch, expected '{}'({}) but got '{}'", WVisualScriptDataType::GetName(expectedDataType), pType->GetTypeName(), WVisualScriptDataType::GetName(givenDataType));
  }
#endif
}

W_FORCE_INLINE WVisualScriptDataDescription::DataOffset WVisualScriptDataDescription::GetOffset(WVisualScriptDataType::Enum dataType, WUInt32 uiIndex, DataOffset::Source::Enum source) const
{
  auto& offsetAndCount = m_PerTypeInfo[dataType];
  WUInt32 uiByteOffset = WInvalidIndex;
  if (uiIndex < offsetAndCount.m_uiCount)
  {
    uiByteOffset = offsetAndCount.m_uiStartOffset + uiIndex * WVisualScriptDataType::GetStorageSize(dataType);
  }

  return DataOffset(uiByteOffset, dataType, source);
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE const WVisualScriptDataDescription& WVisualScriptDataStorage::GetDesc() const
{
  return *m_pDesc;
}

W_ALWAYS_INLINE bool WVisualScriptDataStorage::IsAllocated() const
{
  return m_Storage.IsEmpty() == false;
}

template <typename T>
const T& WVisualScriptDataStorage::GetData(DataOffset dataOffset) const
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, WTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, WGetStaticRTTI<T>());

  return *reinterpret_cast<const T*>(m_Storage.GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
T& WVisualScriptDataStorage::GetWritableData(DataOffset dataOffset)
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, WTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, WGetStaticRTTI<T>());

  return *reinterpret_cast<T*>(m_Storage.GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
void WVisualScriptDataStorage::SetData(DataOffset dataOffset, const T& value)
{
  static_assert(!std::is_pointer<T>::value, "Use SetPointerData instead");

  if (dataOffset.m_uiByteOffset < m_Storage.GetCount())
  {
    m_pDesc->CheckOffset(dataOffset, WGetStaticRTTI<T>());

    auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, WGameObjectHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<WVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, WComponentHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<WVisualScriptComponentHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, WStringView>::value)
    {
      *reinterpret_cast<WString*>(pData) = value;
    }
    else
    {
      *reinterpret_cast<T*>(pData) = value;
    }
  }
}

template <typename T>
void WVisualScriptDataStorage::SetPointerData(DataOffset dataOffset, T ptr, const WRTTI* pType, WUInt32 uiExecutionCounter)
{
  static_assert(std::is_pointer<T>::value);

  if (dataOffset.m_uiByteOffset < m_Storage.GetCount())
  {
    auto pData = m_Storage.GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, WGameObject*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, WGetStaticRTTI<WGameObject>());

      auto& storedHandle = *reinterpret_cast<WVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else if constexpr (std::is_same<T, WComponent*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, WGetStaticRTTI<WComponent>());

      auto& storedHandle = *reinterpret_cast<WVisualScriptComponentHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else
    {
      const bool bIsAllowedType = !pType || (pType->IsDerivedFrom<WComponent>() == false && pType->IsDerivedFrom<WGameObject>() == false);
      W_ASSERT_DEBUG(bIsAllowedType,
        "GameObject or Component type '{}' is stored as typed pointer, cast to WGameObject or WComponent first to ensure correct storage", pType->GetTypeName());

      m_pDesc->CheckOffset(dataOffset, pType);

      auto& typedPointer = *reinterpret_cast<WTypedPointer*>(pData);
      typedPointer.m_pObject = ptr;
      typedPointer.m_pType = pType;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

inline WResult WVisualScriptInstanceData::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(m_DataOffset.Serialize(inout_stream));

  if (m_DataOffset.GetType() != WVisualScriptDataType::GameObject &&
    m_DataOffset.GetType() != WVisualScriptDataType::Component &&
    m_DataOffset.GetType() != WVisualScriptDataType::TypedPointer)
  {
    inout_stream << m_DefaultValue;
  }

  return W_SUCCESS;
}

inline WResult WVisualScriptInstanceData::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(m_DataOffset.Deserialize(inout_stream));

  if (m_DataOffset.GetType() == WVisualScriptDataType::GameObject)
  {
    m_DefaultValue = WGameObjectHandle();
  }
  else if (m_DataOffset.GetType() == WVisualScriptDataType::Component)
  {
    m_DefaultValue = WComponentHandle();
  }
  else if (m_DataOffset.GetType() == WVisualScriptDataType::TypedPointer)
  {
    m_DefaultValue = WTypedPointer();
  }
  else
  {
    inout_stream >> m_DefaultValue;
  }

  return W_SUCCESS;
}
