#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>


template <typename T>
void TestSerialization(const T& source)
{
  WDefaultMemoryStreamStorage StreamStorage;

  W_TEST_BLOCK(WTestBlock::Enabled, "WriteObjectToDDL")
  {
    WMemoryStreamWriter FileOut(&StreamStorage);

    WReflectionSerializer::WriteObjectToDDL(FileOut, WGetStaticRTTI<T>(), &source, false, WOpenDdlWriter::TypeStringMode::Compliant);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectPropertiesFromDDL")
  {
    WMemoryStreamReader FileIn(&StreamStorage);
    T data;
    WReflectionSerializer::ReadObjectPropertiesFromDDL(FileIn, *WGetStaticRTTI<T>(), &data);

    W_TEST_BOOL(data == source);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectFromDDL")
  {
    WMemoryStreamReader FileIn(&StreamStorage);

    const WRTTI* pRtti;
    void* pObject = WReflectionSerializer::ReadObjectFromDDL(FileIn, pRtti);

    T& c2 = *((T*)pObject);

    W_TEST_BOOL(c2 == source);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  WDefaultMemoryStreamStorage StreamStorageBinary;
  W_TEST_BLOCK(WTestBlock::Enabled, "WriteObjectToBinary")
  {
    WMemoryStreamWriter FileOut(&StreamStorageBinary);

    WReflectionSerializer::WriteObjectToBinary(FileOut, WGetStaticRTTI<T>(), &source);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectPropertiesFromBinary")
  {
    WMemoryStreamReader FileIn(&StreamStorageBinary);
    T data;
    WReflectionSerializer::ReadObjectPropertiesFromBinary(FileIn, *WGetStaticRTTI<T>(), &data);

    W_TEST_BOOL(data == source);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "ReadObjectFromBinary")
  {
    WMemoryStreamReader FileIn(&StreamStorageBinary);

    const WRTTI* pRtti;
    void* pObject = WReflectionSerializer::ReadObjectFromBinary(FileIn, pRtti);

    T& c2 = *((T*)pObject);

    W_TEST_BOOL(c2 == source);

    if (pObject)
    {
      pRtti->GetAllocator()->Deallocate(pObject);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Clone")
  {
    {
      T clone;
      WReflectionSerializer::Clone(&source, &clone, WGetStaticRTTI<T>());
      W_TEST_BOOL(clone == source);
      W_TEST_BOOL(WReflectionUtils::IsEqual(&clone, &source, WGetStaticRTTI<T>()));
    }

    {
      T* pClone = WReflectionSerializer::Clone(&source);
      W_TEST_BOOL(*pClone == source);
      W_TEST_BOOL(WReflectionUtils::IsEqual(pClone, &source));
      WGetStaticRTTI<T>()->GetAllocator()->Deallocate(pClone);
    }
  }
}


W_CREATE_SIMPLE_TEST_GROUP(Reflection);

W_CREATE_SIMPLE_TEST(Reflection, Types)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Iterate All")
  {
    bool bFoundStruct = false;
    bool bFoundClass1 = false;
    bool bFoundClass2 = false;

    WRTTI::ForEachType([&](const WRTTI* pRtti)
      {
        if (pRtti->GetTypeName() == "WTestStruct")
          bFoundStruct = true;
        if (pRtti->GetTypeName() == "WTestClass1")
          bFoundClass1 = true;
        if (pRtti->GetTypeName() == "WTestClass2")
          bFoundClass2 = true;

        W_TEST_STRING(pRtti->GetPluginName(), "Static"); });

    W_TEST_BOOL(bFoundStruct);
    W_TEST_BOOL(bFoundClass1);
    W_TEST_BOOL(bFoundClass2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "IsDerivedFrom")
  {
    WDynamicArray<const WRTTI*> allTypes;
    WRTTI::ForEachType([&](const WRTTI* pRtti)
      { allTypes.PushBack(pRtti); });

    // ground truth - traversing up the parent list
    auto ManualIsDerivedFrom = [](const WRTTI* t, const WRTTI* pBaseType) -> bool
    {
      while (t != nullptr)
      {
        if (t == pBaseType)
          return true;

        t = t->GetParentType();
      }

      return false;
    };

    // test each type against every other:
    for (const WRTTI* typeA : allTypes)
    {
      for (const WRTTI* typeB : allTypes)
      {
        bool derived = typeA->IsDerivedFrom(typeB);
        bool manualCheck = ManualIsDerivedFrom(typeA, typeB);
        W_TEST_BOOL(derived == manualCheck);
      }
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "PropertyFlags")
  {
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<void>() == (WPropertyFlags::Void));
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<const char*>() == (WPropertyFlags::StandardType | WPropertyFlags::Const));
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<int>() == WPropertyFlags::StandardType);
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<int&>() == (WPropertyFlags::StandardType | WPropertyFlags::Reference));
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<int*>() == (WPropertyFlags::StandardType | WPropertyFlags::Pointer));

    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<const int>() == (WPropertyFlags::StandardType | WPropertyFlags::Const));
    W_TEST_BOOL(
      WPropertyFlags::GetParameterFlags<const int&>() == (WPropertyFlags::StandardType | WPropertyFlags::Reference | WPropertyFlags::Const));
    W_TEST_BOOL(
      WPropertyFlags::GetParameterFlags<const int*>() == (WPropertyFlags::StandardType | WPropertyFlags::Pointer | WPropertyFlags::Const));

    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<WVariant>() == (WPropertyFlags::StandardType));

    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<WExampleEnum::Enum>() == WPropertyFlags::IsEnum);
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<WEnum<WExampleEnum>>() == WPropertyFlags::IsEnum);
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<WBitflags<WExampleBitflags>>() == WPropertyFlags::Bitflags);

    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<WTestStruct3>() == WPropertyFlags::Class);
    W_TEST_BOOL(WPropertyFlags::GetParameterFlags<WTestClass2>() == WPropertyFlags::Class);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "TypeFlags")
  {
    W_TEST_INT(WGetStaticRTTI<bool>()->GetTypeFlags().GetValue(), WTypeFlags::StandardType);
    W_TEST_INT(WGetStaticRTTI<WUuid>()->GetTypeFlags().GetValue(), WTypeFlags::StandardType);
    W_TEST_INT(WGetStaticRTTI<const char*>()->GetTypeFlags().GetValue(), WTypeFlags::StandardType);
    W_TEST_INT(WGetStaticRTTI<WString>()->GetTypeFlags().GetValue(), WTypeFlags::StandardType);
    W_TEST_INT(WGetStaticRTTI<WMat4>()->GetTypeFlags().GetValue(), WTypeFlags::StandardType);
    W_TEST_INT(WGetStaticRTTI<WVariant>()->GetTypeFlags().GetValue(), WTypeFlags::StandardType);

    W_TEST_INT(WGetStaticRTTI<WAbstractTestClass>()->GetTypeFlags().GetValue(), (WTypeFlags::Class | WTypeFlags::Abstract).GetValue());
    W_TEST_INT(WGetStaticRTTI<WAbstractTestStruct>()->GetTypeFlags().GetValue(), (WTypeFlags::Class | WTypeFlags::Abstract).GetValue());

    W_TEST_INT(WGetStaticRTTI<WTestStruct3>()->GetTypeFlags().GetValue(), WTypeFlags::Class);
    W_TEST_INT(WGetStaticRTTI<WTestClass2>()->GetTypeFlags().GetValue(), WTypeFlags::Class);

    W_TEST_INT(WGetStaticRTTI<WExampleEnum>()->GetTypeFlags().GetValue(), WTypeFlags::IsEnum);
    W_TEST_INT(WGetStaticRTTI<WExampleBitflags>()->GetTypeFlags().GetValue(), WTypeFlags::Bitflags);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindTypeByName")
  {
    const WRTTI* pFloat = WRTTI::FindTypeByName("float");
    if (W_TEST_BOOL(pFloat != nullptr))
    {
      W_ANALYSIS_ASSUME(pFloat != nullptr);
      W_TEST_STRING(pFloat->GetTypeName(), "float");
    }

    const WRTTI* pStruct = WRTTI::FindTypeByName("WTestStruct");
    if (W_TEST_BOOL(pStruct != nullptr))
    {
      W_ANALYSIS_ASSUME(pStruct != nullptr);
      W_TEST_STRING(pStruct->GetTypeName(), "WTestStruct");
    }

    const WRTTI* pClass2 = WRTTI::FindTypeByName("WTestClass2");
    if (W_TEST_BOOL(pClass2 != nullptr))
    {
      W_ANALYSIS_ASSUME(pClass2 != nullptr);
      W_TEST_STRING(pClass2->GetTypeName(), "WTestClass2");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FindTypeByNameHash")
  {
    const WRTTI* pFloat = WRTTI::FindTypeByName("float");
    const WRTTI* pFloat2 = WRTTI::FindTypeByNameHash(pFloat->GetTypeNameHash());
    W_TEST_BOOL(pFloat == pFloat2);

    const WRTTI* pStruct = WRTTI::FindTypeByName("WTestStruct");
    const WRTTI* pStruct2 = WRTTI::FindTypeByNameHash(pStruct->GetTypeNameHash());
    W_TEST_BOOL(pStruct == pStruct2);

    const WRTTI* pClass = WRTTI::FindTypeByName("WTestClass2");
    const WRTTI* pClass2 = WRTTI::FindTypeByNameHash(pClass->GetTypeNameHash());
    W_TEST_BOOL(pClass == pClass2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "GetProperties")
  {
    {
      const WRTTI* pType = WRTTI::FindTypeByName("WTestStruct");

      auto Props = pType->GetProperties();
      W_TEST_INT(Props.GetCount(), 9);
      W_TEST_STRING(Props[0]->GetPropertyName(), "Float");
      W_TEST_STRING(Props[1]->GetPropertyName(), "Vector");
      W_TEST_STRING(Props[2]->GetPropertyName(), "Int");
      W_TEST_STRING(Props[3]->GetPropertyName(), "UInt8");
      W_TEST_STRING(Props[4]->GetPropertyName(), "Variant");
      W_TEST_STRING(Props[5]->GetPropertyName(), "Angle");
      W_TEST_STRING(Props[6]->GetPropertyName(), "DataBuffer");
      W_TEST_STRING(Props[7]->GetPropertyName(), "vVec3I");
      W_TEST_STRING(Props[8]->GetPropertyName(), "VarianceAngle");
    }

    {
      const WRTTI* pType = WRTTI::FindTypeByName("WTestClass2");

      auto Props = pType->GetProperties();
      W_TEST_INT(Props.GetCount(), 8);
      W_TEST_STRING(Props[0]->GetPropertyName(), "CharPtr");
      W_TEST_STRING(Props[1]->GetPropertyName(), "String");
      W_TEST_STRING(Props[2]->GetPropertyName(), "StringView");
      W_TEST_STRING(Props[3]->GetPropertyName(), "Time");
      W_TEST_STRING(Props[4]->GetPropertyName(), "Enum");
      W_TEST_STRING(Props[5]->GetPropertyName(), "Bitflags");
      W_TEST_STRING(Props[6]->GetPropertyName(), "Array");
      W_TEST_STRING(Props[7]->GetPropertyName(), "Variant");

      WTempHybridArray<const WAbstractProperty*, 32> AllProps;
      pType->GetAllProperties(AllProps);

      W_TEST_INT(AllProps.GetCount(), 11);
      W_TEST_STRING(AllProps[0]->GetPropertyName(), "SubStruct");
      W_TEST_STRING(AllProps[1]->GetPropertyName(), "Color");
      W_TEST_STRING(AllProps[2]->GetPropertyName(), "SubVector");
      W_TEST_STRING(AllProps[3]->GetPropertyName(), "CharPtr");
      W_TEST_STRING(AllProps[4]->GetPropertyName(), "String");
      W_TEST_STRING(AllProps[5]->GetPropertyName(), "StringView");
      W_TEST_STRING(AllProps[6]->GetPropertyName(), "Time");
      W_TEST_STRING(AllProps[7]->GetPropertyName(), "Enum");
      W_TEST_STRING(AllProps[8]->GetPropertyName(), "Bitflags");
      W_TEST_STRING(AllProps[9]->GetPropertyName(), "Array");
      W_TEST_STRING(AllProps[10]->GetPropertyName(), "Variant");
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Casts")
  {
    WTestClass2 test;
    WTestClass1* pTestClass1 = &test;
    const WTestClass1* pConstTestClass1 = &test;

    WTestClass2* pTestClass2 = WStaticCast<WTestClass2*>(pTestClass1);
    const WTestClass2* pConstTestClass2 = WStaticCast<const WTestClass2*>(pConstTestClass1);

    pTestClass2 = WDynamicCast<WTestClass2*>(pTestClass1);
    pConstTestClass2 = WDynamicCast<const WTestClass2*>(pConstTestClass1);
    W_TEST_BOOL(pTestClass2 != nullptr);
    W_TEST_BOOL(pConstTestClass2 != nullptr);

    WTestClass1 otherTest;
    pTestClass1 = &otherTest;
    pConstTestClass1 = &otherTest;

    pTestClass2 = WDynamicCast<WTestClass2*>(pTestClass1);
    pConstTestClass2 = WDynamicCast<const WTestClass2*>(pConstTestClass1);
    W_TEST_BOOL(pTestClass2 == nullptr);
    W_TEST_BOOL(pConstTestClass2 == nullptr);
  }

#if W_ENABLED(W_SUPPORTS_DYNAMIC_PLUGINS) && W_ENABLED(W_COMPILE_ENGINE_AS_DLL)

  W_TEST_BLOCK(WTestBlock::Enabled, "Types From Plugin")
  {
    WResult loadPlugin = WPlugin::LoadPlugin(WFoundationTest_Plugin1);
    W_TEST_BOOL(loadPlugin == W_SUCCESS);

    if (loadPlugin.Failed())
      return;

    const WRTTI* pStruct2 = WRTTI::FindTypeByName("WTestStruct2");
    W_TEST_BOOL(pStruct2 != nullptr);

    if (pStruct2)
    {
      W_TEST_STRING(pStruct2->GetTypeName(), "WTestStruct2");
    }

    bool bFoundStruct2 = false;

    WRTTI::ForEachType(
      [&](const WRTTI* pRtti)
      {
        if (pRtti->GetTypeName() == "WTestStruct2")
        {
          bFoundStruct2 = true;

          W_TEST_STRING(pRtti->GetPluginName(), WFoundationTest_Plugin1);

          void* pInstance = pRtti->GetAllocator()->Allocate<void>();
          W_TEST_BOOL(pInstance != nullptr);

          const WAbstractProperty* pProp = pRtti->FindPropertyByName("Float2");

          if (W_TEST_BOOL(pProp != nullptr))
          {
            W_ANALYSIS_ASSUME(pProp != nullptr);
            W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);
            auto pAbsMember = static_cast<const WAbstractMemberProperty*>(pProp);

            W_TEST_BOOL(pAbsMember->GetSpecificType() == WGetStaticRTTI<float>());

            auto pMember = static_cast<const WTypedMemberProperty<float>*>(pAbsMember);

            W_TEST_FLOAT(pMember->GetValue(pInstance), 42.0f, 0);
            pMember->SetValue(pInstance, 43.0f);
            W_TEST_FLOAT(pMember->GetValue(pInstance), 43.0f, 0);
          }

          pRtti->GetAllocator()->Deallocate(pInstance);
        }
        else
        {
          W_TEST_STRING(pRtti->GetPluginName(), "Static");
        }
      });

    W_TEST_BOOL(bFoundStruct2);

    WPlugin::UnloadAllPlugins();
  }
#endif
}


W_CREATE_SIMPLE_TEST(Reflection, Hierarchies)
{
  WTestClass2Allocator::m_iAllocs = 0;
  WTestClass2Allocator::m_iDeallocs = 0;

  W_TEST_BLOCK(WTestBlock::Enabled, "WTestStruct")
  {
    const WRTTI* pRtti = WGetStaticRTTI<WTestStruct>();

    W_TEST_STRING(pRtti->GetTypeName(), "WTestStruct");
    W_TEST_INT(pRtti->GetTypeSize(), sizeof(WTestStruct));
    W_TEST_BOOL(pRtti->GetVariantType() == WVariant::Type::Invalid);

    W_TEST_BOOL(pRtti->GetParentType() == nullptr);

    W_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTestClass1")
  {
    const WRTTI* pRtti = WGetStaticRTTI<WTestClass1>();

    W_TEST_STRING(pRtti->GetTypeName(), "WTestClass1");
    W_TEST_INT(pRtti->GetTypeSize(), sizeof(WTestClass1));
    W_TEST_BOOL(pRtti->GetVariantType() == WVariant::Type::Invalid);

    W_TEST_BOOL(pRtti->GetParentType() == WGetStaticRTTI<WReflectedClass>());

    W_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    WTestClass1* pInstance = pRtti->GetAllocator()->Allocate<WTestClass1>();
    if (W_TEST_BOOL(pInstance != nullptr))
    {
      W_ANALYSIS_ASSUME(pInstance != nullptr);
      W_TEST_BOOL(pInstance->GetDynamicRTTI() == WGetStaticRTTI<WTestClass1>());
      pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);
    }

    W_TEST_BOOL(pRtti->IsDerivedFrom<WReflectedClass>());
    W_TEST_BOOL(pRtti->IsDerivedFrom(WGetStaticRTTI<WReflectedClass>()));

    W_TEST_BOOL(pRtti->IsDerivedFrom<WTestClass1>());
    W_TEST_BOOL(pRtti->IsDerivedFrom(WGetStaticRTTI<WTestClass1>()));

    W_TEST_BOOL(!pRtti->IsDerivedFrom<WVec3>());
    W_TEST_BOOL(!pRtti->IsDerivedFrom(WGetStaticRTTI<WVec3>()));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTestClass2")
  {
    const WRTTI* pRtti = WGetStaticRTTI<WTestClass2>();

    W_TEST_STRING(pRtti->GetTypeName(), "WTestClass2");
    W_TEST_INT(pRtti->GetTypeSize(), sizeof(WTestClass2));
    W_TEST_BOOL(pRtti->GetVariantType() == WVariant::Type::Invalid);

    W_TEST_BOOL(pRtti->GetParentType() == WGetStaticRTTI<WTestClass1>());

    W_TEST_BOOL(pRtti->GetAllocator()->CanAllocate());

    W_TEST_INT(WTestClass2Allocator::m_iAllocs, 0);
    W_TEST_INT(WTestClass2Allocator::m_iDeallocs, 0);

    WTestClass2* pInstance = pRtti->GetAllocator()->Allocate<WTestClass2>();
    if (W_TEST_BOOL(pInstance != nullptr))
    {
      W_ANALYSIS_ASSUME(pInstance != nullptr);
      W_TEST_BOOL(pInstance->GetDynamicRTTI() == WGetStaticRTTI<WTestClass2>());

      W_TEST_INT(WTestClass2Allocator::m_iAllocs, 1);
      W_TEST_INT(WTestClass2Allocator::m_iDeallocs, 0);

      pInstance->GetDynamicRTTI()->GetAllocator()->Deallocate(pInstance);

      W_TEST_INT(WTestClass2Allocator::m_iAllocs, 1);
      W_TEST_INT(WTestClass2Allocator::m_iDeallocs, 1);
    }

    W_TEST_BOOL(pRtti->IsDerivedFrom<WTestClass1>());
    W_TEST_BOOL(pRtti->IsDerivedFrom(WGetStaticRTTI<WTestClass1>()));

    W_TEST_BOOL(pRtti->IsDerivedFrom<WTestClass2>());
    W_TEST_BOOL(pRtti->IsDerivedFrom(WGetStaticRTTI<WTestClass2>()));

    W_TEST_BOOL(pRtti->IsDerivedFrom<WReflectedClass>());
    W_TEST_BOOL(pRtti->IsDerivedFrom(WGetStaticRTTI<WReflectedClass>()));

    W_TEST_BOOL(!pRtti->IsDerivedFrom<WVec3>());
    W_TEST_BOOL(!pRtti->IsDerivedFrom(WGetStaticRTTI<WVec3>()));
  }
}


template <typename T, typename T2>
void TestMemberProperty(const char* szPropName, void* pObject, const WRTTI* pRtti, WBitflags<WPropertyFlags> expectedFlags, T2 expectedValue, T2 testValue, bool bTestDefaultValue = true)
{
  const WAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!W_TEST_BOOL(pProp != nullptr))
    return;

  W_ANALYSIS_ASSUME(pProp != nullptr);

  W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);

  W_TEST_BOOL(pProp->GetSpecificType() == WGetStaticRTTI<T>());
  auto pMember = static_cast<const WTypedMemberProperty<T>*>(pProp);

  W_TEST_INT(pMember->GetFlags().GetValue(), expectedFlags.GetValue());

  T value = pMember->GetValue(pObject);
  W_TEST_BOOL(expectedValue == value);

  if (bTestDefaultValue)
  {
    // Default value
    WVariant defaultValue = WReflectionUtils::GetDefaultValue(pProp);
    W_TEST_BOOL(WVariant(expectedValue) == defaultValue);
  }

  if (!pMember->GetFlags().IsSet(WPropertyFlags::ReadOnly))
  {
    pMember->SetValue(pObject, testValue);

    W_TEST_BOOL(testValue == pMember->GetValue(pObject));

    WReflectionUtils::SetMemberPropertyValue(pMember, pObject, WVariant(expectedValue));
    WVariant res = WReflectionUtils::GetMemberPropertyValue(pMember, pObject);

    W_TEST_BOOL(res == WVariant(expectedValue));
    W_TEST_BOOL(res != WVariant(testValue));

    WReflectionUtils::SetMemberPropertyValue(pMember, pObject, WVariant(testValue));
    res = WReflectionUtils::GetMemberPropertyValue(pMember, pObject);

    W_TEST_BOOL(res != WVariant(expectedValue));
    W_TEST_BOOL(res == WVariant(testValue));
  }
}

W_CREATE_SIMPLE_TEST(Reflection, MemberProperties)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "WTestStruct")
  {
    WTestStruct data;
    const WRTTI* pRtti = WGetStaticRTTI<WTestStruct>();

    TestMemberProperty<float>("Float", &data, pRtti, WPropertyFlags::StandardType, 1.1f, 5.0f);
    TestMemberProperty<WInt32>("Int", &data, pRtti, WPropertyFlags::StandardType, 2, -8);
    TestMemberProperty<WVec3>("Vector", &data, pRtti, WPropertyFlags::StandardType | WPropertyFlags::ReadOnly, WVec3(3, 4, 5),
      WVec3(0, -1.0f, 3.14f));
    TestMemberProperty<WVariant>("Variant", &data, pRtti, WPropertyFlags::StandardType, WVariant("Test"),
      WVariant(WVec3(0, -1.0f, 3.14f)));
    TestMemberProperty<WAngle>("Angle", &data, pRtti, WPropertyFlags::StandardType, WAngle::MakeFromDegree(0.5f), WAngle::MakeFromDegree(1.0f));
    WVarianceTypeAngle expectedVA(WAngle::MakeFromDegree(90.0f), 0.5f);
    WVarianceTypeAngle testVA(WAngle::MakeFromDegree(45.0f), 0.1f);
    TestMemberProperty<WVarianceTypeAngle>("VarianceAngle", &data, pRtti, WPropertyFlags::Class, expectedVA, testVA);

    WDataBuffer expected;
    expected.PushBack(255);
    expected.PushBack(0);
    expected.PushBack(127);

    WDataBuffer newValue;
    newValue.PushBack(1);
    newValue.PushBack(2);

    TestMemberProperty<WDataBuffer>("DataBuffer", &data, pRtti, WPropertyFlags::StandardType, expected, newValue);
    TestMemberProperty<WVec3I32>("vVec3I", &data, pRtti, WPropertyFlags::StandardType, WVec3I32(1, 2, 3), WVec3I32(5, 6, 7));

    TestSerialization<WTestStruct>(data);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WTestClass2")
  {
    WTestClass2 Instance;
    const WRTTI* pRtti = WGetStaticRTTI<WTestClass2>();

    {
      TestMemberProperty<const char*>("CharPtr", &Instance, pRtti, WPropertyFlags::StandardType | WPropertyFlags::Const, WString("AAA"), WString("aaaa"));

      TestMemberProperty<WString>("String", &Instance, pRtti, WPropertyFlags::StandardType, WString("BBB"), WString("bbbb"));

      TestMemberProperty<WStringView>("StringView", &Instance, pRtti, WPropertyFlags::StandardType, "CCC"_wsv, "cccc"_wsv);

      Instance.SetStringView("CCC");
      TestMemberProperty<WStringView>("StringView", &Instance, pRtti, WPropertyFlags::StandardType, WString("CCC"), WString("cccc"));

      const WAbstractProperty* pProp = pRtti->FindPropertyByName("SubVector", false);
      W_TEST_BOOL(pProp == nullptr);
    }

    {
      TestMemberProperty<WVec3>("SubVector", &Instance, pRtti, WPropertyFlags::StandardType | WPropertyFlags::ReadOnly, WVec3(3, 4, 5), WVec3(3, 4, 5));
      const WAbstractProperty* pProp = pRtti->FindPropertyByName("SubStruct", false);
      W_TEST_BOOL(pProp == nullptr);
    }

    {
      const WAbstractProperty* pProp = pRtti->FindPropertyByName("SubStruct");
      if (W_TEST_BOOL(pProp != nullptr))
      {
        W_ANALYSIS_ASSUME(pProp != nullptr);
        W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);
        WAbstractMemberProperty* pAbs = (WAbstractMemberProperty*)pProp;

        const WRTTI* pStruct = pAbs->GetSpecificType();
        void* pSubStruct = pAbs->GetPropertyPointer(&Instance);

        W_TEST_BOOL(pSubStruct != nullptr);

        TestMemberProperty<float>("Float", pSubStruct, pStruct, WPropertyFlags::StandardType, 33.3f, 44.4f, false);
      }
    }

    TestSerialization<WTestClass2>(Instance);
  }
}


W_CREATE_SIMPLE_TEST(Reflection, Enum)
{
  const WRTTI* pEnumRTTI = WGetStaticRTTI<WExampleEnum>();
  const WRTTI* pRTTI = WGetStaticRTTI<WTestEnumStruct>();

  W_TEST_BLOCK(WTestBlock::Enabled, "Enum Constants")
  {
    W_TEST_BOOL(pEnumRTTI->IsDerivedFrom<WEnumBase>());
    auto props = pEnumRTTI->GetProperties();
    W_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Constant);
      W_TEST_BOOL(pProp->GetSpecificType() == WGetStaticRTTI<WInt8>());
    }
    W_TEST_INT(WExampleEnum::Default, WReflectionUtils::DefaultEnumerationValue(pEnumRTTI));

    W_TEST_STRING(props[0]->GetPropertyName(), "WExampleEnum::Default");
    W_TEST_STRING(props[1]->GetPropertyName(), "WExampleEnum::Value1");
    W_TEST_STRING(props[2]->GetPropertyName(), "WExampleEnum::Value2");
    W_TEST_STRING(props[3]->GetPropertyName(), "WExampleEnum::Value3");

    auto pTypedConstantProp0 = static_cast<const WTypedConstantProperty<WInt8>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<const WTypedConstantProperty<WInt8>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<const WTypedConstantProperty<WInt8>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<const WTypedConstantProperty<WInt8>*>(props[3]);
    W_TEST_INT(pTypedConstantProp0->GetValue(), WExampleEnum::Default);
    W_TEST_INT(pTypedConstantProp1->GetValue(), WExampleEnum::Value1);
    W_TEST_INT(pTypedConstantProp2->GetValue(), WExampleEnum::Value2);
    W_TEST_INT(pTypedConstantProp3->GetValue(), WExampleEnum::Value3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Enum Property")
  {
    WTestEnumStruct data;
    auto props = pRTTI->GetProperties();
    W_TEST_INT(props.GetCount(), 4);

    for (auto pProp : props)
    {
      W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);
      W_TEST_INT(pProp->GetFlags().GetValue(), WPropertyFlags::IsEnum);
      W_TEST_BOOL(pProp->GetSpecificType() == pEnumRTTI);
      auto pEnumProp = static_cast<const WAbstractEnumerationProperty*>(pProp);
      W_TEST_BOOL(pEnumProp->GetValue(&data) == WExampleEnum::Value1);

      const WRTTI* pEnumPropertyRTTI = pEnumProp->GetSpecificType();
      // Set and get all valid enum values.
      for (auto pProp2 : pEnumPropertyRTTI->GetProperties().GetSubArray(1))
      {
        auto pConstantProp = static_cast<const WTypedConstantProperty<WInt8>*>(pProp2);
        pEnumProp->SetValue(&data, pConstantProp->GetValue());
        W_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        // Enum <-> string
        WStringBuilder sValue;
        W_TEST_BOOL(WReflectionUtils::EnumerationToString(pEnumPropertyRTTI, pConstantProp->GetValue(), sValue));
        W_TEST_STRING(sValue, pConstantProp->GetPropertyName());

        // Setting the value via a string also works.
        pEnumProp->SetValue(&data, WExampleEnum::Value1);
        WReflectionUtils::SetMemberPropertyValue(pEnumProp, &data, sValue.GetData());
        W_TEST_INT(pEnumProp->GetValue(&data), pConstantProp->GetValue());

        WInt64 iValue = 0;
        W_TEST_BOOL(WReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        W_TEST_INT(iValue, pConstantProp->GetValue());

        // Testing the short enum name version
        W_TEST_BOOL(WReflectionUtils::EnumerationToString(
          pEnumPropertyRTTI, pConstantProp->GetValue(), sValue, WReflectionUtils::EnumConversionMode::ValueNameOnly));
        W_TEST_BOOL(sValue.IsEqual(pConstantProp->GetPropertyName()) ||
                     sValue.IsEqual(WStringUtils::FindLastSubString(pConstantProp->GetPropertyName(), "::") + 2));

        W_TEST_BOOL(WReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        W_TEST_INT(iValue, pConstantProp->GetValue());

        // Testing the short enum name version
        W_TEST_BOOL(WReflectionUtils::EnumerationToString(
          pEnumPropertyRTTI, pConstantProp->GetValue(), sValue, WReflectionUtils::EnumConversionMode::ValueNameOnly));
        W_TEST_BOOL(sValue.IsEqual(pConstantProp->GetPropertyName()) ||
                     sValue.IsEqual(WStringUtils::FindLastSubString(pConstantProp->GetPropertyName(), "::") + 2));

        W_TEST_BOOL(WReflectionUtils::StringToEnumeration(pEnumPropertyRTTI, sValue, iValue));
        W_TEST_INT(iValue, pConstantProp->GetValue());

        W_TEST_INT(iValue, WReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue));
        W_TEST_INT(WExampleEnum::Default, WReflectionUtils::MakeEnumerationValid(pEnumPropertyRTTI, iValue + 666));
      }
    }

    W_TEST_BOOL(data.m_enum == WExampleEnum::Value3);
    W_TEST_BOOL(data.m_enumClass == WExampleEnum::Value3);

    W_TEST_BOOL(data.GetEnum() == WExampleEnum::Value3);
    W_TEST_BOOL(data.GetEnumClass() == WExampleEnum::Value3);

    TestSerialization<WTestEnumStruct>(data);
  }
}


W_CREATE_SIMPLE_TEST(Reflection, Bitflags)
{
  const WRTTI* pBitflagsRTTI = WGetStaticRTTI<WExampleBitflags>();
  const WRTTI* pRTTI = WGetStaticRTTI<WTestBitflagsStruct>();

  W_TEST_BLOCK(WTestBlock::Enabled, "Bitflags Constants")
  {
    W_TEST_BOOL(pBitflagsRTTI->IsDerivedFrom<WBitflagsBase>());
    auto props = pBitflagsRTTI->GetProperties();
    W_TEST_INT(props.GetCount(), 4); // Default + 3

    for (auto pProp : props)
    {
      W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Constant);
      W_TEST_BOOL(pProp->GetSpecificType() == WGetStaticRTTI<WUInt64>());
    }
    W_TEST_INT(WExampleBitflags::Default, WReflectionUtils::DefaultEnumerationValue(pBitflagsRTTI));

    W_TEST_STRING(props[0]->GetPropertyName(), "WExampleBitflags::Default");
    W_TEST_STRING(props[1]->GetPropertyName(), "WExampleBitflags::Value1");
    W_TEST_STRING(props[2]->GetPropertyName(), "WExampleBitflags::Value2");
    W_TEST_STRING(props[3]->GetPropertyName(), "WExampleBitflags::Value3");

    auto pTypedConstantProp0 = static_cast<const WTypedConstantProperty<WUInt64>*>(props[0]);
    auto pTypedConstantProp1 = static_cast<const WTypedConstantProperty<WUInt64>*>(props[1]);
    auto pTypedConstantProp2 = static_cast<const WTypedConstantProperty<WUInt64>*>(props[2]);
    auto pTypedConstantProp3 = static_cast<const WTypedConstantProperty<WUInt64>*>(props[3]);
    W_TEST_BOOL(pTypedConstantProp0->GetValue() == WExampleBitflags::Default);
    W_TEST_BOOL(pTypedConstantProp1->GetValue() == WExampleBitflags::Value1);
    W_TEST_BOOL(pTypedConstantProp2->GetValue() == WExampleBitflags::Value2);
    W_TEST_BOOL(pTypedConstantProp3->GetValue() == WExampleBitflags::Value3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bitflags Property")
  {
    WTestBitflagsStruct data;
    auto props = pRTTI->GetProperties();
    W_TEST_INT(props.GetCount(), 2);

    for (auto pProp : props)
    {
      W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);
      W_TEST_BOOL(pProp->GetSpecificType() == pBitflagsRTTI);
      W_TEST_INT(pProp->GetFlags().GetValue(), WPropertyFlags::Bitflags);
      auto pBitflagsProp = static_cast<const WAbstractEnumerationProperty*>(pProp);
      W_TEST_BOOL(pBitflagsProp->GetValue(&data) == WExampleBitflags::Value1);

      const WRTTI* pBitflagsPropertyRTTI = pBitflagsProp->GetSpecificType();

      // Set and get all valid bitflags values. (skip default value)
      WUInt64 constants[] = {
        static_cast<const WTypedConstantProperty<WUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[1])->GetValue(),
        static_cast<const WTypedConstantProperty<WUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[2])->GetValue(),
        static_cast<const WTypedConstantProperty<WUInt64>*>(pBitflagsPropertyRTTI->GetProperties()[3])->GetValue(),
      };

      const char* stringValues[] = {"",
        "WExampleBitflags::Value1",
        "WExampleBitflags::Value2",
        "WExampleBitflags::Value1|WExampleBitflags::Value2",
        "WExampleBitflags::Value3",
        "WExampleBitflags::Value1|WExampleBitflags::Value3",
        "WExampleBitflags::Value2|WExampleBitflags::Value3",
        "WExampleBitflags::Value1|WExampleBitflags::Value2|WExampleBitflags::Value3"};

      const char* stringValuesShort[] = {"",
        "Value1",
        "Value2",
        "Value1|Value2",
        "Value3",
        "Value1|Value3",
        "Value2|Value3",
        "Value1|Value2|Value3"};
      for (WInt32 i = 0; i < 8; ++i)
      {
        WUInt64 uiBitflagValue = 0;
        uiBitflagValue |= (i & W_BIT(0)) != 0 ? constants[0] : 0;
        uiBitflagValue |= (i & W_BIT(1)) != 0 ? constants[1] : 0;
        uiBitflagValue |= (i & W_BIT(2)) != 0 ? constants[2] : 0;

        pBitflagsProp->SetValue(&data, uiBitflagValue);
        W_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        // Bitflags <-> string
        WStringBuilder sValue;
        W_TEST_BOOL(WReflectionUtils::EnumerationToString(pBitflagsPropertyRTTI, uiBitflagValue, sValue));
        W_TEST_STRING(sValue, stringValues[i]);

        // Setting the value via a string also works.
        pBitflagsProp->SetValue(&data, 0);
        WReflectionUtils::SetMemberPropertyValue(pBitflagsProp, &data, sValue.GetData());
        W_TEST_INT(pBitflagsProp->GetValue(&data), uiBitflagValue);

        WInt64 iValue = 0;
        W_TEST_BOOL(WReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        W_TEST_INT(iValue, uiBitflagValue);

        // Testing the short enum name version
        W_TEST_BOOL(WReflectionUtils::EnumerationToString(
          pBitflagsPropertyRTTI, uiBitflagValue, sValue, WReflectionUtils::EnumConversionMode::ValueNameOnly));
        W_TEST_BOOL(sValue.IsEqual(stringValuesShort[i]));

        W_TEST_BOOL(WReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        W_TEST_INT(iValue, uiBitflagValue);

        // Testing the short enum name version
        W_TEST_BOOL(WReflectionUtils::EnumerationToString(
          pBitflagsPropertyRTTI, uiBitflagValue, sValue, WReflectionUtils::EnumConversionMode::ValueNameOnly));
        W_TEST_BOOL(sValue.IsEqual(stringValuesShort[i]));

        W_TEST_BOOL(WReflectionUtils::StringToEnumeration(pBitflagsPropertyRTTI, sValue, iValue));
        W_TEST_INT(iValue, uiBitflagValue);

        W_TEST_INT(iValue, WReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue));
        W_TEST_INT(iValue, WReflectionUtils::MakeEnumerationValid(pBitflagsPropertyRTTI, iValue | W_BIT(16)));
      }
    }

    W_TEST_BOOL(data.m_bitflagsClass == (WExampleBitflags::Value1 | WExampleBitflags::Value2 | WExampleBitflags::Value3));
    W_TEST_BOOL(data.GetBitflagsClass() == (WExampleBitflags::Value1 | WExampleBitflags::Value2 | WExampleBitflags::Value3));
    TestSerialization<WTestBitflagsStruct>(data);
  }
}


