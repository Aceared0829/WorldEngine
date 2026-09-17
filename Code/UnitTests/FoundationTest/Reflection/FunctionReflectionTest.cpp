#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Reflection/ReflectionUtils.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

struct FunctionTest
{
  int StandardTypeFunction(int v, const WVec2 vCv, WVec3& ref_vRv, const WVec4& vCrv, WVec2U32* pPv, const WVec3U32* pCpv)
  {
    W_TEST_BOOL(m_values[0] == v);
    W_TEST_BOOL(m_values[1] == vCv);
    W_TEST_BOOL(m_values[2] == ref_vRv);
    W_TEST_BOOL(m_values[3] == vCrv);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPv);
      W_TEST_BOOL(!pCpv);
    }
    else
    {
      W_TEST_BOOL(m_values[4] == *pPv);
      W_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_vRv.Set(1, 2, 3);
    if (pPv)
    {
      pPv->Set(1, 2);
    }
    return 5;
  }

  WVarianceTypeAngle CustomTypeFunction(WVarianceTypeAngle v, const WVarianceTypeAngle cv, WVarianceTypeAngle& ref_rv, const WVarianceTypeAngle& crv, WVarianceTypeAngle* pPv, const WVarianceTypeAngle* pCpv)
  {
    W_TEST_BOOL(m_values[0] == v);
    W_TEST_BOOL(m_values[1] == cv);
    W_TEST_BOOL(m_values[2] == ref_rv);
    W_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPv);
      W_TEST_BOOL(!pCpv);
    }
    else
    {
      W_TEST_BOOL(m_values[4] == *pPv);
      W_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = WVarianceTypeAngle(WAngle::MakeFromDegree(200.0f), 2.0f);
    if (pPv)
    {
      *pPv = WVarianceTypeAngle(WAngle::MakeFromDegree(400.0f), 4.0f);
    }
    return WVarianceTypeAngle(WAngle::MakeFromDegree(60.0f), 0.6f);
  }

  WVarianceTypeAngle CustomTypeFunction2(WVarianceTypeAngle v, const WVarianceTypeAngle cv, WVarianceTypeAngle& ref_rv, const WVarianceTypeAngle& crv, WVarianceTypeAngle* pPv, const WVarianceTypeAngle* pCpv)
  {
    W_TEST_BOOL(*m_values[0].Get<WVarianceTypeAngle*>() == v);
    W_TEST_BOOL(*m_values[1].Get<WVarianceTypeAngle*>() == cv);
    W_TEST_BOOL(*m_values[2].Get<WVarianceTypeAngle*>() == ref_rv);
    W_TEST_BOOL(*m_values[3].Get<WVarianceTypeAngle*>() == crv);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPv);
      W_TEST_BOOL(!pCpv);
    }
    else
    {
      W_TEST_BOOL(*m_values[4].Get<WVarianceTypeAngle*>() == *pPv);
      W_TEST_BOOL(*m_values[5].Get<WVarianceTypeAngle*>() == *pCpv);
    }
    ref_rv = WVarianceTypeAngle(WAngle::MakeFromDegree(200.0f), 2.0f);
    if (pPv)
    {
      *pPv = WVarianceTypeAngle(WAngle::MakeFromDegree(400.0f), 4.0f);
    }
    return WVarianceTypeAngle(WAngle::MakeFromDegree(60.0f), 0.6f);
  }

  const char* StringTypeFunction(const char* szString, WString& ref_sString, WStringView sView)
  {
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!szString);
    }
    else
    {
      W_TEST_BOOL(m_values[0] == szString);
    }
    W_TEST_BOOL(m_values[1] == ref_sString);
    W_TEST_BOOL(m_values[2] == sView);
    return "StringRet";
  }

  WEnum<WExampleEnum> EnumFunction(
    WEnum<WExampleEnum> e, WEnum<WExampleEnum>& ref_re, const WEnum<WExampleEnum>& cre, WEnum<WExampleEnum>* pPe, const WEnum<WExampleEnum>* pCpe)
  {
    W_TEST_BOOL(m_values[0].Get<WInt64>() == e.GetValue());
    W_TEST_BOOL(m_values[1].Get<WInt64>() == ref_re.GetValue());
    W_TEST_BOOL(m_values[2].Get<WInt64>() == cre.GetValue());
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPe);
      W_TEST_BOOL(!pCpe);
    }
    else
    {
      W_TEST_BOOL(m_values[3].Get<WInt64>() == pPe->GetValue());
      W_TEST_BOOL(m_values[4].Get<WInt64>() == pCpe->GetValue());
    }
    return WExampleEnum::Value1;
  }

  WBitflags<WExampleBitflags> BitflagsFunction(WBitflags<WExampleBitflags> e, WBitflags<WExampleBitflags>& ref_re,
    const WBitflags<WExampleBitflags>& cre, WBitflags<WExampleBitflags>* pPe, const WBitflags<WExampleBitflags>* pCpe)
  {
    W_TEST_BOOL(e == m_values[0].Get<WInt64>());
    W_TEST_BOOL(ref_re == m_values[1].Get<WInt64>());
    W_TEST_BOOL(cre == m_values[2].Get<WInt64>());
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPe);
      W_TEST_BOOL(!pCpe);
    }
    else
    {
      W_TEST_BOOL(*pPe == m_values[3].Get<WInt64>());
      W_TEST_BOOL(*pCpe == m_values[4].Get<WInt64>());
    }
    return WExampleBitflags::Value1 | WExampleBitflags::Value2;
  }

  WTestStruct3 StructFunction(
    WTestStruct3 s, const WTestStruct3 cs, WTestStruct3& ref_rs, const WTestStruct3& crs, WTestStruct3* pPs, const WTestStruct3* pCps)
  {
    W_TEST_BOOL(*static_cast<WTestStruct3*>(m_values[0].Get<void*>()) == s);
    W_TEST_BOOL(*static_cast<WTestStruct3*>(m_values[1].Get<void*>()) == cs);
    W_TEST_BOOL(*static_cast<WTestStruct3*>(m_values[2].Get<void*>()) == ref_rs);
    W_TEST_BOOL(*static_cast<WTestStruct3*>(m_values[3].Get<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPs);
      W_TEST_BOOL(!pCps);
    }
    else
    {
      W_TEST_BOOL(*static_cast<WTestStruct3*>(m_values[4].Get<void*>()) == *pPs);
      W_TEST_BOOL(*static_cast<WTestStruct3*>(m_values[5].Get<void*>()) == *pCps);
    }
    ref_rs.m_fFloat1 = 999.0f;
    ref_rs.m_UInt8 = 666;
    if (pPs)
    {
      pPs->m_fFloat1 = 666.0f;
      pPs->m_UInt8 = 999;
    }
    WTestStruct3 retS;
    retS.m_fFloat1 = 42;
    retS.m_UInt8 = 42;
    return retS;
  }

  WTestClass1 ReflectedClassFunction(
    WTestClass1 s, const WTestClass1 cs, WTestClass1& ref_rs, const WTestClass1& crs, WTestClass1* pPs, const WTestClass1* pCps)
  {
    W_TEST_BOOL(*static_cast<WTestClass1*>(m_values[0].ConvertTo<void*>()) == s);
    W_TEST_BOOL(*static_cast<WTestClass1*>(m_values[1].ConvertTo<void*>()) == cs);
    W_TEST_BOOL(*static_cast<WTestClass1*>(m_values[2].ConvertTo<void*>()) == ref_rs);
    W_TEST_BOOL(*static_cast<WTestClass1*>(m_values[3].ConvertTo<void*>()) == crs);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pPs);
      W_TEST_BOOL(!pCps);
    }
    else
    {
      W_TEST_BOOL(*static_cast<WTestClass1*>(m_values[4].ConvertTo<void*>()) == *pPs);
      W_TEST_BOOL(*static_cast<WTestClass1*>(m_values[5].ConvertTo<void*>()) == *pCps);
    }
    ref_rs.m_Color.SetRGB(1, 2, 3);
    ref_rs.m_MyVector.Set(1, 2, 3);
    if (pPs)
    {
      pPs->m_Color.SetRGB(1, 2, 3);
      pPs->m_MyVector.Set(1, 2, 3);
    }
    WTestClass1 retS;
    retS.m_Color.SetRGB(42, 42, 42);
    retS.m_MyVector.Set(42, 42, 42);
    return retS;
  }

  WVariant VariantFunction(WVariant v, const WVariant cv, WVariant& ref_rv, const WVariant& crv, WVariant* pPv, const WVariant* pCpv)
  {
    W_TEST_BOOL(m_values[0] == v);
    W_TEST_BOOL(m_values[1] == cv);
    W_TEST_BOOL(m_values[2] == ref_rv);
    W_TEST_BOOL(m_values[3] == crv);
    if (m_bPtrAreNull)
    {
      // Can't have variant as nullptr as it must exist in the array and there is no further
      // way of distinguishing a between a WVariant* and a WVariant that is invalid.
      W_TEST_BOOL(!pPv->IsValid());
      W_TEST_BOOL(!pCpv->IsValid());
    }
    else
    {
      W_TEST_BOOL(m_values[4] == *pPv);
      W_TEST_BOOL(m_values[5] == *pCpv);
    }
    ref_rv = WVec3(1, 2, 3);
    if (pPv)
    {
      *pPv = WVec2U32(1, 2);
    }
    return 5;
  }

  WVariantArray VariantArrayFunction(WVariantArray a, const WVariantArray ca, WVariantArray& ref_a, const WVariantArray& cra, WVariantArray* pA, const WVariantArray* pCa)
  {
    W_TEST_BOOL(m_values[0].Get<WVariantArray>() == a);
    W_TEST_BOOL(m_values[1].Get<WVariantArray>() == ca);
    W_TEST_BOOL(m_values[2].Get<WVariantArray>() == ref_a);
    W_TEST_BOOL(m_values[3].Get<WVariantArray>() == cra);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pA);
      W_TEST_BOOL(!pCa);
    }
    else
    {
      W_TEST_BOOL(m_values[4] == *pA);
      W_TEST_BOOL(m_values[5] == *pCa);
    }
    ref_a.Clear();
    ref_a.PushBack(1.0f);
    ref_a.PushBack("Test");
    if (pA)
    {
      pA->Clear();
      pA->PushBack(2.0f);
      pA->PushBack("Test2");
    }

    WVariantArray ret;
    ret.PushBack(3.0f);
    ret.PushBack("RetTest");
    return ret;
  }

  WVariantDictionary VariantDictionaryFunction(WVariantDictionary a, const WVariantDictionary ca, WVariantDictionary& ref_a, const WVariantDictionary& cra, WVariantDictionary* pA, const WVariantDictionary* pCa)
  {
    W_TEST_BOOL(m_values[0].Get<WVariantDictionary>() == a);
    W_TEST_BOOL(m_values[1].Get<WVariantDictionary>() == ca);
    W_TEST_BOOL(m_values[2].Get<WVariantDictionary>() == ref_a);
    W_TEST_BOOL(m_values[3].Get<WVariantDictionary>() == cra);
    if (m_bPtrAreNull)
    {
      W_TEST_BOOL(!pA);
      W_TEST_BOOL(!pCa);
    }
    else
    {
      W_TEST_BOOL(m_values[4] == *pA);
      W_TEST_BOOL(m_values[5] == *pCa);
    }
    ref_a.Clear();
    ref_a.Insert("f", 1.0f);
    ref_a.Insert("s", "Test");
    if (pA)
    {
      pA->Clear();
      pA->Insert("f", 2.0f);
      pA->Insert("s", "Test2");
    }

    WVariantDictionary ret;
    ret.Insert("f", 3.0f);
    ret.Insert("s", "RetTest");
    return ret;
  }

  static void StaticFunction(bool b, WVariant v)
  {
    W_TEST_BOOL(b == true);
    W_TEST_BOOL(v == 4.0f);
  }

  static int StaticFunction2() { return 42; }

  bool m_bPtrAreNull = false;
  WDynamicArray<WVariant> m_values;
};

