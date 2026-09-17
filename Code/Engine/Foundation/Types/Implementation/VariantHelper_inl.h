

// for some reason MSVC does not accept the template keyword here
#if W_ENABLED(W_COMPILER_MSVC_PURE) && (_MSC_VER < 1950)
#  define CALL_FUNCTOR(functor, type) return functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) return functor.template operator()<type>(std::forward<Args>(args)...)
#endif

template <typename Functor, class... Args>
auto WVariant::DispatchTo(Functor& ref_functor, Type::Enum type, Args&&... args)
{
  switch (type)
  {
    case Type::Bool:
      CALL_FUNCTOR(ref_functor, bool);
      break;

    case Type::Int8:
      CALL_FUNCTOR(ref_functor, WInt8);
      break;

    case Type::UInt8:
      CALL_FUNCTOR(ref_functor, WUInt8);
      break;

    case Type::Int16:
      CALL_FUNCTOR(ref_functor, WInt16);
      break;

    case Type::UInt16:
      CALL_FUNCTOR(ref_functor, WUInt16);
      break;

    case Type::Int32:
      CALL_FUNCTOR(ref_functor, WInt32);
      break;

    case Type::UInt32:
      CALL_FUNCTOR(ref_functor, WUInt32);
      break;

    case Type::Int64:
      CALL_FUNCTOR(ref_functor, WInt64);
      break;

    case Type::UInt64:
      CALL_FUNCTOR(ref_functor, WUInt64);
      break;

    case Type::Float:
      CALL_FUNCTOR(ref_functor, float);
      break;

    case Type::Double:
      CALL_FUNCTOR(ref_functor, double);
      break;

    case Type::Color:
      CALL_FUNCTOR(ref_functor, WColor);
      break;

    case Type::ColorGamma:
      CALL_FUNCTOR(ref_functor, WColorGammaUB);
      break;

    case Type::Vector2:
      CALL_FUNCTOR(ref_functor, WVec2);
      break;

    case Type::Vector3:
      CALL_FUNCTOR(ref_functor, WVec3);
      break;

    case Type::Vector4:
      CALL_FUNCTOR(ref_functor, WVec4);
      break;

    case Type::Vector2I:
      CALL_FUNCTOR(ref_functor, WVec2I32);
      break;

    case Type::Vector3I:
      CALL_FUNCTOR(ref_functor, WVec3I32);
      break;

    case Type::Vector4I:
      CALL_FUNCTOR(ref_functor, WVec4I32);
      break;

    case Type::Vector2U:
      CALL_FUNCTOR(ref_functor, WVec2U32);
      break;

    case Type::Vector3U:
      CALL_FUNCTOR(ref_functor, WVec3U32);
      break;

    case Type::Vector4U:
      CALL_FUNCTOR(ref_functor, WVec4U32);
      break;

    case Type::Quaternion:
      CALL_FUNCTOR(ref_functor, WQuat);
      break;

    case Type::Matrix3:
      CALL_FUNCTOR(ref_functor, WMat3);
      break;

    case Type::Matrix4:
      CALL_FUNCTOR(ref_functor, WMat4);
      break;

    case Type::Transform:
      CALL_FUNCTOR(ref_functor, WTransform);
      break;

    case Type::String:
      CALL_FUNCTOR(ref_functor, WString);
      break;

    case Type::StringView:
      CALL_FUNCTOR(ref_functor, WStringView);
      break;

    case Type::DataBuffer:
      CALL_FUNCTOR(ref_functor, WDataBuffer);
      break;

    case Type::Time:
      CALL_FUNCTOR(ref_functor, WTime);
      break;

    case Type::Uuid:
      CALL_FUNCTOR(ref_functor, WUuid);
      break;

    case Type::Angle:
      CALL_FUNCTOR(ref_functor, WAngle);
      break;

    case Type::HashedString:
      CALL_FUNCTOR(ref_functor, WHashedString);
      break;

    case Type::TempHashedString:
      CALL_FUNCTOR(ref_functor, WTempHashedString);
      break;

    case Type::VariantArray:
      CALL_FUNCTOR(ref_functor, WVariantArray);
      break;

    case Type::VariantDictionary:
      CALL_FUNCTOR(ref_functor, WVariantDictionary);
      break;

    case Type::TypedObject:
      CALL_FUNCTOR(ref_functor, WTypedObject);
      break;

    default:
      W_REPORT_FAILURE("Could not dispatch type '{0}'", type);
      // Intended fall through to disable warning.
    case Type::TypedPointer:
      CALL_FUNCTOR(ref_functor, WTypedPointer);
      break;
  }
}

