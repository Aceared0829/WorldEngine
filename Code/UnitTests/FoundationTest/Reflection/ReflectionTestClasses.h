#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/VarianceTypes.h>

struct WExampleEnum
{
  using StorageType = WInt8;
  enum Enum
  {
    Value1 = 1,      // normal value
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

W_DECLARE_FLAGS_OPERATORS(WExampleBitflags);

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WExampleBitflags);


class WAbstractTestClass : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAbstractTestClass, WReflectedClass);

  virtual void AbstractFunction() = 0;
};


struct WAbstractTestStruct
{
  virtual void AbstractFunction() = 0;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WAbstractTestStruct);


struct WTestStruct
{
  W_ALLOW_PRIVATE_PROPERTIES(WTestStruct);

public:
  static WDataBuffer GetDefaultDataBuffer()
  {
    WDataBuffer data;
    data.PushBack(255);
    data.PushBack(0);
    data.PushBack(127);
    return data;
  }

  WTestStruct()
  {
    m_fFloat1 = 1.1f;
    m_iInt2 = 2;
    m_vProperty3.Set(3, 4, 5);
    m_UInt8 = 6;
    m_variant = "Test";
    m_Angle = WAngle::MakeFromDegree(0.5);
    m_DataBuffer = GetDefaultDataBuffer();
    m_vVec3I = WVec3I32(1, 2, 3);
    m_VarianceAngle.m_fVariance = 0.5f;
    m_VarianceAngle.m_Value = WAngle::MakeFromDegree(90.0f);
  }



  bool operator==(const WTestStruct& rhs) const
  {
    return m_fFloat1 == rhs.m_fFloat1 && m_UInt8 == rhs.m_UInt8 && m_variant == rhs.m_variant && m_iInt2 == rhs.m_iInt2 && m_vProperty3 == rhs.m_vProperty3 && m_Angle == rhs.m_Angle && m_DataBuffer == rhs.m_DataBuffer && m_vVec3I == rhs.m_vVec3I && m_VarianceAngle == rhs.m_VarianceAngle;
  }

  float m_fFloat1;
  WUInt8 m_UInt8;
  WVariant m_variant;
  WAngle m_Angle;
  WDataBuffer m_DataBuffer;
  WVec3I32 m_vVec3I;
  WVarianceTypeAngle m_VarianceAngle;

private:
  void SetInt(WInt32 i) { m_iInt2 = i; }
  WInt32 GetInt() const { return m_iInt2; }

  WInt32 m_iInt2;
  WVec3 m_vProperty3;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTestStruct);


struct WTestStruct3
{
  W_ALLOW_PRIVATE_PROPERTIES(WTestStruct3);

public:
  WTestStruct3()
  {
    m_fFloat1 = 1.1f;
    m_UInt8 = 6;
    m_iInt32 = 2;
  }
  WTestStruct3(double a, WInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8 = b;
    m_iInt32 = 32;
  }

  bool operator==(const WTestStruct3& rhs) const { return m_fFloat1 == rhs.m_fFloat1 && m_iInt32 == rhs.m_iInt32 && m_UInt8 == rhs.m_UInt8; }

  bool operator!=(const WTestStruct3& rhs) const { return !(*this == rhs); }

  double m_fFloat1;
  WInt16 m_UInt8;

  WUInt32 GetIntPublic() const { return m_iInt32; }

private:
  void SetInt(WUInt32 i) { m_iInt32 = i; }
  WUInt32 GetInt() const { return m_iInt32; }

  WInt32 m_iInt32;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTestStruct3);

struct WTypedObjectStruct
{
  W_ALLOW_PRIVATE_PROPERTIES(WTypedObjectStruct);

public:
  WTypedObjectStruct()
  {
    m_fFloat1 = 1.1f;
    m_UInt8 = 6;
    m_iInt32 = 2;
  }
  WTypedObjectStruct(double a, WInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8 = b;
    m_iInt32 = 32;
  }

  double m_fFloat1;
  WInt16 m_UInt8;
  WInt32 m_iInt32;
};
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTypedObjectStruct);
W_DECLARE_CUSTOM_VARIANT_TYPE(WTypedObjectStruct);

