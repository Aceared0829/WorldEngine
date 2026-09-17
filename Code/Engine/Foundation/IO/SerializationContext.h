
#pragma once

/// Base class for serialization contexts. A serialization context can be used to add high level logic to serialization, e.g.
/// de-duplicating objects.
///
/// Typically a context is created before any serialization happens and can then be accessed anywhere through the GetContext method.
template <typename Derived>
class WSerializationContext
{
  W_DISALLOW_COPY_AND_ASSIGN(WSerializationContext);

public:
  WSerializationContext() { Derived::SetContext(this); }
  ~WSerializationContext() { Derived::SetContext(nullptr); }

  /// Set the context as active which means it can be accessed via GetContext in serialization methods.
  ///
  /// It can be useful to manually set a context as active if a serialization process is spread across multiple scopes
  /// and other serialization can happen in between.
  void SetActive(bool bActive) { Derived::SetContext(bActive ? this : nullptr); }
};

/// Declares the necessary functions to access a serialization context
#define W_DECLARE_SERIALIZATION_CONTEXT(type) \
public:                                        \
  static type* GetContext();                   \
                                               \
protected:                                     \
  friend class WSerializationContext<type>;   \
  static void SetContext(WSerializationContext* pContext);


/// Implements the necessary functions to access a serialization context through GetContext.
#define W_IMPLEMENT_SERIALIZATION_CONTEXT(type)                                                                                        \
  thread_local type* W_PP_CONCAT(s_pActiveContext, type);                                                                              \
  type* type::GetContext()                                                                                                              \
  {                                                                                                                                     \
    return W_PP_CONCAT(s_pActiveContext, type);                                                                                        \
  }                                                                                                                                     \
  void type::SetContext(WSerializationContext* pContext)                                                                               \
  {                                                                                                                                     \
    W_ASSERT_DEV(pContext == nullptr || W_PP_CONCAT(s_pActiveContext, type) == nullptr, "Only one context can be active at a time."); \
    W_PP_CONCAT(s_pActiveContext, type) = static_cast<type*>(pContext);                                                                \
  }