template <typename T>
void TestArrayPropertyVariant(const WAbstractArrayProperty* pArrayProp, void* pObject, const WRTTI* pRtti, T& value)
{
  T temp = {};

  // Reflection Utils
  WVariant value0 = WReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 0);
  W_TEST_BOOL(value0 == WVariant(value));
  // insert
  WReflectionUtils::InsertArrayPropertyValue(pArrayProp, pObject, WVariant(temp), 2);
  W_TEST_INT(pArrayProp->GetCount(pObject), 3);
  WVariant value2 = WReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 2);
  W_TEST_BOOL(value0 != value2);
  WReflectionUtils::SetArrayPropertyValue(pArrayProp, pObject, 2, value);
  value2 = WReflectionUtils::GetArrayPropertyValue(pArrayProp, pObject, 2);
  W_TEST_BOOL(value0 == value2);
  // remove again
  WReflectionUtils::RemoveArrayPropertyValue(pArrayProp, pObject, 2);
  W_TEST_INT(pArrayProp->GetCount(pObject), 2);
}

template <>
void TestArrayPropertyVariant<WTestArrays>(const WAbstractArrayProperty* pArrayProp, void* pObject, const WRTTI* pRtti, WTestArrays& value)
{
}

template <>
void TestArrayPropertyVariant<WTestStruct3>(const WAbstractArrayProperty* pArrayProp, void* pObject, const WRTTI* pRtti, WTestStruct3& value)
{
}