class WTestClass1 : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTestClass1, WReflectedClass);

public:
  WTestClass1()
  {
    m_MyVector.Set(3, 4, 5);

    m_Struct.m_fFloat1 = 33.3f;

    m_Color = WColor::CornflowerBlue; // The Original!
  }

  WTestClass1(const WColor& c, const WTestStruct& s)
  {
    m_Color = c;
    m_Struct = s;
    m_MyVector.Set(1, 2, 3);
  }

  bool operator==(const WTestClass1& rhs) const { return m_Struct == rhs.m_Struct && m_MyVector == rhs.m_MyVector && m_Color == rhs.m_Color; }

  WVec3 GetVector() const { return m_MyVector; }

  WTestStruct m_Struct;
  WVec3 m_MyVector;
  WColor m_Color;
};


class WTestClass2 : public WTestClass1
{
  W_ADD_DYNAMIC_REFLECTION(WTestClass2, WTestClass1);

public:
  WTestClass2()
  {
    m_sCharPtr = "AAA";
    m_sString = "BBB";
    m_sStringView = "CCC";
  }

  bool operator==(const WTestClass2& rhs) const { return m_Time == rhs.m_Time && m_enumClass == rhs.m_enumClass && m_bitflagsClass == rhs.m_bitflagsClass && m_array == rhs.m_array && m_Variant == rhs.m_Variant && m_sCharPtr == rhs.m_sCharPtr && m_sString == rhs.m_sString && m_sStringView == rhs.m_sStringView; }

  const char* GetCharPtr() const { return m_sCharPtr.GetData(); }
  void SetCharPtr(const char* szSz) { m_sCharPtr = szSz; }

  const WString& GetString() const { return m_sString; }
  void SetString(const WString& sStr) { m_sString = sStr; }

  WStringView GetStringView() const { return m_sStringView.GetView(); }
  void SetStringView(WStringView sStrView) { m_sStringView = sStrView; }

  WTime m_Time;
  WEnum<WExampleEnum> m_enumClass;
  WBitflags<WExampleBitflags> m_bitflagsClass;
  WHybridArray<float, 4> m_array;
  WVariant m_Variant;

private:
  WString m_sCharPtr;
  WString m_sString;
  WString m_sStringView;
};


struct WTestClass2Allocator : public WRTTIAllocator
{
  virtual WInternal::NewInstance<void> AllocateInternal(WAllocator* pAllocator) override
  {
    ++m_iAllocs;

    return W_DEFAULT_NEW(WTestClass2);
  }

  virtual void Deallocate(void* pObject, WAllocator* pAllocator) override
  {
    ++m_iDeallocs;

    WTestClass2* pPointer = (WTestClass2*)pObject;
    W_DEFAULT_DELETE(pPointer);
  }

  static WInt32 m_iAllocs;
  static WInt32 m_iDeallocs;
};


class WTestClass2b : WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTestClass2b, WReflectedClass);

public:
  WTestClass2b() { m_sText = "Tut"; }

  const char* GetText() const { return m_sText.GetData(); }
  void SetText(const char* szSz) { m_sText = szSz; }

  WTestStruct3 m_Struct;
  WColor m_Color;

private:
  WString m_sText;
};


class WTestArrays : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTestArrays, WReflectedClass);

