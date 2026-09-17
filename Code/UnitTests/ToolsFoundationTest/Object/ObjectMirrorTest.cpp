#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundationTest/Object/TestObjectManager.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

void MirrorCheck(WTestDocument* pDoc, const WDocumentObject* pObject)
{
  // Create native object graph
  WAbstractObjectGraph graph;
  WAbstractObjectNode* pRootNode = nullptr;
  {
    WRttiConverterWriter rttiConverter(&graph, &pDoc->m_Context, true, true);
    pRootNode = rttiConverter.AddObjectToGraph(pObject->GetType(), pDoc->m_ObjectMirror.GetNativeObjectPointer(pObject), "Object");
  }

  // Create object manager graph
  WAbstractObjectGraph origGraph;
  WAbstractObjectNode* pOrigRootNode = nullptr;
  {
    WDocumentObjectConverterWriter writer(&origGraph, pDoc->GetObjectManager());
    pOrigRootNode = writer.AddObjectToGraph(pObject);
  }

  // Remap native guids so they match the object manager (stuff like embedded classes will not have a guid on the native side).
  graph.ReMapNodeGuidsToMatchGraph(pRootNode, origGraph, pOrigRootNode);
  WDeque<WAbstractGraphDiffOperation> diffResult;

  graph.CreateDiffWithBaseGraph(origGraph, diffResult);

  W_TEST_BOOL(diffResult.GetCount() == 0);
}


WVariant GetVariantFromType(WVariant::Type::Enum type)
{
  switch (type)
  {
    case WVariant::Type::Invalid:
      return WVariant();
    case WVariant::Type::Bool:
      return WVariant(true);
    case WVariant::Type::Int8:
      return WVariant((WInt8)-55);
    case WVariant::Type::UInt8:
      return WVariant((WUInt8)44);
    case WVariant::Type::Int16:
      return WVariant((WInt16)-444);
    case WVariant::Type::UInt16:
      return WVariant((WUInt16)666);
    case WVariant::Type::Int32:
      return WVariant((WInt32)-88880);
    case WVariant::Type::UInt32:
      return WVariant((WUInt32)123445);
    case WVariant::Type::Int64:
      return WVariant((WInt64)-888800000);
    case WVariant::Type::UInt64:
      return WVariant((WUInt64)123445000);
    case WVariant::Type::Float:
      return WVariant(1024.0f);
    case WVariant::Type::Double:
      return WVariant(-2048.0f);
    case WVariant::Type::Color:
      return WVariant(WColor(0.5f, 33.0f, 2.0f, 0.3f));
    case WVariant::Type::ColorGamma:
      return WVariant(WColorGammaUB(WColor(0.5f, 33.0f, 2.0f, 0.3f)));
    case WVariant::Type::Vector2:
      return WVariant(WVec2(2.0f, 4.0f));
    case WVariant::Type::Vector3:
      return WVariant(WVec3(2.0f, 4.0f, -8.0f));
    case WVariant::Type::Vector4:
      return WVariant(WVec4(1.0f, 7.0f, 8.0f, -10.0f));
    case WVariant::Type::Vector2I:
      return WVariant(WVec2I32(1, 2));
    case WVariant::Type::Vector3I:
      return WVariant(WVec3I32(3, 4, 5));
    case WVariant::Type::Vector4I:
      return WVariant(WVec4I32(6, 7, 8, 9));
    case WVariant::Type::Quaternion:
    {
      WQuat quat;
      quat = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(30), WAngle::MakeFromDegree(-15), WAngle::MakeFromDegree(20));
      return WVariant(quat);
    }
    case WVariant::Type::Matrix3:
    {
      WMat3 mat = WMat3::MakeIdentity();

      mat = WMat3::MakeAxisRotation(WVec3(1.0f, 0.0f, 0.0f), WAngle::MakeFromDegree(30));
      return WVariant(mat);
    }
    case WVariant::Type::Matrix4:
    {
      WMat4 mat = WMat4::MakeIdentity();

      mat = WMat4::MakeAxisRotation(WVec3(0.0f, 1.0f, 0.0f), WAngle::MakeFromDegree(30));
      mat.SetTranslationVector(WVec3(1.0f, 2.0f, 3.0f));
      return WVariant(mat);
    }
    case WVariant::Type::String:
      return WVariant("Test");
    case WVariant::Type::StringView:
      return WVariant("Test");
    case WVariant::Type::Time:
      return WVariant(WTime::MakeFromSeconds(123.0f));
    case WVariant::Type::Uuid:
    {
      return WVariant(WUuid::MakeUuid());
    }
    case WVariant::Type::Angle:
      return WVariant(WAngle::MakeFromDegree(30.0f));
    case WVariant::Type::DataBuffer:
    {
      WDataBuffer data;
      data.PushBack(12);
      data.PushBack(55);
      data.PushBack(88);
      return WVariant(data);
    }
    case WVariant::Type::VariantArray:
      return WVariantArray();
    case WVariant::Type::VariantDictionary:
      return WVariantDictionary();
    case WVariant::Type::TypedPointer:
      return WVariant(WTypedPointer(nullptr, nullptr));
    case WVariant::Type::TypedObject:
      W_ASSERT_NOT_IMPLEMENTED;

    default:
      W_REPORT_FAILURE("Invalid case statement");
      return WVariant();
  }
  return WVariant();
}

