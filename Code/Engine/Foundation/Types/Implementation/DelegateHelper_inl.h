
/// [Internal] Storage for lambdas with captures in WDelegate.
struct W_FOUNDATION_DLL WLambdaDelegateStorageBase
{
  WLambdaDelegateStorageBase() = default;
  virtual ~WLambdaDelegateStorageBase() = default;
  virtual WLambdaDelegateStorageBase* Clone(WAllocator* pAllocator) const = 0;
  virtual void InplaceCopy(WUInt8* pBuffer) const = 0;
  virtual void InplaceMove(WUInt8* pBuffer) = 0;

private:
  WLambdaDelegateStorageBase(const WLambdaDelegateStorageBase&) = delete;
  WLambdaDelegateStorageBase& operator=(const WLambdaDelegateStorageBase&) = delete;
  WLambdaDelegateStorageBase(WLambdaDelegateStorageBase&&) = delete;
  WLambdaDelegateStorageBase& operator=(WLambdaDelegateStorageBase&&) = delete;
};

template <typename Function>
struct WLambdaDelegateStorage : public WLambdaDelegateStorageBase
{
  WLambdaDelegateStorage(Function&& func)
    : m_func(std::move(func))
  {
  }

private:
  template <typename = typename std::enable_if<std::is_copy_constructible<Function>::value>>
  WLambdaDelegateStorage(const Function& func)
    : m_func(func)
  {
  }

public:
  virtual WLambdaDelegateStorageBase* Clone(WAllocator* pAllocator) const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      return W_NEW(pAllocator, WLambdaDelegateStorage<Function>, m_func);
    }
    else
    {
      W_REPORT_FAILURE("The WDelegate stores a lambda that is not copyable. Copying this WDelegate is not supported.");
      return nullptr;
    }
  }

  virtual void InplaceCopy(WUInt8* pBuffer) const override
  {
    if constexpr (std::is_copy_constructible<Function>::value)
    {
      new (pBuffer) WLambdaDelegateStorage<Function>(m_func);
    }
    else
    {
      W_REPORT_FAILURE("The WDelegate stores a lambda that is not copyable. Copying this WDelegate is not supported.");
    }
  }

  virtual void InplaceMove(WUInt8* pBuffer) override
  {
    if constexpr (std::is_move_constructible<Function>::value)
    {
      new (pBuffer) WLambdaDelegateStorage<Function>(std::move(m_func));
    }
    else
    {
      W_REPORT_FAILURE("The WDelegate stores a lambda that is not movable. Moving this WDelegate is not supported.");
    }
  }

  Function m_func;
};


template <typename R, class... Args, WUInt32 DataSize, typename AllocatorWrapper>
struct WDelegate<R(Args...), DataSize, AllocatorWrapper> : public WDelegateBase
{
private:
  using SelfType = WDelegate<R(Args...), DataSize, AllocatorWrapper>;
  constexpr const void* HeapLambda() const { return reinterpret_cast<const void*>((size_t)-1); }
  constexpr const void* InplaceLambda() const { return reinterpret_cast<const void*>((size_t)-2); }

public:
  W_ALWAYS_INLINE WDelegate()
    : m_DispatchFunction(nullptr)
  {
  }

  W_ALWAYS_INLINE WDelegate(const SelfType& other) { *this = other; }

  W_ALWAYS_INLINE WDelegate(SelfType&& other) { *this = std::move(other); }

  /// Constructs the delegate from a member function type and takes the class instance on which to call the function later.
  template <typename Method, typename Class>
  W_FORCE_INLINE WDelegate(Method method, Class* pInstance)
  {
    CopyMemberFunctionToInplaceStorage(method);

    m_Instance.m_Ptr = pInstance;
    m_DispatchFunction = &DispatchToMethod<Method, Class>;
  }

  /// Constructs the delegate from a member function type and takes the (const) class instance on which to call the function later.
  template <typename Method, typename Class>
  W_FORCE_INLINE WDelegate(Method method, const Class* pInstance)
  {
    CopyMemberFunctionToInplaceStorage(method);

    m_Instance.m_ConstPtr = pInstance;
    m_DispatchFunction = &DispatchToConstMethod<Method, Class>;
  }