template <typename T>
void TestArrayProperty(const char* szPropName, void* pObject, const WRTTI* pRtti, T& value)
{
  const WAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  W_TEST_BOOL(pProp != nullptr);
  if (pProp == nullptr)
    return;

  W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Array);
  auto pArrayProp = static_cast<const WAbstractArrayProperty*>(pProp);
  const WRTTI* pElemRtti = pProp->GetSpecificType();
  W_TEST_BOOL(pElemRtti == WGetStaticRTTI<T>());
  if (!pArrayProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
  {
    // If we don't know the element type T but we can allocate it, we can handle it anyway.
    if (pElemRtti->GetAllocator()->CanAllocate())
    {
      void* pData = pElemRtti->GetAllocator()->Allocate<void>();

      pArrayProp->SetCount(pObject, 2);
      W_TEST_INT(pArrayProp->GetCount(pObject), 2);
      // Push default constructed object in both slots.
      pArrayProp->SetValue(pObject, 0, pData);
      pArrayProp->SetValue(pObject, 1, pData);

      // Retrieve it again and compare to function parameter, they should be different.
      pArrayProp->GetValue(pObject, 0, pData);
      W_TEST_BOOL(*static_cast<T*>(pData) != value);
      pArrayProp->GetValue(pObject, 1, pData);
      W_TEST_BOOL(*static_cast<T*>(pData) != value);

      pElemRtti->GetAllocator()->Deallocate(pData);
    }

    pArrayProp->Clear(pObject);
    W_TEST_INT(pArrayProp->GetCount(pObject), 0);
    pArrayProp->SetCount(pObject, 2);
    pArrayProp->SetValue(pObject, 0, &value);
    pArrayProp->SetValue(pObject, 1, &value);

    // Insert default init values
    T temp = {};
    pArrayProp->Insert(pObject, 2, &temp);
    W_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Insert(pObject, 0, &temp);
    W_TEST_INT(pArrayProp->GetCount(pObject), 4);

    // Remove them again
    pArrayProp->Remove(pObject, 3);
    W_TEST_INT(pArrayProp->GetCount(pObject), 3);
    pArrayProp->Remove(pObject, 0);
    W_TEST_INT(pArrayProp->GetCount(pObject), 2);

    TestArrayPropertyVariant<T>(pArrayProp, pObject, pRtti, value);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  W_TEST_INT(pArrayProp->GetCount(pObject), 2);

  T v1 = {};
  pArrayProp->GetValue(pObject, 0, &v1);
  if constexpr (std::is_same<const char*, T>::value)
  {
    W_TEST_BOOL(WStringUtils::IsEqual(v1, value));
  }
  else
  {
    W_TEST_BOOL(v1 == value);
  }

  T v2 = {};
  pArrayProp->GetValue(pObject, 1, &v2);
  if constexpr (std::is_same<const char*, T>::value)
  {
    W_TEST_BOOL(WStringUtils::IsEqual(v2, value));
  }
  else
  {
    W_TEST_BOOL(v2 == value);
  }

  if (pElemRtti->GetAllocator()->CanAllocate())
  {
    // Current values should be different from default constructed version.
    void* pData = pElemRtti->GetAllocator()->Allocate<void>();

    W_TEST_BOOL(*static_cast<T*>(pData) != v1);
    W_TEST_BOOL(*static_cast<T*>(pData) != v2);

    pElemRtti->GetAllocator()->Deallocate(pData);
  }
}

W_CREATE_SIMPLE_TEST(Reflection, Arrays)
{
  WTestArrays containers;
  const WRTTI* pRtti = WGetStaticRTTI<WTestArrays>();
  W_TEST_BOOL(pRtti != nullptr);

  W_TEST_BLOCK(WTestBlock::Enabled, "POD Array")
  {
    double fValue = 5;
    TestArrayProperty<double>("Hybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("HybridRO", &containers, pRtti, fValue);

    TestArrayProperty<double>("AcHybrid", &containers, pRtti, fValue);
    TestArrayProperty<double>("AcHybridRO", &containers, pRtti, fValue);

    const char* szValue = "Bla";
    const char* szValue2 = "LongString------------------------------------------------------------------------------------";
    WString sValue = szValue;
    WString sValue2 = szValue2;

    TestArrayProperty<WString>("HybridChar", &containers, pRtti, sValue);
    TestArrayProperty<WString>("HybridCharRO", &containers, pRtti, sValue);

    TestArrayProperty<const char*>("AcHybridChar", &containers, pRtti, szValue);
    TestArrayProperty<const char*>("AcHybridCharRO", &containers, pRtti, szValue);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Struct Array")
  {
    WTestStruct3 data;
    data.m_fFloat1 = 99.0f;
    data.m_UInt8 = 127;

    TestArrayProperty<WTestStruct3>("Dynamic", &containers, pRtti, data);
    TestArrayProperty<WTestStruct3>("DynamicRO", &containers, pRtti, data);

    TestArrayProperty<WTestStruct3>("AcDynamic", &containers, pRtti, data);
    TestArrayProperty<WTestStruct3>("AcDynamicRO", &containers, pRtti, data);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WReflectedClass Array")
  {
    WTestArrays data;
    data.m_Hybrid.PushBack(42.0);

    TestArrayProperty<WTestArrays>("Deque", &containers, pRtti, data);
    TestArrayProperty<WTestArrays>("DequeRO", &containers, pRtti, data);

    TestArrayProperty<WTestArrays>("AcDeque", &containers, pRtti, data);
    TestArrayProperty<WTestArrays>("AcDequeRO", &containers, pRtti, data);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Custom Variant Array")
  {
    WVarianceTypeAngle data(WAngle::MakeFromDegree(45.0f), 0.1f);

    TestArrayProperty<WVarianceTypeAngle>("Custom", &containers, pRtti, data);
    TestArrayProperty<WVarianceTypeAngle>("CustomRO", &containers, pRtti, data);

    TestArrayProperty<WVarianceTypeAngle>("AcCustom", &containers, pRtti, data);
    TestArrayProperty<WVarianceTypeAngle>("AcCustomRO", &containers, pRtti, data);
  }

  TestSerialization<WTestArrays>(containers);
}

/// Determines whether a type is a pointer.
template <typename T>
struct WIsPointer
{
  static constexpr bool value = false;
};

template <typename T>
struct WIsPointer<T*>
{
  static constexpr bool value = true;
};

template <typename T>
void TestSetProperty(const char* szPropName, void* pObject, const WRTTI* pRtti, T& ref_value1, T& ref_value2)
{
  const WAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!W_TEST_BOOL(pProp != nullptr))
    return;

  W_ANALYSIS_ASSUME(pProp != nullptr);

  W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Set);
  auto pSetProp = static_cast<const WAbstractSetProperty*>(pProp);
  const WRTTI* pElemRtti = pProp->GetSpecificType();
  W_TEST_BOOL(pElemRtti == WGetStaticRTTI<T>());

  if (!pSetProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
  {
    pSetProp->Clear(pObject);
    W_TEST_BOOL(pSetProp->IsEmpty(pObject));
    pSetProp->Insert(pObject, &ref_value1);
    W_TEST_BOOL(!pSetProp->IsEmpty(pObject));
    W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
    W_TEST_BOOL(!pSetProp->Contains(pObject, &ref_value2));
    pSetProp->Insert(pObject, &ref_value2);
    W_TEST_BOOL(!pSetProp->IsEmpty(pObject));
    W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
    W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));

    // Insert default init value
    if (!WIsPointer<T>::value)
    {
      T temp = T{};
      pSetProp->Insert(pObject, &temp);
      W_TEST_BOOL(!pSetProp->IsEmpty(pObject));
      W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
      W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));
      W_TEST_BOOL(pSetProp->Contains(pObject, &temp));

      // Remove it again
      pSetProp->Remove(pObject, &temp);
      W_TEST_BOOL(!pSetProp->IsEmpty(pObject));
      W_TEST_BOOL(!pSetProp->Contains(pObject, &temp));
    }
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  W_TEST_BOOL(!pSetProp->IsEmpty(pObject));
  W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value1));
  W_TEST_BOOL(pSetProp->Contains(pObject, &ref_value2));


  WTempHybridArray<WVariant, 16> keys;
  pSetProp->GetValues(pObject, keys);
  W_TEST_INT(keys.GetCount(), 2);
}

