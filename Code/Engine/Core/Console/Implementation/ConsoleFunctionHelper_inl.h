// NO #pragma once in this file !

template <typename R W_COMMA_IF(ARG_COUNT) W_LIST(typename P, ARG_COUNT)>
class WConsoleFunction<R(W_LIST(P, ARG_COUNT))> : public WConsoleFunctionBase
{
public:
  using FUNC = WDelegate<R(W_LIST(P, ARG_COUNT))>;

  FUNC m_Func;

  WConsoleFunction(WStringView sFunctionName, WStringView sDescription, FUNC f)
    : WConsoleFunctionBase(sFunctionName, sDescription)
  {
    m_Func = f;
  }

  WUInt32 GetNumParameters() const override { return ARG_COUNT; }

  virtual WVariant::Type::Enum GetParameterType(WUInt32 uiParam) const override
  {
    W_ASSERT_DEV(uiParam < GetNumParameters(), "Invalid Parameter Index {0}", uiParam);
    W_IGNORE_UNUSED(uiParam);

#if (ARG_COUNT > 0)

    switch (uiParam)
    {
      case 0:
        return static_cast<WVariant::Type::Enum>(WVariant::TypeDeduction<P0>::value);

#  if (ARG_COUNT > 1)
      case 1:
        return static_cast<WVariant::Type::Enum>(WVariant::TypeDeduction<P1>::value);
#  endif
#  if (ARG_COUNT > 2)
      case 2:
        return static_cast<WVariant::Type::Enum>(WVariant::TypeDeduction<P2>::value);
#  endif
#  if (ARG_COUNT > 3)
      case 3:
        return static_cast<WVariant::Type::Enum>(WVariant::TypeDeduction<P3>::value);
#  endif
#  if (ARG_COUNT > 4)
      case 4:
        return static_cast<WVariant::Type::Enum>(WVariant::TypeDeduction<P4>::value);
#  endif
#  if (ARG_COUNT > 5)
      case 5:
        return static_cast<WVariant::Type::Enum>(WVariant::TypeDeduction<P5>::value);
#  endif
    }

#endif
    return WVariant::Type::Invalid;
  }

  virtual WResult Call(WArrayPtr<WVariant> params) override
  {
    W_IGNORE_UNUSED(params);

    WResult r = W_FAILURE;
    W_IGNORE_UNUSED(r);

#if (ARG_COUNT > 0)
    P0 param0 = params[0].ConvertTo<P0>(&r);

    if (r.Failed())
      return W_FAILURE;
#endif

#if (ARG_COUNT > 1)
    P1 param1 = params[1].ConvertTo<P1>(&r);

    if (r.Failed())
      return W_FAILURE;
#endif

#if (ARG_COUNT > 2)
    P2 param2 = params[2].ConvertTo<P2>(&r);

    if (r.Failed())
      return W_FAILURE;
#endif

#if (ARG_COUNT > 3)
    P3 param3 = params[3].ConvertTo<P3>(&r);

    if (r.Failed())
      return W_FAILURE;
#endif

#if (ARG_COUNT > 4)
    P4 param4 = params[4].ConvertTo<P4>(&r);

    if (r.Failed())
      return W_FAILURE;
#endif

#if (ARG_COUNT > 5)
    P5 param5 = params[5].ConvertTo<P5>(&r);

    if (r.Failed())
      return W_FAILURE;
#endif

    m_Func(W_LIST(param, ARG_COUNT));
    return W_SUCCESS;
  }
};
