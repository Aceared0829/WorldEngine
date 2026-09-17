#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

W_CREATE_SIMPLE_TEST_GROUP(Reflection);


void VariantToPropertyTest(void* pIntStruct, const WRTTI* pRttiInt, const char* szPropName, WVariant::Type::Enum type)
{
  const WAbstractMemberProperty* pProp = WReflectionUtils::GetMemberProperty(pRttiInt, szPropName);
  W_TEST_BOOL(pProp != nullptr);
  if (pProp)
  {
    WVariant oldValue = WReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    W_TEST_BOOL(oldValue.IsValid());
    W_TEST_BOOL(oldValue.GetType() == type);

    WVariant defaultValue = WReflectionUtils::GetDefaultValue(pProp);
    W_TEST_BOOL(defaultValue.GetType() == type);
    WReflectionUtils::SetMemberPropertyValue(pProp, pIntStruct, defaultValue);

    WVariant newValue = WReflectionUtils::GetMemberPropertyValue(pProp, pIntStruct);
    W_TEST_BOOL(newValue.IsValid());
    W_TEST_BOOL(newValue.GetType() == type);
    W_TEST_BOOL(newValue == defaultValue);
    W_TEST_BOOL(newValue != oldValue);
  }
}

W_CREATE_SIMPLE_TEST(Reflection, ReflectionUtils)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Integer Properties")
  {
    WIntegerStruct intStruct;
    const WRTTI* pRttiInt = WRTTI::FindTypeByName("WIntegerStruct");
    W_TEST_BOOL(pRttiInt != nullptr);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int8", WVariant::Type::Int8);
    W_TEST_INT(0, intStruct.GetInt8());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt8", WVariant::Type::UInt8);
    W_TEST_INT(0, intStruct.GetUInt8());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int16", WVariant::Type::Int16);
    W_TEST_INT(0, intStruct.m_iInt16);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt16", WVariant::Type::UInt16);
    W_TEST_INT(0, intStruct.m_iUInt16);

    VariantToPropertyTest(&intStruct, pRttiInt, "Int32", WVariant::Type::Int32);
    W_TEST_INT(0, intStruct.GetInt32());
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt32", WVariant::Type::UInt32);
    W_TEST_INT(0, intStruct.GetUInt32());

    VariantToPropertyTest(&intStruct, pRttiInt, "Int64", WVariant::Type::Int64);
    W_TEST_INT(0, intStruct.m_iInt64);
    VariantToPropertyTest(&intStruct, pRttiInt, "UInt64", WVariant::Type::UInt64);
    W_TEST_INT(0, intStruct.m_iUInt64);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Float Properties")
  {
    WFloatStruct floatStruct;
    const WRTTI* pRttiFloat = WRTTI::FindTypeByName("WFloatStruct");
    W_TEST_BOOL(pRttiFloat != nullptr);

    VariantToPropertyTest(&floatStruct, pRttiFloat, "Float", WVariant::Type::Float);
    W_TEST_FLOAT(0, floatStruct.GetFloat(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Double", WVariant::Type::Double);
    W_TEST_FLOAT(0, floatStruct.GetDouble(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Time", WVariant::Type::Time);
    W_TEST_FLOAT(0, floatStruct.GetTime().GetSeconds(), 0);
    VariantToPropertyTest(&floatStruct, pRttiFloat, "Angle", WVariant::Type::Angle);
    W_TEST_FLOAT(0, floatStruct.GetAngle().GetDegree(), 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Misc Properties")
  {
    WPODClass podClass;
    const WRTTI* pRttiPOD = WRTTI::FindTypeByName("WPODClass");
    W_TEST_BOOL(pRttiPOD != nullptr);

    VariantToPropertyTest(&podClass, pRttiPOD, "Bool", WVariant::Type::Bool);
    W_TEST_BOOL(podClass.GetBool() == false);
    VariantToPropertyTest(&podClass, pRttiPOD, "Color", WVariant::Type::Color);
    W_TEST_BOOL(podClass.GetColor() == WColor(1.0f, 1.0f, 1.0f, 1.0f));

    VariantToPropertyTest(&podClass, pRttiPOD, "CharPtr", WVariant::Type::String);
    W_TEST_STRING(podClass.GetCharPtr(), "");

    VariantToPropertyTest(&podClass, pRttiPOD, "String", WVariant::Type::String);
    W_TEST_STRING(podClass.GetString(), "");

    // An WStringView is special, WReflectionUtils::GetMemberPropertyValue will return an WString as that is the default assignment behaviour of WStringView to WVariant. However, WReflectionUtils::GetDefaultValue will still return an WStringView.
    {
      const WAbstractMemberProperty* pProp = WReflectionUtils::GetMemberProperty(pRttiPOD, "StringView");
      W_TEST_BOOL(pProp != nullptr);
      if (pProp)
      {
        WVariant oldValue = WReflectionUtils::GetMemberPropertyValue(pProp, &podClass);
        W_TEST_BOOL(oldValue.IsValid());
        W_TEST_BOOL(oldValue.GetType() == WVariant::Type::String);

        WVariant defaultValue = WReflectionUtils::GetDefaultValue(pProp);
        W_TEST_BOOL(defaultValue.GetType() == WVariant::Type::StringView);
        WReflectionUtils::SetMemberPropertyValue(pProp, &podClass, defaultValue);

        WVariant newValue = WReflectionUtils::GetMemberPropertyValue(pProp, &podClass);
        W_TEST_BOOL(newValue.IsValid());
        W_TEST_BOOL(newValue.GetType() == WVariant::Type::String);
        W_TEST_BOOL(newValue == defaultValue);
        W_TEST_BOOL(newValue != oldValue);
      }
      W_TEST_STRING(podClass.GetStringView(), "");
    }

    VariantToPropertyTest(&podClass, pRttiPOD, "Buffer", WVariant::Type::DataBuffer);
    W_TEST_BOOL(podClass.GetBuffer() == WDataBuffer());
    VariantToPropertyTest(&podClass, pRttiPOD, "VarianceAngle", WVariant::Type::TypedObject);
    W_TEST_BOOL(podClass.GetCustom() == WVarianceTypeAngle{});
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Math Properties")
  {
    WMathClass mathClass;
    const WRTTI* pRttiMath = WRTTI::FindTypeByName("WMathClass");
    W_TEST_BOOL(pRttiMath != nullptr);

    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2", WVariant::Type::Vector2);
    W_TEST_BOOL(mathClass.GetVec2() == WVec2(0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3", WVariant::Type::Vector3);
    W_TEST_BOOL(mathClass.GetVec3() == WVec3(0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4", WVariant::Type::Vector4);
    W_TEST_BOOL(mathClass.GetVec4() == WVec4(0.0f, 0.0f, 0.0f, 0.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec2I", WVariant::Type::Vector2I);
    W_TEST_BOOL(mathClass.m_Vec2I == WVec2I32(0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec3I", WVariant::Type::Vector3I);
    W_TEST_BOOL(mathClass.m_Vec3I == WVec3I32(0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Vec4I", WVariant::Type::Vector4I);
    W_TEST_BOOL(mathClass.m_Vec4I == WVec4I32(0, 0, 0, 0));
    VariantToPropertyTest(&mathClass, pRttiMath, "Quat", WVariant::Type::Quaternion);
    W_TEST_BOOL(mathClass.GetQuat() == WQuat(0.0f, 0.0f, 0.0f, 1.0f));
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat3", WVariant::Type::Matrix3);
    W_TEST_BOOL(mathClass.GetMat3() == WMat3::MakeIdentity());
    VariantToPropertyTest(&mathClass, pRttiMath, "Mat4", WVariant::Type::Matrix4);
    W_TEST_BOOL(mathClass.GetMat4() == WMat4::MakeIdentity());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Enumeration Properties")
  {
    WEnumerationsClass enumClass;
    const WRTTI* pRttiEnum = WRTTI::FindTypeByName("WEnumerationsClass");
    W_TEST_BOOL(pRttiEnum != nullptr);

    VariantToPropertyTest(&enumClass, pRttiEnum, "Enum", WVariant::Type::Int64);
    W_TEST_BOOL(enumClass.GetEnum() == WExampleEnum::Value1);
    VariantToPropertyTest(&enumClass, pRttiEnum, "Bitflags", WVariant::Type::Int64);
    W_TEST_BOOL(enumClass.GetBitflags() == WExampleBitflags::Value1);
  }
}

void AccessorPropertyTest(WIReflectedTypeAccessor& ref_accessor, const char* szProperty, WVariant::Type::Enum type)
{
  WVariant oldValue = ref_accessor.GetValue(szProperty);
  W_TEST_BOOL(oldValue.IsValid());
  W_TEST_BOOL(oldValue.GetType() == type);

  const WAbstractProperty* pProp = ref_accessor.GetType()->FindPropertyByName(szProperty);
  WVariant defaultValue = WToolsReflectionUtils::GetStorageDefault(pProp);
  W_TEST_BOOL(defaultValue.GetType() == type);
  bool bSetSuccess = ref_accessor.SetValue(szProperty, defaultValue);
  W_TEST_BOOL(bSetSuccess);

  WVariant newValue = ref_accessor.GetValue(szProperty);
  W_TEST_BOOL(newValue.IsValid());
  W_TEST_BOOL(newValue.GetType() == type);
  W_TEST_BOOL(newValue == defaultValue);
}

WUInt32 AccessorPropertiesTest(WIReflectedTypeAccessor& ref_accessor, const WRTTI* pType)
{
  WUInt32 uiPropertiesSet = 0;
  if (!W_TEST_BOOL(pType != nullptr))
    return 0;

  W_ANALYSIS_ASSUME(pType != nullptr);

  // Call for base class
  if (pType->GetParentType() != nullptr)
  {
    uiPropertiesSet += AccessorPropertiesTest(ref_accessor, pType->GetParentType());
  }

  // Test properties
  WUInt32 uiPropCount = pType->GetProperties().GetCount();
  for (WUInt32 i = 0; i < uiPropCount; ++i)
  {
    const WAbstractProperty* pProp = pType->GetProperties()[i];
    const bool bIsValueType = WReflectionUtils::IsValueType(pProp);

    switch (pProp->GetCategory())
    {
      case WPropertyCategory::Member:
      {
        auto pProp3 = static_cast<const WAbstractMemberProperty*>(pProp);
        if (pProp->GetFlags().IsSet(WPropertyFlags::IsEnum))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), WVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (pProp->GetFlags().IsSet(WPropertyFlags::Bitflags))
        {
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), WVariant::Type::Int64);
          uiPropertiesSet++;
        }
        else if (bIsValueType)
        {
          WVariantType::Enum storageType = WToolsReflectionUtils::GetStorageType(pProp);
          AccessorPropertyTest(ref_accessor, pProp->GetPropertyName(), storageType);
          uiPropertiesSet++;
        }
        else // WPropertyFlags::Class
        {
          // Recurs into sub-classes
          const WUuid& subObjectGuid = ref_accessor.GetValue(pProp->GetPropertyName()).Get<WUuid>();
          WDocumentObject* pEmbeddedClassObject = const_cast<WDocumentObject*>(ref_accessor.GetOwner()->GetChild(subObjectGuid));
          uiPropertiesSet += AccessorPropertiesTest(pEmbeddedClassObject->GetTypeAccessor(), pProp3->GetSpecificType());
        }
      }
      break;
      case WPropertyCategory::Array:
      {
        // WAbstractArrayProperty* pProp3 = static_cast<WAbstractArrayProperty*>(pProp);
        // TODO
      }
      break;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }
  return uiPropertiesSet;
}

WUInt32 AccessorPropertiesTest(WIReflectedTypeAccessor& ref_accessor)
{
  const WRTTI* handle = ref_accessor.GetType();
  return AccessorPropertiesTest(ref_accessor, handle);
}

static WUInt32 GetTypeCount()
{
  WUInt32 uiCount = 0;
  WRTTI::ForEachType([&](const WRTTI* pRtti)
    { uiCount++; });
  return uiCount;
}

static const WRTTI* RegisterType(const char* szTypeName)
{
  const WRTTI* pRtti = WRTTI::FindTypeByName(szTypeName);
  W_TEST_BOOL(pRtti != nullptr);

  WReflectedTypeDescriptor desc;
  WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRtti, desc);
  return WPhantomRttiManager::RegisterType(desc);
}

W_CREATE_SIMPLE_TEST(Reflection, ReflectedType)
{
  WTestDocumentObjectManager manager;

  /*const WRTTI* pRttiBase =*/RegisterType("WReflectedClass");
  /*const WRTTI* pRttiEnumBase =*/RegisterType("WEnumBase");
  /*const WRTTI* pRttiBitflagsBase =*/RegisterType("WBitflagsBase");

  const WRTTI* pRttiInt = RegisterType("WIntegerStruct");
  const WRTTI* pRttiFloat = RegisterType("WFloatStruct");
  const WRTTI* pRttiPOD = RegisterType("WPODClass");
  const WRTTI* pRttiMath = RegisterType("WMathClass");
  /*const WRTTI* pRttiEnum =*/RegisterType("WExampleEnum");
  /*const WRTTI* pRttiFlags =*/RegisterType("WExampleBitflags");
  const WRTTI* pRttiEnumerations = RegisterType("WEnumerationsClass");

  W_TEST_BLOCK(WTestBlock::Enabled, "WReflectedTypeStorageAccessor")
  {
    {
      WDocumentObject* pObject = manager.CreateObject(pRttiInt);
      W_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 8);
      manager.DestroyObject(pObject);
    }
    {
      WDocumentObject* pObject = manager.CreateObject(pRttiFloat);
      W_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 4);
      manager.DestroyObject(pObject);
    }
    {
      WDocumentObject* pObject = manager.CreateObject(pRttiPOD);
      W_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 20);
      manager.DestroyObject(pObject);
    }
    {
      WDocumentObject* pObject = manager.CreateObject(pRttiMath);
      W_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 29);
      manager.DestroyObject(pObject);
    }
    {
      WDocumentObject* pObject = manager.CreateObject(pRttiEnumerations);
      W_TEST_INT(AccessorPropertiesTest(pObject->GetTypeAccessor()), 2);
      manager.DestroyObject(pObject);
    }
  }
}


W_CREATE_SIMPLE_TEST(Reflection, ReflectedTypeReloading)
{
  WTestDocumentObjectManager manager;

  const WRTTI* pRttiInner = WRTTI::FindTypeByName("InnerStruct");
  const WRTTI* pRttiInnerP = nullptr;
  WReflectedTypeDescriptor descInner;

  const WRTTI* pRttiOuter = WRTTI::FindTypeByName("OuterClass");
  const WRTTI* pRttiOuterP = nullptr;
  WReflectedTypeDescriptor descOuter;

  WUInt32 uiRegisteredBaseTypes = GetTypeCount();
  W_TEST_BLOCK(WTestBlock::Enabled, "RegisterType")
  {
    W_TEST_BOOL(pRttiInner != nullptr);
    WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
    descInner.m_sTypeName = "InnerStructP";
    pRttiInnerP = WPhantomRttiManager::RegisterType(descInner);
    W_TEST_BOOL(pRttiInnerP != nullptr);

    W_TEST_BOOL(pRttiOuter != nullptr);
    WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
    descOuter.m_sTypeName = "OuterClassP";
    descOuter.m_Properties[0].m_sType = "InnerStructP";
    pRttiOuterP = WPhantomRttiManager::RegisterType(descOuter);
    W_TEST_BOOL(pRttiOuterP != nullptr);
  }

  {
    WDocumentObject* pInnerObject = manager.CreateObject(pRttiInnerP);
    manager.AddObject(pInnerObject, nullptr, "Children", -1);
    WIReflectedTypeAccessor& innerAccessor = pInnerObject->GetTypeAccessor();

    WDocumentObject* pOuterObject = manager.CreateObject(pRttiOuterP);
    manager.AddObject(pOuterObject, nullptr, "Children", -1);
    WIReflectedTypeAccessor& outerAccessor = pOuterObject->GetTypeAccessor();

    WUuid innerGuid = outerAccessor.GetValue("Inner").Get<WUuid>();
    WDocumentObject* pEmbeddedInnerObject = manager.GetObject(innerGuid);
    WIReflectedTypeAccessor& embeddedInnerAccessor = pEmbeddedInnerObject->GetTypeAccessor();

    W_TEST_BLOCK(WTestBlock::Enabled, "SetValues")
    {
      // Just set a few values to make sure they don't get messed up by the following operations.
      W_TEST_BOOL(innerAccessor.SetValue("IP1", 1.4f));
      W_TEST_BOOL(outerAccessor.SetValue("OP1", 0.9f));
      W_TEST_BOOL(embeddedInnerAccessor.SetValue("IP1", 1.4f));
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "AddProperty")
    {
      // Say we reload the engine and the InnerStruct now has a second property: IP2.
      descInner.m_Properties.PushBack(WReflectedPropertyDescriptor(WPropertyCategory::Member, "IP2", "WVec4",
        WBitflags<WPropertyFlags>(WPropertyFlags::StandardType), WArrayPtr<WPropertyAttribute* const>()));
      const WRTTI* NewInnerHandle = WPhantomRttiManager::RegisterType(descInner);
      W_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // updating a type must not make it stop being phantom - descriptors don't carry that flag
      W_TEST_BOOL(NewInnerHandle->GetTypeFlags().IsSet(WTypeFlags::Phantom));

      // Check that the new property is present.
      AccessorPropertyTest(innerAccessor, "IP2", WVariant::Type::Vector4);

      AccessorPropertyTest(embeddedInnerAccessor, "IP2", WVariant::Type::Vector4);

      // Test that the old properties are still valid.
      W_TEST_BOOL(innerAccessor.GetValue("IP1") == 1.4f);
      W_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
      W_TEST_BOOL(embeddedInnerAccessor.GetValue("IP1") == 1.4f);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "ChangeProperty")
    {
      // Out original inner float now is a Int32!
      descInner.m_Properties[0].m_sType = "WInt32";
      const WRTTI* NewInnerHandle = WPhantomRttiManager::RegisterType(descInner);
      W_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Test if the previous value was converted correctly to its new type.
      WVariant innerValue = innerAccessor.GetValue("IP1");
      W_TEST_BOOL(innerValue.IsValid());
      W_TEST_BOOL(innerValue.GetType() == WVariant::Type::Int32);
      W_TEST_INT(innerValue.Get<WInt32>(), 1);

      WVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      W_TEST_BOOL(outerValue.IsValid());
      W_TEST_BOOL(outerValue.GetType() == WVariant::Type::Int32);
      W_TEST_INT(outerValue.Get<WInt32>(), 1);

      // Test that the old properties are still valid.
      W_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", WVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", WVariant::Type::Vector4);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "DeleteProperty")
    {
      // Lets now delete the original inner property IP1.
      descInner.m_Properties.RemoveAtAndCopy(0);
      const WRTTI* NewInnerHandle = WPhantomRttiManager::RegisterType(descInner);
      W_TEST_BOOL(NewInnerHandle == pRttiInnerP);

      // Check that IP1 is really gone.
      W_TEST_BOOL(!innerAccessor.GetValue("IP1").IsValid());
      W_TEST_BOOL(!embeddedInnerAccessor.GetValue("IP1").IsValid());

      // Test that the old properties are still valid.
      W_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);

      AccessorPropertyTest(innerAccessor, "IP2", WVariant::Type::Vector4);
      AccessorPropertyTest(embeddedInnerAccessor, "IP2", WVariant::Type::Vector4);
    }

    W_TEST_BLOCK(WTestBlock::Enabled, "RevertProperties")
    {
      // Reset all classes to their initial state.
      WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiInner, descInner);
      descInner.m_sTypeName = "InnerStructP";
      WPhantomRttiManager::RegisterType(descInner);

      WToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(pRttiOuter, descOuter);
      descInner.m_sTypeName = "OuterStructP";
      descOuter.m_Properties[0].m_sType = "InnerStructP";
      WPhantomRttiManager::RegisterType(descOuter);

      // Test that the old properties are back again.
      WStringBuilder path = "IP1";
      WVariant innerValue = innerAccessor.GetValue(path);
      W_TEST_BOOL(innerValue.IsValid());
      W_TEST_BOOL(innerValue.GetType() == WVariant::Type::Float);
      W_TEST_FLOAT(innerValue.Get<float>(), 1.0f, 0.0f);

      WVariant outerValue = embeddedInnerAccessor.GetValue("IP1");
      W_TEST_BOOL(outerValue.IsValid());
      W_TEST_BOOL(outerValue.GetType() == WVariant::Type::Float);
      W_TEST_FLOAT(outerValue.Get<float>(), 1.0f, 0.0f);
      W_TEST_BOOL(outerAccessor.GetValue("OP1") == 0.9f);
    }

    manager.RemoveObject(pInnerObject);
    manager.DestroyObject(pInnerObject);

    manager.RemoveObject(pOuterObject);
    manager.DestroyObject(pOuterObject);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "UnregisterType")
  {
    W_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes + 2);
    WPhantomRttiManager::UnregisterType(pRttiOuterP);
    WPhantomRttiManager::UnregisterType(pRttiInnerP);
    W_TEST_INT(GetTypeCount(), uiRegisteredBaseTypes);
  }
}
