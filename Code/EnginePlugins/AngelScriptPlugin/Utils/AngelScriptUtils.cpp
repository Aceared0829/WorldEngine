#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/Variant.h>

thread_local WWorld* tl_pWorld = nullptr;

void WAngelScriptUtils::SetThreadLocalWorld(WWorld* pWorld)
{
  tl_pWorld = pWorld;
}

WWorld* WAngelScriptUtils::GetThreadLocalWorld()
{
  return tl_pWorld;
}

const char* WAngelScriptUtils::GetAsTypeName(asIScriptEngine* pEngine, int iAsTypeID)
{
  switch (iAsTypeID)
  {
    case asTYPEID_BOOL:
      return "bool";
    case asTYPEID_INT8:
      return "int8";
    case asTYPEID_INT16:
      return "int16";
    case asTYPEID_INT32:
      return "int32";
    case asTYPEID_INT64:
      return "int64";
    case asTYPEID_UINT8:
      return "uint8";
    case asTYPEID_UINT16:
      return "uint16";
    case asTYPEID_UINT32:
      return "uint32";
    case asTYPEID_UINT64:
      return "uint64";
    case asTYPEID_FLOAT:
      return "float";
    case asTYPEID_DOUBLE:
      return "double";

    default:
      if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
      {
        return pInfo->GetName();
      }

      return nullptr;
  }
}


WString WAngelScriptUtils::GetNiceFunctionDeclaration(const asIScriptFunction* pFunc, bool bIncludeObjectName, bool bIncludeNamespace)
{
  WStringBuilder tmp;

  tmp = pFunc->GetDeclaration(bIncludeObjectName, bIncludeNamespace, true);

  tmp.ReplaceAll(" :: ", "::");
  tmp.ReplaceAll(" (", "(");
  tmp.ReplaceAll("( ", "(");
  tmp.ReplaceAll(" )", ")");
  tmp.ReplaceAll(") ", ")");
  tmp.ReplaceAll(" ,", ",");
  tmp.ReplaceAll(")const", ") const");

  return tmp;
}

const WRTTI* WAngelScriptUtils::MapToRTTI(int iAsTypeID, asIScriptEngine* pEngine)
{
  if (iAsTypeID == asTYPEID_BOOL)
    return WGetStaticRTTI<bool>();

  if (iAsTypeID == asTYPEID_INT8)
    return WGetStaticRTTI<WInt8>();

  if (iAsTypeID == asTYPEID_INT16)
    return WGetStaticRTTI<WInt16>();

  if (iAsTypeID == asTYPEID_INT32)
    return WGetStaticRTTI<WInt32>();

  if (iAsTypeID == asTYPEID_INT64)
    return WGetStaticRTTI<WInt64>();

  if (iAsTypeID == asTYPEID_UINT8)
    return WGetStaticRTTI<WUInt8>();

  if (iAsTypeID == asTYPEID_UINT16)
    return WGetStaticRTTI<WUInt16>();

  if (iAsTypeID == asTYPEID_UINT32)
    return WGetStaticRTTI<WUInt32>();

  if (iAsTypeID == asTYPEID_UINT64)
    return WGetStaticRTTI<WUInt64>();

  if (iAsTypeID == asTYPEID_FLOAT)
    return WGetStaticRTTI<float>();

  if (iAsTypeID == asTYPEID_DOUBLE)
    return WGetStaticRTTI<double>();

  if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
  {
    return (const WRTTI*)pInfo->GetUserData(WAsUserData::RttiPtr);
  }

  return nullptr;
}

