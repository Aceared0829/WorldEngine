#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>
#include <Foundation/Types/Variant.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

// this file takes ages to compile in a Release build
// since we don't care for runtime performance, just disable all optimizations
#pragma optimize("", off)

class Blubb : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(Blubb, WReflectedClass);

public:
  float u;
  float v;
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(Blubb, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("u", u),
    W_MEMBER_PROPERTY("v", v),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

template <typename T>
void TestVariant(WVariant& v, WVariantType::Enum type)
{
  W_TEST_BOOL(v.IsValid());
  W_TEST_BOOL(v.GetType() == type);
  W_TEST_BOOL(v.CanConvertTo<T>());
  W_TEST_BOOL(v.IsA<T>());
  W_TEST_BOOL(v.GetReflectedType() == WGetStaticRTTI<T>());

  WTypedPointer ptr = v.GetWriteAccess();
  W_TEST_BOOL(ptr.m_pObject == &v.Get<T>());
  W_TEST_BOOL(ptr.m_pObject == &v.GetWritable<T>());
  W_TEST_BOOL(ptr.m_pType == WGetStaticRTTI<T>());

  W_TEST_BOOL(ptr.m_pObject == v.GetData());

  WVariant vCopy = v;
  WTypedPointer ptr2 = vCopy.GetWriteAccess();
  W_TEST_BOOL(ptr2.m_pObject == &vCopy.Get<T>());
  W_TEST_BOOL(ptr2.m_pObject == &vCopy.GetWritable<T>());

  W_TEST_BOOL(ptr2.m_pObject != ptr.m_pObject);
  W_TEST_BOOL(ptr2.m_pType == WGetStaticRTTI<T>());

  W_TEST_BOOL(v.Get<T>() == vCopy.Get<T>());

  W_TEST_BOOL(v.ComputeHash(0) != 0);
}

template <typename T>
inline void TestIntegerVariant(WVariant::Type::Enum type)
{
  WVariant b((T)23);
  TestVariant<T>(b, type);

  W_TEST_BOOL(b.Get<T>() == 23);

  W_TEST_BOOL(b == WVariant(23));
  W_TEST_BOOL(b != WVariant(11));
  W_TEST_BOOL(b == WVariant((T)23));
  W_TEST_BOOL(b != WVariant((T)11));

  W_TEST_BOOL(b == 23);
  W_TEST_BOOL(b != 24);
  W_TEST_BOOL(b == (T)23);
  W_TEST_BOOL(b != (T)24);

  b = (T)17;
  W_TEST_BOOL(b == (T)17);

  b = WVariant((T)19);
  W_TEST_BOOL(b == (T)19);

  W_TEST_BOOL(b.IsNumber());
  W_TEST_BOOL(b.IsFloatingPoint() == false);
  W_TEST_BOOL(!b.IsString());
}

inline void TestNumberCanConvertTo(const WVariant& v)
{
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Invalid) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Bool));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int8));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt8));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int16));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt16));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int32));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt32));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int64));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt64));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Float));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Double));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Color) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2I));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3I));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4I));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Quaternion) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix3) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix4) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Transform) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::String));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::StringView) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::DataBuffer) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Time) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Uuid) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Angle) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::ColorGamma) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::HashedString));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TempHashedString));
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantArray) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantDictionary) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedPointer) == false);
  W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedObject) == false);

  WResult conversionResult = W_FAILURE;
  W_TEST_BOOL(v.ConvertTo<bool>(&conversionResult) == true);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WInt8>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WUInt8>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WInt16>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WUInt16>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WInt32>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WUInt32>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WInt64>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WUInt64>(&conversionResult) == 3);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<float>(&conversionResult) == 3.0f);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<double>(&conversionResult) == 3.0);
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WVec2>(&conversionResult) == WVec2(3));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WVec3>(&conversionResult) == WVec3(3));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WVec4>(&conversionResult) == WVec4(3));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WVec2I32>(&conversionResult) == WVec2I32(3));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WVec3I32>(&conversionResult) == WVec3I32(3));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WVec4I32>(&conversionResult) == WVec4I32(3));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WString>(&conversionResult) == "3");
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WHashedString>(&conversionResult) == WMakeHashedString("3"));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo<WTempHashedString>(&conversionResult) == WTempHashedString("3"));
  W_TEST_BOOL(conversionResult.Succeeded());

  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Bool).Get<bool>() == true);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int8).Get<WInt8>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt8).Get<WUInt8>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int16).Get<WInt16>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt16).Get<WUInt16>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int32).Get<WInt32>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt32).Get<WUInt32>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int64).Get<WInt64>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt64).Get<WUInt64>() == 3);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Float).Get<float>() == 3.0f);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Double).Get<double>() == 3.0);
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2) == WVec2(3));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3) == WVec3(3));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4) == WVec4(3));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2I) == WVec2I32(3));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3I) == WVec3I32(3));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4I) == WVec4I32(3));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "3");
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("3"));
  W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("3"));
}

inline void TestCanOnlyConvertToID(const WVariant& v, WVariant::Type::Enum type)
{
  for (int iType = WVariant::Type::FirstStandardType; iType < WVariant::Type::LastExtendedType; ++iType)
  {
    if (iType == WVariant::Type::LastStandardType)
      iType = WVariant::Type::FirstExtendedType;

    if (iType == type)
    {
      W_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      W_TEST_BOOL(v.CanConvertTo((WVariant::Type::Enum)iType) == false);
    }
  }
}

inline void TestCanOnlyConvertToStringAndID(const WVariant& v, WVariant::Type::Enum type, WVariant::Type::Enum type2 = WVariant::Type::Invalid,
  WVariant::Type::Enum type3 = WVariant::Type::Invalid)
{
  if (type2 == WVariant::Type::Invalid)
    type2 = type;

  for (int iType = WVariant::Type::FirstStandardType; iType < WVariant::Type::LastExtendedType; ++iType)
  {
    if (iType == WVariant::Type::LastStandardType)
      iType = WVariant::Type::FirstExtendedType;

    if (iType == WVariant::Type::String || iType == WVariant::Type::HashedString || iType == WVariant::Type::TempHashedString)
    {
      W_TEST_BOOL(v.CanConvertTo(WVariant::Type::String));
    }
    else if (iType == type || iType == type2 || iType == type3)
    {
      W_TEST_BOOL(v.CanConvertTo(type));
    }
    else
    {
      W_TEST_BOOL(v.CanConvertTo((WVariant::Type::Enum)iType) == false);
    }
  }
}

