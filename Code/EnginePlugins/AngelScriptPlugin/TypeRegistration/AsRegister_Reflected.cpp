#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Foundation/Reflection/ReflectionUtils.h>

bool WAngelScriptEngineSingleton::AppendType(WStringBuilder& decl, const WRTTI* pRtti, const WScriptableFunctionAttribute* pFuncAttr, WUInt32 uiArg)
{
  const bool bIsReturnValue = uiArg == WInvalidIndex;
  const auto argType = pFuncAttr ? pFuncAttr->GetArgumentType(uiArg) : WScriptableFunctionAttribute::ArgType::In;

  if (pRtti == nullptr || pRtti == WGetStaticRTTI<WVariantArray>())
  {
    decl.Append("void");
    return bIsReturnValue;
  }

  if (argType == WScriptableFunctionAttribute::ArgType::Inout)
  {
    // not yet supported for most types
    return false;
  }

  if (const char* szTypeName = WAngelScriptUtils::VariantTypeToString(pRtti->GetVariantType()); szTypeName != nullptr)
  {
    decl.Append(szTypeName);

    if (argType == WScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }

    return true;
  }

  if (pRtti->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags))
  {
    decl.Append(WAngelScriptUtils::RegisterEnumType(m_pEngine, pRtti));

    if (argType == WScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }
    return true;
  }

  if (!bIsReturnValue)
  {
    if (pRtti == WGetStaticRTTI<WVariant>())
    {
      decl.Append("?& in");
      return true;
    }

    if (pRtti == WGetStaticRTTI<WWorld>())
    {
      // skip
      return true;
    }
  }

  if (pRtti == WGetStaticRTTI<WGameObjectHandle>() || pRtti == WGetStaticRTTI<WComponentHandle>())
  {
    decl.Append(pRtti->GetTypeName());

    if (argType == WScriptableFunctionAttribute::ArgType::Out)
    {
      decl.Append("& out");
    }

    return true;
  }

  if (m_WhitelistedRefTypes.Contains(pRtti->GetTypeName()))
  {
    decl.Append(pRtti->GetTypeName(), "@");
    return true;
  }

  decl.Append(pRtti->GetTypeName());
  m_NotRegistered.Insert(decl);
  return false;
}

bool WAngelScriptEngineSingleton::AppendFuncArgs(WStringBuilder& decl, const WAbstractFunctionProperty* pFunc, const WScriptableFunctionAttribute* pFuncAttr, WUInt32 uiArg)
{
  if (uiArg > 12)
  {
    W_ASSERT_DEBUG(false, "Too many function arguments");
    return false;
  }

  if (uiArg > 0 && !decl.EndsWith("("))
  {
    decl.Append(", ");
  }

  return AppendType(decl, pFunc->GetArgumentType(uiArg), pFuncAttr, uiArg);
}

void WAngelScriptEngineSingleton::Register_GlobalReflectedFunctions()
{
  W_LOG_BLOCK("Register_GlobalReflectedFunctions");

  WRTTI::ForEachType([&](const WRTTI* pRtti)
    {
      if (pRtti->GetParentType() != nullptr && pRtti->GetParentType() != WGetStaticRTTI<WNoBase>())
        return;

      for (auto pFunc : pRtti->GetFunctions())
      {
        auto pFuncAttr = pFunc->GetAttributeByType<WScriptableFunctionAttribute>();
        if (!pFuncAttr)
          continue;

        if (pFunc->GetFunctionType() != WFunctionType::StaticMember)
          continue;

        RegisterGenericFunction(pRtti->GetTypeName().GetStartPointer(), pFunc, pFuncAttr, false);
      }

      //
    });
}