  /// Constructs the delegate from a regular C function type.
  template <typename Function>
  W_FORCE_INLINE WDelegate(Function function, WAllocator* pAllocator = AllocatorWrapper::GetAllocator())
  {
    static_assert(DataSize >= 16, "DataSize must be at least 16 bytes");

    // Pure function pointers or lambdas that can be cast into pure functions (no captures) can be
    // copied directly into the inplace storage of the delegate.
    // Lambdas with captures need to be wrapped into an WLambdaDelegateStorage object as they can
    // capture non-pod or non-memmoveable data. This wrapper can also be stored inplace if it is small enough,
    // otherwise it will be heap allocated with the specified allocator.
    constexpr size_t functionSize = sizeof(Function);
    using signature = R(Args...);
    if constexpr (functionSize <= DataSize && std::is_assignable<signature*&, Function>::value)
    {
      // Lambdas with no capture have a size of 1.
      // Lambdas with no capture actually have no data. Do not copy the 1 uninitialized byte.
      // Propper function pointers have a size of > 4 or 8 (depending on pointer size)
      if constexpr (functionSize > 1)
      {
        CopyFunctionToInplaceStorage(function);
      }
      else
      {
        memset(m_Data, 0, DataSize);
      }

      m_Instance.m_ConstPtr = nullptr;
      m_DispatchFunction = &DispatchToFunction<Function>;
    }
    else
    {
      constexpr size_t storageSize = sizeof(WLambdaDelegateStorage<Function>);
      if constexpr (storageSize <= DataSize)
      {
        m_Instance.m_ConstPtr = InplaceLambda();
        new (m_Data) WLambdaDelegateStorage<Function>(std::move(function));
        memset(m_Data + storageSize, 0, DataSize - storageSize);
        m_DispatchFunction = &DispatchToInplaceLambda<Function>;
      }
      else
      {
        m_Instance.m_ConstPtr = HeapLambda();
        m_pLambdaStorage = W_NEW(pAllocator, WLambdaDelegateStorage<Function>, std::move(function));
        m_pAllocator = pAllocator;
        memset(m_Data + 2 * sizeof(void*), 0, DataSize - 2 * sizeof(void*));
        m_DispatchFunction = &DispatchToHeapLambda<Function>;
      }
    }
  }

  W_ALWAYS_INLINE ~WDelegate() { Invalidate(); }

  /// Copies the data from another delegate.
  W_FORCE_INLINE void operator=(const SelfType& other)
  {
    Invalidate();

    if (other.IsHeapLambda())
    {
      m_pAllocator = other.m_pAllocator;
      m_pLambdaStorage = other.m_pLambdaStorage->Clone(m_pAllocator);
    }
    else if (other.IsInplaceLambda())
    {
      auto pOtherLambdaStorage = reinterpret_cast<WLambdaDelegateStorageBase*>(&other.m_Data);
      pOtherLambdaStorage->InplaceCopy(m_Data);
    }
    else
    {
      memcpy(m_Data, other.m_Data, DataSize);
    }

    m_Instance = other.m_Instance;
    m_DispatchFunction = other.m_DispatchFunction;
  }

  /// Moves the data from another delegate.
  W_FORCE_INLINE void operator=(SelfType&& other)
  {
    Invalidate();
    m_Instance = other.m_Instance;
    m_DispatchFunction = other.m_DispatchFunction;

    if (other.IsInplaceLambda())
    {
      auto pOtherLambdaStorage = reinterpret_cast<WLambdaDelegateStorageBase*>(&other.m_Data);
      pOtherLambdaStorage->InplaceMove(m_Data);
    }
    else
    {
      memcpy(m_Data, other.m_Data, DataSize);
    }

    other.m_Instance.m_Ptr = nullptr;
    other.m_DispatchFunction = nullptr;
    memset(other.m_Data, 0, DataSize);
  }

  /// Resets a delegate to an invalid state.
  W_FORCE_INLINE void operator=(std::nullptr_t) { Invalidate(); }

  /// Function call operator. This will call the function that is bound to the delegate, or assert if nothing was bound.
  W_FORCE_INLINE R operator()(Args... params) const
  {
    W_ASSERT_DEBUG(m_DispatchFunction != nullptr, "Delegate is not bound.");
    return (*m_DispatchFunction)(*this, params...);
  }

  /// This function only exists to make code compile, but it will assert when used. Use IsEqualIfNotHeapAllocated() instead.
  W_ALWAYS_INLINE bool operator==(const SelfType& other) const
  {
    W_REPORT_FAILURE("operator== for WDelegate must not be used. Use IsEqualIfNotHeapAllocated() and read its documentation!");
    return false;
  }

  /// Checks whether two delegates are bound to the exact same function, including the class instance.
  /// \note If \a this or \a other or both return false for IsComparable(), the function returns always false!
  /// Therefore, do not use this to search for delegates that are not comparable. WEvent uses this function, but goes to great lengths to
  /// assert that it is used correctly. It is best to not use this function at all.
  W_ALWAYS_INLINE bool IsEqualIfComparable(const SelfType& other) const
  {
    return m_Instance.m_Ptr == other.m_Instance.m_Ptr && m_DispatchFunction == other.m_DispatchFunction &&
           memcmp(m_Data, other.m_Data, DataSize) == 0;
  }

  /// Returns true when the delegate is bound to a valid non-nullptr function.
  W_ALWAYS_INLINE bool IsValid() const { return m_DispatchFunction != nullptr; }