WResult WAngelScriptUtils::WriteToAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, const WVariant& value)
{
  void* pMemDst = pMemoryLocation;

  switch (iAsTypeID)
  {
    case asTYPEID_BOOL:
      *static_cast<WInt8*>(pMemDst) = value.ConvertTo<bool>() ? 1 : 0;
      return W_SUCCESS;
    case asTYPEID_INT8:
      *static_cast<WInt8*>(pMemDst) = value.ConvertTo<WInt8>();
      return W_SUCCESS;
    case asTYPEID_INT16:
      *static_cast<WInt16*>(pMemDst) = value.ConvertTo<WInt16>();
      return W_SUCCESS;
    case asTYPEID_INT32:
      *static_cast<WInt32*>(pMemDst) = value.ConvertTo<WInt32>();
      return W_SUCCESS;
    case asTYPEID_INT64:
      *static_cast<WInt64*>(pMemDst) = value.ConvertTo<WInt64>();
      return W_SUCCESS;
    case asTYPEID_UINT8:
      *static_cast<WUInt8*>(pMemDst) = value.ConvertTo<WUInt8>();
      return W_SUCCESS;
    case asTYPEID_UINT16:
      *static_cast<WUInt16*>(pMemDst) = value.ConvertTo<WUInt16>();
      return W_SUCCESS;
    case asTYPEID_UINT32:
      *static_cast<WUInt32*>(pMemDst) = value.ConvertTo<WUInt32>();
      return W_SUCCESS;
    case asTYPEID_UINT64:
      *static_cast<WUInt64*>(pMemDst) = value.ConvertTo<WUInt64>();
      return W_SUCCESS;
    case asTYPEID_FLOAT:
      *static_cast<float*>(pMemDst) = value.ConvertTo<float>();
      return W_SUCCESS;
    case asTYPEID_DOUBLE:
      *static_cast<double*>(pMemDst) = value.ConvertTo<double>();
      return W_SUCCESS;
  }

  if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
  {
    const WRTTI* pRtti = (const WRTTI*)pInfo->GetUserData(WAsUserData::RttiPtr);

    if (pRtti->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags))
    {
      *static_cast<WUInt32*>(pMemDst) = value.ConvertTo<WUInt32>();
      return W_SUCCESS;
    }

    if (pRtti == value.GetReflectedType() || value.CanConvertTo(pRtti->GetVariantType()))
    {
      if (pRtti == WGetStaticRTTI<WString>())
      {
        *static_cast<WString*>(pMemDst) = value.ConvertTo<WString>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WHashedString>())
      {
        static_cast<WHashedString*>(pMemDst)->Assign(value.ConvertTo<WString>());
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WAngle>())
      {
        *static_cast<WAngle*>(pMemDst) = value.ConvertTo<WAngle>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WTime>())
      {
        *static_cast<WTime*>(pMemDst) = value.ConvertTo<WTime>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WColor>())
      {
        *static_cast<WColor*>(pMemDst) = value.ConvertTo<WColor>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WColorGammaUB>())
      {
        *static_cast<WColorGammaUB*>(pMemDst) = value.ConvertTo<WColorGammaUB>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WVec2>())
      {
        *static_cast<WVec2*>(pMemDst) = value.ConvertTo<WVec2>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WVec3>())
      {
        *static_cast<WVec3*>(pMemDst) = value.ConvertTo<WVec3>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WVec4>())
      {
        *static_cast<WVec4*>(pMemDst) = value.ConvertTo<WVec4>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WQuat>())
      {
        *static_cast<WQuat*>(pMemDst) = value.ConvertTo<WQuat>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WTransform>())
      {
        *static_cast<WTransform*>(pMemDst) = value.ConvertTo<WTransform>();
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WGameObjectHandle>())
      {
        *static_cast<WGameObjectHandle*>(pMemDst) = *((const WGameObjectHandle*)value.GetData());
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WComponentHandle>())
      {
        *static_cast<WComponentHandle*>(pMemDst) = *((const WComponentHandle*)value.GetData());
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WWorld>())
      {
        *static_cast<WWorld**>(pMemDst) = ((WWorld*)value.GetData());
        return W_SUCCESS;
      }
      else if (pRtti == WGetStaticRTTI<WGameObject>())
      {
        *static_cast<WGameObject**>(pMemDst) = ((WGameObject*)value.GetData());
        return W_SUCCESS;
      }
    }
  }

  // currently unsupported type for exposed parameter
  return W_FAILURE;
}