public:
  WTestArrays() = default;

  bool operator==(const WTestArrays& rhs) const
  {
    return m_Hybrid == rhs.m_Hybrid && m_Dynamic == rhs.m_Dynamic && m_Deque == rhs.m_Deque && m_HybridChar == rhs.m_HybridChar && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const WTestArrays& rhs) const { return !(*this == rhs); }

  WUInt32 GetCount() const;
  double GetValue(WUInt32 uiIndex) const;
  void SetValue(WUInt32 uiIndex, double value);
  void Insert(WUInt32 uiIndex, double value);
  void Remove(WUInt32 uiIndex);

  WUInt32 GetCountChar() const;
  const char* GetValueChar(WUInt32 uiIndex) const;
  void SetValueChar(WUInt32 uiIndex, const char* value);
  void InsertChar(WUInt32 uiIndex, const char* value);
  void RemoveChar(WUInt32 uiIndex);

  WUInt32 GetCountDyn() const;
  const WTestStruct3& GetValueDyn(WUInt32 uiIndex) const;
  void SetValueDyn(WUInt32 uiIndex, const WTestStruct3& value);
  void InsertDyn(WUInt32 uiIndex, const WTestStruct3& value);
  void RemoveDyn(WUInt32 uiIndex);

  WUInt32 GetCountDeq() const;
  const WTestArrays& GetValueDeq(WUInt32 uiIndex) const;
  void SetValueDeq(WUInt32 uiIndex, const WTestArrays& value);
  void InsertDeq(WUInt32 uiIndex, const WTestArrays& value);
  void RemoveDeq(WUInt32 uiIndex);

  WUInt32 GetCountCustom() const;
  WVarianceTypeAngle GetValueCustom(WUInt32 uiIndex) const;
  void SetValueCustom(WUInt32 uiIndex, WVarianceTypeAngle value);
  void InsertCustom(WUInt32 uiIndex, WVarianceTypeAngle value);
  void RemoveCustom(WUInt32 uiIndex);

  WHybridArray<double, 5> m_Hybrid;
  WHybridArray<WString, 2> m_HybridChar;
  WDynamicArray<WTestStruct3> m_Dynamic;
  WDeque<WTestArrays> m_Deque;
  WHybridArray<WVarianceTypeAngle, 1> m_CustomVariant;
};


class WTestSets : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTestSets, WReflectedClass);

public:
  WTestSets() = default;

  bool operator==(const WTestSets& rhs) const
  {
    return m_SetMember == rhs.m_SetMember && m_SetAccessor == rhs.m_SetAccessor && m_Deque == rhs.m_Deque && m_Array == rhs.m_Array && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const WTestSets& rhs) const { return !(*this == rhs); }

  const WSet<double>& GetSet() const;
  void Insert(double value);
  void Remove(double value);

  const WHashSet<WInt64>& GetHashSet() const;
  void HashInsert(WInt64 value);
  void HashRemove(WInt64 value);

  const WDeque<int>& GetPseudoSet() const;
  void PseudoInsert(int value);
  void PseudoRemove(int value);

  WArrayPtr<const WString> GetPseudoSet2() const;
  void PseudoInsert2(const WString& value);
  void PseudoRemove2(const WString& value);

  void PseudoInsert2b(const char* value);
  void PseudoRemove2b(const char* value);

  const WHashSet<WVarianceTypeAngle>& GetCustomHashSet() const;
  void CustomHashInsert(WVarianceTypeAngle value);
  void CustomHashRemove(WVarianceTypeAngle value);

  WSet<WInt8> m_SetMember;
  WSet<double> m_SetAccessor;

  WHashSet<WInt32> m_HashSetMember;
  WHashSet<WInt64> m_HashSetAccessor;

  WDeque<int> m_Deque;
  WDynamicArray<WString> m_Array;
  WHashSet<WVarianceTypeAngle> m_CustomVariant;
};


class WTestMaps : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTestMaps, WReflectedClass);

public:
  WTestMaps() = default;

  bool operator==(const WTestMaps& rhs) const;

  const WMap<WString, WInt64>& GetContainer() const;
  void Insert(const char* szKey, WInt64 value);
  void Remove(const char* szKey);

  const WHashTable<WString, WString>& GetContainer2() const;
  void Insert2(const char* szKey, const WString& value);
  void Remove2(const char* szKey);

  const WRangeView<const char*, WUInt32> GetKeys3() const;
  void Insert3(const char* szKey, const WVariant& value);
  void Remove3(const char* szKey);
  bool GetValue3(const char* szKey, WVariant& out_value) const;

  WMap<WString, int> m_MapMember;
  WMap<WString, WInt64> m_MapAccessor;

  WHashTable<WString, double> m_HashTableMember;
  WHashTable<WString, WString> m_HashTableAccessor;

  WMap<WString, WVarianceTypeAngle> m_CustomVariant;

  struct Tuple
  {
    WString m_Key;
    WVariant m_Value;
  };
  WHybridArray<Tuple, 2> m_Accessor3;
};

class WTestPtr : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WTestPtr, WReflectedClass);

