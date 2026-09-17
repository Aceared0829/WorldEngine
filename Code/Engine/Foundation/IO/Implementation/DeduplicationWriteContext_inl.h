
#include <Foundation/IO/Stream.h>

namespace WInternal
{
  // This internal helper is needed to differentiate between reference and pointer which is not possible with regular function overloading
  // in this case.
  template <typename T>
  struct WriteObjectHelper
  {
    static const T* GetAddress(const T& obj) { return &obj; }
  };

  template <typename T>
  struct WriteObjectHelper<T*>
  {
    static const T* GetAddress(const T* pObj) { return pObj; }
  };
} // namespace WInternal

template <typename T>
W_ALWAYS_INLINE WResult WDeduplicationWriteContext::WriteObject(WStreamWriter& inout_stream, const T& obj)
{
  return WriteObjectInternal(inout_stream, WInternal::WriteObjectHelper<T>::GetAddress(obj));
}

template <typename T>
W_ALWAYS_INLINE WResult WDeduplicationWriteContext::WriteObject(WStreamWriter& inout_stream, const WSharedPtr<T>& pObject)
{
  return WriteObjectInternal(inout_stream, pObject.Borrow());
}

template <typename T>
W_ALWAYS_INLINE WResult WDeduplicationWriteContext::WriteObject(WStreamWriter& inout_stream, const WUniquePtr<T>& pObject)
{
  return WriteObjectInternal(inout_stream, pObject.Borrow());
}

template <typename ArrayType, typename ValueType>
WResult WDeduplicationWriteContext::WriteArray(WStreamWriter& inout_stream, const WArrayBase<ValueType, ArrayType>& array)
{
  const WUInt64 uiCount = array.GetCount();
  W_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiCount));

  for (WUInt32 i = 0; i < static_cast<WUInt32>(uiCount); ++i)
  {
    W_SUCCEED_OR_RETURN(WriteObject(inout_stream, array[i]));
  }

  return W_SUCCESS;
}

template <typename KeyType, typename Comparer>
WResult WDeduplicationWriteContext::WriteSet(WStreamWriter& inout_stream, const WSetBase<KeyType, Comparer>& set)
{
  const WUInt64 uiWriteSize = set.GetCount();
  W_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiWriteSize));

  for (const auto& item : set)
  {
    W_SUCCEED_OR_RETURN(WriteObject(inout_stream, item));
  }

  return W_SUCCESS;
}

template <typename KeyType, typename ValueType, typename Comparer>
WResult WDeduplicationWriteContext::WriteMap(WStreamWriter& inout_stream, const WMapBase<KeyType, ValueType, Comparer>& map, WriteMapMode mode)
{
  const WUInt64 uiWriteSize = map.GetCount();
  W_SUCCEED_OR_RETURN(inout_stream.WriteQWordValue(&uiWriteSize));

  if (mode == WriteMapMode::DedupKey)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      W_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Key()));
      W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<ValueType>(inout_stream, It.Value()));
    }
  }
  else if (mode == WriteMapMode::DedupValue)
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      W_SUCCEED_OR_RETURN(WStreamWriterUtil::Serialize<KeyType>(inout_stream, It.Key()));
      W_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Value()));
    }
  }
  else
  {
    for (auto It = map.GetIterator(); It.IsValid(); ++It)
    {
      W_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Key()));
      W_SUCCEED_OR_RETURN(WriteObject(inout_stream, It.Value()));
    }
  }

  return W_SUCCESS;
}

template <typename T>
WResult WDeduplicationWriteContext::WriteObjectInternal(WStreamWriter& stream, const T* pObject)
{
  WUInt32 uiIndex = WInvalidIndex;

  if (pObject)
  {
    bool bIsRealObject = !m_Objects.TryGetValue(pObject, uiIndex);
    stream << bIsRealObject;

    if (bIsRealObject)
    {
      uiIndex = m_Objects.GetCount();
      m_Objects.Insert(pObject, uiIndex);

      return WStreamWriterUtil::Serialize<T>(stream, *pObject);
    }
    else
    {
      stream << uiIndex;
    }
  }
  else
  {
    stream << false;
    stream << WInvalidIndex;
  }

  return W_SUCCESS;
}