WResult WAngelScriptUtils::ReadFromAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, WVariant& out_value)
{
  void* pMemLoc = pMemoryLocation;

  if ((iAsTypeID & asTYPEID_APPOBJECT) == 0)
  {
    switch (iAsTypeID)
    {
      case asTYPEID_VOID:
        return W_FAILURE;

      case asTYPEID_BOOL:
        out_value = (*static_cast<WInt8*>(pMemLoc) != 0) ? true : false;
        return W_SUCCESS;

      case asTYPEID_INT8:
        out_value = *static_cast<WInt8*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_INT16:
        out_value = *static_cast<WInt16*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_INT32:
        out_value = *static_cast<WInt32*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_INT64:
        out_value = *static_cast<WInt64*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_UINT8:
        out_value = *static_cast<WUInt8*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_UINT16:
        out_value = *static_cast<WUInt16*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_UINT32:
        out_value = *static_cast<WUInt32*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_UINT64:
        out_value = *static_cast<WUInt64*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_FLOAT:
        out_value = *static_cast<float*>(pMemLoc);
        return W_SUCCESS;

      case asTYPEID_DOUBLE:
        out_value = *static_cast<double*>(pMemLoc);
        return W_SUCCESS;
    }

    if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
    {
      if (pInfo->GetFlags() & asOBJ_ENUM)
      {
        out_value = *static_cast<WInt32*>(pMemLoc);
        return W_SUCCESS;
      }
    }
  }
  else if (const asITypeInfo* pInfo = pEngine->GetTypeInfoById(iAsTypeID))
  {
    const WRTTI* pRtti = (const WRTTI*)pInfo->GetUserData(WAsUserData::RttiPtr);

    if (pRtti->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags))
    {
      out_value = *static_cast<WUInt32*>(pMemLoc);
      return W_SUCCESS;
    }

    if (pRtti == WGetStaticRTTI<WAngle>())
    {
      out_value = *static_cast<WAngle*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WTime>())
    {
      out_value = *static_cast<WTime*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WColor>())
    {
      out_value = *static_cast<WColor*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WColorGammaUB>())
    {
      out_value = *static_cast<WColorGammaUB*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WVec2>())
    {
      out_value = *static_cast<WVec2*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WVec3>())
    {
      out_value = *static_cast<WVec3*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WVec4>())
    {
      out_value = *static_cast<WVec4*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WQuat>())
    {
      out_value = *static_cast<WQuat*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WTransform>())
    {
      out_value = *static_cast<WTransform*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WString>())
    {
      out_value = *static_cast<WString*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WStringView>())
    {
      out_value = *static_cast<WStringView*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WHashedString>())
    {
      out_value = *static_cast<WHashedString*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WTempHashedString>())
    {
      out_value = *static_cast<WTempHashedString*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WGameObjectHandle>())
    {
      out_value = *static_cast<WGameObjectHandle*>(pMemLoc);
      return W_SUCCESS;
    }
    else if (pRtti == WGetStaticRTTI<WComponentHandle>())
    {
      out_value = *static_cast<WComponentHandle*>(pMemLoc);
      return W_SUCCESS;
    }
  }

  // currently unsupported type for exposed parameter
  return W_FAILURE;
}

