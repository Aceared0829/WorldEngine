#pragma once

#if W_ENABLED(W_PLATFORM_BIG_ENDIAN)

template <typename T>
WResult WStreamReader::ReadWordValue(T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt16));

  WUInt16 uiTemp;

  const WUInt32 uiRead = ReadBytes(reinterpret_cast<WUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<WUInt16*>(pWordValue) = WEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? W_SUCCESS : W_FAILURE;
}

template <typename T>
WResult WStreamReader::ReadDWordValue(T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt32));

  WUInt32 uiTemp;

  const WUInt32 uiRead = ReadBytes(reinterpret_cast<WUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<WUInt32*>(pDWordValue) = WEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? W_SUCCESS : W_FAILURE;
}

template <typename T>
WResult WStreamReader::ReadQWordValue(T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt64));

  WUInt64 uiTemp;

  const WUInt32 uiRead = ReadBytes(reinterpret_cast<WUInt8*>(&uiTemp), sizeof(T));

  *reinterpret_cast<WUInt64*>(pQWordValue) = WEndianHelper::Switch(uiTemp);

  return (uiRead == sizeof(T)) ? W_SUCCESS : W_FAILURE;
}



template <typename T>
WResult WStreamWriter::WriteWordValue(const T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt16));

  WUInt16 uiTemp = *reinterpret_cast<const WUInt16*>(pWordValue);
  uiTemp = WEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<WUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
WResult WStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt32));

  WUInt32 uiTemp = *reinterpret_cast<const WUInt32*>(pDWordValue);
  uiTemp = WEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<WUInt8*>(&uiTemp), sizeof(T));
}

template <typename T>
WResult WStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt64));

  WUInt64 uiTemp = *reinterpret_cast<const WUInt64*>(pQWordValue);
  uiTemp = WEndianHelper::Switch(uiTemp);

  return WriteBytes(reinterpret_cast<WUInt8*>(&uiTemp), sizeof(T));
}

#else

template <typename T>
WResult WStreamReader::ReadWordValue(T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt16));

  if (ReadBytes(reinterpret_cast<WUInt8*>(pWordValue), sizeof(T)) != sizeof(T))
    return W_FAILURE;

  return W_SUCCESS;
}

template <typename T>
WResult WStreamReader::ReadDWordValue(T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt32));

  if (ReadBytes(reinterpret_cast<WUInt8*>(pDWordValue), sizeof(T)) != sizeof(T))
    return W_FAILURE;

  return W_SUCCESS;
}

template <typename T>
WResult WStreamReader::ReadQWordValue(T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt64));

  if (ReadBytes(reinterpret_cast<WUInt8*>(pQWordValue), sizeof(T)) != sizeof(T))
    return W_FAILURE;

  return W_SUCCESS;
}

template <typename T>
WResult WStreamWriter::WriteWordValue(const T* pWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt16));

  return WriteBytes(reinterpret_cast<const WUInt8*>(pWordValue), sizeof(T));
}

template <typename T>
WResult WStreamWriter::WriteDWordValue(const T* pDWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt32));

  return WriteBytes(reinterpret_cast<const WUInt8*>(pDWordValue), sizeof(T));
}

template <typename T>
WResult WStreamWriter::WriteQWordValue(const T* pQWordValue)
{
  static_assert(sizeof(T) == sizeof(WUInt64));

  return WriteBytes(reinterpret_cast<const WUInt8*>(pQWordValue), sizeof(T));
}

#endif

WTypeVersion WStreamReader::ReadVersion()
{
  WTypeVersion v = 0;
  ReadWordValue(&v).IgnoreResult();
  return v;
}

WTypeVersion WStreamReader::ReadVersion(WTypeVersion expectedMaxVersion)
{
  WTypeVersion v = ReadVersion();

  W_ASSERT_ALWAYS(v <= expectedMaxVersion, "Read version ({0}) is larger than expected max version ({1}).", v, expectedMaxVersion);
  W_ASSERT_ALWAYS(v > 0, "Invalid version.");

  return v;
}

void WStreamWriter::WriteVersion(WTypeVersion version)
{
  W_ASSERT_ALWAYS(version > 0, "Version cannot be zero.");

  WriteWordValue(&version).IgnoreResult();
}


namespace WStreamWriterUtil
{
  // single element serialization

  template <class T>
  W_ALWAYS_INLINE auto SerializeImpl(WStreamWriter& inout_stream, const T& obj, int) -> decltype(inout_stream << obj, WResult(W_SUCCESS))
  {
    inout_stream << obj;

    return W_SUCCESS;
  }

  template <class T>
  W_ALWAYS_INLINE auto SerializeImpl(WStreamWriter& inout_stream, const T& obj, long) -> decltype(obj.Serialize(inout_stream).IgnoreResult(), WResult(W_SUCCESS))
  {
    return WToResult(obj.Serialize(inout_stream));
  }