W_CREATE_SIMPLE_TEST(Basics, Variant)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Invalid")
  {
    WVariant b;
    W_TEST_BOOL(b.GetType() == WVariant::Type::Invalid);
    W_TEST_BOOL(b == WVariant());
    W_TEST_BOOL(b != WVariant(0));
    W_TEST_BOOL(!b.IsValid());
    W_TEST_BOOL(!b[0].IsValid());
    W_TEST_BOOL(!b["x"].IsValid());
    W_TEST_BOOL(b.GetReflectedType() == nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "bool")
  {
    WVariant b(true);
    TestVariant<bool>(b, WVariantType::Bool);

    W_TEST_BOOL(b.Get<bool>() == true);

    W_TEST_BOOL(b == WVariant(true));
    W_TEST_BOOL(b != WVariant(false));

    W_TEST_BOOL(b == true);
    W_TEST_BOOL(b != false);

    b = false;
    W_TEST_BOOL(b == false);

    b = WVariant(true);
    W_TEST_BOOL(b == true);
    W_TEST_BOOL(!b[0].IsValid());

    W_TEST_BOOL(b.IsNumber());
    W_TEST_BOOL(!b.IsString());
    W_TEST_BOOL(b.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WInt8")
  {
    TestIntegerVariant<WInt8>(WVariant::Type::Int8);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WUInt8")
  {
    TestIntegerVariant<WUInt8>(WVariant::Type::UInt8);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WInt16")
  {
    TestIntegerVariant<WInt16>(WVariant::Type::Int16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WUInt16")
  {
    TestIntegerVariant<WUInt16>(WVariant::Type::UInt16);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WInt32")
  {
    TestIntegerVariant<WInt32>(WVariant::Type::Int32);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WUInt32")
  {
    TestIntegerVariant<WUInt32>(WVariant::Type::UInt32);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WInt64")
  {
    TestIntegerVariant<WInt64>(WVariant::Type::Int64);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WUInt64")
  {
    TestIntegerVariant<WUInt64>(WVariant::Type::UInt64);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "float")
  {
    WVariant b(42.0f);
    TestVariant<float>(b, WVariantType::Float);

    W_TEST_BOOL(b.Get<float>() == 42.0f);

    W_TEST_BOOL(b == WVariant(42));
    W_TEST_BOOL(b != WVariant(11));
    W_TEST_BOOL(b == WVariant(42.0));
    W_TEST_BOOL(b != WVariant(11.0));
    W_TEST_BOOL(b == WVariant(42.0f));
    W_TEST_BOOL(b != WVariant(11.0f));

    W_TEST_BOOL(b == 42);
    W_TEST_BOOL(b != 41);
    W_TEST_BOOL(b == 42.0);
    W_TEST_BOOL(b != 41.0);
    W_TEST_BOOL(b == 42.0f);
    W_TEST_BOOL(b != 41.0f);

    b = 17.0f;
    W_TEST_BOOL(b == 17.0f);

    b = WVariant(19.0f);
    W_TEST_BOOL(b == 19.0f);

    W_TEST_BOOL(b.IsNumber());
    W_TEST_BOOL(!b.IsString());
    W_TEST_BOOL(b.IsFloatingPoint());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "double")
  {
    WVariant b(42.0);
    TestVariant<double>(b, WVariantType::Double);
    W_TEST_BOOL(b.Get<double>() == 42.0);

    W_TEST_BOOL(b == WVariant(42));
    W_TEST_BOOL(b != WVariant(11));
    W_TEST_BOOL(b == WVariant(42.0));
    W_TEST_BOOL(b != WVariant(11.0));
    W_TEST_BOOL(b == WVariant(42.0f));
    W_TEST_BOOL(b != WVariant(11.0f));

    W_TEST_BOOL(b == 42);
    W_TEST_BOOL(b != 41);
    W_TEST_BOOL(b == 42.0);
    W_TEST_BOOL(b != 41.0);
    W_TEST_BOOL(b == 42.0f);
    W_TEST_BOOL(b != 41.0f);

    b = 17.0;
    W_TEST_BOOL(b == 17.0);

    b = WVariant(19.0);
    W_TEST_BOOL(b == 19.0);

    W_TEST_BOOL(b.IsNumber());
    W_TEST_BOOL(!b.IsString());
    W_TEST_BOOL(b.IsFloatingPoint());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WColor")
  {
    WVariant v(WColor(1, 2, 3, 1));
    TestVariant<WColor>(v, WVariantType::Color);

    W_TEST_BOOL(v.CanConvertTo<WColorGammaUB>());
    W_TEST_BOOL(v.ConvertTo<WColorGammaUB>() == static_cast<WColorGammaUB>(WColor(1, 2, 3, 1)));
    W_TEST_BOOL(v.Get<WColor>() == WColor(1, 2, 3, 1));

    W_TEST_BOOL(v == WVariant(WColor(1, 2, 3)));
    W_TEST_BOOL(v != WVariant(WColor(1, 1, 1)));

    W_TEST_BOOL(v == WColor(1, 2, 3));
    W_TEST_BOOL(v != WColor(1, 4, 3));

    v = WColor(5, 8, 9);
    W_TEST_BOOL(v == WColor(5, 8, 9));

    v = WVariant(WColor(7, 9, 4));
    W_TEST_BOOL(v == WColor(7, 9, 4));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v[2] == 4);
    W_TEST_BOOL(v[3] == 1);
    W_TEST_BOOL(v[4] == WVariant());
    W_TEST_BOOL(!v[4].IsValid());
    W_TEST_BOOL(v["r"] == 7);
    W_TEST_BOOL(v["g"] == 9);
    W_TEST_BOOL(v["b"] == 4);
    W_TEST_BOOL(v["a"] == 1);
    W_TEST_BOOL(v["x"] == WVariant());
    W_TEST_BOOL(!v["x"].IsValid());

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WColorGammaUB")
  {
    WVariant v(WColorGammaUB(64, 128, 255, 255));
    TestVariant<WColorGammaUB>(v, WVariantType::ColorGamma);

    W_TEST_BOOL(v.CanConvertTo<WColor>());
    W_TEST_BOOL(v.Get<WColorGammaUB>() == WColorGammaUB(64, 128, 255, 255));

    W_TEST_BOOL(v == WVariant(WColorGammaUB(64, 128, 255, 255)));
    W_TEST_BOOL(v != WVariant(WColorGammaUB(255, 128, 255, 255)));

    W_TEST_BOOL(v == WColorGammaUB(64, 128, 255, 255));
    W_TEST_BOOL(v != WColorGammaUB(64, 42, 255, 255));

    v = WColorGammaUB(10, 50, 200);
    W_TEST_BOOL(v == WColorGammaUB(10, 50, 200));

    v = WVariant(WColorGammaUB(17, 120, 200));
    W_TEST_BOOL(v == WColorGammaUB(17, 120, 200));
    W_TEST_BOOL(v[0] == 17);
    W_TEST_BOOL(v[1] == 120);
    W_TEST_BOOL(v[2] == 200);
    W_TEST_BOOL(v[3] == 255);
    W_TEST_BOOL(v[4] == WVariant());
    W_TEST_BOOL(!v[4].IsValid());
    W_TEST_BOOL(v["r"] == 17);
    W_TEST_BOOL(v["g"] == 120);
    W_TEST_BOOL(v["b"] == 200);
    W_TEST_BOOL(v["a"] == 255);
    W_TEST_BOOL(v["x"] == WVariant());
    W_TEST_BOOL(!v["x"].IsValid());

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVec2")
  {
    WVariant v(WVec2(1, 2));
    TestVariant<WVec2>(v, WVariantType::Vector2);

    W_TEST_BOOL(v.Get<WVec2>() == WVec2(1, 2));

    W_TEST_BOOL(v == WVariant(WVec2(1, 2)));
    W_TEST_BOOL(v != WVariant(WVec2(1, 1)));

    W_TEST_BOOL(v == WVec2(1, 2));
    W_TEST_BOOL(v != WVec2(1, 4));

    v = WVec2(5, 8);
    W_TEST_BOOL(v == WVec2(5, 8));

    v = WVariant(WVec2(7, 9));
    W_TEST_BOOL(v == WVec2(7, 9));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVec3")
  {
    WVariant v(WVec3(1, 2, 3));
    TestVariant<WVec3>(v, WVariantType::Vector3);

    W_TEST_BOOL(v.Get<WVec3>() == WVec3(1, 2, 3));

    W_TEST_BOOL(v == WVariant(WVec3(1, 2, 3)));
    W_TEST_BOOL(v != WVariant(WVec3(1, 1, 3)));

    W_TEST_BOOL(v == WVec3(1, 2, 3));
    W_TEST_BOOL(v != WVec3(1, 4, 3));

    v = WVec3(5, 8, 9);
    W_TEST_BOOL(v == WVec3(5, 8, 9));

    v = WVariant(WVec3(7, 9, 8));
    W_TEST_BOOL(v == WVec3(7, 9, 8));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v[2] == 8);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);
    W_TEST_BOOL(v["z"] == 8);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVec4")
  {
    WVariant v(WVec4(1, 2, 3, 4));
    TestVariant<WVec4>(v, WVariantType::Vector4);

    W_TEST_BOOL(v.Get<WVec4>() == WVec4(1, 2, 3, 4));

    W_TEST_BOOL(v == WVariant(WVec4(1, 2, 3, 4)));
    W_TEST_BOOL(v != WVariant(WVec4(1, 1, 3, 4)));

    W_TEST_BOOL(v == WVec4(1, 2, 3, 4));
    W_TEST_BOOL(v != WVec4(1, 4, 3, 4));

    v = WVec4(5, 8, 9, 3);
    W_TEST_BOOL(v == WVec4(5, 8, 9, 3));

    v = WVariant(WVec4(7, 9, 8, 4));
    W_TEST_BOOL(v == WVec4(7, 9, 8, 4));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v[2] == 8);
    W_TEST_BOOL(v[3] == 4);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);
    W_TEST_BOOL(v["z"] == 8);
    W_TEST_BOOL(v["w"] == 4);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVec2I32")
  {
    WVariant v(WVec2I32(1, 2));
    TestVariant<WVec2I32>(v, WVariantType::Vector2I);

    W_TEST_BOOL(v.Get<WVec2I32>() == WVec2I32(1, 2));

    W_TEST_BOOL(v == WVariant(WVec2I32(1, 2)));
    W_TEST_BOOL(v != WVariant(WVec2I32(1, 1)));

    W_TEST_BOOL(v == WVec2I32(1, 2));
    W_TEST_BOOL(v != WVec2I32(1, 4));

    v = WVec2I32(5, 8);
    W_TEST_BOOL(v == WVec2I32(5, 8));

    v = WVariant(WVec2I32(7, 9));
    W_TEST_BOOL(v == WVec2I32(7, 9));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVec3I32")
  {
    WVariant v(WVec3I32(1, 2, 3));
    TestVariant<WVec3I32>(v, WVariantType::Vector3I);

    W_TEST_BOOL(v.Get<WVec3I32>() == WVec3I32(1, 2, 3));

    W_TEST_BOOL(v == WVariant(WVec3I32(1, 2, 3)));
    W_TEST_BOOL(v != WVariant(WVec3I32(1, 1, 3)));

    W_TEST_BOOL(v == WVec3I32(1, 2, 3));
    W_TEST_BOOL(v != WVec3I32(1, 4, 3));

    v = WVec3I32(5, 8, 9);
    W_TEST_BOOL(v == WVec3I32(5, 8, 9));

    v = WVariant(WVec3I32(7, 9, 8));
    W_TEST_BOOL(v == WVec3I32(7, 9, 8));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v[2] == 8);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);
    W_TEST_BOOL(v["z"] == 8);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVec4I32")
  {
    WVariant v(WVec4I32(1, 2, 3, 4));
    TestVariant<WVec4I32>(v, WVariantType::Vector4I);

    W_TEST_BOOL(v.Get<WVec4I32>() == WVec4I32(1, 2, 3, 4));

    W_TEST_BOOL(v == WVariant(WVec4I32(1, 2, 3, 4)));
    W_TEST_BOOL(v != WVariant(WVec4I32(1, 1, 3, 4)));

    W_TEST_BOOL(v == WVec4I32(1, 2, 3, 4));
    W_TEST_BOOL(v != WVec4I32(1, 4, 3, 4));

    v = WVec4I32(5, 8, 9, 3);
    W_TEST_BOOL(v == WVec4I32(5, 8, 9, 3));

    v = WVariant(WVec4I32(7, 9, 8, 4));
    W_TEST_BOOL(v == WVec4I32(7, 9, 8, 4));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v[2] == 8);
    W_TEST_BOOL(v[3] == 4);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);
    W_TEST_BOOL(v["z"] == 8);
    W_TEST_BOOL(v["w"] == 4);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WQuat")
  {
    WVariant v(WQuat(1, 2, 3, 4));
    TestVariant<WQuat>(v, WVariantType::Quaternion);

    W_TEST_BOOL(v.Get<WQuat>() == WQuat(1, 2, 3, 4));

    W_TEST_BOOL(v == WQuat(1, 2, 3, 4));
    W_TEST_BOOL(v != WQuat(1, 2, 3, 5));

    W_TEST_BOOL(v == WQuat(1, 2, 3, 4));
    W_TEST_BOOL(v != WQuat(1, 4, 3, 4));

    v = WQuat(5, 8, 9, 3);
    W_TEST_BOOL(v == WQuat(5, 8, 9, 3));

    v = WVariant(WQuat(7, 9, 8, 4));
    W_TEST_BOOL(v == WQuat(7, 9, 8, 4));
    W_TEST_BOOL(v[0] == 7);
    W_TEST_BOOL(v[1] == 9);
    W_TEST_BOOL(v[2] == 8);
    W_TEST_BOOL(v[3] == 4);
    W_TEST_BOOL(v["x"] == 7);
    W_TEST_BOOL(v["y"] == 9);
    W_TEST_BOOL(v["z"] == 8);
    W_TEST_BOOL(v["w"] == 4);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);

    WTypedPointer ptr = v.GetWriteAccess();
    W_TEST_BOOL(ptr.m_pObject == &v.Get<WQuat>());
    W_TEST_BOOL(ptr.m_pObject == &v.GetWritable<WQuat>());
    W_TEST_BOOL(ptr.m_pType == WGetStaticRTTI<WQuat>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WMat3")
  {
    WVariant v(WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
    TestVariant<WMat3>(v, WVariantType::Matrix3);

    W_TEST_BOOL(v.Get<WMat3>() == WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));

    W_TEST_BOOL(v == WVariant(WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9)));
    W_TEST_BOOL(v != WVariant(WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 8)));

    W_TEST_BOOL(v == WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9));
    W_TEST_BOOL(v != WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 8));

    v = WMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5);
    W_TEST_BOOL(v == WMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5));

    v = WVariant(WMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 4));
    W_TEST_BOOL(v == WMat3::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 4));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WMat4")
  {
    WVariant v(WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    TestVariant<WMat4>(v, WVariantType::Matrix4);

    W_TEST_BOOL(v.Get<WMat4>() == WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    W_TEST_BOOL(v == WVariant(WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16)));
    W_TEST_BOOL(v != WVariant(WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 15)));

    W_TEST_BOOL(v == WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16));
    W_TEST_BOOL(v != WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 2, 8, 9, 10, 11, 12, 13, 14, 15, 16));

    v = WMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8);
    W_TEST_BOOL(v == WMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 3, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    v = WVariant(WMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));
    W_TEST_BOOL(v == WMat4::MakeFromValues(5, 8, 9, 3, 1, 2, 1, 4, 5, 3, 7, 3, 6, 8, 6, 8));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTransform")
  {
    WVariant v(WTransform(WVec3(1, 2, 3), WQuat(4, 5, 6, 7), WVec3(8, 9, 10)));
    TestVariant<WTransform>(v, WVariantType::Transform);

    W_TEST_BOOL(v.Get<WTransform>() == WTransform(WVec3(1, 2, 3), WQuat(4, 5, 6, 7), WVec3(8, 9, 10)));

    W_TEST_BOOL(v == WTransform(WVec3(1, 2, 3), WQuat(4, 5, 6, 7), WVec3(8, 9, 10)));
    W_TEST_BOOL(v != WTransform(WVec3(1, 2, 3), WQuat(4, 5, 6, 7), WVec3(8, 9, 11)));

    v = WTransform(WVec3(5, 8, 9), WQuat(3, 1, 2, 3), WVec3(4, 5, 3));
    W_TEST_BOOL(v == WTransform(WVec3(5, 8, 9), WQuat(3, 1, 2, 3), WVec3(4, 5, 3)));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "const char*")
  {
    WVariant v("This is a const char array");
    TestVariant<WString>(v, WVariantType::String);

    W_TEST_BOOL(v.IsA<const char*>());
    W_TEST_BOOL(v.IsA<char*>());
    W_TEST_BOOL(v.Get<WString>() == WString("This is a const char array"));

    W_TEST_BOOL(v == WVariant("This is a const char array"));
    W_TEST_BOOL(v != WVariant("This is something else"));

    W_TEST_BOOL(v == WString("This is a const char array"));
    W_TEST_BOOL(v != WString("This is another string"));

    W_TEST_BOOL(v == "This is a const char array");
    W_TEST_BOOL(v != "This is another string");

    W_TEST_BOOL(v == (const char*)"This is a const char array");
    W_TEST_BOOL(v != (const char*)"This is another string");

    v = "blurg!";
    W_TEST_BOOL(v == WString("blurg!"));

    v = WVariant("blärg!");
    W_TEST_BOOL(v == WString("blärg!"));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(v.IsString());
    W_TEST_BOOL(v.CanConvertTo<WStringView>());
    WStringView view = v.ConvertTo<WStringView>();
    W_TEST_BOOL(view == v.Get<WString>());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WString")
  {
    WVariant v(WString("This is an WString"));
    TestVariant<WString>(v, WVariantType::String);

    W_TEST_BOOL(v.Get<WString>() == WString("This is an WString"));

    W_TEST_BOOL(v == WVariant(WString("This is an WString")));
    W_TEST_BOOL(v == WVariant(WStringView("This is an WString"), false));
    W_TEST_BOOL(v != WVariant(WString("This is something else")));

    W_TEST_BOOL(v == WString("This is an WString"));
    W_TEST_BOOL(v != WString("This is another WString"));

    v = WString("blurg!");
    W_TEST_BOOL(v == WString("blurg!"));

    v = WVariant(WString("blärg!"));
    W_TEST_BOOL(v == WString("blärg!"));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(v.IsString());
    W_TEST_BOOL(v.CanConvertTo<WStringView>());
    WStringView view = v.ConvertTo<WStringView>();
    W_TEST_BOOL(view == v.Get<WString>());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WStringView")
  {
    const char* szTemp = "This is an WStringView";
    WStringView bla(szTemp);
    WVariant v(bla, false);
    TestVariant<WStringView>(v, WVariantType::StringView);

    const WString sCopy = szTemp;
    W_TEST_BOOL(v.Get<WStringView>() == sCopy);

    W_TEST_BOOL(v == WVariant(WStringView(sCopy.GetData()), false));
    W_TEST_BOOL(v == WVariant(WString("This is an WStringView")));
    W_TEST_BOOL(v != WVariant(WStringView("This is something else"), false));

    W_TEST_BOOL(v == WStringView(sCopy.GetData()));
    W_TEST_BOOL(v != WStringView("This is something else"));

    v = WVariant(WStringView("blurg!"), false);
    W_TEST_BOOL(v == WStringView("blurg!"));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(v.IsString());
    W_TEST_BOOL(v.CanConvertTo<WString>());
    WString sString = v.ConvertTo<WString>();
    W_TEST_BOOL(sString == v.Get<WStringView>());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WDataBuffer")
  {
    WDataBuffer a, a2;
    a.PushBack(WUInt8(1));
    a.PushBack(WUInt8(2));
    a.PushBack(WUInt8(255));

    WVariant va(a);
    TestVariant<WDataBuffer>(va, WVariantType::DataBuffer);

    const WDataBuffer& b = va.Get<WDataBuffer>();
    WArrayPtr<const WUInt8> b2 = va.Get<WDataBuffer>();

    W_TEST_BOOL(a == b);
    W_TEST_BOOL(a == b2);

    W_TEST_BOOL(a != a2);

    W_TEST_BOOL(va == a);
    W_TEST_BOOL(va != a2);

    W_TEST_BOOL(va.IsNumber() == false);
    W_TEST_BOOL(!va.IsString());
    W_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTime")
  {
    WVariant v(WTime::MakeFromSeconds(1337));
    TestVariant<WTime>(v, WVariantType::Time);

    W_TEST_BOOL(v.Get<WTime>() == WTime::MakeFromSeconds(1337));

    W_TEST_BOOL(v == WVariant(WTime::MakeFromSeconds(1337)));
    W_TEST_BOOL(v != WVariant(WTime::MakeFromSeconds(1336)));

    W_TEST_BOOL(v == WTime::MakeFromSeconds(1337));
    W_TEST_BOOL(v != WTime::MakeFromSeconds(1338));

    v = WTime::MakeFromSeconds(8472);
    W_TEST_BOOL(v == WTime::MakeFromSeconds(8472));

    v = WVariant(WTime::MakeFromSeconds(13));
    W_TEST_BOOL(v == WTime::MakeFromSeconds(13));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WUuid")
  {
    WUuid id;
    WVariant v(id);
    TestVariant<WUuid>(v, WVariantType::Uuid);

    W_TEST_BOOL(v.Get<WUuid>() == WUuid());

    const WUuid uuid = WUuid::MakeUuid();
    W_TEST_BOOL(v != WVariant(uuid));
    W_TEST_BOOL(WVariant(uuid).Get<WUuid>() == uuid);

    const WUuid uuid2 = WUuid::MakeUuid();
    W_TEST_BOOL(WVariant(uuid) != WVariant(uuid2));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WAngle")
  {
    WVariant v(WAngle::MakeFromDegree(1337));
    TestVariant<WAngle>(v, WVariantType::Angle);

    W_TEST_BOOL(v.Get<WAngle>() == WAngle::MakeFromDegree(1337));

    W_TEST_BOOL(v == WVariant(WAngle::MakeFromDegree(1337)));
    W_TEST_BOOL(v != WVariant(WAngle::MakeFromDegree(1336)));

    W_TEST_BOOL(v == WAngle::MakeFromDegree(1337));
    W_TEST_BOOL(v != WAngle::MakeFromDegree(1338));

    v = WAngle::MakeFromDegree(8472);
    W_TEST_BOOL(v == WAngle::MakeFromDegree(8472));

    v = WVariant(WAngle::MakeFromDegree(13));
    W_TEST_BOOL(v == WAngle::MakeFromDegree(13));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WHashedString")
  {
    WVariant v(WMakeHashedString("ABCDE"));
    TestVariant<WHashedString>(v, WVariantType::HashedString);

    W_TEST_BOOL(v.Get<WHashedString>() == WMakeHashedString("ABCDE"));

    W_TEST_BOOL(v == WVariant(WMakeHashedString("ABCDE")));
    W_TEST_BOOL(v != WVariant(WMakeHashedString("ABCDK")));
    W_TEST_BOOL(v == WVariant(WTempHashedString("ABCDE")));
    W_TEST_BOOL(v != WVariant(WTempHashedString("ABCDK")));

    W_TEST_BOOL(v == WMakeHashedString("ABCDE"));
    W_TEST_BOOL(v != WMakeHashedString("ABCDK"));
    W_TEST_BOOL(v == WTempHashedString("ABCDE"));
    W_TEST_BOOL(v != WTempHashedString("ABCDK"));

    v = WMakeHashedString("HHH");
    W_TEST_BOOL(v == WMakeHashedString("HHH"));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(v.IsString() == false);
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTempHashedString")
  {
    WVariant v(WTempHashedString("ABCDE"));
    TestVariant<WTempHashedString>(v, WVariantType::TempHashedString);

    W_TEST_BOOL(v.Get<WTempHashedString>() == WTempHashedString("ABCDE"));

    W_TEST_BOOL(v == WVariant(WTempHashedString("ABCDE")));
    W_TEST_BOOL(v != WVariant(WTempHashedString("ABCDK")));
    W_TEST_BOOL(v == WVariant(WMakeHashedString("ABCDE")));
    W_TEST_BOOL(v != WVariant(WMakeHashedString("ABCDK")));

    W_TEST_BOOL(v == WTempHashedString("ABCDE"));
    W_TEST_BOOL(v != WTempHashedString("ABCDK"));
    W_TEST_BOOL(v == WMakeHashedString("ABCDE"));
    W_TEST_BOOL(v != WMakeHashedString("ABCDK"));

    v = WTempHashedString("HHH");
    W_TEST_BOOL(v == WTempHashedString("HHH"));

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(v.IsString() == false);
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVariantArray")
  {
    WVariantArray a, a2;
    a.PushBack("This");
    a.PushBack("is a");
    a.PushBack("test");

    WVariant va(a);
    W_TEST_BOOL(va.IsValid());
    W_TEST_BOOL(va.GetType() == WVariant::Type::VariantArray);
    W_TEST_BOOL(va.IsA<WVariantArray>());
    W_TEST_BOOL(va.GetReflectedType() == nullptr);

    const WArrayPtr<const WVariant>& b = va.Get<WVariantArray>();
    WArrayPtr<const WVariant> b2 = va.Get<WVariantArray>();

    W_TEST_BOOL(a == b);
    W_TEST_BOOL(a == b2);

    W_TEST_BOOL(a != a2);

    W_TEST_BOOL(va == a);
    W_TEST_BOOL(va != a2);

    W_TEST_BOOL(va[0] == WString("This"));
    W_TEST_BOOL(va[1] == WString("is a"));
    W_TEST_BOOL(va[2] == WString("test"));
    W_TEST_BOOL(va[4] == WVariant());
    W_TEST_BOOL(!va[4].IsValid());

    W_TEST_BOOL(va.IsNumber() == false);
    W_TEST_BOOL(!va.IsString());
    W_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WVariantDictionary")
  {
    WVariantDictionary a, a2;
    a["my"] = true;
    a["luv"] = 4;
    a["pon"] = "ies";

    WVariant va(a);
    W_TEST_BOOL(va.IsValid());
    W_TEST_BOOL(va.GetType() == WVariant::Type::VariantDictionary);
    W_TEST_BOOL(va.IsA<WVariantDictionary>());
    W_TEST_BOOL(va.GetReflectedType() == nullptr);

    const WVariantDictionary& d1 = va.Get<WVariantDictionary>();
    WVariantDictionary d2 = va.Get<WVariantDictionary>();

    W_TEST_BOOL(a == d1);
    W_TEST_BOOL(a == d2);
    W_TEST_BOOL(d1 == d2);

    W_TEST_BOOL(va == a);
    W_TEST_BOOL(va != a2);

    W_TEST_BOOL(va["my"] == true);
    W_TEST_BOOL(va["luv"] == 4);
    W_TEST_BOOL(va["pon"] == WString("ies"));
    W_TEST_BOOL(va["x"] == WVariant());
    W_TEST_BOOL(!va["x"].IsValid());

    W_TEST_BOOL(va.IsNumber() == false);
    W_TEST_BOOL(!va.IsString());
    W_TEST_BOOL(va.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTypedPointer")
  {
    Blubb blubb;
    blubb.u = 1.0f;
    blubb.v = 2.0f;

    Blubb blubb2;

    WVariant v(&blubb);

    W_TEST_BOOL(v.IsValid());
    W_TEST_BOOL(v.GetType() == WVariant::Type::TypedPointer);
    W_TEST_BOOL(v.IsA<Blubb*>());
    W_TEST_BOOL(v.Get<Blubb*>() == &blubb);
    W_TEST_BOOL(v.IsA<WReflectedClass*>());
    W_TEST_BOOL(v.Get<WReflectedClass*>() == &blubb);
    W_TEST_BOOL(v.Get<WReflectedClass*>() != &blubb2);
    W_TEST_BOOL(WDynamicCast<Blubb*>(v) == &blubb);
    W_TEST_BOOL(WDynamicCast<WVec3*>(v) == nullptr);
    W_TEST_BOOL(v.IsA<void*>());
    W_TEST_BOOL(v.Get<void*>() == &blubb);
    W_TEST_BOOL(v.IsA<const void*>());
    W_TEST_BOOL(v.Get<const void*>() == &blubb);
    W_TEST_BOOL(v.GetData() == &blubb);
    W_TEST_BOOL(v.IsA<WTypedPointer>());
    W_TEST_BOOL(v.GetReflectedType() == WGetStaticRTTI<Blubb>());
    W_TEST_BOOL(!v.IsA<WVec3*>());

    WTypedPointer ptr = v.Get<WTypedPointer>();
    W_TEST_BOOL(ptr.m_pObject == &blubb);
    W_TEST_BOOL(ptr.m_pType == WGetStaticRTTI<Blubb>());

    WTypedPointer ptr2 = v.GetWriteAccess();
    W_TEST_BOOL(ptr2.m_pObject == &blubb);
    W_TEST_BOOL(ptr2.m_pType == WGetStaticRTTI<Blubb>());

    W_TEST_BOOL(v[0] == 1.0f);
    W_TEST_BOOL(v[1] == 2.0f);
    W_TEST_BOOL(v["u"] == 1.0f);
    W_TEST_BOOL(v["v"] == 2.0f);
    WVariant v2 = &blubb;
    W_TEST_BOOL(v == v2);
    WVariant v3 = ptr;
    W_TEST_BOOL(v == v3);

    W_TEST_BOOL(v.IsNumber() == false);
    W_TEST_BOOL(!v.IsString());
    W_TEST_BOOL(v.IsFloatingPoint() == false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTypedPointer nullptr")
  {
    WTypedPointer ptr = {nullptr, WGetStaticRTTI<Blubb>()};
    WVariant v = ptr;
    W_TEST_BOOL(v.IsValid());
    W_TEST_BOOL(v.GetType() == WVariant::Type::TypedPointer);
    W_TEST_BOOL(v.IsA<Blubb*>());
    W_TEST_BOOL(v.Get<Blubb*>() == nullptr);
    W_TEST_BOOL(v.IsA<WReflectedClass*>());
    W_TEST_BOOL(v.Get<WReflectedClass*>() == nullptr);
    W_TEST_BOOL(WDynamicCast<Blubb*>(v) == nullptr);
    W_TEST_BOOL(WDynamicCast<WVec3*>(v) == nullptr);
    W_TEST_BOOL(v.IsA<void*>());
    W_TEST_BOOL(v.Get<void*>() == nullptr);
    W_TEST_BOOL(v.IsA<const void*>());
    W_TEST_BOOL(v.Get<const void*>() == nullptr);
    W_TEST_BOOL(v.IsA<WTypedPointer>());
    W_TEST_BOOL(v.GetReflectedType() == WGetStaticRTTI<Blubb>());
    W_TEST_BOOL(!v.IsA<WVec3*>());

    WTypedPointer ptr2 = v.Get<WTypedPointer>();
    W_TEST_BOOL(ptr2.m_pObject == nullptr);
    W_TEST_BOOL(ptr2.m_pType == WGetStaticRTTI<Blubb>());

    W_TEST_BOOL(!v[0].IsValid());
    W_TEST_BOOL(!v["u"].IsValid());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTypedObject inline")
  {
    // WAngle::MakeFromDegree(90.0f) was replaced with radian as release builds generate a different float then debug.
    WVarianceTypeAngle value(WAngle::MakeFromRadian(1.57079637f), 0.1f);
    WVarianceTypeAngle value2(WAngle::MakeFromRadian(1.57079637f), 0.2f);

    WVariant v(value);
    TestVariant<WVarianceTypeAngle>(v, WVariantType::TypedObject);

    W_TEST_BOOL(v.IsA<WTypedObject>());
    W_TEST_BOOL(!v.IsA<void*>());
    W_TEST_BOOL(!v.IsA<const void*>());
    W_TEST_BOOL(!v.IsA<WVec3*>());
    W_TEST_BOOL(WDynamicCast<WVec3*>(v) == nullptr);

    const WVarianceTypeAngle& valueGet = v.Get<WVarianceTypeAngle>();
    W_TEST_BOOL(value == valueGet);

    WVariant va = value;
    W_TEST_BOOL(v == va);

    WVariant v2 = value2;
    W_TEST_BOOL(v != v2);

    WUInt64 uiHash = v.ComputeHash(0);
    W_TEST_INT(uiHash, 8527525522777555267ul);

    WVarianceTypeAngle* pTypedAngle = W_DEFAULT_NEW(WVarianceTypeAngle, WAngle::MakeFromRadian(1.57079637f), 0.1f);
    WVariant copy;
    copy.CopyTypedObject(pTypedAngle, WGetStaticRTTI<WVarianceTypeAngle>());
    WVariant move;
    move.MoveTypedObject(pTypedAngle, WGetStaticRTTI<WVarianceTypeAngle>());
    W_TEST_BOOL(v == copy);
    W_TEST_BOOL(v == move);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTypedObject shared")
  {
    WTypedObjectStruct data;
    WVariant v = data;
    W_TEST_BOOL(v.IsValid());
    W_TEST_BOOL(v.GetType() == WVariant::Type::TypedObject);
    W_TEST_BOOL(v.IsA<WTypedObject>());
    W_TEST_BOOL(v.IsA<WTypedObjectStruct>());
    W_TEST_BOOL(!v.IsA<void*>());
    W_TEST_BOOL(!v.IsA<const void*>());
    W_TEST_BOOL(!v.IsA<WVec3*>());
    W_TEST_BOOL(WDynamicCast<WVec3*>(v) == nullptr);
    W_TEST_BOOL(v.GetReflectedType() == WGetStaticRTTI<WTypedObjectStruct>());

    WVariant v2 = v;

    WTypedPointer ptr = v.GetWriteAccess();
    W_TEST_BOOL(ptr.m_pObject == &v.Get<WTypedObjectStruct>());
    W_TEST_BOOL(ptr.m_pObject == &v.GetWritable<WTypedObjectStruct>());
    W_TEST_BOOL(ptr.m_pObject != &v2.Get<WTypedObjectStruct>());
    W_TEST_BOOL(ptr.m_pType == WGetStaticRTTI<WTypedObjectStruct>());

    W_TEST_BOOL(WReflectionUtils::IsEqual(ptr.m_pObject, &v2.Get<WTypedObjectStruct>(), WGetStaticRTTI<WTypedObjectStruct>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (bool)")
  {
    WVariant v(true);

    W_TEST_BOOL(v.CanConvertTo<bool>());
    W_TEST_BOOL(v.CanConvertTo<WInt32>());

    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Invalid) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Bool));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int8));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt8));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int16));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt16));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int32));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt32));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int64));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt64));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Float));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Double));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Color) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2I));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3I));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4I));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Quaternion) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix3) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix4) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::String));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::StringView) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::DataBuffer) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Time) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Angle) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantArray) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantDictionary) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedPointer) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedObject) == false);

    W_TEST_BOOL(v.ConvertTo<bool>() == true);
    W_TEST_BOOL(v.ConvertTo<WInt8>() == 1);
    W_TEST_BOOL(v.ConvertTo<WUInt8>() == 1);
    W_TEST_BOOL(v.ConvertTo<WInt16>() == 1);
    W_TEST_BOOL(v.ConvertTo<WUInt16>() == 1);
    W_TEST_BOOL(v.ConvertTo<WInt32>() == 1);
    W_TEST_BOOL(v.ConvertTo<WUInt32>() == 1);
    W_TEST_BOOL(v.ConvertTo<WInt64>() == 1);
    W_TEST_BOOL(v.ConvertTo<WUInt64>() == 1);
    W_TEST_BOOL(v.ConvertTo<float>() == 1.0f);
    W_TEST_BOOL(v.ConvertTo<double>() == 1.0);
    W_TEST_BOOL(v.ConvertTo<WVec2>() == WVec2(1));
    W_TEST_BOOL(v.ConvertTo<WVec3>() == WVec3(1));
    W_TEST_BOOL(v.ConvertTo<WVec4>() == WVec4(1));
    W_TEST_BOOL(v.ConvertTo<WVec2I32>() == WVec2I32(1));
    W_TEST_BOOL(v.ConvertTo<WVec3I32>() == WVec3I32(1));
    W_TEST_BOOL(v.ConvertTo<WVec4I32>() == WVec4I32(1));
    W_TEST_BOOL(v.ConvertTo<WString>() == "true");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("true"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("true"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Bool).Get<bool>() == true);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int8).Get<WInt8>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt8).Get<WUInt8>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int16).Get<WInt16>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt16).Get<WUInt16>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int32).Get<WInt32>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt32).Get<WUInt32>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int64).Get<WInt64>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt64).Get<WUInt64>() == 1);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Float).Get<float>() == 1.0f);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Double).Get<double>() == 1.0);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "true");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("true"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("true"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WInt8)")
  {
    WVariant v((WInt8)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WUInt8)")
  {
    WVariant v((WUInt8)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WInt16)")
  {
    WVariant v((WInt16)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WUInt16)")
  {
    WVariant v((WUInt16)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WInt32)")
  {
    WVariant v((WInt32)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WUInt32)")
  {
    WVariant v((WUInt32)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WInt64)")
  {
    WVariant v((WInt64)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WUInt64)")
  {
    WVariant v((WUInt64)3);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (float)")
  {
    WVariant v((float)3.0f);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (double)")
  {
    WVariant v((double)3.0f);
    TestNumberCanConvertTo(v);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (Color)")
  {
    WColor c(3, 3, 4, 0);
    WVariant v(c);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Color, WVariant::Type::ColorGamma);

    WResult conversionResult = W_FAILURE;
    W_TEST_BOOL(v.ConvertTo<WColor>(&conversionResult) == c);
    W_TEST_BOOL(conversionResult.Succeeded());

    W_TEST_BOOL(v.ConvertTo<WString>(&conversionResult) == "{ r=3, g=3, b=4, a=0 }");
    W_TEST_BOOL(conversionResult.Succeeded());

    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ r=3, g=3, b=4, a=0 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ r=3, g=3, b=4, a=0 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Color).Get<WColor>() == c);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ r=3, g=3, b=4, a=0 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ r=3, g=3, b=4, a=0 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ r=3, g=3, b=4, a=0 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (ColorGamma)")
  {
    WColorGammaUB c(0, 128, 64, 255);
    WVariant v(c);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::ColorGamma, WVariant::Type::Color);

    WResult conversionResult = W_FAILURE;
    W_TEST_BOOL(v.ConvertTo<WColorGammaUB>(&conversionResult) == c);
    W_TEST_BOOL(conversionResult.Succeeded());

    WString val = v.ConvertTo<WString>(&conversionResult);
    W_TEST_BOOL(val == "{ r=0, g=128, b=64, a=255 }");
    W_TEST_BOOL(conversionResult.Succeeded());

    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ r=0, g=128, b=64, a=255 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ r=0, g=128, b=64, a=255 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::ColorGamma).Get<WColorGammaUB>() == c);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ r=0, g=128, b=64, a=255 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ r=0, g=128, b=64, a=255 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ r=0, g=128, b=64, a=255 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVec2)")
  {
    WVec2 vec(3.0f, 4.0f);
    WVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Vector2, WVariant::Type::Vector2I, WVariant::Type::Vector2U);

    W_TEST_BOOL(v.ConvertTo<WVec2>() == vec);
    W_TEST_BOOL(v.ConvertTo<WVec2I32>() == WVec2I32(3, 4));
    W_TEST_BOOL(v.ConvertTo<WVec2U32>() == WVec2U32(3, 4));
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2).Get<WVec2>() == vec);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2I).Get<WVec2I32>() == WVec2I32(3, 4));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2U).Get<WVec2U32>() == WVec2U32(3, 4));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVec3)")
  {
    WVec3 vec(3.0f, 4.0f, 6.0f);
    WVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Vector3, WVariant::Type::Vector3I, WVariant::Type::Vector3U);

    W_TEST_BOOL(v.ConvertTo<WVec3>() == vec);
    W_TEST_BOOL(v.ConvertTo<WVec3I32>() == WVec3I32(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo<WVec3U32>() == WVec3U32(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4, z=6 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=6 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=6 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3).Get<WVec3>() == vec);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3I).Get<WVec3I32>() == WVec3I32(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3U).Get<WVec3U32>() == WVec3U32(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4, z=6 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=6 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=6 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVec4)")
  {
    WVec4 vec(3.0f, 4.0f, 3, 56);
    WVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Vector4, WVariant::Type::Vector4I, WVariant::Type::Vector4U);

    W_TEST_BOOL(v.ConvertTo<WVec4>() == vec);
    W_TEST_BOOL(v.ConvertTo<WVec4I32>() == WVec4I32(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo<WVec4U32>() == WVec4U32(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4, z=3, w=56 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4).Get<WVec4>() == vec);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4I).Get<WVec4I32>() == WVec4I32(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4U).Get<WVec4U32>() == WVec4U32(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4, z=3, w=56 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVec2I32)")
  {
    WVec2I32 vec(3, 4);
    WVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Vector2I, WVariant::Type::Vector2U, WVariant::Type::Vector2);

    W_TEST_BOOL(v.ConvertTo<WVec2I32>() == vec);
    W_TEST_BOOL(v.ConvertTo<WVec2>() == WVec2(3, 4));
    W_TEST_BOOL(v.ConvertTo<WVec2U32>() == WVec2U32(3, 4));
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2I).Get<WVec2I32>() == vec);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2).Get<WVec2>() == WVec2(3, 4));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector2U).Get<WVec2U32>() == WVec2U32(3, 4));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVec3I32)")
  {
    WVec3I32 vec(3, 4, 6);
    WVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Vector3I, WVariant::Type::Vector3U, WVariant::Type::Vector3);

    W_TEST_BOOL(v.ConvertTo<WVec3I32>() == vec);
    W_TEST_BOOL(v.ConvertTo<WVec3>() == WVec3(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo<WVec3U32>() == WVec3U32(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4, z=6 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=6 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=6 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3I).Get<WVec3I32>() == vec);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3).Get<WVec3>() == WVec3(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector3U).Get<WVec3U32>() == WVec3U32(3, 4, 6));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4, z=6 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=6 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=6 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVec4I32)")
  {
    WVec4I32 vec(3, 4, 3, 56);
    WVariant v(vec);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Vector4I, WVariant::Type::Vector4U, WVariant::Type::Vector4);

    W_TEST_BOOL(v.ConvertTo<WVec4I32>() == vec);
    W_TEST_BOOL(v.ConvertTo<WVec4>() == WVec4(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo<WVec4U32>() == WVec4U32(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4, z=3, w=56 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4I).Get<WVec4I32>() == vec);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4).Get<WVec4>() == WVec4(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Vector4U).Get<WVec4U32>() == WVec4U32(3, 4, 3, 56));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4, z=3, w=56 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WQuat)")
  {
    WQuat q(3.0f, 4.0f, 3, 56);
    WVariant v(q);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Quaternion);

    W_TEST_BOOL(v.ConvertTo<WQuat>() == q);
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ x=3, y=4, z=3, w=56 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=3, w=56 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Quaternion).Get<WQuat>() == q);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ x=3, y=4, z=3, w=56 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ x=3, y=4, z=3, w=56 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ x=3, y=4, z=3, w=56 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WMat3)")
  {
    WMat3 m = WMat3::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9);
    WVariant v(m);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Matrix3);

    W_TEST_BOOL(v.ConvertTo<WMat3>() == m);
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Matrix3).Get<WMat3>() == m);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c1r2=4, c2r2=5, c3r2=6, c1r3=7, c2r3=8, c3r3=9 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WMat4)")
  {
    WMat4 m = WMat4::MakeFromValues(1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6);
    WVariant v(m);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Matrix4);

    W_TEST_BOOL(v.ConvertTo<WMat4>() == m);
    W_TEST_BOOL(v.ConvertTo<WString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                            "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                            "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                            "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                     "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                     "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                     "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                         "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                         "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                         "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Matrix4).Get<WMat4>() == m);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                         "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                         "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                         "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                                                        "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                                                        "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                                                        "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{ c1r1=1, c2r1=2, c3r1=3, c4r1=4, "
                                                                                                                "c1r2=5, c2r2=6, c3r2=7, c4r2=8, "
                                                                                                                "c1r3=9, c2r3=0, c3r3=1, c4r3=2, "
                                                                                                                "c1r4=3, c2r4=4, c3r4=5, c4r4=6 }"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WString)")
  {
    WVariant v("ich hab keine Lust mehr");

    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Invalid) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Bool));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int8));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt8));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int16));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt16));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int32));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt32));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int64));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt64));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Float));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Double));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Color) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2I) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3I) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4I) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Quaternion) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix3) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix4) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::String));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::StringView));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::DataBuffer) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Time) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Angle) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::ColorGamma) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::HashedString));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TempHashedString));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantArray) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantDictionary) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedPointer) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedObject) == false);

    {
      WResult ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WInt8>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WUInt8>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WInt16>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WUInt16>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WInt32>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WUInt32>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WInt64>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WUInt64>(&ConversionStatus) == 0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.0f);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.0);
      W_TEST_BOOL(ConversionStatus == W_FAILURE);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WHashedString>(&ConversionStatus) == WMakeHashedString("ich hab keine Lust mehr"));
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_SUCCESS;
      W_TEST_BOOL(v.ConvertTo<WTempHashedString>(&ConversionStatus) == WTempHashedString("ich hab keine Lust mehr"));
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "true";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == true);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Bool, &ConversionStatus).Get<bool>() == true);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "-128";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WInt8>(&ConversionStatus) == -128);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int8, &ConversionStatus).Get<WInt8>() == -128);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "255";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WUInt8>(&ConversionStatus) == 255);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt8, &ConversionStatus).Get<WUInt8>() == 255);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "-5643";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WInt16>(&ConversionStatus) == -5643);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int16, &ConversionStatus).Get<WInt16>() == -5643);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "9001";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WUInt16>(&ConversionStatus) == 9001);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt16, &ConversionStatus).Get<WUInt16>() == 9001);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "46";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WInt32>(&ConversionStatus) == 46);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int32, &ConversionStatus).Get<WInt32>() == 46);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "356";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WUInt32>(&ConversionStatus) == 356);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt32, &ConversionStatus).Get<WUInt32>() == 356);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "64";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WInt64>(&ConversionStatus) == 64);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Int64, &ConversionStatus).Get<WInt64>() == 64);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "6464";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<WUInt64>(&ConversionStatus) == 6464);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::UInt64, &ConversionStatus).Get<WUInt64>() == 6464);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "0.07564f";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 0.07564f);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Float, &ConversionStatus).Get<float>() == 0.07564f);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }

    {
      v = "0.4453";
      WResult ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 0.4453);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);

      ConversionStatus = W_FAILURE;
      W_TEST_BOOL(v.ConvertTo(WVariant::Type::Double, &ConversionStatus).Get<double>() == 0.4453);
      W_TEST_BOOL(ConversionStatus == W_SUCCESS);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WStringView)")
  {
    WStringView va0("Test String");
    WVariant v(va0, false);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::StringView);

    W_TEST_BOOL(v.ConvertTo<WStringView>() == va0);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::StringView).Get<WStringView>() == va0);

    {
      WVariant va, va2;

      va = "Bla";
      W_TEST_BOOL(va.IsA<WString>());
      W_TEST_BOOL(va.CanConvertTo<WString>());
      W_TEST_BOOL(va.CanConvertTo<WStringView>());

      va = WVariant("Bla"_wsv, false);
      W_TEST_BOOL(va.IsA<WStringView>());
      W_TEST_BOOL(va.CanConvertTo<WString>());
      W_TEST_BOOL(va.CanConvertTo<WStringView>());

      va2 = va;
      W_TEST_BOOL(va2.IsA<WStringView>());
      W_TEST_BOOL(va2.CanConvertTo<WString>());
      W_TEST_BOOL(va2.CanConvertTo<WStringView>());
      W_TEST_BOOL(va2.ConvertTo<WStringView>() == "Bla");
      W_TEST_BOOL(va2.ConvertTo<WString>() == "Bla");

      WVariant va3 = va2.ConvertTo(WVariantType::StringView);
      W_TEST_BOOL(va3.IsA<WStringView>());
      W_TEST_BOOL(va3.ConvertTo<WString>() == "Bla");

      va = "Blub";
      W_TEST_BOOL(va.IsA<WString>());

      WVariant va4 = va.ConvertTo(WVariantType::StringView);
      W_TEST_BOOL(va4.IsA<WStringView>());
      W_TEST_BOOL(va4.ConvertTo<WString>() == "Blub");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WDataBuffer)")
  {
    WDataBuffer va;
    va.PushBack(255);
    va.PushBack(4);
    WVariant v(va);

    TestCanOnlyConvertToID(v, WVariant::Type::DataBuffer);

    W_TEST_BOOL(v.ConvertTo<WDataBuffer>() == va);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::DataBuffer).Get<WDataBuffer>() == va);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WTime)")
  {
    WTime t = WTime::MakeFromSeconds(123.0);
    WVariant v(t);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Time);

    W_TEST_BOOL(v.ConvertTo<WTime>() == t);
    // W_TEST_BOOL(v.ConvertTo<WString>() == "");

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Time).Get<WTime>() == t);
    // W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WUuid)")
  {
    const WUuid uuid = WUuid::MakeUuid();
    WVariant v(uuid);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Uuid);

    W_TEST_BOOL(v.ConvertTo<WUuid>() == uuid);
    // W_TEST_BOOL(v.ConvertTo<WString>() == "");

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Uuid).Get<WUuid>() == uuid);
    // W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WAngle)")
  {
    WAngle t = WAngle::MakeFromDegree(123.0);
    WVariant v(t);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::Angle);

    W_TEST_BOOL(v.ConvertTo<WAngle>() == t);
    W_TEST_BOOL(v.ConvertTo<WString>() == "123.0°");
    // W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("123.0°")); // For some reason the compiler stumbles upon the degree sign, encoding weirdness most likely
    // W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("123.0°"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::Angle).Get<WAngle>() == t);
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "123.0°");
    // W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("123.0°"));
    // W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("123.0°"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WHashedString)")
  {
    WVariant v(WMakeHashedString("78"));

    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Invalid) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Bool));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int8));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt8));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int16));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt16));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int32));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt32));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Int64));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::UInt64));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Float));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Double));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Color) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector2I) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector3I) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Vector4I) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Quaternion) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix3) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Matrix4) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::String));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::StringView));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::DataBuffer) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Time) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::Angle) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::ColorGamma) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::HashedString));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TempHashedString));
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantArray) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::VariantDictionary) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedPointer) == false);
    W_TEST_BOOL(v.CanConvertTo(WVariant::Type::TypedObject) == false);

    WResult ConversionStatus = W_SUCCESS;
    W_TEST_BOOL(v.ConvertTo<bool>(&ConversionStatus) == false);
    W_TEST_BOOL(ConversionStatus == W_FAILURE);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WInt8>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WUInt8>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WInt16>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WUInt16>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WInt32>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WUInt32>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WInt64>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_INT(v.ConvertTo<WUInt64>(&ConversionStatus), 78);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_BOOL(v.ConvertTo<float>(&ConversionStatus) == 78.0f);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_BOOL(v.ConvertTo<double>(&ConversionStatus) == 78.0);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_STRING(v.ConvertTo<WString>(&ConversionStatus), "78");
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_BOOL(v.ConvertTo<WStringView>(&ConversionStatus) == "78"_wsv);
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);

    ConversionStatus = W_FAILURE;
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>(&ConversionStatus) == WTempHashedString("78"));
    W_TEST_BOOL(ConversionStatus == W_SUCCESS);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WTempHashedString)")
  {
    WTempHashedString s("VVVV");
    WVariant v(s);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::TempHashedString);

    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("VVVV"));
    W_TEST_BOOL(v.ConvertTo<WString>() == "0x69d489c8b7fa5f47");

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("VVVV"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::String).Get<WString>() == "0x69d489c8b7fa5f47");
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (VariantArray)")
  {
    WVariantArray va;
    va.PushBack(2.5);
    va.PushBack("ABC");
    va.PushBack(WVariant());
    WVariant v(va);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::VariantArray);

    W_TEST_BOOL(v.ConvertTo<WVariantArray>() == va);
    W_TEST_STRING(v.ConvertTo<WString>(), "[2.5, ABC, <Invalid>]");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("[2.5, ABC, <Invalid>]"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("[2.5, ABC, <Invalid>]"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::VariantArray).Get<WVariantArray>() == va);
    W_TEST_STRING(v.ConvertTo(WVariant::Type::String).Get<WString>(), "[2.5, ABC, <Invalid>]");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("[2.5, ABC, <Invalid>]"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("[2.5, ABC, <Invalid>]"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "(Can)ConvertTo (WVariantDictionary)")
  {
    WVariantDictionary va;
    va.Insert("A", 2.5);
    va.Insert("B", "ABC");
    va.Insert("C", WVariant());
    WVariant v(va);

    TestCanOnlyConvertToStringAndID(v, WVariant::Type::VariantDictionary);

    W_TEST_BOOL(v.ConvertTo<WVariantDictionary>() == va);
    W_TEST_STRING(v.ConvertTo<WString>(), "{A=2.5, C=<Invalid>, B=ABC}");
    W_TEST_BOOL(v.ConvertTo<WHashedString>() == WMakeHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
    W_TEST_BOOL(v.ConvertTo<WTempHashedString>() == WTempHashedString("{A=2.5, C=<Invalid>, B=ABC}"));

    W_TEST_BOOL(v.ConvertTo(WVariant::Type::VariantDictionary).Get<WVariantDictionary>() == va);
    W_TEST_STRING(v.ConvertTo(WVariant::Type::String).Get<WString>(), "{A=2.5, C=<Invalid>, B=ABC}");
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::HashedString).Get<WHashedString>() == WMakeHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
    W_TEST_BOOL(v.ConvertTo(WVariant::Type::TempHashedString).Get<WTempHashedString>() == WTempHashedString("{A=2.5, C=<Invalid>, B=ABC}"));
  }
}

#pragma optimize("", on)