const char* WAngelScriptUtils::VariantTypeToString(WVariantType::Enum type)
{
  switch (type)
  {
    case WVariantType::Bool:
      return "bool";
    case WVariantType::Double:
      return "double";
    case WVariantType::Float:
      return "float";
    case WVariantType::Int8:
      return "int8";
    case WVariantType::Int16:
      return "int16";
    case WVariantType::Int32:
      return "int32";
    case WVariantType::Int64:
      return "int64";
    case WVariantType::UInt8:
      return "uint8";
    case WVariantType::UInt16:
      return "uint16";
    case WVariantType::UInt32:
      return "uint32";
    case WVariantType::UInt64:
      return "uint64";
    case WVariantType::Angle:
      return "WAngle";
    case WVariantType::Matrix3:
      return "WMat3";
    case WVariantType::Matrix4:
      return "WMat4";
    case WVariantType::Quaternion:
      return "WQuat";
    case WVariantType::Time:
      return "WTime";
    case WVariantType::Transform:
      return "WTransform";
    case WVariantType::Vector2:
      return "WVec2";
    case WVariantType::Vector3:
      return "WVec3";
    case WVariantType::Vector4:
      return "WVec4";
    case WVariantType::Color:
      return "WColor";
    case WVariantType::ColorGamma:
      return "WColorGammaUB";
    case WVariantType::String:
      return "WString";
    case WVariantType::HashedString:
      return "WHashedString";
    case WVariantType::StringView:
      return "WStringView";
    case WVariantType::TempHashedString:
      return "WTempHashedString";

    default:
      break;
  }

  return nullptr;
}

WString WAngelScriptUtils::DefaultValueToString(const WVariant& value, WVariantType::Enum expectedType)
{
  WStringBuilder s;
  switch (expectedType)
  {
    case WVariantType::Angle:
      s.SetFormat("WAngle::MakeDegrees({})", value.Get<WAngle>().GetDegree());
      return s;
    case WVariantType::Bool:
      s.Set(value.ConvertTo<bool>() ? "true" : "false");
      return s;
    case WVariantType::Color:
      s.SetFormat("WColor({}, {}, {}, {})", value.Get<WColor>().r, value.Get<WColor>().g, value.Get<WColor>().b, value.Get<WColor>().a);
      return s;

    case WVariantType::ColorGamma:
      s.SetFormat("WColorGammaUB({}, {}, {}, {})", value.Get<WColorGammaUB>().r, value.Get<WColorGammaUB>().g, value.Get<WColorGammaUB>().b, value.Get<WColorGammaUB>().a);
      return s;

    case WVariantType::Double:
      s.SetFormat("{}", value.ConvertTo<double>());
      return s;

    case WVariantType::Float:
      s.SetFormat("{}", value.ConvertTo<float>());
      return s;

    case WVariantType::HashedString:
    case WVariantType::String:
    case WVariantType::StringView:
      s.SetFormat("\"{}\"", value.ConvertTo<WString>());
      return s;

    case WVariantType::Int8:
    case WVariantType::Int16:
    case WVariantType::Int32:
    case WVariantType::Int64:
    {
      // the value may be stored with a different signedness or width than the argument
      // (e.g. WInvalidIndex as WUInt32 for an WInt32 argument), so truncate it to the
      // argument's width - otherwise AngelScript reports "Value is too large for data type"
      // at every call site that uses the default argument
      WInt64 iValue = value.ConvertTo<WInt64>();

      if (expectedType == WVariantType::Int8)
        iValue = static_cast<WInt8>(iValue & 0xFF);
      else if (expectedType == WVariantType::Int16)
        iValue = static_cast<WInt16>(iValue & 0xFFFF);
      else if (expectedType == WVariantType::Int32)
        iValue = static_cast<WInt32>(iValue & 0xFFFFFFFF);

      s.SetFormat("{}", iValue);
      return s;
    }

    case WVariantType::UInt8:
    case WVariantType::UInt16:
    case WVariantType::UInt32:
    case WVariantType::UInt64:
    {
      // same as above: e.g. (WInt32)WInvalidIndex == -1 would sign-extend to a 64 bit value
      WUInt64 uiValue = value.ConvertTo<WUInt64>();

      if (expectedType == WVariantType::UInt8)
        uiValue &= 0xFFu;
      else if (expectedType == WVariantType::UInt16)
        uiValue &= 0xFFFFu;
      else if (expectedType == WVariantType::UInt32)
        uiValue &= 0xFFFFFFFFu;

      s.SetFormat("{}", uiValue);
      return s;
    }

    case WVariantType::Matrix3:
      s.Set("WMat3::MakeIdentity()");
      return s;
    case WVariantType::Matrix4:
      s.Set("WMat4::MakeIdentity()");
      return s;

    case WVariantType::Quaternion:
      s.Set("WQuat::MakeIdentity()");
      return s;

    case WVariantType::Time:
      s.SetFormat("WTime::Seconds({})", value.Get<WTime>().GetSeconds());
      return s;

    case WVariantType::Transform:
      s.Set("WTransform::MakeIdentity()");
      return s;

    case WVariantType::Vector2:
      s.SetFormat("WVec2({}, {})", value.Get<WVec2>().x, value.Get<WVec2>().y);
      return s;
    case WVariantType::Vector3:
      s.SetFormat("WVec3({}, {}, {})", value.Get<WVec3>().x, value.Get<WVec3>().y, value.Get<WVec3>().z);
      return s;
    case WVariantType::Vector4:
      s.SetFormat("WVec4({}, {}, {}, {})", value.Get<WVec4>().x, value.Get<WVec4>().y, value.Get<WVec4>().z, value.Get<WVec4>().w);
      return s;

    default:
      break;
  }

  W_ASSERT_NOT_IMPLEMENTED;
  return "";
}

