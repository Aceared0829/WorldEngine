#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>

struct WVariantTestStruct
{
public:
  WVariant m_Variant;
  WVariantArray m_VariantArray;
  WVariantDictionary m_VariantDictionary;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WVariantTestStruct);

struct WIntegerStruct
{
public:
  WIntegerStruct()
  {
    m_iInt8 = 1;
    m_uiUInt8 = 1;
    m_iInt16 = 1;
    m_iUInt16 = 1;
    m_iInt32 = 1;
    m_uiUInt32 = 1;
    m_iInt64 = 1;
    m_iUInt64 = 1;
  }

  void SetInt8(WInt8 i) { m_iInt8 = i; }
  WInt8 GetInt8() const { return m_iInt8; }
  void SetUInt8(WUInt8 i) { m_uiUInt8 = i; }
  WUInt8 GetUInt8() const { return m_uiUInt8; }
  void SetInt32(WInt32 i) { m_iInt32 = i; }
  WInt32 GetInt32() const { return m_iInt32; }
  void SetUInt32(WUInt32 i) { m_uiUInt32 = i; }
  WUInt32 GetUInt32() const { return m_uiUInt32; }

  WInt16 m_iInt16;
  WUInt16 m_iUInt16;
  WInt64 m_iInt64;
  WUInt64 m_iUInt64;

private:
  WInt8 m_iInt8;
  WUInt8 m_uiUInt8;
  WInt32 m_iInt32;
  WUInt32 m_uiUInt32;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WIntegerStruct);


struct WFloatStruct
{
public:
  WFloatStruct()
  {
    m_fFloat = 1.0f;
    m_fDouble = 1.0;
    m_Time = WTime::MakeFromSeconds(1.0);
    m_Angle = WAngle::MakeFromDegree(45.0f);
  }

  void SetFloat(float f) { m_fFloat = f; }
  float GetFloat() const { return m_fFloat; }
  void SetDouble(double d) { m_fDouble = d; }
  double GetDouble() const { return m_fDouble; }
  void SetTime(WTime t) { m_Time = t; }
  WTime GetTime() const { return m_Time; }
  WAngle GetAngle() const { return m_Angle; }
  void SetAngle(WAngle t) { m_Angle = t; }

private:
  float m_fFloat;
  double m_fDouble;
  WTime m_Time;
  WAngle m_Angle;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WFloatStruct);


class WPODClass : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WPODClass, WReflectedClass);

public:
  WPODClass()
  {
    m_bBool = true;
    m_Color = WColor(1.0f, 0.0f, 0.0f, 0.0f);
    m_Color2 = WColorGammaUB(255, 10, 1);
    m_sCharPtr = "Test";
    m_sString = "Test";
    m_sStringView = "Test";
    m_Buffer.PushBack(0xFF);
    m_Buffer.PushBack(0x0);
    m_Buffer.PushBack(0xCD);
    m_VarianceAngle = WVarianceTypeAngle(WAngle::MakeFromDegree(90.0f), 0.1f);
  }

  WIntegerStruct m_IntegerStruct;
  WFloatStruct m_FloatStruct;

  bool GetBool() const { return m_bBool; }
  void SetBool(bool b) { m_bBool = b; }

  WColor GetColor() const { return m_Color; }
  void SetColor(WColor c) { m_Color = c; }

  const char* GetCharPtr() const { return m_sCharPtr.GetData(); }
  void SetCharPtr(const char* szSz) { m_sCharPtr = szSz; }

  const WString& GetString() const { return m_sString; }
  void SetString(const WString& sStr) { m_sString = sStr; }

  WStringView GetStringView() const { return m_sStringView.GetView(); }
  void SetStringView(WStringView sStrView) { m_sStringView = sStrView; }

  const WDataBuffer& GetBuffer() const { return m_Buffer; }
  void SetBuffer(const WDataBuffer& data) { m_Buffer = data; }

  WVarianceTypeAngle GetCustom() const { return m_VarianceAngle; }
  void SetCustom(WVarianceTypeAngle value) { m_VarianceAngle = value; }

private:
  bool m_bBool;
  WColor m_Color;
  WColorGammaUB m_Color2;
  WString m_sCharPtr;
  WString m_sString;
  WString m_sStringView;
  WDataBuffer m_Buffer;
  WVarianceTypeAngle m_VarianceAngle;
};


class WMathClass : public WPODClass
{
  W_ADD_DYNAMIC_REFLECTION(WMathClass, WPODClass);

public:
  WMathClass()
  {
    m_vVec2 = WVec2(1.0f, 1.0f);
    m_vVec3 = WVec3(1.0f, 1.0f, 1.0f);
    m_vVec4 = WVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Vec2I = WVec2I32(1, 1);
    m_Vec3I = WVec3I32(1, 1, 1);
    m_Vec4I = WVec4I32(1, 1, 1, 1);
    m_qQuat = WQuat(1.0f, 1.0f, 1.0f, 1.0f);
    m_mMat3.SetZero();
    m_mMat4.SetZero();
  }

