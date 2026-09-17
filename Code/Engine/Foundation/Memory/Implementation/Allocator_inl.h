
W_ALWAYS_INLINE WAllocator::WAllocator() = default;

W_ALWAYS_INLINE WAllocator::~WAllocator() = default;


namespace WMath
{
  // due to #include order issues, we have to forward declare this function here

  W_FOUNDATION_DLL WUInt64 SafeMultiply64(WUInt64 a, WUInt64 b, WUInt64 c, WUInt64 d);
} // namespace WMath

namespace WInternal
{
  template <typename T>
  struct NewInstance
  {
    W_ALWAYS_INLINE NewInstance(T* pInstance, WAllocator* pAllocator)
    {
      m_pInstance = pInstance;
      m_pAllocator = pAllocator;
    }

    template <typename U>
    W_ALWAYS_INLINE NewInstance(NewInstance<U>&& other)
    {
      m_pInstance = other.m_pInstance;
      m_pAllocator = other.m_pAllocator;

      other.m_pInstance = nullptr;
      other.m_pAllocator = nullptr;
    }

    W_ALWAYS_INLINE NewInstance(std::nullptr_t) {}

    template <typename U>
    W_ALWAYS_INLINE NewInstance<U> Cast()
    {
      return NewInstance<U>(static_cast<U*>(m_pInstance), m_pAllocator);
    }

    W_ALWAYS_INLINE operator T*() { return m_pInstance; }

    W_ALWAYS_INLINE T* operator->() { return m_pInstance; }

    T* m_pInstance = nullptr;
    WAllocator* m_pAllocator = nullptr;
  };

  template <typename T>
  W_ALWAYS_INLINE bool operator<(const NewInstance<T>& lhs, T* rhs)
  {
    return lhs.m_pInstance < rhs;
  }

  template <typename T>
  W_ALWAYS_INLINE bool operator<(T* lhs, const NewInstance<T>& rhs)
  {
    return lhs < rhs.m_pInstance;
  }

  template <typename T>
  W_FORCE_INLINE void Delete(WAllocator* pAllocator, T* pPtr)
  {
    if (pPtr != nullptr)
    {
      WMemoryUtils::Destruct(pPtr, 1);
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  W_FORCE_INLINE T* CreateRawBuffer(WAllocator* pAllocator, size_t uiCount)
  {
    WUInt64 safeAllocationSize = WMath::SafeMultiply64(uiCount, sizeof(T));
    return static_cast<T*>(pAllocator->Allocate(static_cast<size_t>(safeAllocationSize), alignof(T))); // Down-cast to size_t for 32-bit
  }

  W_FORCE_INLINE void DeleteRawBuffer(WAllocator* pAllocator, void* pPtr)
  {
    if (pPtr != nullptr)
    {
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  inline WArrayPtr<T> CreateArray(WAllocator* pAllocator, WUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    WMemoryUtils::Construct<SkipTrivialTypes>(buffer, uiCount);

    return WArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  inline void DeleteArray(WAllocator* pAllocator, WArrayPtr<T> arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != nullptr)
    {
      WMemoryUtils::Destruct(buffer, arrayPtr.GetCount());
      pAllocator->Deallocate(buffer);
    }
  }

  template <typename T>
  W_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, WAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, WTypeIsPod)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), alignof(T));
  }

  template <typename T>
  W_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, WAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, WTypeIsMemRelocatable)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), alignof(T));
  }

  template <typename T>
  W_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, WAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, WTypeIsClass)
  {
    static_assert(!std::is_trivial<T>::value,
      "POD type is treated as class. Use W_DECLARE_POD_TYPE(YourClass) or W_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.");

    T* pNewMem = CreateRawBuffer<T>(pAllocator, uiNewCount);
    WMemoryUtils::RelocateConstruct(pNewMem, pPtr, uiCurrentCount);
    DeleteRawBuffer(pAllocator, pPtr);
    return pNewMem;
  }

  template <typename T>
  W_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, WAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount)
  {
    W_ASSERT_DEV(uiCurrentCount < uiNewCount, "Shrinking of a buffer is not implemented yet");
    W_ASSERT_DEV(!(uiCurrentCount == uiNewCount), "Same size passed in twice.");
    if (pPtr == nullptr)
    {
      W_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is nullptr");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(pPtr, pAllocator, uiCurrentCount, uiNewCount, WGetTypeClass<T>());
  }
} // namespace WInternal
