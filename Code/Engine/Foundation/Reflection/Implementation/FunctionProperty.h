#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/VariantAdapter.h>


template <class R, class... Args>
class WTypedFunctionProperty : public WAbstractFunctionProperty
{
public:
  WTypedFunctionProperty(const char* szPropertyName)
    : WAbstractFunctionProperty(szPropertyName)
  {
  }

  virtual const WRTTI* GetReturnType() const override { return WGetStaticRTTI<typename WCleanType<R>::RttiType>(); }
  virtual WBitflags<WPropertyFlags> GetReturnFlags() const override { return WPropertyFlags::GetParameterFlags<R>(); }

  virtual WUInt32 GetArgumentCount() const override { return sizeof...(Args); }

  template <std::size_t... I>
  const WRTTI* GetParameterTypeImpl(WUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static const WRTTI* params[] = {WGetStaticRTTI<typename WCleanType<typename getArgument<I, Args...>::Type>::RttiType>()..., nullptr};
    return params[uiParamIndex];
  }

  virtual const WRTTI* GetArgumentType(WUInt32 uiParamIndex) const override
  {
    return GetParameterTypeImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }

  template <std::size_t... I>
  WBitflags<WPropertyFlags> GetParameterFlagsImpl(WUInt32 uiParamIndex, std::index_sequence<I...>) const
  {
    // There is a dummy entry at the end to support zero parameter functions (can't have zero-size arrays).
    static WBitflags<WPropertyFlags> params[] = {
      WPropertyFlags::GetParameterFlags<typename getArgument<I, Args...>::Type>()..., WPropertyFlags::Void};
    return params[uiParamIndex];
  }

  virtual WBitflags<WPropertyFlags> GetArgumentFlags(WUInt32 uiParamIndex) const override
  {
    return GetParameterFlagsImpl(uiParamIndex, std::make_index_sequence<sizeof...(Args)>{});
  }
};

template <typename FUNC>
class WFunctionProperty
{
};

template <class CLASS, class R, class... Args>
class WFunctionProperty<R (CLASS::*)(Args...)> : public WTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (CLASS::*)(Args...);

  WFunctionProperty(const char* szPropertyName, TargetFunction func)
    : WTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual WFunctionType::Enum GetFunctionType() const override
  {
    return WFunctionType::Member;
  }

  template <std::size_t... I>
  W_FORCE_INLINE void ExecuteImpl(void* pInstance, WVariant& out_returnValue, WArrayPtr<WVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pTargetInstance = static_cast<CLASS*>(pInstance);
    if constexpr (std::is_same<R, void>::value)
    {
      (pTargetInstance->*m_Function)(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
      out_returnValue = WVariant();
    }
    else
    {
      WVariantAssignmentAdapter<R> returnWrapper(out_returnValue);
      returnWrapper = (pTargetInstance->*m_Function)(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    }
  }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override
  {
    ExecuteImpl(pInstance, out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};

template <class CLASS, class R, class... Args>
class WFunctionProperty<R (CLASS::*)(Args...) const> : public WTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (CLASS::*)(Args...) const;

  WFunctionProperty(const char* szPropertyName, TargetFunction func)
    : WTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
    this->AddFlags(WPropertyFlags::Const);
  }

  virtual WFunctionType::Enum GetFunctionType() const override
  {
    return WFunctionType::Member;
  }

  template <std::size_t... I>
  W_FORCE_INLINE void ExecuteImpl(const void* pInstance, WVariant& out_returnValue, WArrayPtr<WVariant> arguments, std::index_sequence<I...>) const
  {
    const CLASS* pTargetInstance = static_cast<const CLASS*>(pInstance);
    if constexpr (std::is_same<R, void>::value)
    {
      (pTargetInstance->*m_Function)(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
      out_returnValue = WVariant();
    }
    else
    {
      WVariantAssignmentAdapter<R> returnWrapper(out_returnValue);
      returnWrapper = (pTargetInstance->*m_Function)(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    }
  }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override
  {
    ExecuteImpl(pInstance, out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};

template <class R, class... Args>
class WFunctionProperty<R (*)(Args...)> : public WTypedFunctionProperty<R, Args...>
{
public:
  using TargetFunction = R (*)(Args...);

  WFunctionProperty(const char* szPropertyName, TargetFunction func)
    : WTypedFunctionProperty<R, Args...>(szPropertyName)
  {
    m_Function = func;
  }

  virtual WFunctionType::Enum GetFunctionType() const override { return WFunctionType::StaticMember; }

  template <std::size_t... I>
  void ExecuteImpl(WTraitInt<1>, WVariant& out_returnValue, WArrayPtr<WVariant> arguments, std::index_sequence<I...>) const
  {
    (*m_Function)(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    out_returnValue = WVariant();
  }

  template <std::size_t... I>
  void ExecuteImpl(WTraitInt<0>, WVariant& out_returnValue, WArrayPtr<WVariant> arguments, std::index_sequence<I...>) const
  {
    WVariantAssignmentAdapter<R> returnWrapper(out_returnValue);
    returnWrapper = (*m_Function)(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
  }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override
  {
    W_IGNORE_UNUSED(pInstance);
    ExecuteImpl(WTraitInt<std::is_same<R, void>::value>(), out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }

private:
  TargetFunction m_Function;
};


template <class CLASS, class... Args>
class WConstructorFunctionProperty : public WTypedFunctionProperty<CLASS*, Args...>
{
public:
  WConstructorFunctionProperty()
    : WTypedFunctionProperty<CLASS*, Args...>("Constructor")
  {
  }

  virtual WFunctionType::Enum GetFunctionType() const override { return WFunctionType::Constructor; }

  template <std::size_t... I>
  void ExecuteImpl(WTraitInt<1>, WVariant& out_returnValue, WArrayPtr<WVariant> arguments, std::index_sequence<I...>) const
  {
    out_returnValue = CLASS(WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // returnValue = CLASS(static_cast<typename getArgument<I, Args...>::Type>(WVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
  }

  template <std::size_t... I>
  void ExecuteImpl(WTraitInt<0>, WVariant& out_returnValue, WArrayPtr<WVariant> arguments, std::index_sequence<I...>) const
  {
    CLASS* pInstance = W_DEFAULT_NEW(CLASS, WVariantAdapter<typename getArgument<I, Args...>::Type>(arguments[I])...);
    // CLASS* pInstance = W_DEFAULT_NEW(CLASS, static_cast<typename getArgument<I, Args...>::Type>(WVariantAdapter<typename getArgument<I,
    // Args...>::Type>(arguments[I]))...);
    out_returnValue = pInstance;
  }

  virtual void Execute(void* pInstance, WArrayPtr<WVariant> arguments, WVariant& out_returnValue) const override
  {
    W_IGNORE_UNUSED(pInstance);
    ExecuteImpl(WTraitInt<WIsStandardType<CLASS>::value>(), out_returnValue, arguments, std::make_index_sequence<sizeof...(Args)>{});
  }
};
