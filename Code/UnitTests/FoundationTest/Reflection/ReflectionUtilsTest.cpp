#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

template <typename T>
static void SetComponentTest(WVec2Template<T> vVector, T value)
{
  WVariant var = vVector;
  WReflectionUtils::SetComponent(var, 0, value);
  W_TEST_BOOL(var.Get<WVec2Template<T>>().x == value);
  WReflectionUtils::SetComponent(var, 1, value);
  W_TEST_BOOL(var.Get<WVec2Template<T>>().y == value);
}

template <typename T>
static void SetComponentTest(WVec3Template<T> vVector, T value)
{
  WVariant var = vVector;
  WReflectionUtils::SetComponent(var, 0, value);
  W_TEST_BOOL(var.Get<WVec3Template<T>>().x == value);
  WReflectionUtils::SetComponent(var, 1, value);
  W_TEST_BOOL(var.Get<WVec3Template<T>>().y == value);
  WReflectionUtils::SetComponent(var, 2, value);
  W_TEST_BOOL(var.Get<WVec3Template<T>>().z == value);
}

template <typename T>
static void SetComponentTest(WVec4Template<T> vVector, T value)
{
  WVariant var = vVector;
  WReflectionUtils::SetComponent(var, 0, value);
  W_TEST_BOOL(var.Get<WVec4Template<T>>().x == value);
  WReflectionUtils::SetComponent(var, 1, value);
  W_TEST_BOOL(var.Get<WVec4Template<T>>().y == value);
  WReflectionUtils::SetComponent(var, 2, value);
  W_TEST_BOOL(var.Get<WVec4Template<T>>().z == value);
  WReflectionUtils::SetComponent(var, 3, value);
  W_TEST_BOOL(var.Get<WVec4Template<T>>().w == value);
}

