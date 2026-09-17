
#include <Foundation/IO/Stream.h>

template <typename T>
W_ALWAYS_INLINE WResult WDeduplicationReadContext::ReadObjectInplace(WStreamReader& inout_stream, T& inout_obj)
{
  return ReadObject(inout_stream, inout_obj, nullptr);
}

template <typename T>
WResult WDeduplicationReadContext::ReadObject(WStreamReader& inout_stream, T& obj, WAllocator* pAllocator)
{
  bool bIsRealObject;
  inout_stream >> bIsRealObject;

  W_ASSERT_DEV(bIsRealObject, "Reading an object inplace only works for the first occurrence");

  W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize<T>(inout_stream, obj));

  m_Objects.PushBack(&obj);

  return W_SUCCESS;
}

template <typename T>
WResult WDeduplicationReadContext::ReadObject(WStreamReader& inout_stream, T*& ref_pObject, WAllocator* pAllocator)
{
  bool bIsRealObject;
  inout_stream >> bIsRealObject;

  if (bIsRealObject)
  {
    W_ASSERT_DEBUG(pAllocator != nullptr, "Valid allocator required");
    ref_pObject = W_NEW(pAllocator, T);
    W_SUCCEED_OR_RETURN(WStreamReaderUtil::Deserialize<T>(inout_stream, *ref_pObject));

    m_Objects.PushBack(ref_pObject);
  }
  else
  {
    WUInt32 uiIndex;
    inout_stream >> uiIndex;

    if (uiIndex < m_Objects.GetCount())
    {
      ref_pObject = static_cast<T*>(m_Objects[uiIndex]);
    }
    else if (uiIndex == WInvalidIndex)
    {
      ref_pObject = nullptr;
    }
    else
    {
      return W_FAILURE;
    }
  }

  return W_SUCCESS;
}

template <typename T>
WResult WDeduplicationReadContext::ReadObject(WStreamReader& inout_stream, WSharedPtr<T>& ref_pObject, WAllocator* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(inout_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = WSharedPtr<T>(ptr, pAllocator);
    return W_SUCCESS;
  }
  return W_FAILURE;
}

template <typename T>
WResult WDeduplicationReadContext::ReadObject(WStreamReader& inout_stream, WUniquePtr<T>& ref_pObject, WAllocator* pAllocator)
{
  T* ptr = nullptr;
  if (ReadObject(inout_stream, ptr, pAllocator).Succeeded())
  {
    ref_pObject = std::move(WUniquePtr<T>(ptr, pAllocator));
    return W_SUCCESS;
  }
  return W_FAILURE;
}

template <typename ArrayType, typename ValueType>
WResult WDeduplicationReadContext::ReadArray(WStreamReader& inout_stream, WArrayBase<ValueType, ArrayType>& ref_array, WAllocator* pAllocator)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  W_ASSERT_DEV(uiCount < std::numeric_limits<WUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_array.Clear();

  if (uiCount > 0)
  {
    static_cast<ArrayType&>(ref_array).Reserve(static_cast<WUInt32>(uiCount));

    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      W_SUCCEED_OR_RETURN(ReadObject(inout_stream, ref_array.ExpandAndGetRef(), pAllocator));
    }
  }

  return W_SUCCESS;
}

template <typename KeyType, typename Comparer>
WResult WDeduplicationReadContext::ReadSet(WStreamReader& inout_stream, WSetBase<KeyType, Comparer>& ref_set, WAllocator* pAllocator)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  W_ASSERT_DEV(uiCount < std::numeric_limits<WUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_set.Clear();

  for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
  {
    KeyType key;
    W_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pAllocator));

    ref_set.Insert(std::move(key));
  }

  return W_SUCCESS;
}

namespace WInternal
{
  // Internal helper to prevent the compiler from trying to find a de-serialization method for pointer types or other types which don't have
  // one.
  struct DeserializeHelper
  {
    template <typename T>
    static auto Deserialize(WStreamReader& inout_stream, T& ref_obj, int) -> decltype(WStreamReaderUtil::Deserialize(inout_stream, ref_obj))
    {
      return WStreamReaderUtil::Deserialize(inout_stream, ref_obj);
    }

    template <typename T>
    static WResult Deserialize(WStreamReader& inout_stream, T& ref_obj, float)
    {
      W_REPORT_FAILURE("No deserialize method available");
      return W_FAILURE;
    }
  };
} // namespace WInternal

template <typename KeyType, typename ValueType, typename Comparer>
WResult WDeduplicationReadContext::ReadMap(WStreamReader& inout_stream, WMapBase<KeyType, ValueType, Comparer>& ref_map, ReadMapMode mode, WAllocator* pKeyAllocator, WAllocator* pValueAllocator)
{
  WUInt64 uiCount = 0;
  W_SUCCEED_OR_RETURN(inout_stream.ReadQWordValue(&uiCount));

  W_ASSERT_DEV(uiCount < std::numeric_limits<WUInt32>::max(), "Containers currently use 32 bit for counts internally. Value from file is too large.");

  ref_map.Clear();

  if (mode == ReadMapMode::DedupKey)
  {
    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      W_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pKeyAllocator));
      W_SUCCEED_OR_RETURN(WInternal::DeserializeHelper::Deserialize<ValueType>(inout_stream, value, 0));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else if (mode == ReadMapMode::DedupValue)
  {
    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      W_SUCCEED_OR_RETURN(WInternal::DeserializeHelper::Deserialize<KeyType>(inout_stream, key, 0));
      W_SUCCEED_OR_RETURN(ReadObject(inout_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }
  else
  {
    for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
    {
      KeyType key;
      ValueType value;
      W_SUCCEED_OR_RETURN(ReadObject(inout_stream, key, pKeyAllocator));
      W_SUCCEED_OR_RETURN(ReadObject(inout_stream, value, pValueAllocator));

      ref_map.Insert(std::move(key), std::move(value));
    }
  }

  return W_SUCCESS;
}