static void CastToBase(asIScriptGeneric* pGen)
{
  int derivedTypeId = pGen->GetObjectTypeId();
  auto derivedTypeInfo = pGen->GetEngine()->GetTypeInfoById(derivedTypeId);
  const WRTTI* pDerivedRtti = (const WRTTI*)derivedTypeInfo->GetUserData(WAsUserData::RttiPtr);

  const WRTTI* pBaseRtti = (const WRTTI*)pGen->GetAuxiliary();

  if (pDerivedRtti != nullptr && pBaseRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      pGen->SetReturnObject(pGen->GetObject());
      return;
    }
  }

  pGen->SetReturnObject(nullptr);
}

static void CastToDerived(asIScriptGeneric* pGen)
{
  int baseTypeId = pGen->GetObjectTypeId();
  auto baseTypeInfo = pGen->GetEngine()->GetTypeInfoById(baseTypeId);
  const WRTTI* pBaseRtti = (const WRTTI*)baseTypeInfo->GetUserData(WAsUserData::RttiPtr);

  const WRTTI* pDerivedRtti = (const WRTTI*)pGen->GetAuxiliary();

  if (pBaseRtti != nullptr && pDerivedRtti != nullptr)
  {
    if (pDerivedRtti->IsDerivedFrom(pBaseRtti))
    {
      pGen->SetReturnObject(pGen->GetObject());
      return;
    }
  }

  pGen->SetReturnObject(nullptr);
}

struct RefInstance
{
  WUInt32 m_uiRefCount = 1;
  const WRTTI* m_pRtti = nullptr;
};

static WMutex s_RefCountMutex;
static WMap<void*, RefInstance> s_RefCounts;

static void* WRtti_Create(const WRTTI* pRtti)
{
  auto inst = pRtti->GetAllocator()->Allocate<WReflectedClass>();

  W_LOCK(s_RefCountMutex);
  auto& ref = s_RefCounts[inst.m_pInstance];
  ref.m_pRtti = pRtti;

  return inst.m_pInstance;
}

static void WRtti_AddRef(void* pInstance)
{
  W_LOCK(s_RefCountMutex);
  ++s_RefCounts[pInstance].m_uiRefCount;
}

static void WRtti_Release(void* pInstance)
{
  W_LOCK(s_RefCountMutex);
  auto it = s_RefCounts.Find(pInstance);
  RefInstance& ri = it.Value();
  if (--ri.m_uiRefCount == 0)
  {
    ri.m_pRtti->GetAllocator()->Deallocate(pInstance);
    s_RefCounts.Remove(it);
  }
}