void WAngelScriptUtils::RetrieveArg(asIScriptGeneric* pGen, WUInt32 uiRealArg, WInt32& ref_iSkippedArg, const WAbstractFunctionProperty* pAbstractFuncProp, WVariant& out_arg)
{
  const WRTTI* pArgRtti = pAbstractFuncProp->GetArgumentType(uiRealArg);

  if (pArgRtti->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags))
  {
    out_arg = (WInt32)pGen->GetArgDWord(ref_iSkippedArg);
    return;
  }

  const WVariantType::Enum type = pArgRtti->GetVariantType();
  switch (type)
  {
    case WVariantType::Bool:
      out_arg = pGen->GetArgByte(ref_iSkippedArg) != 0;
      return;
    case WVariantType::Double:
      out_arg = pGen->GetArgDouble(ref_iSkippedArg);
      return;
    case WVariantType::Float:
      out_arg = pGen->GetArgFloat(ref_iSkippedArg);
      return;
    case WVariantType::Int8:
      out_arg = (WInt8)pGen->GetArgByte(ref_iSkippedArg);
      return;
    case WVariantType::Int16:
      out_arg = (WInt16)pGen->GetArgWord(ref_iSkippedArg);
      return;
    case WVariantType::Int32:
      out_arg = (WInt32)pGen->GetArgDWord(ref_iSkippedArg);
      return;
    case WVariantType::Int64:
      out_arg = (WInt64)pGen->GetArgQWord(ref_iSkippedArg);
      return;
    case WVariantType::UInt8:
      out_arg = (WUInt8)pGen->GetArgByte(ref_iSkippedArg);
      return;
    case WVariantType::UInt16:
      out_arg = (WUInt16)pGen->GetArgWord(ref_iSkippedArg);
      return;
    case WVariantType::UInt32:
      out_arg = (WUInt32)pGen->GetArgDWord(ref_iSkippedArg);
      return;
    case WVariantType::UInt64:
      out_arg = (WUInt64)pGen->GetArgQWord(ref_iSkippedArg);
      return;

    case WVariantType::Vector2:
      out_arg = *((const WVec2*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Vector3:
      out_arg = *((const WVec3*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Vector4:
      out_arg = *((const WVec4*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Quaternion:
      out_arg = *((const WQuat*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Matrix3:
      out_arg = *((const WMat3*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Matrix4:
      out_arg = *((const WMat4*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Transform:
      out_arg = *((const WTransform*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Time:
      out_arg = *((const WTime*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Angle:
      out_arg = *((const WAngle*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::Color:
      out_arg = *((const WColor*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::ColorGamma:
      out_arg = *((const WColorGammaUB*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::String:
      out_arg = *((const WString*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::HashedString:
      out_arg = *((const WHashedString*)pGen->GetArgObject(ref_iSkippedArg));
      return;
    case WVariantType::StringView:
      out_arg = WVariant(*(const WStringView*)pGen->GetArgObject(ref_iSkippedArg), false);
      return;
    case WVariantType::TempHashedString:
      out_arg = *((const WTempHashedString*)pGen->GetArgObject(ref_iSkippedArg));
      return;

    case WVariantType::VariantArray:
      RetrieveVarArgs(pGen, ref_iSkippedArg, pAbstractFuncProp, out_arg);
      return;

    case WVariantType::Invalid:
    case WVariantType::TypedObject:
      // handled below
      break;

      W_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (pArgRtti == WGetStaticRTTI<WWorld>())
  {
    out_arg = WAngelScriptUtils::GetThreadLocalWorld();
    // out_arg = (WWorld*)pGen->GetArgObject(uiArg);
    --ref_iSkippedArg;
    return;
  }

  if (pArgRtti == WGetStaticRTTI<WGameObjectHandle>())
  {
    out_arg = (WGameObjectHandle*)pGen->GetArgObject(ref_iSkippedArg);
    return;
  }

  if (pArgRtti == WGetStaticRTTI<WComponentHandle>())
  {
    out_arg = (WComponentHandle*)pGen->GetArgObject(ref_iSkippedArg);
    return;
  }

  if (pArgRtti == WGetStaticRTTI<WGameObject>())
  {
    out_arg = (WGameObject*)pGen->GetArgObject(ref_iSkippedArg);
    return;
  }

  if (pArgRtti->IsDerivedFrom(WGetStaticRTTI<WComponent>()))
  {
    out_arg = (WComponent*)pGen->GetArgObject(ref_iSkippedArg);
    return;
  }

  if (pArgRtti->GetTypeName().StartsWith("WVariant"))
  {
    auto argTypeId = pGen->GetArgTypeId(ref_iSkippedArg);

    if (WAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), argTypeId, pGen->GetArgAddress(ref_iSkippedArg), out_arg).Succeeded())
      return;

    const char* typeName = "null";
    if (const asITypeInfo* pInfo = pGen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    WLog::Error("Call to '{}': Argument {} got an unsupported type '{}' ({})", pAbstractFuncProp->GetPropertyName(), ref_iSkippedArg, typeName, argTypeId);
    return;
  }

  W_ASSERT_NOT_IMPLEMENTED;
  // out_arg = WVariant(pGen->GetArgObject(uiArg), pArgRtti); // works, but currently isn't needed
}

void WAngelScriptUtils::RetrieveVarArgs(asIScriptGeneric* pGen, WUInt32 uiStartArg, const WAbstractFunctionProperty* pAbstractFuncProp, WVariant& out_arg)
{
  WVariantArray resArr;

  for (WUInt32 uiArg = uiStartArg; uiArg < (WUInt32)pGen->GetArgCount(); ++uiArg)
  {
    auto argTypeId = pGen->GetArgTypeId(uiArg);

    WVariant res;
    if (WAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), argTypeId, pGen->GetArgAddress(uiArg), res).Succeeded())
    {
      resArr.PushBack(res);
      continue;
    }

    WStringBuilder typeName("null");
    if (const asITypeInfo* pInfo = pGen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    WLog::Error("Call to '{}': Argument {} got an unsupported type '{}' ({})", pAbstractFuncProp->GetPropertyName(), uiArg, typeName, argTypeId);
    break;
  }

  out_arg = resArr;
}

void WAngelScriptUtils::MakeGenericFunctionCall(asIScriptGeneric* pGen)
{
  const WAbstractFunctionProperty* pAbstractFuncProp = (const WAbstractFunctionProperty*)pGen->GetAuxiliary();
  const WScriptableFunctionAttribute* pFuncAttr = pAbstractFuncProp->GetAttributeByType<WScriptableFunctionAttribute>();
  void* pObject = pGen->GetObject();

  W_ASSERT_DEBUG(pAbstractFuncProp->GetArgumentCount() < 12, "Too many arguments");
  WVariant args[12];
  bool bHasOutArgs = false;

  WInt32 iNextArg = 0;
  for (WUInt32 uiArg = 0; uiArg < pAbstractFuncProp->GetArgumentCount(); ++uiArg)
  {
    if (pFuncAttr->GetArgumentType(uiArg) == WScriptableFunctionAttribute::ArgType::Out)
    {
      bHasOutArgs = true;
    }

    WAngelScriptUtils::RetrieveArg(pGen, uiArg, iNextArg, pAbstractFuncProp, args[uiArg]);
    ++iNextArg;
  }

  WVariant ret;
  pAbstractFuncProp->Execute(pObject, args, ret);

  if (bHasOutArgs)
  {
    for (WUInt32 uiArg = 0; uiArg < pAbstractFuncProp->GetArgumentCount(); ++uiArg)
    {
      if (pFuncAttr->GetArgumentType(uiArg) == WScriptableFunctionAttribute::ArgType::Out)
      {
        WAngelScriptUtils::WriteToAsTypeAtLocation(pGen->GetEngine(), pGen->GetArgTypeId(uiArg), pGen->GetArgAddress(uiArg), args[uiArg]).AssertSuccess();
      }
    }
  }

  const WRTTI* pReturnType = pAbstractFuncProp->GetReturnType();
  if (pReturnType != nullptr && pReturnType != WGetStaticRTTI<WVariantArray>())
  {
    const WRTTI* pRetRtti = WAngelScriptUtils::MapToRTTI(pGen->GetReturnTypeId(), pGen->GetEngine());
    WAngelScriptUtils::DefaultConstructInPlace(pGen->GetAddressOfReturnLocation(), pRetRtti);
    WAngelScriptUtils::WriteToAsTypeAtLocation(pGen->GetEngine(), pGen->GetReturnTypeId(), pGen->GetAddressOfReturnLocation(), ret).AssertSuccess();
  }
}

void WAngelScriptUtils::DefaultConstructInPlace(void* pPtr, const WRTTI* pRtti)
{
  if (pRtti == WGetStaticRTTI<WString>())
  {
    new (pPtr) WString();
    return;
  }

  if (pRtti == WGetStaticRTTI<WStringBuilder>())
  {
    new (pPtr) WStringBuilder();
    return;
  }

  if (pRtti == WGetStaticRTTI<WHashedString>())
  {
    new (pPtr) WHashedString();
    return;
  }

  // TODO: add other non-POD types here as needed
}

WString WAngelScriptUtils::RegisterEnumType(asIScriptEngine* pEngine, const WRTTI* pEnumType)
{
  WStringBuilder enumName = pEnumType->GetTypeName();
  enumName.ReplaceAll("::", "_");

  asITypeInfo* pEnumTypeInfo = pEngine->GetTypeInfoByName(enumName);
  if (pEnumTypeInfo != nullptr)
    return enumName;

  pEngine->RegisterEnum(enumName);

  pEnumTypeInfo = pEngine->GetTypeInfoByName(enumName);
  pEnumTypeInfo->SetUserData((void*)pEnumType, WAsUserData::RttiPtr);

  WTempHybridArray<WReflectionUtils::EnumKeyValuePair, 16> enumValues;
  WReflectionUtils::GetEnumKeysAndValues(pEnumType, enumValues, WReflectionUtils::EnumConversionMode::ValueNameOnly);
  for (auto& enumValue : enumValues)
  {
    pEngine->RegisterEnumValue(enumName, enumValue.m_sKey, enumValue.m_iValue);
  }

  return enumName;
}

static void SetPropertyGeneric(asIScriptGeneric* pGen)
{
  const WAbstractMemberProperty* pMember = static_cast<const WAbstractMemberProperty*>(pGen->GetAuxiliary());

  WVariant value;
  WAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), pGen->GetArgTypeId(0), pGen->GetAddressOfArg(0), value).AssertSuccess();

  WReflectionUtils::SetMemberPropertyValue(pMember, pGen->GetObject(), value);
}

static void GetPropertyGeneric(asIScriptGeneric* pGen)
{
  const WAbstractMemberProperty* pMember = static_cast<const WAbstractMemberProperty*>(pGen->GetAuxiliary());

  const WVariant value = WReflectionUtils::GetMemberPropertyValue(pMember, pGen->GetObject());

  const WRTTI* pRtti = WAngelScriptUtils::MapToRTTI(pGen->GetReturnTypeId(), pGen->GetEngine());
  WAngelScriptUtils::DefaultConstructInPlace(pGen->GetAddressOfReturnLocation(), pRtti);

  WAngelScriptUtils::WriteToAsTypeAtLocation(pGen->GetEngine(), pGen->GetReturnTypeId(), pGen->GetAddressOfReturnLocation(), value).AssertSuccess();
}

void WAngelScriptUtils::RegisterTypeProperties(asIScriptEngine* pEngine, const char* szTypeName, const WRTTI* pRtti, bool bIsInherited)
{
  if (pRtti == nullptr)
    return;

  intptr_t flags = 0;
  if (bIsInherited)
  {
    flags |= 0x01;
  }

  WArrayPtr<const WAbstractProperty* const> properties = pRtti->GetProperties();

  WStringBuilder funcName, sVarTypeName;

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() == WPropertyCategory::Member)
    {
      const WAbstractMemberProperty* pMember = static_cast<const WAbstractMemberProperty*>(pProp);

      sVarTypeName.Clear();

      if (pMember->GetFlags().IsAnySet(WPropertyFlags::IsEnum | WPropertyFlags::Bitflags))
      {
        const WAbstractEnumerationProperty* pEnumProp = static_cast<const WAbstractEnumerationProperty*>(pMember);

        sVarTypeName = WAngelScriptUtils::RegisterEnumType(pEngine, pEnumProp->GetSpecificType());
      }
      else
      {
        const WRTTI* pPropRtti = pMember->GetSpecificType();

        sVarTypeName = WAngelScriptUtils::VariantTypeToString(pPropRtti->GetVariantType());

        if (sVarTypeName.IsEmpty())
        {
          if (pPropRtti->GetTypeName() == "WGameObjectHandle" || pPropRtti->GetTypeName() == "WComponentHandle")
          {
            sVarTypeName = pPropRtti->GetTypeName();
          }
        }
      }

      if (!sVarTypeName.IsEmpty())
      {
        if (!pMember->GetFlags().IsAnySet(WPropertyFlags::Const | WPropertyFlags::ReadOnly))
        {
          funcName.Set("void set_", pMember->GetPropertyName(), "(", sVarTypeName, ") property ");
          const int funcID = pEngine->RegisterObjectMethod(szTypeName, funcName, asFUNCTION(SetPropertyGeneric), asCALL_GENERIC, (void*)pMember);
          AS_CHECK(funcID);
          pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);
        }

        {
          funcName.Set(sVarTypeName, " get_", pMember->GetPropertyName(), "() const property");
          const int funcID = pEngine->RegisterObjectMethod(szTypeName, funcName, asFUNCTION(GetPropertyGeneric), asCALL_GENERIC, (void*)pMember);
          AS_CHECK(funcID);
          pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);
        }
      }
    }
  }

  RegisterTypeProperties(pEngine, szTypeName, pRtti->GetParentType(), true);
}