W_CREATE_SIMPLE_TEST(Reflection, Sets)
{
  WTestSets containers;
  const WRTTI* pRtti = WGetStaticRTTI<WTestSets>();
  W_TEST_BOOL(pRtti != nullptr);

  // Disabled because MSVC 2017 has code generation issues in Release builds
  W_TEST_BLOCK(WTestBlock::Disabled, "WSet")
  {
    WInt8 iValue1 = -5;
    WInt8 iValue2 = 127;
    TestSetProperty<WInt8>("Set", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<WInt8>("SetRO", &containers, pRtti, iValue1, iValue2);

    double fValue1 = 5;
    double fValue2 = -3;
    TestSetProperty<double>("AcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<double>("AcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WHashSet")
  {
    WInt32 iValue1 = -5;
    WInt32 iValue2 = 127;
    TestSetProperty<WInt32>("HashSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<WInt32>("HashSetRO", &containers, pRtti, iValue1, iValue2);

    WInt64 fValue1 = 5;
    WInt64 fValue2 = -3;
    TestSetProperty<WInt64>("HashAcSet", &containers, pRtti, fValue1, fValue2);
    TestSetProperty<WInt64>("HashAcSetRO", &containers, pRtti, fValue1, fValue2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WDeque Pseudo Set")
  {
    int iValue1 = -5;
    int iValue2 = 127;

    TestSetProperty<int>("AcPseudoSet", &containers, pRtti, iValue1, iValue2);
    TestSetProperty<int>("AcPseudoSetRO", &containers, pRtti, iValue1, iValue2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WSetPtr Pseudo Set")
  {
    WString sValue1 = "TestString1";
    WString sValue2 = "Test String Deus";

    TestSetProperty<WString>("AcPseudoSet2", &containers, pRtti, sValue1, sValue2);
    TestSetProperty<WString>("AcPseudoSet2RO", &containers, pRtti, sValue1, sValue2);

    const char* szValue1 = "TestString1";
    const char* szValue2 = "Test String Deus";
    TestSetProperty<const char*>("AcPseudoSet2b", &containers, pRtti, szValue1, szValue2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Custom Variant HashSet")
  {
    WVarianceTypeAngle value1(WAngle::MakeFromDegree(-45.0f), -0.1f);
    WVarianceTypeAngle value2(WAngle::MakeFromDegree(45.0f), 0.1f);

    TestSetProperty<WVarianceTypeAngle>("CustomHashSet", &containers, pRtti, value1, value2);
    TestSetProperty<WVarianceTypeAngle>("CustomHashSetRO", &containers, pRtti, value1, value2);

    WVarianceTypeAngle value3(WAngle::MakeFromDegree(-90.0f), -0.2f);
    WVarianceTypeAngle value4(WAngle::MakeFromDegree(90.0f), 0.2f);
    TestSetProperty<WVarianceTypeAngle>("CustomHashAcSet", &containers, pRtti, value3, value4);
    TestSetProperty<WVarianceTypeAngle>("CustomHashAcSetRO", &containers, pRtti, value3, value4);
  }
  TestSerialization<WTestSets>(containers);
}

template <typename T>
void TestMapProperty(const char* szPropName, void* pObject, const WRTTI* pRtti, T& ref_value1, T& ref_value2)
{
  const WAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!W_TEST_BOOL(pProp != nullptr))
    return;
  W_ANALYSIS_ASSUME(pProp != nullptr);
  W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Map);
  auto pMapProp = static_cast<const WAbstractMapProperty*>(pProp);
  const WRTTI* pElemRtti = pProp->GetSpecificType();
  W_TEST_BOOL(pElemRtti == WGetStaticRTTI<T>());
  W_TEST_BOOL(WReflectionUtils::IsBasicType(pElemRtti) || pElemRtti == WGetStaticRTTI<WVariant>() || pElemRtti == WGetStaticRTTI<WVarianceTypeAngle>());

  if (!pMapProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
  {
    pMapProp->Clear(pObject);
    W_TEST_BOOL(pMapProp->IsEmpty(pObject));
    pMapProp->Insert(pObject, "value1", &ref_value1);
    W_TEST_BOOL(!pMapProp->IsEmpty(pObject));
    W_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
    W_TEST_BOOL(!pMapProp->Contains(pObject, "value2"));
    T getValue;
    W_TEST_BOOL(!pMapProp->GetValue(pObject, "value2", &getValue));
    W_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue));
    W_TEST_BOOL(getValue == ref_value1);

    pMapProp->Insert(pObject, "value2", &ref_value2);
    W_TEST_BOOL(!pMapProp->IsEmpty(pObject));
    W_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
    W_TEST_BOOL(pMapProp->Contains(pObject, "value2"));
    W_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue));
    W_TEST_BOOL(getValue == ref_value1);
    W_TEST_BOOL(pMapProp->GetValue(pObject, "value2", &getValue));
    W_TEST_BOOL(getValue == ref_value2);
  }

  // Assumes this function gets called first by a writeable property, and then immediately by the same data as a read-only property.
  // So the checks are valid for the read-only version, too.
  T getValue2;
  W_TEST_BOOL(!pMapProp->IsEmpty(pObject));
  W_TEST_BOOL(pMapProp->Contains(pObject, "value1"));
  W_TEST_BOOL(pMapProp->Contains(pObject, "value2"));
  W_TEST_BOOL(pMapProp->GetValue(pObject, "value1", &getValue2));
  W_TEST_BOOL(getValue2 == ref_value1);
  W_TEST_BOOL(pMapProp->GetValue(pObject, "value2", &getValue2));
  W_TEST_BOOL(getValue2 == ref_value2);

  WTempHybridArray<WString, 16> keys;
  pMapProp->GetKeys(pObject, keys);
  W_TEST_INT(keys.GetCount(), 2);
  keys.Sort();
  W_TEST_BOOL(keys[0] == "value1");
  W_TEST_BOOL(keys[1] == "value2");
}

W_CREATE_SIMPLE_TEST(Reflection, Maps)
{
  WTestMaps containers;
  const WRTTI* pRtti = WGetStaticRTTI<WTestMaps>();
  W_TEST_BOOL(pRtti != nullptr);

  W_TEST_BLOCK(WTestBlock::Enabled, "WMap")
  {
    int iValue1 = -5;
    int iValue2 = 127;
    TestMapProperty<int>("Map", &containers, pRtti, iValue1, iValue2);
    TestMapProperty<int>("MapRO", &containers, pRtti, iValue1, iValue2);

    WInt64 iValue1b = 5;
    WInt64 iValue2b = -3;
    TestMapProperty<WInt64>("AcMap", &containers, pRtti, iValue1b, iValue2b);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "WHashMap")
  {
    double fValue1 = -5;
    double fValue2 = 127;
    TestMapProperty<double>("HashTable", &containers, pRtti, fValue1, fValue2);
    TestMapProperty<double>("HashTableRO", &containers, pRtti, fValue1, fValue2);

    WString sValue1 = "Bla";
    WString sValue2 = "Test";
    TestMapProperty<WString>("AcHashTable", &containers, pRtti, sValue1, sValue2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Accessor")
  {
    WVariant sValue1 = "Test";
    WVariant sValue2 = WVec4(1, 2, 3, 4);
    TestMapProperty<WVariant>("Accessor", &containers, pRtti, sValue1, sValue2);
    TestMapProperty<WVariant>("AccessorRO", &containers, pRtti, sValue1, sValue2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "CustomVariant")
  {
    WVarianceTypeAngle value1(WAngle::MakeFromDegree(-45.0f), -0.1f);
    WVarianceTypeAngle value2(WAngle::MakeFromDegree(45.0f), 0.1f);

    TestMapProperty<WVarianceTypeAngle>("CustomVariant", &containers, pRtti, value1, value2);
    TestMapProperty<WVarianceTypeAngle>("CustomVariantRO", &containers, pRtti, value1, value2);
  }
  TestSerialization<WTestMaps>(containers);
}


template <typename T>
void TestPointerMemberProperty(const char* szPropName, void* pObject, const WRTTI* pRtti, WBitflags<WPropertyFlags> expectedFlags, T* pExpectedValue)
{
  const WAbstractProperty* pProp = pRtti->FindPropertyByName(szPropName);
  if (!W_TEST_BOOL(pProp != nullptr))
    return;
  W_ANALYSIS_ASSUME(pProp != nullptr);
  W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);
  auto pAbsMember = static_cast<const WAbstractMemberProperty*>(pProp);
  W_TEST_INT(pProp->GetFlags().GetValue(), expectedFlags.GetValue());
  W_TEST_BOOL(pProp->GetSpecificType() == WGetStaticRTTI<T>());
  void* pData = nullptr;
  pAbsMember->GetValuePtr(pObject, &pData);
  W_TEST_BOOL(pData == pExpectedValue);

  // Set value to null.
  {
    void* pDataNull = nullptr;
    pAbsMember->SetValuePtr(pObject, &pDataNull);
    void* pDataNull2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pDataNull2);
    W_TEST_BOOL(pDataNull == pDataNull2);
  }

  // Set value to new instance.
  {
    void* pNewData = pAbsMember->GetSpecificType()->GetAllocator()->Allocate<void>();
    pAbsMember->SetValuePtr(pObject, &pNewData);
    void* pData2 = nullptr;
    pAbsMember->GetValuePtr(pObject, &pData2);
    W_TEST_BOOL(pNewData == pData2);
  }

  // Delete old value
  pAbsMember->GetSpecificType()->GetAllocator()->Deallocate(pData);
}

W_CREATE_SIMPLE_TEST(Reflection, Pointer)
{
  const WRTTI* pRtti = WGetStaticRTTI<WTestPtr>();
  if (!W_TEST_BOOL(pRtti != nullptr))
    return;
  W_ANALYSIS_ASSUME(pRtti != nullptr);

  W_TEST_BLOCK(WTestBlock::Enabled, "Member Property Ptr")
  {
    WTestPtr containers;
    {
      const WAbstractProperty* pProp = pRtti->FindPropertyByName("ConstCharPtr");
      if (W_TEST_BOOL(pProp != nullptr))
      {
        W_ANALYSIS_ASSUME(pProp != nullptr);
        W_TEST_BOOL(pProp->GetCategory() == WPropertyCategory::Member);
        W_TEST_INT(pProp->GetFlags().GetValue(), (WPropertyFlags::StandardType | WPropertyFlags::Const).GetValue());
        W_TEST_BOOL(pProp->GetSpecificType() == WGetStaticRTTI<const char*>());
      }
    }

    TestPointerMemberProperty<WTestArrays>(
      "ArraysPtr", &containers, pRtti, WPropertyFlags::Class | WPropertyFlags::Pointer | WPropertyFlags::PointerOwner, containers.m_pArrays);
    TestPointerMemberProperty<WTestArrays>("ArraysPtrDirect", &containers, pRtti,
      WPropertyFlags::Class | WPropertyFlags::Pointer | WPropertyFlags::PointerOwner, containers.m_pArraysDirect);
  }

  WTestPtr containers;
  WDefaultMemoryStreamStorage StreamStorage;

  W_TEST_BLOCK(WTestBlock::Enabled, "Serialize Property Ptr")
  {
    containers.m_sString = "Test";

    containers.m_pArrays = W_DEFAULT_NEW(WTestArrays);
    containers.m_pArrays->m_Deque.PushBack(WTestArrays());

    containers.m_ArrayPtr.PushBack(W_DEFAULT_NEW(WTestArrays));
    containers.m_ArrayPtr[0]->m_Hybrid.PushBack(5.0);

    containers.m_SetPtr.Insert(W_DEFAULT_NEW(WTestSets));
    containers.m_SetPtr.GetIterator().Key()->m_Array.PushBack("BLA");
  }

  TestSerialization<WTestPtr>(containers);
}