void WAngelScriptEngineSingleton::Register_ReflectedType(const WRTTI* pBaseType, bool bCreatable)
{
  W_LOG_BLOCK("Register_ReflectedType", pBaseType->GetTypeName());

  // first register the type
  WRTTI::ForEachDerivedType(pBaseType, [&](const WRTTI* pRtti)
    {
      if (pRtti->GetAttributeByType<WHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<WExcludeFromScript>() != nullptr)
        return;

      WStringBuilder typeName = pRtti->GetTypeName();
      auto pTypeInfo = m_pEngine->GetTypeInfoByName(typeName);

      m_WhitelistedRefTypes.Insert(typeName);

      if (pTypeInfo == nullptr)
      {
        if (bCreatable)
        {
          const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF);
          AS_CHECK(typeId);
          pTypeInfo = m_pEngine->GetTypeInfoById(typeId);

          if (pRtti->GetAllocator() != nullptr && pRtti->GetAllocator()->CanAllocate())
          {
            const WStringBuilder sFactoryOp(typeName, "@ f()");
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_FACTORY, sFactoryOp, asFUNCTION(WRtti_Create), asCALL_CDECL_OBJLAST, (void*)pRtti);
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_ADDREF, "void f()", asFUNCTION(WRtti_AddRef), asCALL_CDECL_OBJLAST, (void*)pRtti);
            m_pEngine->RegisterObjectBehaviour(typeName, asBEHAVE_RELEASE, "void f()", asFUNCTION(WRtti_Release), asCALL_CDECL_OBJLAST, (void*)pRtti);
          }
        }
        else
        {

          const int typeId = m_pEngine->RegisterObjectType(typeName, 0, asOBJ_REF | asOBJ_NOCOUNT);
          AS_CHECK(typeId);

          pTypeInfo = m_pEngine->GetTypeInfoById(typeId);
        }
      }

      pTypeInfo->SetUserData((void*)pRtti, WAsUserData::RttiPtr);

      AddForbiddenType(typeName);

      RegisterTypeFunctions(typeName, pRtti, false);
      WAngelScriptUtils::RegisterTypeProperties(m_pEngine, typeName, pRtti, false);

      //
    },
    WRTTI::ForEachOptions::None);

  // then register the type hierarchy
  WRTTI::ForEachDerivedType(pBaseType, [&](const WRTTI* pRtti)
    {
      if (pRtti == pBaseType)
        return;

      if (pRtti->GetAttributeByType<WHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<WExcludeFromScript>() != nullptr)
        return;

      const WStringBuilder typeName = pRtti->GetTypeName();

      const WRTTI* pParentRtti = pRtti->GetParentType();

      WStringBuilder parentName, castOp;

      while (pParentRtti)
      {
        parentName = pParentRtti->GetTypeName();
        castOp.Set(parentName, "@ opImplCast()");

        AS_CHECK(m_pEngine->RegisterObjectMethod(typeName, castOp, asFUNCTION(CastToBase), asCALL_GENERIC, (void*)pParentRtti));

        castOp.Set(typeName, "@ opCast()");
        AS_CHECK(m_pEngine->RegisterObjectMethod(parentName, castOp, asFUNCTION(CastToDerived), asCALL_GENERIC, (void*)pRtti));

        if (pParentRtti == pBaseType)
          break;

        pParentRtti = pParentRtti->GetParentType();
      }
      //
    },
    WRTTI::ForEachOptions::None);
}

void WAngelScriptEngineSingleton::RegisterTypeFunctions(const char* szTypeName, const WRTTI* pRtti, bool bIsInherited)
{
  if (pRtti == nullptr || pRtti == WGetStaticRTTI<WReflectedClass>())
    return;

  for (auto pFunc : pRtti->GetFunctions())
  {
    auto pFuncAttr = pFunc->GetAttributeByType<WScriptableFunctionAttribute>();

    if (!pFuncAttr)
      continue;

    RegisterGenericFunction(szTypeName, pFunc, pFuncAttr, bIsInherited);
  }

  RegisterTypeFunctions(szTypeName, pRtti->GetParentType(), true);
}

static void CollectFunctionArgumentAttributes(const WAbstractFunctionProperty* pFuncProp, WDynamicArray<const WFunctionArgumentAttributes*>& out_attributes)
{
  for (auto pAttr : pFuncProp->GetAttributes())
  {
    if (auto pFuncArgAttr = WDynamicCast<const WFunctionArgumentAttributes*>(pAttr))
    {
      WUInt32 uiArgIndex = pFuncArgAttr->GetArgumentIndex();
      out_attributes.EnsureCount(uiArgIndex + 1);
      W_ASSERT_DEV(out_attributes[uiArgIndex] == nullptr, "Multiple argument attributes for argument {} of '{}'", uiArgIndex, pFuncProp->GetPropertyName());
      W_ASSERT_DEV(uiArgIndex < pFuncProp->GetArgumentCount(), "Function argument attribute for argument {} of '{}' which only has {} arguments.", uiArgIndex, pFuncProp->GetPropertyName(), pFuncProp->GetArgumentCount());
      out_attributes[uiArgIndex] = pFuncArgAttr;
    }
  }
}

static bool ExistsGlobalFunc(asIScriptEngine* pEngine, const char* szNamespace, const char* szName)
{
  for (WUInt32 i = 0; i < pEngine->GetGlobalFunctionCount(); ++i)
  {
    auto pFunc = pEngine->GetGlobalFunctionByIndex(i);

    if (WStringUtils::IsEqual(pFunc->GetNamespace(), szNamespace) && WStringUtils::IsEqual(pFunc->GetName(), szName))
    {
      return true;
    }
  }

  return false;
}