using ParamSig = std::tuple<const WRTTI*, WBitflags<WPropertyFlags>>;

void VerifyFunctionSignature(const WAbstractFunctionProperty* pFunc, WArrayPtr<ParamSig> params, ParamSig ret)
{
  W_TEST_INT(params.GetCount(), pFunc->GetArgumentCount());
  for (WUInt32 i = 0; i < WMath::Min(params.GetCount(), pFunc->GetArgumentCount()); i++)
  {
    W_TEST_BOOL(pFunc->GetArgumentType(i) == std::get<0>(params[i]));
    W_TEST_BOOL(pFunc->GetArgumentFlags(i) == std::get<1>(params[i]));
  }
  W_TEST_BOOL(pFunc->GetReturnType() == std::get<0>(ret));
  W_TEST_BOOL(pFunc->GetReturnFlags() == std::get<1>(ret));
}

W_CREATE_SIMPLE_TEST(Reflection, Functions)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - StandardTypes")
  {
    WFunctionProperty<decltype(&FunctionTest::StandardTypeFunction)> funccall("", &FunctionTest::StandardTypeFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<int>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<WVec2>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<WVec3>(), WPropertyFlags::StandardType | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVec4>(), WPropertyFlags::StandardType | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVec2U32>(), WPropertyFlags::StandardType | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WVec3U32>(), WPropertyFlags::StandardType | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<int>(), WPropertyFlags::StandardType));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(WVec2(2));
    test.m_values.PushBack(WVec3(3));
    test.m_values.PushBack(WVec4(4));
    test.m_values.PushBack(WVec2U32(5));
    test.m_values.PushBack(WVec3U32(6));

    WVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int32);
    W_TEST_BOOL(ret == 5);
    W_TEST_BOOL(test.m_values[2] == WVec3(1, 2, 3));
    W_TEST_BOOL(test.m_values[4] == WVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4] = WVariant();
    test.m_values[5] = WVariant();
    ret = WVariant();
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int32);
    W_TEST_BOOL(ret == 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - CustomType")
  {
    WFunctionProperty<decltype(&FunctionTest::CustomTypeFunction)> funccall("", &FunctionTest::CustomTypeFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WVarianceTypeAngle>(), WPropertyFlags::Class));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    {
      FunctionTest test;
      test.m_values.PushBack(WVarianceTypeAngle(WAngle::MakeFromDegree(0.0f), 0.0f));
      test.m_values.PushBack(WVarianceTypeAngle(WAngle::MakeFromDegree(10.0f), 0.1f));
      test.m_values.PushBack(WVarianceTypeAngle(WAngle::MakeFromDegree(20.0f), 0.2f));
      test.m_values.PushBack(WVarianceTypeAngle(WAngle::MakeFromDegree(30.0f), 0.3f));
      test.m_values.PushBack(WVarianceTypeAngle(WAngle::MakeFromDegree(40.0f), 0.4f));
      test.m_values.PushBack(WVarianceTypeAngle(WAngle::MakeFromDegree(50.0f), 0.5f));

      WVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::TypedObject);
      W_TEST_BOOL(ret == WVariant(WVarianceTypeAngle(WAngle::MakeFromDegree(60.0f), 0.6f)));
      W_TEST_BOOL(test.m_values[2] == WVariant(WVarianceTypeAngle(WAngle::MakeFromDegree(200.0f), 2.0f)));
      W_TEST_BOOL(test.m_values[4] == WVariant(WVarianceTypeAngle(WAngle::MakeFromDegree(400.0f), 4.0f)));

      test.m_bPtrAreNull = true;
      test.m_values[4] = WVariant();
      test.m_values[5] = WVariant();
      ret = WVariant();
      funccall.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::TypedObject);
      W_TEST_BOOL(ret == WVariant(WVarianceTypeAngle(WAngle::MakeFromDegree(60.0f), 0.6f)));
    }

    {
      WFunctionProperty<decltype(&FunctionTest::CustomTypeFunction2)> funccall2("", &FunctionTest::CustomTypeFunction2);

      FunctionTest test;
      WVarianceTypeAngle v0(WAngle::MakeFromDegree(0.0f), 0.0f);
      WVarianceTypeAngle v1(WAngle::MakeFromDegree(10.0f), 0.1f);
      WVarianceTypeAngle v2(WAngle::MakeFromDegree(20.0f), 0.2f);
      WVarianceTypeAngle v3(WAngle::MakeFromDegree(30.0f), 0.3f);
      WVarianceTypeAngle v4(WAngle::MakeFromDegree(40.0f), 0.4f);
      WVarianceTypeAngle v5(WAngle::MakeFromDegree(50.0f), 0.5f);
      test.m_values.PushBack(&v0);
      test.m_values.PushBack(&v1);
      test.m_values.PushBack(&v2);
      test.m_values.PushBack(&v3);
      test.m_values.PushBack(&v4);
      test.m_values.PushBack(&v5);

      WVariant ret;
      funccall2.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::TypedObject);
      W_TEST_BOOL(ret == WVariant(WVarianceTypeAngle(WAngle::MakeFromDegree(60.0f), 0.6f)));
      W_TEST_BOOL((*test.m_values[2].Get<WVarianceTypeAngle*>() == WVarianceTypeAngle(WAngle::MakeFromDegree(200.0f), 2.0f)));
      W_TEST_BOOL((*test.m_values[4].Get<WVarianceTypeAngle*>() == WVarianceTypeAngle(WAngle::MakeFromDegree(400.0f), 4.0f)));

      test.m_bPtrAreNull = true;
      test.m_values[4] = WVariant();
      test.m_values[5] = WVariant();
      ret = WVariant();
      funccall2.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::TypedObject);
      W_TEST_BOOL(ret == WVariant(WVarianceTypeAngle(WAngle::MakeFromDegree(60.0f), 0.6f)));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - Strings")
  {
    WFunctionProperty<decltype(&FunctionTest::StringTypeFunction)> funccall("", &FunctionTest::StringTypeFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<const char*>(), WPropertyFlags::StandardType | WPropertyFlags::Const),
      ParamSig(WGetStaticRTTI<WString>(), WPropertyFlags::StandardType | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WStringView>(), WPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<const char*>(), WPropertyFlags::StandardType | WPropertyFlags::Const));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(WVariant(WString("String0")));
    test.m_values.PushBack(WVariant(WString("String1")));
    test.m_values.PushBack(WVariant(WStringView("String2"), false));

    {
      // Exact types
      WVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::String);
      W_TEST_BOOL(ret == WString("StringRet"));
    }

    {
      // Using WString instead of WStringView
      test.m_values[2] = WString("String2");
      WVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::String);
      W_TEST_BOOL(ret == WString("StringRet"));
      test.m_values[2] = WVariant(WStringView("String2"), false);
    }

    {
      // Using nullptr instead of const char*
      test.m_bPtrAreNull = true;
      test.m_values[0] = WVariant();
      WVariant ret;
      funccall.Execute(&test, test.m_values, ret);
      W_TEST_BOOL(ret.GetType() == WVariantType::String);
      W_TEST_BOOL(ret == WString("StringRet"));
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - Enum")
  {
    WFunctionProperty<decltype(&FunctionTest::EnumFunction)> funccall("", &FunctionTest::EnumFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WExampleEnum>(), WPropertyFlags::IsEnum),
      ParamSig(WGetStaticRTTI<WExampleEnum>(), WPropertyFlags::IsEnum | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WExampleEnum>(), WPropertyFlags::IsEnum | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WExampleEnum>(), WPropertyFlags::IsEnum | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WExampleEnum>(), WPropertyFlags::IsEnum | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WExampleEnum>(), WPropertyFlags::IsEnum));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack((WInt64)WExampleEnum::Value1);
    test.m_values.PushBack((WInt64)WExampleEnum::Value2);
    test.m_values.PushBack((WInt64)WExampleEnum::Value3);
    test.m_values.PushBack((WInt64)WExampleEnum::Default);
    test.m_values.PushBack((WInt64)WExampleEnum::Value3);

    WVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int64);
    W_TEST_BOOL(ret == (WInt64)WExampleEnum::Value1);

    test.m_bPtrAreNull = true;
    test.m_values[3] = WVariant();
    test.m_values[4] = WVariant();
    ret = WVariant();
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int64);
    W_TEST_BOOL(ret == (WInt64)WExampleEnum::Value1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - Bitflags")
  {
    WFunctionProperty<decltype(&FunctionTest::BitflagsFunction)> funccall("", &FunctionTest::BitflagsFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WExampleBitflags>(), WPropertyFlags::Bitflags),
      ParamSig(WGetStaticRTTI<WExampleBitflags>(), WPropertyFlags::Bitflags | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WExampleBitflags>(), WPropertyFlags::Bitflags | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WExampleBitflags>(), WPropertyFlags::Bitflags | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WExampleBitflags>(), WPropertyFlags::Bitflags | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WExampleBitflags>(), WPropertyFlags::Bitflags));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack((WInt64)(0));
    test.m_values.PushBack((WInt64)(WExampleBitflags::Value2));
    test.m_values.PushBack((WInt64)(WExampleBitflags::Value3 | WExampleBitflags::Value2).GetValue());
    test.m_values.PushBack((WInt64)(WExampleBitflags::Value1 | WExampleBitflags::Value2 | WExampleBitflags::Value3).GetValue());
    test.m_values.PushBack((WInt64)(WExampleBitflags::Value3));

    WVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int64);
    W_TEST_BOOL(ret == (WInt64)(WExampleBitflags::Value1 | WExampleBitflags::Value2).GetValue());

    test.m_bPtrAreNull = true;
    test.m_values[3] = WVariant();
    test.m_values[4] = WVariant();
    ret = WVariant();
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int64);
    W_TEST_BOOL(ret == (WInt64)(WExampleBitflags::Value1 | WExampleBitflags::Value2).GetValue());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - Structs")
  {
    WFunctionProperty<decltype(&FunctionTest::StructFunction)> funccall("", &FunctionTest::StructFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    WTestStruct3 retS;
    retS.m_fFloat1 = 0;
    retS.m_UInt8 = 0;
    WTestStruct3 value;
    value.m_fFloat1 = 0;
    value.m_UInt8 = 0;
    WTestStruct3 rs;
    rs.m_fFloat1 = 42;
    WTestStruct3 ps;
    ps.m_fFloat1 = 18;

    test.m_values.PushBack(WVariant(&value));
    test.m_values.PushBack(WVariant(&value));
    test.m_values.PushBack(WVariant(&rs));
    test.m_values.PushBack(WVariant(&value));
    test.m_values.PushBack(WVariant(&ps));
    test.m_values.PushBack(WVariant(&value));

    // WVariantAdapter<WTestStruct3 const*> aa(WVariant(&value));
    // auto bla = WIsStandardType<WTestStruct3 const*>::value;

    WVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_FLOAT(retS.m_fFloat1, 42, 0);
    W_TEST_INT(retS.m_UInt8, 42);

    W_TEST_FLOAT(rs.m_fFloat1, 999, 0);
    W_TEST_INT(rs.m_UInt8, 666);

    W_TEST_DOUBLE(ps.m_fFloat1, 666, 0);
    W_TEST_INT(ps.m_UInt8, 999);

    test.m_bPtrAreNull = true;
    test.m_values[4] = WVariant();
    test.m_values[5] = WVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - Reflected Classes")
  {
    WFunctionProperty<decltype(&FunctionTest::ReflectedClassFunction)> funccall("", &FunctionTest::ReflectedClassFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    WTestClass1 retS;
    retS.m_Color = WColor::Chocolate;
    WTestClass1 value;
    value.m_Color = WColor::AliceBlue;
    WTestClass1 rs;
    rs.m_Color = WColor::Beige;
    WTestClass1 ps;
    ps.m_Color = WColor::DarkBlue;

    test.m_values.PushBack(WVariant(&value));
    test.m_values.PushBack(WVariant(&value));
    test.m_values.PushBack(WVariant(&rs));
    test.m_values.PushBack(WVariant(&value));
    test.m_values.PushBack(WVariant(&ps));
    test.m_values.PushBack(WVariant(&value));

    rs.m_Color.SetRGB(1, 2, 3);
    rs.m_MyVector.Set(1, 2, 3);


    WVariant ret(&retS);
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(retS.m_Color == WColor(42, 42, 42));
    W_TEST_BOOL(retS.m_MyVector == WVec3(42, 42, 42));

    W_TEST_BOOL(rs.m_Color == WColor(1, 2, 3));
    W_TEST_BOOL(rs.m_MyVector == WVec3(1, 2, 3));

    W_TEST_BOOL(ps.m_Color == WColor(1, 2, 3));
    W_TEST_BOOL(ps.m_MyVector == WVec3(1, 2, 3));

    test.m_bPtrAreNull = true;
    test.m_values[4] = WVariant();
    test.m_values[5] = WVariant();
    funccall.Execute(&test, test.m_values, ret);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - Variant")
  {
    WFunctionProperty<decltype(&FunctionTest::VariantFunction)> funccall("", &FunctionTest::VariantFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    FunctionTest test;
    test.m_values.PushBack(1);
    test.m_values.PushBack(WVec2(2));
    test.m_values.PushBack(WVec3(3));
    test.m_values.PushBack(WVec4(4));
    test.m_values.PushBack(WVec2U32(5));
    test.m_values.PushBack(WVec3U32(6));

    WVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int32);
    W_TEST_BOOL(ret == 5);
    W_TEST_BOOL(test.m_values[2] == WVec3(1, 2, 3));
    W_TEST_BOOL(test.m_values[4] == WVec2U32(1, 2));

    test.m_bPtrAreNull = true;
    test.m_values[4] = WVariant();
    test.m_values[5] = WVariant();
    ret = WVariant();
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int32);
    W_TEST_BOOL(ret == 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - VariantArray")
  {
    WFunctionProperty<decltype(&FunctionTest::VariantArrayFunction)> funccall("", &FunctionTest::VariantArrayFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WVariantArray>(), WPropertyFlags::Class));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    WVariantArray testA;
    testA.PushBack(WVec3(3));
    testA.PushBack(WTime::MakeFromHours(22));
    testA.PushBack("Hello");

    FunctionTest test;
    for (WUInt32 i = 0; i < 6; ++i)
    {
      test.m_values.PushBack(testA);
      testA.PushBack(i);
    }

    WVariantArray expectedOutRef;
    expectedOutRef.PushBack(1.0f);
    expectedOutRef.PushBack("Test");

    WVariantArray expectedOutPtr;
    expectedOutPtr.PushBack(2.0f);
    expectedOutPtr.PushBack("Test2");

    WVariantArray expectedRet;
    expectedRet.PushBack(3.0f);
    expectedRet.PushBack("RetTest");

    WVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::VariantArray);
    W_TEST_BOOL(ret.Get<WVariantArray>() == expectedRet);
    W_TEST_BOOL(test.m_values[2] == expectedOutRef);
    W_TEST_BOOL(test.m_values[4] == expectedOutPtr);

    test.m_bPtrAreNull = true;
    test.m_values[4] = WVariant();
    test.m_values[5] = WVariant();
    ret = WVariant();
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::VariantArray);
    W_TEST_BOOL(ret.Get<WVariantArray>() == expectedRet);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Functions - VariantDictionary")
  {
    WFunctionProperty<decltype(&FunctionTest::VariantDictionaryFunction)> funccall("", &FunctionTest::VariantDictionaryFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class),
      ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class | WPropertyFlags::Pointer),
      ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Pointer),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WVariantDictionary>(), WPropertyFlags::Class));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Member);

    WVariantDictionary testA;
    testA.Insert("v", WVec3(3));
    testA.Insert("t", WTime::MakeFromHours(22));
    testA.Insert("s", "Hello");

    WStringBuilder tmp;
    FunctionTest test;
    for (WUInt32 i = 0; i < 6; ++i)
    {
      test.m_values.PushBack(testA);
      testA.Insert(WConversionUtils::ToString(i, tmp), i);
    }

    WVariantDictionary expectedOutRef;
    expectedOutRef.Insert("f", 1.0f);
    expectedOutRef.Insert("s", "Test");

    WVariantDictionary expectedOutPtr;
    expectedOutPtr.Insert("f", 2.0f);
    expectedOutPtr.Insert("s", "Test2");

    WVariantDictionary expectedRet;
    expectedRet.Insert("f", 3.0f);
    expectedRet.Insert("s", "RetTest");

    WVariant ret;
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::VariantDictionary);
    W_TEST_BOOL(ret.Get<WVariantDictionary>() == expectedRet);
    W_TEST_BOOL(test.m_values[2] == expectedOutRef);
    W_TEST_BOOL(test.m_values[4] == expectedOutPtr);

    test.m_bPtrAreNull = true;
    test.m_values[4] = WVariant();
    test.m_values[5] = WVariant();
    ret = WVariant();
    funccall.Execute(&test, test.m_values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::VariantDictionary);
    W_TEST_BOOL(ret.Get<WVariantDictionary>() == expectedRet);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Static Functions")
  {
    // Void return
    WFunctionProperty<decltype(&FunctionTest::StaticFunction)> funccall("", &FunctionTest::StaticFunction);
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<bool>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<WVariant>(), WPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(&funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<void>(), WPropertyFlags::Void));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::StaticMember);

    WDynamicArray<WVariant> values;
    values.PushBack(true);
    values.PushBack(4.0f);
    WVariant ret;
    funccall.Execute(nullptr, values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Invalid);

    // Zero parameter
    WFunctionProperty<decltype(&FunctionTest::StaticFunction2)> funccall2("", &FunctionTest::StaticFunction2);
    VerifyFunctionSignature(&funccall2, WArrayPtr<ParamSig>(), ParamSig(WGetStaticRTTI<int>(), WPropertyFlags::StandardType));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::StaticMember);
    values.Clear();
    funccall2.Execute(nullptr, values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Int32);
    W_TEST_BOOL(ret == 42);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor Functions - StandardTypes")
  {
    WConstructorFunctionProperty<WVec4, float, float, float, float> funccall;
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<float>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<float>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<float>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<float>(), WPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WVec4>(), WPropertyFlags::StandardType | WPropertyFlags::Pointer));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Constructor);

    WDynamicArray<WVariant> values;
    values.PushBack(1.0f);
    values.PushBack(2.0f);
    values.PushBack(3.0f);
    values.PushBack(4.0f);
    WVariant ret;
    funccall.Execute(nullptr, values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::Vector4);
    W_TEST_BOOL(ret == WVec4(1.0f, 2.0f, 3.0f, 4.0f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor Functions - Struct")
  {
    WConstructorFunctionProperty<WTestStruct3, double, WInt16> funccall;
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<double>(), WPropertyFlags::StandardType),
      ParamSig(WGetStaticRTTI<WInt16>(), WPropertyFlags::StandardType),
    };
    VerifyFunctionSignature(
      &funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WTestStruct3>(), WPropertyFlags::Class | WPropertyFlags::Pointer));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Constructor);

    WDynamicArray<WVariant> values;
    values.PushBack(59.0);
    values.PushBack((WInt16)666);
    WVariant ret;
    funccall.Execute(nullptr, values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::TypedPointer);
    WTestStruct3* pRet = static_cast<WTestStruct3*>(ret.ConvertTo<void*>());
    W_TEST_BOOL(pRet != nullptr);

    W_TEST_FLOAT(pRet->m_fFloat1, 59.0, 0);
    W_TEST_INT(pRet->m_UInt8, 666);
    W_TEST_INT(pRet->GetIntPublic(), 32);

    W_DEFAULT_DELETE(pRet);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constructor Functions - Reflected Classes")
  {
    // The function signature does not actually need to match the ctor 100% as long as implicit conversion is possible.
    WConstructorFunctionProperty<WTestClass1, const WColor&, const WTestStruct&> funccall;
    ParamSig testSet[] = {
      ParamSig(WGetStaticRTTI<WColor>(), WPropertyFlags::StandardType | WPropertyFlags::Const | WPropertyFlags::Reference),
      ParamSig(WGetStaticRTTI<WTestStruct>(), WPropertyFlags::Class | WPropertyFlags::Const | WPropertyFlags::Reference),
    };
    VerifyFunctionSignature(
      &funccall, WArrayPtr<ParamSig>(testSet), ParamSig(WGetStaticRTTI<WTestClass1>(), WPropertyFlags::Class | WPropertyFlags::Pointer));
    W_TEST_BOOL(funccall.GetFunctionType() == WFunctionType::Constructor);

    WDynamicArray<WVariant> values;
    WTestStruct s;
    s.m_fFloat1 = 1.0f;
    s.m_UInt8 = 255;
    values.PushBack(WColor::CornflowerBlue);
    values.PushBack(WVariant(&s));
    WVariant ret;
    funccall.Execute(nullptr, values, ret);
    W_TEST_BOOL(ret.GetType() == WVariantType::TypedPointer);
    WTestClass1* pRet = static_cast<WTestClass1*>(ret.ConvertTo<void*>());
    W_TEST_BOOL(pRet != nullptr);

    W_TEST_BOOL(pRet->m_Color == WColor::CornflowerBlue);
    W_TEST_BOOL(pRet->m_Struct == s);
    W_TEST_BOOL(pRet->m_MyVector == WVec3(1, 2, 3));

    W_DEFAULT_DELETE(pRet);
  }
}