  /// Resets a delegate to an invalid state.
  W_FORCE_INLINE void Invalidate()
  {
    m_DispatchFunction = nullptr;
    if (IsHeapLambda())
    {
      W_DELETE(m_pAllocator, m_pLambdaStorage);
    }
    else if (IsInplaceLambda())
    {
      auto pLambdaStorage = reinterpret_cast<WLambdaDelegateStorageBase*>(&m_Data);
      pLambdaStorage->~WLambdaDelegateStorageBase();
    }

    m_Instance.m_Ptr = nullptr;
    memset(m_Data, 0, DataSize);
  }

  /// Returns the class instance that is used to call a member function pointer on.
  W_ALWAYS_INLINE void* GetClassInstance() const { return IsComparable() ? m_Instance.m_Ptr : nullptr; }

  /// Returns whether the delegate is comparable with other delegates of the same type. This is not the case for i.e. lambdas with captures.
  W_ALWAYS_INLINE bool IsComparable() const { return m_Instance.m_ConstPtr < InplaceLambda(); } // [tested]

private:
  template <typename Function>
  W_FORCE_INLINE void CopyFunctionToInplaceStorage(Function function)
  {
    W_ASSERT_DEBUG(
      WMemoryUtils::IsAligned(&m_Data, alignof(Function)), "Wrong alignment. Expected {0} bytes alignment", alignof(Function));

    memcpy(m_Data, &function, sizeof(Function));
    memset(m_Data + sizeof(Function), 0, DataSize - sizeof(Function));
  }

  template <typename Method>
  W_FORCE_INLINE void CopyMemberFunctionToInplaceStorage(Method method)
  {
    static_assert(DataSize >= 16, "DataSize must be at least 16 bytes");
    static_assert(sizeof(Method) <= DataSize, "Member function pointer must not be bigger than 16 bytes");

    CopyFunctionToInplaceStorage(method);

    // Member Function Pointers in MSVC are 12 bytes in size and have 4 byte padding
    // MSVC builds a member function pointer on the stack writing only 12 bytes and then copies it
    // to the final location by copying 16 bytes. Thus the 4 byte padding get a random value (whatever is on the stack at that time).
    // To make the delegate comparable by memcmp we zero out those 4 byte padding.
    // Apparently clang does the same on windows but not on linux etc.
#if W_ENABLED(W_COMPILER_MSVC)
    *reinterpret_cast<WUInt32*>(m_Data + 12) = 0;
#endif
  }

  W_ALWAYS_INLINE bool IsInplaceLambda() const { return m_Instance.m_ConstPtr == InplaceLambda(); }
  W_ALWAYS_INLINE bool IsHeapLambda() const { return m_Instance.m_ConstPtr == HeapLambda(); }

  template <typename Method, typename Class>
  static W_FORCE_INLINE R DispatchToMethod(const SelfType& self, Args... params)
  {
    W_ASSERT_DEBUG(self.m_Instance.m_Ptr != nullptr, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<Class*>(self.m_Instance.m_Ptr)->*method)(params...);
  }

  template <typename Method, typename Class>
  static W_FORCE_INLINE R DispatchToConstMethod(const SelfType& self, Args... params)
  {
    W_ASSERT_DEBUG(self.m_Instance.m_ConstPtr != nullptr, "Instance must not be null.");
    Method method = *reinterpret_cast<Method*>(&self.m_Data);
    return (static_cast<const Class*>(self.m_Instance.m_ConstPtr)->*method)(params...);
  }

  template <typename Function>
  static W_ALWAYS_INLINE R DispatchToFunction(const SelfType& self, Args... params)
  {
    return (*reinterpret_cast<Function*>(&self.m_Data))(params...);
  }

  template <typename Function>
  static W_ALWAYS_INLINE R DispatchToHeapLambda(const SelfType& self, Args... params)
  {
    return static_cast<WLambdaDelegateStorage<Function>*>(self.m_pLambdaStorage)->m_func(params...);
  }

  template <typename Function>
  static W_ALWAYS_INLINE R DispatchToInplaceLambda(const SelfType& self, Args... params)
  {
    return reinterpret_cast<WLambdaDelegateStorage<Function>*>(&self.m_Data)->m_func(params...);
  }

  using DispatchFunction = R (*)(const SelfType&, Args...);
  DispatchFunction m_DispatchFunction;

  union
  {
    mutable WUInt8 m_Data[DataSize];
    struct
    {
      WLambdaDelegateStorageBase* m_pLambdaStorage;
      WAllocator* m_pAllocator;
    };
  };
};

template <typename T>
struct WMakeDelegateHelper
{
};

template <typename Class, typename R, typename... Args>
struct WMakeDelegateHelper<R (Class::*)(Args...)>
{
  using DelegateType = WDelegate<R(Args...)>;
};

template <typename Class, typename R, typename... Args>
struct WMakeDelegateHelper<R (Class::*)(Args...) const>
{
  using DelegateType = WDelegate<R(Args...)>;
};