  void SetVec2(WVec2 v) { m_vVec2 = v; }
  WVec2 GetVec2() const { return m_vVec2; }
  void SetVec3(WVec3 v) { m_vVec3 = v; }
  WVec3 GetVec3() const { return m_vVec3; }
  void SetVec4(WVec4 v) { m_vVec4 = v; }
  WVec4 GetVec4() const { return m_vVec4; }
  void SetQuat(WQuat q) { m_qQuat = q; }
  WQuat GetQuat() const { return m_qQuat; }
  void SetMat3(WMat3 m) { m_mMat3 = m; }
  WMat3 GetMat3() const { return m_mMat3; }
  void SetMat4(WMat4 m) { m_mMat4 = m; }
  WMat4 GetMat4() const { return m_mMat4; }

  WVec2I32 m_Vec2I;
  WVec3I32 m_Vec3I;
  WVec4I32 m_Vec4I;

private:
  WVec2 m_vVec2;
  WVec3 m_vVec3;
  WVec4 m_vVec4;
  WQuat m_qQuat;
  WMat3 m_mMat3;
  WMat4 m_mMat4;
};


struct WExampleEnum
{
  using StorageType = WInt8;
  enum Enum
  {
    Value1 = 0,      // normal value
    Value2 = -2,     // normal value
    Value3 = 4,      // normal value
    Default = Value1 // Default initialization value (required)
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WExampleEnum);


struct WExampleBitflags
{
  using StorageType = WUInt64;
  enum Enum : WUInt64
  {
    Value1 = W_BIT(0),  // normal value
    Value2 = W_BIT(31), // normal value
    Value3 = W_BIT(63), // normal value
    Default = Value1     // Default initialization value (required)
  };

  struct Bits
  {
    StorageType Value1 : 1;
    StorageType Padding : 30;
    StorageType Value2 : 1;
    StorageType Padding2 : 31;
    StorageType Value3 : 1;
  };
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WExampleBitflags);


class WEnumerationsClass : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WEnumerationsClass, WReflectedClass);

public:
  WEnumerationsClass()
  {
    m_EnumClass = WExampleEnum::Value2;
    m_BitflagsClass = WExampleBitflags::Value2;
  }

  void SetEnum(WExampleEnum::Enum e) { m_EnumClass = e; }
  WExampleEnum::Enum GetEnum() const { return m_EnumClass; }
  void SetBitflags(WBitflags<WExampleBitflags> e) { m_BitflagsClass = e; }
  WBitflags<WExampleBitflags> GetBitflags() const { return m_BitflagsClass; }

private:
  WEnum<WExampleEnum> m_EnumClass;
  WBitflags<WExampleBitflags> m_BitflagsClass;
};


struct InnerStruct
{
  W_DECLARE_POD_TYPE();

public:
  float m_fP1;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, InnerStruct);


class OuterClass : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(OuterClass, WReflectedClass);

public:
  InnerStruct m_Inner1;
  float m_fP1;
};

class ExtendedOuterClass : public OuterClass
{
  W_ADD_DYNAMIC_REFLECTION(ExtendedOuterClass, OuterClass);

public:
  WString m_more;
};

class WObjectTest : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WObjectTest, WReflectedClass);

public:
  WObjectTest() = default;
  ~WObjectTest()
  {
    for (OuterClass* pTest : m_ClassPtrArray)
    {
      WGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(pTest);
    }
    for (WObjectTest* pTest : m_SubObjectSet)
    {
      WGetStaticRTTI<WObjectTest>()->GetAllocator()->Deallocate(pTest);
    }
    for (auto it = m_ClassPtrMap.GetIterator(); it.IsValid(); ++it)
    {
      WGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(it.Value());
    }
  }

  WArrayPtr<const WString> GetStandardTypeSet() const;
  void StandardTypeSetInsert(const WString& value);
  void StandardTypeSetRemove(const WString& value);

  OuterClass m_MemberClass;

  WDynamicArray<double> m_StandardTypeArray;
  WDynamicArray<OuterClass> m_ClassArray;
  WDeque<OuterClass*> m_ClassPtrArray;

  WDynamicArray<WString> m_StandardTypeSet;
  WSet<WObjectTest*> m_SubObjectSet;

  WMap<WString, double> m_StandardTypeMap;
  WHashTable<WString, OuterClass> m_ClassMap;
  WMap<WString, OuterClass*> m_ClassPtrMap;
};


class WMirrorTest : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WMirrorTest, WReflectedClass);

public:
  WMirrorTest() = default;

  WMathClass m_math;
  WObjectTest m_object;
};