  template <class T>
  W_ALWAYS_INLINE auto SerializeImpl(WStreamWriter& inout_stream, const T& obj, float) -> decltype(obj.serialize(inout_stream).IgnoreResult(), WResult(W_SUCCESS))
  {
    return WToResult(obj.serialize(inout_stream));
  }

  template <class T>
  W_ALWAYS_INLINE auto Serialize(WStreamWriter& inout_stream, const T& obj) -> decltype(SerializeImpl(inout_stream, obj, 0).IgnoreResult(), WResult(W_SUCCESS))
  {
    return SerializeImpl(inout_stream, obj, 0);
  }

  // serialization of array

  template <class T>
  W_ALWAYS_INLINE auto SerializeArrayImpl(WStreamWriter& inout_stream, const T* pArray, WUInt64 uiCount, int) -> decltype(SerializeArray(inout_stream, pArray, uiCount), WResult(W_SUCCESS))
  {
    return SerializeArray(inout_stream, pArray, uiCount);
  }

  template <class T>
  WResult SerializeArrayImpl(WStreamWriter& inout_stream, const T* pArray, WUInt64 uiCount, long)
  {
    for (WUInt64 i = 0; i < uiCount; ++i)
    {
      W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<T>(inout_stream, pArray[i]));
    }

    return W_SUCCESS;
  }

  template <class T>
  W_ALWAYS_INLINE WResult SerializeArray(WStreamWriter& inout_stream, const T* pArray, WUInt64 uiCount)
  {
    return SerializeArrayImpl(inout_stream, pArray, uiCount, 0);
  }
} // namespace WStreamWriterUtil

template <typename ArrayType, typename ValueType>
WResult WStreamWriter::WriteArray(const WArrayBase<ValueType, ArrayType>& array)
{
  const WUInt64 uiCount = array.GetCount();
  W_SUCCEED_OR_RETURN(WriteQWordValue(&uiCount));

  return WStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, WUInt16 uiSize>
WResult WStreamWriter::WriteArray(const WSmallArrayBase<ValueType, uiSize>& array)
{
  const WUInt32 uiCount = array.GetCount();
  W_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));

  return WStreamWriterUtil::SerializeArray<ValueType>(*this, array.GetData(), array.GetCount());
}

template <typename ValueType, WUInt32 uiSize>
WResult WStreamWriter::WriteArray(const ValueType (&array)[uiSize])
{
  const WUInt64 uiWriteSize = uiSize;
  W_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  return WStreamWriterUtil::SerializeArray<ValueType>(*this, array, uiSize);
}

template <typename KeyType, typename Comparer>
WResult WStreamWriter::WriteSet(const WSetBase<KeyType, Comparer>& set)
{
  const WUInt64 uiWriteSize = set.GetCount();
  W_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<KeyType>(*this, item));
  }

  return W_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
WResult WStreamWriter::WriteMap(const WMapBase<KeyType, ValueType, Comparer>& map)
{
  const WUInt64 uiWriteSize = map.GetCount();
  W_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = map.GetIterator(); It.IsValid(); ++It)
  {
    W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return W_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Hasher>
WResult WStreamWriter::WriteHashTable(const WHashTableBase<KeyType, ValueType, Hasher>& hashTable)
{
  const WUInt64 uiWriteSize = hashTable.GetCount();
  W_SUCCEED_OR_RETURN(WriteQWordValue(&uiWriteSize));

  for (auto It = hashTable.GetIterator(); It.IsValid(); ++It)
  {
    W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<KeyType>(*this, It.Key()));
    W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<ValueType>(*this, It.Value()));
  }

  return W_SUCCESS;
}

namespace WStreamReaderUtil
{
  template <class T>
  W_ALWAYS_INLINE auto DeserializeImpl(WStreamReader& inout_stream, T& ref_obj, int) -> decltype(inout_stream >> ref_obj, WResult(W_SUCCESS))
  {
    inout_stream >> ref_obj;

    return W_SUCCESS;
  }

  template <class T>
  W_ALWAYS_INLINE auto DeserializeImpl(WStreamReader& inout_stream, T& inout_obj, long) -> decltype(inout_obj.Deserialize(inout_stream).IgnoreResult(), WResult(W_SUCCESS))
  {
    return WToResult(inout_obj.Deserialize(inout_stream));
  }

  template <class T>
  W_ALWAYS_INLINE auto DeserializeImpl(WStreamReader& inout_stream, T& inout_obj, float) -> decltype(inout_obj.deserialize(inout_stream).IgnoreResult(), WResult(W_SUCCESS))
  {
    return WToResult(inout_obj.deserialize(inout_stream));
  }

