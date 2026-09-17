#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/Uuid.h>
#include <Foundation/Types/Variant.h>
#include <Foundation/Types/VariantTypeRegistry.h>

WResult WOpenDdlUtils::ConvertToColor(const WOpenDdlReaderElement* pElement, WColor& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return W_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 1.0f;

      return W_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt8)
  {
    const WUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = WColorGammaUB(pValues[0], pValues[1], pValues[2], pValues[3]);

      return W_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = WColorGammaUB(pValues[0], pValues[1], pValues[2]);

      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToColorGamma(const WOpenDdlReaderElement* pElement, WColorGammaUB& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result = WColor(pValues[0], pValues[1], pValues[2], pValues[3]);

      return W_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result = WColor(pValues[0], pValues[1], pValues[2]);

      return W_SUCCESS;
    }
  }
  else if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt8)
  {
    const WUInt8* pValues = pElement->GetPrimitivesUInt8();

    if (pElement->GetNumPrimitives() == 4)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = pValues[3];

      return W_SUCCESS;
    }

    if (pElement->GetNumPrimitives() == 3)
    {
      out_result.r = pValues[0];
      out_result.g = pValues[1];
      out_result.b = pValues[2];
      out_result.a = 255;

      return W_SUCCESS;
    }
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToTime(const WOpenDdlReaderElement* pElement, WTime& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result = WTime::MakeFromSeconds(pValues[0]);

    return W_SUCCESS;
  }

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Double)
  {
    const double* pValues = pElement->GetPrimitivesDouble();

    out_result = WTime::MakeFromSeconds(pValues[0]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec2(const WOpenDdlReaderElement* pElement, WVec2& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec3(const WOpenDdlReaderElement* pElement, WVec3& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec4(const WOpenDdlReaderElement* pElement, WVec4& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec2I(const WOpenDdlReaderElement* pElement, WVec2I32& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Int32)
  {
    const WInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec3I(const WOpenDdlReaderElement* pElement, WVec3I32& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Int32)
  {
    const WInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec4I(const WOpenDdlReaderElement* pElement, WVec4I32& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Int32)
  {
    const WInt32* pValues = pElement->GetPrimitivesInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}


WResult WOpenDdlUtils::ConvertToVec2U(const WOpenDdlReaderElement* pElement, WVec2U32& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt32)
  {
    const WUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec3U(const WOpenDdlReaderElement* pElement, WVec3U32& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 3)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt32)
  {
    const WUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVec4U(const WOpenDdlReaderElement* pElement, WVec4U32& out_vResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt32)
  {
    const WUInt32* pValues = pElement->GetPrimitivesUInt32();

    out_vResult.Set(pValues[0], pValues[1], pValues[2], pValues[3]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}



WResult WOpenDdlUtils::ConvertToMat3(const WOpenDdlReaderElement* pElement, WMat3& out_mResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 9)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_mResult = WMat3::MakeFromColumnMajorArray(pValues);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToMat4(const WOpenDdlReaderElement* pElement, WMat4& out_mResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 16)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_mResult = WMat4::MakeFromColumnMajorArray(pValues);

    return W_SUCCESS;
  }

  return W_FAILURE;
}


WResult WOpenDdlUtils::ConvertToTransform(const WOpenDdlReaderElement* pElement, WTransform& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 10)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_result.m_vPosition.x = pValues[0];
    out_result.m_vPosition.y = pValues[1];
    out_result.m_vPosition.z = pValues[2];
    out_result.m_qRotation.x = pValues[3];
    out_result.m_qRotation.y = pValues[4];
    out_result.m_qRotation.z = pValues[5];
    out_result.m_qRotation.w = pValues[6];
    out_result.m_vScale.x = pValues[7];
    out_result.m_vScale.y = pValues[8];
    out_result.m_vScale.z = pValues[9];

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToQuat(const WOpenDdlReaderElement* pElement, WQuat& out_qResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 4)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    out_qResult = WQuat(pValues[0], pValues[1], pValues[2], pValues[3]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToUuid(const WOpenDdlReaderElement* pElement, WUuid& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 2)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt64)
  {
    const WUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_result = WUuid(pValues[0], pValues[1]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToAngle(const WOpenDdlReaderElement* pElement, WAngle& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::Float)
  {
    const float* pValues = pElement->GetPrimitivesFloat();

    // have to use radians to prevent precision loss
    out_result = WAngle::MakeFromRadian(pValues[0]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToHashedString(const WOpenDdlReaderElement* pElement, WHashedString& out_sResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::String)
  {
    const WStringView* pValues = pElement->GetPrimitivesString();

    out_sResult.Assign(pValues[0]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToTempHashedString(const WOpenDdlReaderElement* pElement, WTempHashedString& out_sResult)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // go into the element, if we are at the group level
  if (pElement->IsCustomType())
  {
    if (pElement->GetNumChildObjects() != 1)
      return W_FAILURE;

    pElement = pElement->GetFirstChild();
  }

  if (pElement->GetNumPrimitives() != 1)
    return W_FAILURE;

  if (pElement->GetPrimitivesType() == WOpenDdlPrimitiveType::UInt64)
  {
    const WUInt64* pValues = pElement->GetPrimitivesUInt64();

    out_sResult = WTempHashedString(pValues[0]);

    return W_SUCCESS;
  }

  return W_FAILURE;
}

WResult WOpenDdlUtils::ConvertToVariant(const WOpenDdlReaderElement* pElement, WVariant& out_result)
{
  if (pElement == nullptr)
    return W_FAILURE;

  // expect a custom type
  if (pElement->IsCustomType())
  {
    if (pElement->GetCustomType() == "VarArray")
    {
      WVariantArray value;
      WVariant varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const WOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        if (ConvertToVariant(pChild, varChild).Failed())
          return W_FAILURE;

        value.PushBack(varChild);
      }

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "VarDict")
    {
      WVariantDictionary value;
      WVariant varChild;

      /// \test This is just quickly hacked
      /// \todo Store array size for reserving var array length

      for (const WOpenDdlReaderElement* pChild = pElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
      {
        // no name -> invalid dictionary entry
        if (!pChild->HasName())
          continue;

        if (ConvertToVariant(pChild, varChild).Failed())
          return W_FAILURE;

        value[pChild->GetName()] = varChild;
      }

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "VarDataBuffer")
    {
      /// \test This is just quickly hacked

      WDataBuffer value;

      const WOpenDdlReaderElement* pString = pElement->GetFirstChild();

      if (!pString->HasPrimitives(WOpenDdlPrimitiveType::String))
        return W_FAILURE;

      const WStringView* pValues = pString->GetPrimitivesString();

      value.SetCountUninitialized(pValues[0].GetElementCount() / 2);
      WConversionUtils::ConvertHexToBinary(pValues[0], value.GetData(), value.GetCount());

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Color")
    {
      WColor value;
      if (ConvertToColor(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "ColorGamma")
    {
      WColorGammaUB value;
      if (ConvertToColorGamma(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Time")
    {
      WTime value;
      if (ConvertToTime(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2")
    {
      WVec2 value;
      if (ConvertToVec2(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3")
    {
      WVec3 value;
      if (ConvertToVec3(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4")
    {
      WVec4 value;
      if (ConvertToVec4(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2i")
    {
      WVec2I32 value;
      if (ConvertToVec2I(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3i")
    {
      WVec3I32 value;
      if (ConvertToVec3I(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4i")
    {
      WVec4I32 value;
      if (ConvertToVec4I(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec2u")
    {
      WVec2U32 value;
      if (ConvertToVec2U(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec3u")
    {
      WVec3U32 value;
      if (ConvertToVec3U(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Vec4u")
    {
      WVec4U32 value;
      if (ConvertToVec4U(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat3")
    {
      WMat3 value;
      if (ConvertToMat3(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Mat4")
    {
      WMat4 value;
      if (ConvertToMat4(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Transform")
    {
      WTransform value;
      if (ConvertToTransform(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Quat")
    {
      WQuat value;
      if (ConvertToQuat(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Uuid")
    {
      WUuid value;
      if (ConvertToUuid(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Angle")
    {
      WAngle value;
      if (ConvertToAngle(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "HashedString")
    {
      WHashedString value;
      if (ConvertToHashedString(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "TempHashedString")
    {
      WTempHashedString value;
      if (ConvertToTempHashedString(pElement, value).Failed())
        return W_FAILURE;

      out_result = value;
      return W_SUCCESS;
    }

    if (pElement->GetCustomType() == "Invalid")
    {
      out_result = WVariant();
      return W_SUCCESS;
    }

    if (const WRTTI* pRTTI = WRTTI::FindTypeByName(pElement->GetCustomType()))
    {
      if (WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pRTTI))
      {
        if (pElement == nullptr)
          return W_FAILURE;

        void* pObject = pRTTI->GetAllocator()->Allocate<void>();

        for (const WOpenDdlReaderElement* pChildElement = pElement->GetFirstChild(); pChildElement != nullptr; pChildElement = pChildElement->GetSibling())
        {
          if (!pChildElement->HasName())
            continue;

          if (const WAbstractProperty* pProp = pRTTI->FindPropertyByName(pChildElement->GetName()))
          {
            // Custom types should be POD and only consist of member properties.
            if (pProp->GetCategory() == WPropertyCategory::Member)
            {
              WVariant subValue;
              if (ConvertToVariant(pChildElement, subValue).Succeeded())
              {
                WReflectionUtils::SetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), pObject, subValue);
              }
            }
          }
        }
        out_result.MoveTypedObject(pObject, pRTTI);
        return W_SUCCESS;
      }
      else
      {
        WLog::Error("The type '{0}' was declared but not defined, add W_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", pElement->GetCustomType());
      }
    }
    else
    {
      WLog::Error("The type '{0}' is unknown.", pElement->GetCustomType());
    }
  }
  else
  {
    // always expect exactly one value
    if (pElement->GetNumPrimitives() != 1)
      return W_FAILURE;

    switch (pElement->GetPrimitivesType())
    {
      case WOpenDdlPrimitiveType::Bool:
        out_result = pElement->GetPrimitivesBool()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::Int8:
        out_result = pElement->GetPrimitivesInt8()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::Int16:
        out_result = pElement->GetPrimitivesInt16()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::Int32:
        out_result = pElement->GetPrimitivesInt32()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::Int64:
        out_result = pElement->GetPrimitivesInt64()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::UInt8:
        out_result = pElement->GetPrimitivesUInt8()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::UInt16:
        out_result = pElement->GetPrimitivesUInt16()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::UInt32:
        out_result = pElement->GetPrimitivesUInt32()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::UInt64:
        out_result = pElement->GetPrimitivesUInt64()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::Float:
        out_result = pElement->GetPrimitivesFloat()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::Double:
        out_result = pElement->GetPrimitivesDouble()[0];
        return W_SUCCESS;

      case WOpenDdlPrimitiveType::String:
        out_result = WString(pElement->GetPrimitivesString()[0]); // make sure this isn't stored as a string view by copying to to an WString first
        return W_SUCCESS;

      default:
        W_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  return W_FAILURE;
}

void WOpenDdlUtils::StoreColor(WOpenDdlWriter& ref_writer, const WColor& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Color", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreColorGamma(
  WOpenDdlWriter& ref_writer, const WColorGammaUB& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("ColorGamma", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt8);
    ref_writer.WriteUInt8(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreTime(WOpenDdlWriter& ref_writer, const WTime& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Time", sName, bGlobalName, true);
  {
    const double d = value.GetSeconds();

    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Double);
    ref_writer.WriteDouble(&d, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec2(WOpenDdlWriter& ref_writer, const WVec2& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec3(WOpenDdlWriter& ref_writer, const WVec3& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec4(WOpenDdlWriter& ref_writer, const WVec4& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec2I(WOpenDdlWriter& ref_writer, const WVec2I32& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec3I(WOpenDdlWriter& ref_writer, const WVec3I32& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec4I(WOpenDdlWriter& ref_writer, const WVec4I32& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4i", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int32);
    ref_writer.WriteInt32(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec2U(WOpenDdlWriter& ref_writer, const WVec2U32& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec2u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec3U(WOpenDdlWriter& ref_writer, const WVec3U32& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec3u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 3);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVec4U(WOpenDdlWriter& ref_writer, const WVec4U32& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Vec4u", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt32);
    ref_writer.WriteUInt32(value.GetData(), 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}


void WOpenDdlUtils::StoreMat3(WOpenDdlWriter& ref_writer, const WMat3& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat3", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);

    float f[9];
    value.GetAsArray(f, WMatrixLayout::ColumnMajor);
    ref_writer.WriteFloat(f, 9);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreMat4(WOpenDdlWriter& ref_writer, const WMat4& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Mat4", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);

    float f[16];
    value.GetAsArray(f, WMatrixLayout::ColumnMajor);
    ref_writer.WriteFloat(f, 16);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreTransform(WOpenDdlWriter& ref_writer, const WTransform& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Transform", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);

    float f[10];

    f[0] = value.m_vPosition.x;
    f[1] = value.m_vPosition.y;
    f[2] = value.m_vPosition.z;

    f[3] = value.m_qRotation.x;
    f[4] = value.m_qRotation.y;
    f[5] = value.m_qRotation.z;
    f[6] = value.m_qRotation.w;

    f[7] = value.m_vScale.x;
    f[8] = value.m_vScale.y;
    f[9] = value.m_vScale.z;

    ref_writer.WriteFloat(f, 10);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreQuat(WOpenDdlWriter& ref_writer, const WQuat& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Quat", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(&value.x, 4);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreUuid(WOpenDdlWriter& ref_writer, const WUuid& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Uuid", sName, bGlobalName, true);
  {
    WUInt64 ui[2];
    value.GetValues(ui[0], ui[1]);

    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(ui, 2);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreAngle(WOpenDdlWriter& ref_writer, const WAngle& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Angle", sName, bGlobalName, true);
  {
    // have to use radians to prevent precision loss
    const float f = value.GetRadian();

    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float);
    ref_writer.WriteFloat(&f, 1);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreHashedString(WOpenDdlWriter& ref_writer, const WHashedString& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("HashedString", sName, bGlobalName, true);
  {
    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::String);
    ref_writer.WriteString(value.GetView());
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreTempHashedString(WOpenDdlWriter& ref_writer, const WTempHashedString& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("TempHashedString", sName, bGlobalName, true);
  {
    const WUInt64 uiHash = value.GetHash();

    ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt64);
    ref_writer.WriteUInt64(&uiHash);
    ref_writer.EndPrimitiveList();
  }
  ref_writer.EndObject();
}

void WOpenDdlUtils::StoreVariant(WOpenDdlWriter& ref_writer, const WVariant& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  switch (value.GetType())
  {
    case WVariant::Type::Invalid:
      StoreInvalid(ref_writer, sName, bGlobalName);
      return;

    case WVariant::Type::Bool:
      StoreBool(ref_writer, value.Get<bool>(), sName, bGlobalName);
      return;

    case WVariant::Type::Int8:
      StoreInt8(ref_writer, value.Get<WInt8>(), sName, bGlobalName);
      return;

    case WVariant::Type::UInt8:
      StoreUInt8(ref_writer, value.Get<WUInt8>(), sName, bGlobalName);
      return;

    case WVariant::Type::Int16:
      StoreInt16(ref_writer, value.Get<WInt16>(), sName, bGlobalName);
      return;

    case WVariant::Type::UInt16:
      StoreUInt16(ref_writer, value.Get<WUInt16>(), sName, bGlobalName);
      return;

    case WVariant::Type::Int32:
      StoreInt32(ref_writer, value.Get<WInt32>(), sName, bGlobalName);
      return;

    case WVariant::Type::UInt32:
      StoreUInt32(ref_writer, value.Get<WUInt32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Int64:
      StoreInt64(ref_writer, value.Get<WInt64>(), sName, bGlobalName);
      return;

    case WVariant::Type::UInt64:
      StoreUInt64(ref_writer, value.Get<WUInt64>(), sName, bGlobalName);
      return;

    case WVariant::Type::Float:
      StoreFloat(ref_writer, value.Get<float>(), sName, bGlobalName);
      return;

    case WVariant::Type::Double:
      StoreDouble(ref_writer, value.Get<double>(), sName, bGlobalName);
      return;

    case WVariant::Type::String:
      WOpenDdlUtils::StoreString(ref_writer, value.Get<WString>(), sName, bGlobalName);
      return;

    case WVariant::Type::StringView:
      WOpenDdlUtils::StoreString(ref_writer, value.Get<WStringView>(), sName, bGlobalName);
      return;

    case WVariant::Type::Color:
      StoreColor(ref_writer, value.Get<WColor>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector2:
      StoreVec2(ref_writer, value.Get<WVec2>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector3:
      StoreVec3(ref_writer, value.Get<WVec3>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector4:
      StoreVec4(ref_writer, value.Get<WVec4>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector2I:
      StoreVec2I(ref_writer, value.Get<WVec2I32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector3I:
      StoreVec3I(ref_writer, value.Get<WVec3I32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector4I:
      StoreVec4I(ref_writer, value.Get<WVec4I32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector2U:
      StoreVec2U(ref_writer, value.Get<WVec2U32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector3U:
      StoreVec3U(ref_writer, value.Get<WVec3U32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Vector4U:
      StoreVec4U(ref_writer, value.Get<WVec4U32>(), sName, bGlobalName);
      return;

    case WVariant::Type::Quaternion:
      StoreQuat(ref_writer, value.Get<WQuat>(), sName, bGlobalName);
      return;

    case WVariant::Type::Matrix3:
      StoreMat3(ref_writer, value.Get<WMat3>(), sName, bGlobalName);
      return;

    case WVariant::Type::Matrix4:
      StoreMat4(ref_writer, value.Get<WMat4>(), sName, bGlobalName);
      return;

    case WVariant::Type::Transform:
      StoreTransform(ref_writer, value.Get<WTransform>(), sName, bGlobalName);
      return;

    case WVariant::Type::Time:
      StoreTime(ref_writer, value.Get<WTime>(), sName, bGlobalName);
      return;

    case WVariant::Type::Uuid:
      StoreUuid(ref_writer, value.Get<WUuid>(), sName, bGlobalName);
      return;

    case WVariant::Type::Angle:
      StoreAngle(ref_writer, value.Get<WAngle>(), sName, bGlobalName);
      return;

    case WVariant::Type::ColorGamma:
      StoreColorGamma(ref_writer, value.Get<WColorGammaUB>(), sName, bGlobalName);
      return;

    case WVariant::Type::HashedString:
      StoreHashedString(ref_writer, value.Get<WHashedString>(), sName, bGlobalName);
      return;

    case WVariant::Type::TempHashedString:
      StoreTempHashedString(ref_writer, value.Get<WTempHashedString>(), sName, bGlobalName);
      return;

    case WVariant::Type::VariantArray:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarArray", sName, bGlobalName);

      const WVariantArray& arr = value.Get<WVariantArray>();
      for (WUInt32 i = 0; i < arr.GetCount(); ++i)
      {
        WOpenDdlUtils::StoreVariant(ref_writer, arr[i]);
      }

      ref_writer.EndObject();
    }
      return;

    case WVariant::Type::VariantDictionary:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarDict", sName, bGlobalName);

      const WVariantDictionary& dict = value.Get<WVariantDictionary>();
      for (auto it = dict.GetIterator(); it.IsValid(); ++it)
      {
        WOpenDdlUtils::StoreVariant(ref_writer, it.Value(), it.Key(), false);
      }

      ref_writer.EndObject();
    }
      return;

    case WVariant::Type::DataBuffer:
    {
      /// \test This is just quickly hacked

      ref_writer.BeginObject("VarDataBuffer", sName, bGlobalName);
      ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::String);

      const WDataBuffer& db = value.Get<WDataBuffer>();
      ref_writer.WriteBinaryAsString(db.GetData(), db.GetCount());

      ref_writer.EndPrimitiveList();
      ref_writer.EndObject();
    }
      return;

    case WVariant::Type::TypedObject:
    {
      WTypedObject obj = value.Get<WTypedObject>();
      if (WVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(obj.m_pType))
      {
        ref_writer.BeginObject(obj.m_pType->GetTypeName(), sName, bGlobalName);
        {
          WTempHybridArray<const WAbstractProperty*, 32> properties;
          obj.m_pType->GetAllProperties(properties);
          for (const WAbstractProperty* pProp : properties)
          {
            // Custom types should be POD and only consist of member properties.
            switch (pProp->GetCategory())
            {
              case WPropertyCategory::Member:
              {
                WVariant subValue = WReflectionUtils::GetMemberPropertyValue(static_cast<const WAbstractMemberProperty*>(pProp), obj.m_pObject);
                StoreVariant(ref_writer, subValue, pProp->GetPropertyName(), false);
              }
              break;
              case WPropertyCategory::Array:
              case WPropertyCategory::Set:
              case WPropertyCategory::Map:
                W_REPORT_FAILURE("Only member properties are supported in custom variant types!");
                break;
              case WPropertyCategory::Constant:
              case WPropertyCategory::Function:
                break;
            }
          }
        }
        ref_writer.EndObject();
      }
      else
      {
        WLog::Error("The type '{0}' was declared but not defined, add W_DEFINE_CUSTOM_VARIANT_TYPE({0}); to a cpp to enable serialization of this variant type.", obj.m_pType->GetTypeName());
      }
    }
      return;
    default:
      W_REPORT_FAILURE("Can't write this type of Variant");
  }
}

void WOpenDdlUtils::StoreString(WOpenDdlWriter& ref_writer, const WStringView& value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::String, sName, bGlobalName);
  ref_writer.WriteString(value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreBool(WOpenDdlWriter& ref_writer, bool value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Bool, sName, bGlobalName);
  ref_writer.WriteBool(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreFloat(WOpenDdlWriter& ref_writer, float value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Float, sName, bGlobalName);
  ref_writer.WriteFloat(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreDouble(WOpenDdlWriter& ref_writer, double value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Double, sName, bGlobalName);
  ref_writer.WriteDouble(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreInt8(WOpenDdlWriter& ref_writer, WInt8 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int8, sName, bGlobalName);
  ref_writer.WriteInt8(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreInt16(WOpenDdlWriter& ref_writer, WInt16 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int16, sName, bGlobalName);
  ref_writer.WriteInt16(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreInt32(WOpenDdlWriter& ref_writer, WInt32 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int32, sName, bGlobalName);
  ref_writer.WriteInt32(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreInt64(WOpenDdlWriter& ref_writer, WInt64 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::Int64, sName, bGlobalName);
  ref_writer.WriteInt64(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreUInt8(WOpenDdlWriter& ref_writer, WUInt8 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt8, sName, bGlobalName);
  ref_writer.WriteUInt8(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreUInt16(WOpenDdlWriter& ref_writer, WUInt16 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt16, sName, bGlobalName);
  ref_writer.WriteUInt16(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreUInt32(WOpenDdlWriter& ref_writer, WUInt32 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt32, sName, bGlobalName);
  ref_writer.WriteUInt32(&value);
  ref_writer.EndPrimitiveList();
}

void WOpenDdlUtils::StoreUInt64(WOpenDdlWriter& ref_writer, WUInt64 value, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginPrimitiveList(WOpenDdlPrimitiveType::UInt64, sName, bGlobalName);
  ref_writer.WriteUInt64(&value);
  ref_writer.EndPrimitiveList();
}

W_FOUNDATION_DLL void WOpenDdlUtils::StoreInvalid(WOpenDdlWriter& ref_writer, WStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  ref_writer.BeginObject("Invalid", sName, bGlobalName, true);
  ref_writer.EndObject();
}