void RecursiveModifyProperty(const WDocumentObject* pObject, const WAbstractProperty* pProp, WObjectAccessorBase* pObjectAccessor)
{
  if (pProp->GetCategory() == WPropertyCategory::Member)
  {
    if (pProp->GetFlags().IsSet(WPropertyFlags::Pointer))
    {
      if (pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
      {
        const WUuid oldGuid = pObjectAccessor->Get<WUuid>(pObject, pProp);
        WUuid newGuid = WUuid::MakeUuid();
        if (oldGuid.IsValid())
        {
          W_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(oldGuid)).Succeeded());
        }

        W_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, WVariant(), pProp->GetSpecificType(), newGuid).Succeeded());

        const WDocumentObject* pChild = pObject->GetChild(newGuid);
        W_ASSERT_DEV(pChild != nullptr, "References child object does not exist!");
      }
      else
      {
        WVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
        W_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).Succeeded());
      }
    }
    else
    {
      if (pProp->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags | WPropertyFlags::StandardType))
      {
        WVariant value = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
        W_TEST_BOOL(pObjectAccessor->SetValue(pObject, pProp, value).Succeeded());
      }
      else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
      {
        // Noting to do here, value cannot change
      }
    }
  }
  else if (pProp->GetCategory() == WPropertyCategory::Array || pProp->GetCategory() == WPropertyCategory::Set)
  {
    if (pProp->GetFlags().IsAnySet(WPropertyFlags::StandardType | WPropertyFlags::Pointer) &&
        !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
    {
      WInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      for (WInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, i).AssertSuccess();
      }

      WVariant value1 = WReflectionUtils::GetDefaultValue(pProp, 0);
      WVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      W_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, 0).Succeeded());
      W_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, 1).Succeeded());
    }
    else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
    {
      WInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      WTempHybridArray<WVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (WInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        W_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<WUuid>())).Succeeded());
      }

      if (pProp->GetCategory() == WPropertyCategory::Array)
      {
        WUuid newGuid = WUuid::MakeUuid();
        W_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, 0, pProp->GetSpecificType(), newGuid).Succeeded());
      }
    }
  }
  else if (pProp->GetCategory() == WPropertyCategory::Map)
  {
    if (pProp->GetFlags().IsAnySet(WPropertyFlags::StandardType | WPropertyFlags::Pointer) &&
        !pProp->GetFlags().IsSet(WPropertyFlags::PointerOwner))
    {
      WInt32 iCurrentCount = pObjectAccessor->GetCount(pObject, pProp);
      WTempHybridArray<WVariant, 16> keys;
      pObjectAccessor->GetKeys(pObject, pProp, keys).AssertSuccess();
      for (const WVariant& key : keys)
      {
        pObjectAccessor->RemoveValue(pObject, pProp, key).AssertSuccess();
      }

      WVariant value1 = WReflectionUtils::GetDefaultValue(pProp, "Dummy");
      WVariant value2 = GetVariantFromType(pProp->GetSpecificType()->GetVariantType());
      W_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value1, "value1").Succeeded());
      W_TEST_BOOL(pObjectAccessor->InsertValue(pObject, pProp, value2, "value2").Succeeded());
    }
    else if (pProp->GetFlags().IsSet(WPropertyFlags::Class))
    {
      WInt32 iCurrentCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());
      WTempHybridArray<WVariant, 16> currentValues;
      pObject->GetTypeAccessor().GetValues(pProp->GetPropertyName(), currentValues);
      for (WInt32 i = iCurrentCount - 1; i >= 0; --i)
      {
        W_TEST_BOOL(pObjectAccessor->RemoveObject(pObjectAccessor->GetObject(currentValues[i].Get<WUuid>())).Succeeded());
      }

      WUuid newGuid = WUuid::MakeUuid();
      W_TEST_BOOL(pObjectAccessor->AddObject(pObject, pProp, "value1", pProp->GetSpecificType(), newGuid).Succeeded());
    }
  }
}

void RecursiveModifyObject(const WDocumentObject* pObject, WObjectAccessorBase* pAccessor)
{
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pObject->GetTypeAccessor().GetType()->GetAllProperties(properties);
  for (const auto* pProp : properties)
  {
    RecursiveModifyProperty(pObject, pProp, pAccessor);
  }

  for (const WDocumentObject* pSubObject : pObject->GetChildren())
  {
    RecursiveModifyObject(pSubObject, pAccessor);
  }
}

W_CREATE_SIMPLE_TEST(DocumentObject, ObjectMirror)
{
  WTestDocument doc("Test", true);
  doc.InitializeAfterLoading(false);
  WObjectAccessorBase* pAccessor = doc.GetObjectAccessor();
  WUuid mirrorGuid;

  pAccessor->StartTransaction("Init");
  WStatus status = pAccessor->AddObject(nullptr, (const WAbstractProperty*)nullptr, -1, WGetStaticRTTI<WMirrorTest>(), mirrorGuid);
  const WDocumentObject* pObject = pAccessor->GetObject(mirrorGuid);
  W_TEST_BOOL(status.Succeeded());
  pAccessor->FinishTransaction();

  MirrorCheck(&doc, pObject);

  W_TEST_BLOCK(WTestBlock::Enabled, "Document Changes")
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
  {
    pAccessor->StartTransaction("Document Changes");
    RecursiveModifyObject(pObject, pAccessor);
    pAccessor->FinishTransaction();

    MirrorCheck(&doc, pObject);
  }
}