  template <class T>
  W_ALWAYS_INLINE auto Deserialize(WStreamReader& inout_stream, T& inout_obj) -> decltype(DeserializeImpl(inout_stream, inout_obj, 0).IgnoreResult(), WResult(W_SUCCESS))
  {
    return DeserializeImpl(inout_stream, inout_obj, 0);
  }

  // serialization of array

  template <class T>
  W_ALWAYS_INLINE auto DeserializeArrayImpl(WStreamReader& inout_stream, T* pArray, WUInt64 uiCount, int) -> decltype(DeserializeArray(inout_stream, pArray, uiCount), WResult(W_SUCCESS))
  {
    return DeserializeArray(inout_stream, pArray, uiCount);
  }

  template <class T>
  WResult DeserializeArrayImpl(WStreamReader& inout_stream, T* pArray, WUInt64 uiCount, long)
  {
    for (WUInt64 i = 0; i < uiCount; ++i)
    {
      W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize<T>(inout_stream, pArray[i]));
    }

    return W_SUCCESS;
  }

  template <class T>
  W_ALWAYS_INLINE WResult DeserializeArray(WStreamReader& inout_stream, T* pArray, WUInt64 uiCount)
  {
    return DeserializeArrayImpl(inout_stream, pArray, uiCount, 0);
  }

} // namespace WStreamReaderUtil

template <typename ArrayType, typename ValueType>
WResult WStreamReader::ReadArray(WArrayBase<ValueType, ArrayType>& inout_array)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < WMath::MaxValue<WUInt32>())
  {
    inout_array.Clear();

    if (uiCount > 0)
    {
      static_cast<ArrayType&>(inout_array).SetCount(static_cast<WUInt32>(uiCount));

      W_SUCCEED_OR_RETURN(WStreamReaderUtil::DeserializeArray<ValueType>(*this, inout_array.GetData(), uiCount));
    }

    return W_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return W_FAILURE;
  }
}

template <typename ValueType, WUInt16 uiSize, typename AllocatorWrapper>
WResult WStreamReader::ReadArray(WSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array)
{
  WUInt32 uiCount = 0;
  W_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

  if (uiCount < WMath::MaxValue<WUInt16>())
  {
    ref_array.Clear();

    if (uiCount > 0)
    {
      ref_array.SetCount(static_cast<WUInt16>(uiCount));

      W_SUCCEED_OR_RETURN(WStreamReaderUtil::DeserializeArray<ValueType>(*this, ref_array.GetData(), uiCount));
    }

    return W_SUCCESS;
  }
  else
  {
    // Small array uses 16 bit for counts internally. Value from file is too large.
    return W_FAILURE;
  }
}

template <typename ValueType, WUInt32 uiSize>
WResult WStreamReader::ReadArray(ValueType (&array)[uiSize])
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (static_cast<WUInt32>(uiCount) != uiSize)
    return W_FAILURE;

  if (uiCount < WMath::MaxValue<WUInt32>())
  {
    W_SUCCEED_OR_RETURN(WStreamReaderUtil::DeserializeArray<ValueType>(*this, array, uiCount));

    return W_SUCCESS;
  }

  // Containers currently use 32 bit for counts internally. Value from file is too large.
  return W_FAILURE;
}

template <typename KeyType, typename Comparer>
WResult WStreamReader::ReadSet(WSetBase<KeyType, Comparer>& inout_set)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < WMath::MaxValue<WUInt32>())
  {
    inout_set.Clear();

    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      KeyType Item;
      W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize(*this, Item));

      inout_set.Insert(std::move(Item));
    }

    return W_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return W_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Comparer>
WResult WStreamReader::ReadMap(WMapBase<KeyType, ValueType, Comparer>& inout_map)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < WMath::MaxValue<WUInt32>())
  {
    inout_map.Clear();

    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize(*this, Key));
      W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize(*this, Value));

      inout_map.Insert(std::move(Key), std::move(Value));
    }

    return W_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return W_FAILURE;
  }
}

template <typename KeyType, typename ValueType, typename Hasher>
WResult WStreamReader::ReadHashTable(WHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(ReadQWordValue(&uiCount));

  if (uiCount < WMath::MaxValue<WUInt32>())
  {
    inout_hashTable.Clear();
    inout_hashTable.Reserve(static_cast<WUInt32>(uiCount));

    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      KeyType Key;
      ValueType Value;
      W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize(*this, Key));
      W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize(*this, Value));

      inout_hashTable.Insert(std::move(Key), std::move(Value));
    }

    return W_SUCCESS;
  }
  else
  {
    // Containers currently use 32 bit for counts internally. Value from file is too large.
    return W_FAILURE;
  }
}