template <class T>
static void ClampValueTest(T tooSmall, T tooBig, T min, T max)
{
  WClampValueAttribute minClamp(min, {});
  WClampValueAttribute maxClamp({}, max);
  WClampValueAttribute bothClamp(min, max);

  WVariant value = tooSmall;
  W_TEST_BOOL(WReflectionUtils::ClampValue(value, &minClamp).Succeeded());
  W_TEST_BOOL(value == min);

  value = tooSmall;
  W_TEST_BOOL(WReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  W_TEST_BOOL(value == min);

  value = tooBig;
  W_TEST_BOOL(WReflectionUtils::ClampValue(value, &maxClamp).Succeeded());
  W_TEST_BOOL(value == max);

  value = tooBig;
  W_TEST_BOOL(WReflectionUtils::ClampValue(value, &bothClamp).Succeeded());
  W_TEST_BOOL(value == max);
}


W_CREATE_SIMPLE_TEST(Reflection, Utils)
{
  WDefaultMemoryStreamStorage StreamStorage;

  W_TEST_BLOCK(WTestBlock::Enabled, "WriteObjectToDDL")
  {
    WMemoryStreamWriter FileOut(&StreamStorage);

    WTestClass2 c2;
    c2.SetCharPtr("Hallo");
    c2.SetString("World");
    c2.SetStringView("!!!");
    c2.m_MyVector.Set(14, 16, 18);
    c2.m_Struct.m_fFloat1 = 128;
    c2.m_Struct.m_UInt8 = 234;
    c2.m_Struct.m_Angle = WAngle::MakeFromDegree(360);
    c2.m_Struct.m_vVec3I = WVec3I32(9, 8, 7);
    c2.m_Struct.m_DataBuffer.Clear();
    c2.m_Color = WColor(0.1f, 0.2f, 0.3f);
    c2.m_Time = WTime::MakeFromSeconds(91.0f);
    c2.m_enumClass = WExampleEnum::Value3;
    c2.m_bitflagsClass = WExampleBitflags::Value1 | WExampleBitflags::Value2 | WExampleBitflags::Value3;
    c2.m_array.PushBack(5.0f);
    c2.m_array.PushBack(10.0f);
    c2.m_Variant = WVec3(1.0f, 2.0f, 3.0f);

    WReflectionSerializer::WriteObjectToDDL(FileOut, c2.GetDynamicRTTI(), &c2, false, WOpenDdlWriter::TypeStringMode::Compliant);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    WMemoryStreamReader FileIn(&StreamStorage);

    WTestClass2 c2;

    WReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    W_TEST_STRING(c2.GetCharPtr(), "Hallo");
    W_TEST_STRING(c2.GetString(), "World");
    W_TEST_STRING(c2.GetStringView(), "!!!");
    W_TEST_VEC3(c2.m_MyVector, WVec3(3, 4, 5), 0.0f);
    W_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    W_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    W_TEST_INT(c2.m_Struct.m_UInt8, 234);
    W_TEST_BOOL(c2.m_Struct.m_Angle == WAngle::MakeFromDegree(360));
    W_TEST_BOOL(c2.m_Struct.m_vVec3I == WVec3I32(9, 8, 7));
    W_TEST_BOOL(c2.m_Struct.m_DataBuffer == WDataBuffer());
    W_TEST_BOOL(c2.m_enumClass == WExampleEnum::Value3);
    W_TEST_BOOL(c2.m_bitflagsClass == (WExampleBitflags::Value1 | WExampleBitflags::Value2 | WExampleBitflags::Value3));
    W_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      W_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      W_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    W_TEST_VEC3(c2.m_Variant.Get<WVec3>(), WVec3(1.0f, 2.0f, 3.0f), 0.0f);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectPropertiesFromDDL (different type)")
  {
    // here we restore the same properties into a different type of object which has properties that are named the same
    // but may have slightly different types (but which are compatible)

    WMemoryStreamReader FileIn(&StreamStorage);

    WTestClass2b c2;

    WReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *c2.GetDynamicRTTI(), &c2);

    W_TEST_STRING(c2.GetText(), "Tut"); // not restored, different property name
    W_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    W_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    W_TEST_INT(c2.m_Struct.m_UInt8, 234);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectFromDDL")
  {
    WMemoryStreamReader FileIn(&StreamStorage);

    const WRTTI* pRtti;
    void* pObject = WReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    WTestClass2& c2 = *((WTestClass2*)pObject);

    W_TEST_STRING(c2.GetCharPtr(), "Hallo");
    W_TEST_STRING(c2.GetString(), "World");
    W_TEST_STRING(c2.GetStringView(), "!!!");
    W_TEST_VEC3(c2.m_MyVector, WVec3(3, 4, 5), 0.0f);
    W_TEST_FLOAT(c2.m_Time.GetSeconds(), 91.0f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.r, 0.1f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.g, 0.2f, 0.0f);
    W_TEST_FLOAT(c2.m_Color.b, 0.3f, 0.0f);
    W_TEST_FLOAT(c2.m_Struct.m_fFloat1, 128, 0.0f);
    W_TEST_INT(c2.m_Struct.m_UInt8, 234);
    W_TEST_BOOL(c2.m_Struct.m_Angle == WAngle::MakeFromDegree(360));
    W_TEST_BOOL(c2.m_Struct.m_vVec3I == WVec3I32(9, 8, 7));
    W_TEST_BOOL(c2.m_Struct.m_DataBuffer == WDataBuffer());
    W_TEST_BOOL(c2.m_enumClass == WExampleEnum::Value3);
    W_TEST_BOOL(c2.m_bitflagsClass == (WExampleBitflags::Value1 | WExampleBitflags::Value2 | WExampleBitflags::Value3));
    W_TEST_INT(c2.m_array.GetCount(), 2);
    if (c2.m_array.GetCount() == 2)
    {
      W_TEST_FLOAT(c2.m_array[0], 5.0f, 0.0f);
      W_TEST_FLOAT(c2.m_array[1], 10.0f, 0.0f);
    }
    W_TEST_VEC3(c2.m_Variant.Get<WVec3>(), WVec3(1.0f, 2.0f, 3.0f), 0.0f);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  WFileSystem::ClearAllDataDirectories();

  W_TEST_BLOCK(WTestBlock::Enabled, "SetComponent")
  {
    SetComponentTest(WVec2(0.0f, 0.1f), -0.5f);
    SetComponentTest(WVec3(0.0f, 0.1f, 0.2f), -0.5f);
    SetComponentTest(WVec4(0.0f, 0.1f, 0.2f, 0.3f), -0.5f);
    SetComponentTest(WVec2I32(0, 1), -4);
    SetComponentTest(WVec3I32(0, 1, 2), -4);
    SetComponentTest(WVec4I32(0, 1, 2, 3), -4);
    SetComponentTest(WVec2U32(0, 1), 4u);
    SetComponentTest(WVec3U32(0, 1, 2), 4u);
    SetComponentTest(WVec4U32(0, 1, 2, 3), 4u);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ClampValue")
  {
    ClampValueTest<float>(-1, 1000, 2, 4);
    ClampValueTest<double>(-1, 1000, 2, 4);
    ClampValueTest<WInt32>(-1, 1000, 2, 4);
    ClampValueTest<WUInt64>(1, 1000, 2, 4);
    ClampValueTest<WTime>(WTime::MakeFromMilliseconds(1), WTime::MakeFromMilliseconds(1000), WTime::MakeFromMilliseconds(2), WTime::MakeFromMilliseconds(4));
    ClampValueTest<WAngle>(WAngle::MakeFromDegree(1), WAngle::MakeFromDegree(1000), WAngle::MakeFromDegree(2), WAngle::MakeFromDegree(4));
    ClampValueTest<WVec3>(WVec3(1), WVec3(1000), WVec3(2), WVec3(4));
    ClampValueTest<WVec4I32>(WVec4I32(1), WVec4I32(1000), WVec4I32(2), WVec4I32(4));
    ClampValueTest<WVec4U32>(WVec4U32(1), WVec4U32(1000), WVec4U32(2), WVec4U32(4));

    WVarianceTypeFloat vf = {1.0f, 2.0f};
    WVariant variance = vf;
    W_TEST_BOOL(WReflectionUtils::ClampValue(variance, nullptr).Succeeded());

    WVarianceTypeFloat clamp = {2.0f, 3.0f};
    WClampValueAttribute minClamp(clamp, {});
    W_TEST_BOOL(WReflectionUtils::ClampValue(variance, &minClamp).Failed());
  }
}