public:
  WTestPtr()
  {
    m_pArrays = nullptr;
    m_pArraysDirect = nullptr;
  }

  ~WTestPtr()
  {
    W_DEFAULT_DELETE(m_pArrays);
    W_DEFAULT_DELETE(m_pArraysDirect);
    for (auto ptr : m_ArrayPtr)
    {
      W_DEFAULT_DELETE(ptr);
    }
    m_ArrayPtr.Clear();
    for (auto ptr : m_SetPtr)
    {
      W_DEFAULT_DELETE(ptr);
    }
    m_SetPtr.Clear();
  }

  bool operator==(const WTestPtr& rhs) const
  {
    if (m_sString != rhs.m_sString || (m_pArrays != rhs.m_pArrays && *m_pArrays != *rhs.m_pArrays))
      return false;

    if (m_ArrayPtr.GetCount() != rhs.m_ArrayPtr.GetCount())
      return false;

    for (WUInt32 i = 0; i < m_ArrayPtr.GetCount(); i++)
    {
      if (!(*m_ArrayPtr[i] == *rhs.m_ArrayPtr[i]))
        return false;
    }

    // only works for the test data if the test.
    if (m_SetPtr.IsEmpty() && rhs.m_SetPtr.IsEmpty())
      return true;

    if (m_SetPtr.GetCount() != 1 || rhs.m_SetPtr.GetCount() != 1)
      return true;

    return *m_SetPtr.GetIterator().Key() == *rhs.m_SetPtr.GetIterator().Key();
  }

  void SetString(const char* szValue) { m_sString = szValue; }
  const char* GetString() const { return m_sString; }

  void SetArrays(WTestArrays* pValue) { m_pArrays = pValue; }
  WTestArrays* GetArrays() const { return m_pArrays; }


  WString m_sString;
  WTestArrays* m_pArrays;
  WTestArrays* m_pArraysDirect;
  WDeque<WTestArrays*> m_ArrayPtr;
  WSet<WTestSets*> m_SetPtr;
};


struct WTestEnumStruct
{
  W_ALLOW_PRIVATE_PROPERTIES(WTestEnumStruct);

public:
  WTestEnumStruct()
  {
    m_enum = WExampleEnum::Value1;
    m_enumClass = WExampleEnum::Value1;
    m_Enum2 = WExampleEnum::Value1;
    m_EnumClass2 = WExampleEnum::Value1;
  }

  bool operator==(const WTestEnumStruct& rhs) const { return m_Enum2 == rhs.m_Enum2 && m_enum == rhs.m_enum && m_enumClass == rhs.m_enumClass && m_EnumClass2 == rhs.m_EnumClass2; }

  WExampleEnum::Enum m_enum;
  WEnum<WExampleEnum> m_enumClass;

  void SetEnum(WExampleEnum::Enum e) { m_Enum2 = e; }
  WExampleEnum::Enum GetEnum() const { return m_Enum2; }
  void SetEnumClass(WEnum<WExampleEnum> e) { m_EnumClass2 = e; }
  WEnum<WExampleEnum> GetEnumClass() const { return m_EnumClass2; }

private:
  WExampleEnum::Enum m_Enum2;
  WEnum<WExampleEnum> m_EnumClass2;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTestEnumStruct);


struct WTestBitflagsStruct
{
  W_ALLOW_PRIVATE_PROPERTIES(WTestBitflagsStruct);

public:
  WTestBitflagsStruct()
  {
    m_bitflagsClass = WExampleBitflags::Value1;
    m_BitflagsClass2 = WExampleBitflags::Value1;
  }

  bool operator==(const WTestBitflagsStruct& rhs) const { return m_bitflagsClass == rhs.m_bitflagsClass && m_BitflagsClass2 == rhs.m_BitflagsClass2; }

  WBitflags<WExampleBitflags> m_bitflagsClass;

  void SetBitflagsClass(WBitflags<WExampleBitflags> e) { m_BitflagsClass2 = e; }
  WBitflags<WExampleBitflags> GetBitflagsClass() const { return m_BitflagsClass2; }

private:
  WBitflags<WExampleBitflags> m_BitflagsClass2;
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WTestBitflagsStruct);