#undef CALL_FUNCTOR

class WVariantHelper
{
  friend class WVariant;
  friend struct ConvertFunc;

  static void To(const WVariant& value, bool& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<WInt32>() != 0;
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      if (WConversionUtils::StringToBool(s, result) == W_FAILURE)
      {
        result = false;
        bSuccessful = false;
      }
    }
    else
      W_REPORT_FAILURE("Conversion to bool failed");
  }

  static void To(const WVariant& value, WInt8& result, bool& bSuccessful)
  {
    WInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (WInt8)tempResult;
  }

  static void To(const WVariant& value, WUInt8& result, bool& bSuccessful)
  {
    WUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (WUInt8)tempResult;
  }

  static void To(const WVariant& value, WInt16& result, bool& bSuccessful)
  {
    WInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (WInt16)tempResult;
  }

  static void To(const WVariant& value, WUInt16& result, bool& bSuccessful)
  {
    WUInt32 tempResult = 0;
    To(value, tempResult, bSuccessful);
    result = (WUInt16)tempResult;
  }

  static void To(const WVariant& value, WInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<WInt32>();
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      if (WConversionUtils::StringToInt(s, result) == W_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
    }
    else
      W_REPORT_FAILURE("Conversion to int failed");
  }

  static void To(const WVariant& value, WUInt32& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<WUInt32>();
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      WInt64 tmp = result;
      if (WConversionUtils::StringToInt64(s, tmp) == W_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
      else
        result = (WUInt32)tmp;
    }
    else
      W_REPORT_FAILURE("Conversion to uint failed");
  }

  static void To(const WVariant& value, WInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<WInt64>();
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      if (WConversionUtils::StringToInt64(s, result) == W_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
    }
    else
      W_REPORT_FAILURE("Conversion to int64 failed");
  }

  static void To(const WVariant& value, WUInt64& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<WUInt64>();
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      WInt64 tmp = result;
      if (WConversionUtils::StringToInt64(s, tmp) == W_FAILURE)
      {
        result = 0;
        bSuccessful = false;
      }
      else
        result = (WUInt64)tmp;
    }
    else
      W_REPORT_FAILURE("Conversion to uint64 failed");
  }

  static void To(const WVariant& value, float& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<float>();
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      double tmp = result;
      if (WConversionUtils::StringToFloat(s, tmp) == W_FAILURE)
      {
        result = 0.0f;
        bSuccessful = false;
      }
      else
        result = (float)tmp;
    }
    else
      W_REPORT_FAILURE("Conversion to float failed");
  }

  static void To(const WVariant& value, double& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() <= WVariant::Type::Double)
      result = value.ConvertNumber<double>();
    else if (value.GetType() == WVariant::Type::String || value.GetType() == WVariant::Type::HashedString)
    {
      WStringView s = value.IsA<WString>() ? value.Cast<WString>().GetView() : value.Cast<WHashedString>().GetView();
      if (WConversionUtils::StringToFloat(s, result) == W_FAILURE)
      {
        result = 0.0;
        bSuccessful = false;
      }
    }
    else
      W_REPORT_FAILURE("Conversion to double failed");
  }

  static void To(const WVariant& value, WString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsValid() == false)
    {
      result = "<Invalid>";
      return;
    }

    ToStringFunc toStringFunc;
    toStringFunc.m_pThis = &value;
    toStringFunc.m_pResult = &result;

    WVariant::DispatchTo(toStringFunc, value.GetType());
  }

  static void To(const WVariant& value, WStringView& result, bool& bSuccessful)
  {
    bSuccessful = true;

    result = value.IsA<WString>() ? value.Get<WString>().GetView() : value.Get<WHashedString>().GetView();
  }

  static void To(const WVariant& value, WTypedPointer& result, bool& bSuccessful)
  {
    bSuccessful = true;
    W_ASSERT_DEBUG(value.GetType() == WVariant::Type::TypedPointer, "Only ptr can be converted to void*!");
    result = value.Cast<WTypedPointer>();
  }

  static void To(const WVariant& value, WColor& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == WVariant::Type::ColorGamma)
      result = value.Cast<WColorGammaUB>();
    else
      W_REPORT_FAILURE("Conversion to WColor failed");
  }

  static void To(const WVariant& value, WColorGammaUB& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == WVariant::Type::Color)
      result = value.Cast<WColor>();
    else
      W_REPORT_FAILURE("Conversion to WColorGammaUB failed");
  }

  template <typename T, typename V1, typename V2>
  static void ToVec2X(const WVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsNumber())
    {
      auto x = value.ConvertNumber<typename T::ComponentType>();
      result = T(x);
    }
    else if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y));
    }
    else
    {
      W_REPORT_FAILURE("Conversion to WVec2X failed");
      bSuccessful = false;
    }
  }

  static void To(const WVariant& value, WVec2& result, bool& bSuccessful) { ToVec2X<WVec2, WVec2I32, WVec2U32>(value, result, bSuccessful); }

  static void To(const WVariant& value, WVec2I32& result, bool& bSuccessful) { ToVec2X<WVec2I32, WVec2, WVec2U32>(value, result, bSuccessful); }

  static void To(const WVariant& value, WVec2U32& result, bool& bSuccessful) { ToVec2X<WVec2U32, WVec2I32, WVec2>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec3X(const WVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsNumber())
    {
      auto x = value.ConvertNumber<typename T::ComponentType>();
      result = T(x);
    }
    else if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z));
    }
    else
    {
      W_REPORT_FAILURE("Conversion to WVec3X failed");
      bSuccessful = false;
    }
  }

  static void To(const WVariant& value, WVec3& result, bool& bSuccessful) { ToVec3X<WVec3, WVec3I32, WVec3U32>(value, result, bSuccessful); }

  static void To(const WVariant& value, WVec3I32& result, bool& bSuccessful) { ToVec3X<WVec3I32, WVec3, WVec3U32>(value, result, bSuccessful); }

  static void To(const WVariant& value, WVec3U32& result, bool& bSuccessful) { ToVec3X<WVec3U32, WVec3I32, WVec3>(value, result, bSuccessful); }

  template <typename T, typename V1, typename V2>
  static void ToVec4X(const WVariant& value, T& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.IsNumber())
    {
      auto x = value.ConvertNumber<typename T::ComponentType>();
      result = T(x);
    }
    else if (value.IsA<V1>())
    {
      const V1& v = value.Cast<V1>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else if (value.IsA<V2>())
    {
      const V2& v = value.Cast<V2>();
      result = T(static_cast<typename T::ComponentType>(v.x), static_cast<typename T::ComponentType>(v.y), static_cast<typename T::ComponentType>(v.z), static_cast<typename T::ComponentType>(v.w));
    }
    else
    {
      W_REPORT_FAILURE("Conversion to WVec4X failed");
      bSuccessful = false;
    }
  }

  static void To(const WVariant& value, WVec4& result, bool& bSuccessful) { ToVec4X<WVec4, WVec4I32, WVec4U32>(value, result, bSuccessful); }

  static void To(const WVariant& value, WVec4I32& result, bool& bSuccessful) { ToVec4X<WVec4I32, WVec4, WVec4U32>(value, result, bSuccessful); }

  static void To(const WVariant& value, WVec4U32& result, bool& bSuccessful) { ToVec4X<WVec4U32, WVec4I32, WVec4>(value, result, bSuccessful); }

  static void To(const WVariant& value, WHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == WVariantType::String)
      result.Assign(value.Cast<WString>());
    else if (value.GetType() == WVariantType::StringView)
      result.Assign(value.Cast<WStringView>());
    else
    {
      WString s;
      To(value, s, bSuccessful);
      result.Assign(s.GetView());
    }
  }

  static void To(const WVariant& value, WTempHashedString& result, bool& bSuccessful)
  {
    bSuccessful = true;

    if (value.GetType() == WVariantType::String)
      result = value.Cast<WString>();
    else if (value.GetType() == WVariantType::StringView)
      result = value.Cast<WStringView>();
    else if (value.GetType() == WVariant::Type::HashedString)
      result = value.Cast<WHashedString>();
    else
    {
      WString s;
      To(value, s, bSuccessful);
      result = s.GetView();
    }
  }

  template <typename T>
  static void To(const WVariant& value, T& result, bool& bSuccessful)
  {
    W_IGNORE_UNUSED(value);
    W_IGNORE_UNUSED(result);
    W_REPORT_FAILURE("Conversion function not implemented for target type '{0}'", WVariant::TypeDeduction<T>::value);
    bSuccessful = false;
  }

  struct ToStringFunc
  {
    template <typename T>
    W_ALWAYS_INLINE void operator()()
    {
      WStringBuilder tmp;
      *m_pResult = WConversionUtils::ToString(m_pThis->Cast<T>(), tmp); // NOLINT (clang-analyzer-core.CallAndMessage)
    }

    const WVariant* m_pThis;
    WString* m_pResult;
  };
};