void WAngelScriptEngineSingleton::RegisterGenericFunction(const char* szTypeName, const WAbstractFunctionProperty* const pFunc, const WScriptableFunctionAttribute* pFuncAttr, bool bIsInherited)
{
  WStringBuilder sFuncName = pFunc->GetPropertyName();
  sFuncName.TrimWordStart("Reflection_");

  if (pFunc->GetReturnType() == WGetStaticRTTI<WVariant>())
  {
    WStringBuilder sFuncName2;

    sFuncName2.Set(sFuncName, "_asBool");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<bool>());

    sFuncName2.Set(sFuncName, "_asInt32");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WInt32>());

    sFuncName2.Set(sFuncName, "_asFloat");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<float>());

    sFuncName2.Set(sFuncName, "_asTime");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WTime>());

    sFuncName2.Set(sFuncName, "_asAngle");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WAngle>());

    sFuncName2.Set(sFuncName, "_asVec2");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WVec2>());

    sFuncName2.Set(sFuncName, "_asVec3");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WVec3>());

    sFuncName2.Set(sFuncName, "_asVec4");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WVec4>());

    sFuncName2.Set(sFuncName, "_asQuat");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WQuat>());

    sFuncName2.Set(sFuncName, "_asColor");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WColor>());

    sFuncName2.Set(sFuncName, "_asString");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WString>());

    sFuncName2.Set(sFuncName, "_asGameObjectHandle");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WGameObjectHandle>());

    sFuncName2.Set(sFuncName, "_asComponentHandle");
    RegisterSingleGenericFunction(sFuncName2, szTypeName, pFunc, pFuncAttr, bIsInherited, WGetStaticRTTI<WComponentHandle>());
  }
  else
  {
    RegisterSingleGenericFunction(sFuncName, szTypeName, pFunc, pFuncAttr, bIsInherited, pFunc->GetReturnType());
  }
}

void WAngelScriptEngineSingleton::RegisterSingleGenericFunction(const char* szFuncName, const char* szTypeName, const WAbstractFunctionProperty* const pFunc, const WScriptableFunctionAttribute* pFuncAttr, bool bIsInherited, const WRTTI* pReturnType)
{
  bool bVarArgs = false;
  if (const WDynamicPinAttribute* pVarArgsAttr = pFunc->GetAttributeByType<WDynamicPinAttribute>())
  {
    W_ASSERT_DEV(pVarArgsAttr->GetProperty() == pFuncAttr->GetArgumentName(pFuncAttr->GetArgumentCount() - 1), "Var args must be on the last argument of the function");
    bVarArgs = true;
  }

  WStringBuilder decl;

  if (!AppendType(decl, pReturnType, nullptr, WInvalidIndex))
  {
    return;
  }

  WStringBuilder sNamespace;
  decl.Append(" ");

  if (pFunc->GetFunctionType() == WFunctionType::StaticMember)
  {
    // turn things like 'WScriptExtensionClass_CVar' into 'WCVar'
    if (const char* szUnderScore = WStringUtils::FindLastSubString(szTypeName, "_"))
    {
      if (WStringUtils::StartsWith(szTypeName, "W"))
      {
        sNamespace.Append("W");
      }

      sNamespace.Append(szUnderScore + 1);
    }
    else
    {
      sNamespace.Append(szTypeName);
    }
  }

  decl.Append(szFuncName, "(");

  WTempHybridArray<const WFunctionArgumentAttributes*, 8> argAttributes;
  if (const WFunctionArgumentAttributes* pArgAttr = pFunc->GetAttributeByType<WFunctionArgumentAttributes>())
  {
    argAttributes.SetCount(pFunc->GetArgumentCount());
    CollectFunctionArgumentAttributes(pFunc, argAttributes);
  }

  bool bHasDefaultArgs = false;
  WVariant defaultValue;

  const WUInt32 uiArgCount = bVarArgs ? pFunc->GetArgumentCount() - 1 : pFunc->GetArgumentCount();
  for (WUInt32 uiArg = 0; uiArg < uiArgCount; ++uiArg)
  {
    if (!AppendFuncArgs(decl, pFunc, pFuncAttr, uiArg))
      return;

    if (bVarArgs)
    {
      // start with 0 arguments
      decl.TrimRight(" ,");
    }
    else
    {
      if (decl.EndsWith("(") || decl.EndsWith(", "))
      {
        decl.TrimRight(" ,");
        continue;
      }

      const WRTTI* pArgType = pFunc->GetArgumentType(uiArg);

      if (const char* szName = pFuncAttr->GetArgumentName(uiArg))
      {
        decl.Append(" ", szName);
      }

      if (bHasDefaultArgs)
      {
        defaultValue = WReflectionUtils::GetDefaultVariantFromType(pArgType);
      }

      if (!argAttributes.IsEmpty() && argAttributes[uiArg])
      {
        for (auto pArgAttr : argAttributes[uiArg]->GetArgumentAttributes())
        {
          if (const WDefaultValueAttribute* pDef = WDynamicCast<const WDefaultValueAttribute*>(pArgAttr))
          {
            bHasDefaultArgs = true;
            defaultValue = pDef->GetValue();
          }
        }
      }

      if (bHasDefaultArgs)
      {
        const bool bIsEnum = pArgType->GetTypeFlags().IsAnySet(WTypeFlags::IsEnum | WTypeFlags::Bitflags);

        decl.Append(" = ");

        if (bIsEnum)
        {
          decl.Append(pArgType->GetTypeName(), "(");
        }

        if (defaultValue.IsValid())
        {
          // AngelScript enums are 32 bit, so bitflag defaults like 0xFFFFFFFF have to be written as -1
          WVariantType::Enum expectedType = bIsEnum ? WVariantType::Int32 : pArgType->GetVariantType();
          decl.Append(WAngelScriptUtils::DefaultValueToString(defaultValue, expectedType));
        }
        else
        {
          decl.Append("0"); // fallback for enums
        }

        if (bIsEnum)
        {
          decl.Append(")");
        }
      }
    }
  }

  intptr_t flags = 0;
  if (bIsInherited)
  {
    flags |= 0x01;
  }

  for (WUInt32 uiVarArgOpt = 0; uiVarArgOpt < 9; ++uiVarArgOpt)
  {
    decl.Append(")");

    if (pFunc->GetFunctionType() == WFunctionType::Member)
    {
      if (pFunc->GetFlags().IsSet(WPropertyFlags::Const))
        decl.Append(" const");

      // only register methods that have not been registered before
      // this allows us to register more optimized versions first
      if (m_pEngine->GetTypeInfoByName(szTypeName)->GetMethodByDecl(decl) == nullptr)
      {
        const int funcID = m_pEngine->RegisterObjectMethod(szTypeName, decl, asFUNCTION(WAngelScriptUtils::MakeGenericFunctionCall), asCALL_GENERIC, (void*)pFunc);
        AS_CHECK(funcID);

        m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);
      }
    }
    else if (pFunc->GetFunctionType() == WFunctionType::StaticMember)
    {
      m_pEngine->SetDefaultNamespace(sNamespace);

      // only register functions that have not been registered before
      // this allows us to register more optimized versions first
      if (uiVarArgOpt > 0 || !ExistsGlobalFunc(m_pEngine, sNamespace, szFuncName))
      {
        const int funcID = m_pEngine->RegisterGlobalFunction(decl, asFUNCTION(WAngelScriptUtils::MakeGenericFunctionCall), asCALL_GENERIC, (void*)pFunc);
        AS_CHECK(funcID);

        m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);
      }

      m_pEngine->SetDefaultNamespace("");
    }

    if (!bVarArgs)
      break;

    decl.Shrink(0, 1);
    decl.AppendFormat(", ?& in VarArg{}", uiVarArgOpt + 1);
  }
}
