#pragma once

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/AngelScriptPluginDLL.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

class asIScriptEngine;
class asIScriptModule;
class asIStringFactory;
struct asSMessageInfo;
class WAsStringFactory;
class asITypeInfo;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
using WAsAllocatorType = WAllocatorWithPolicy<WAllocPolicyHeap, WAllocatorTrackingMode::AllocationStats>;
#else
using WAsAllocatorType = WAllocatorWithPolicy<WAllocPolicyHeap, WAllocatorTrackingMode::Nothing>;
#endif

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptEngineSingleton
{
  W_DECLARE_SINGLETON(WAngelScriptEngineSingleton);

public:
  static WResult PreprocessCode(WStringView sRefFilePath, WStringView sCode, WStringBuilder* out_pProcessedCode, WSet<WString>* out_pDependencies);
  static void FindCorrectSectionAndLine(const WDynamicArray<WStringView>& lines, WInt32& ref_iLine, WStringView& ref_sSection);

public:
  WAngelScriptEngineSingleton();
  ~WAngelScriptEngineSingleton();

  asIScriptEngine* GetEngine() const { return m_pEngine; }

  asIScriptModule* SetModuleCode(WStringView sModuleName, WStringView sCode, bool bAddExternalSection);
  asIScriptModule* CompileModule(WStringView sModuleName, WStringView sMainClass, WStringView sRefFilePath, WStringView sCode, WStringBuilder* out_pProcessedCode, WSet<WString>* out_pDependencies);
  WResult ValidateModule(asIScriptModule* pModule) const;

  const WSet<WString>& GetNotRegistered() const { return m_NotRegistered; }

private:
  void AddForbiddenType(const char* szTypeName);
  bool IsTypeForbidden(const asITypeInfo* pType) const;

  void CompilerMessageCallback(const asSMessageInfo* msg);
  void ExceptionCallback(asIScriptContext* pContext);

  void RegisterStandardTypes();
  void Register_RTTI();
  void Register_Vec2();
  void Register_Vec3();
  void Register_Vec4();
  void Register_Angle();
  void Register_Quat();
  void Register_Transform();
  void Register_GameObject();
  void Register_Time();
  void Register_Mat3();
  void Register_Mat4();
  void Register_World();
  void Register_Clock();
  void Register_String();
  void Register_StringView();
  void Register_StringBuilder();
  void Register_TempHashedString();
  void Register_HashedString();
  void Register_Color();
  void Register_ColorGammaUB();
  void Register_Random();
  void Register_Math();
  void Register_Spatial();

  void Register_WAngelScriptClass();
  void Register_GlobalReflectedFunctions();
  void Register_ReflectedType(const WRTTI* pBaseType, bool bCreatable);
  void Register_ReflectedTypes();
  void RegisterTypeFunctions(const char* szTypeName, const WRTTI* pRtti, bool bIsInherited);
  void RegisterSingleGenericFunction(const char* szFuncName, const char* szTypeName, const WAbstractFunctionProperty* const pFunc, const WScriptableFunctionAttribute* pFuncAttr, bool bIsInherited, const WRTTI* pReturnType);
  void RegisterGenericFunction(const char* szTypeName, const WAbstractFunctionProperty* const pFunc, const WScriptableFunctionAttribute* pFuncAttr, bool bIsInherited);
  bool AppendType(WStringBuilder& decl, const WRTTI* pRtti, const WScriptableFunctionAttribute* pFuncAttr, WUInt32 uiArg);
  bool AppendFuncArgs(WStringBuilder& decl, const WAbstractFunctionProperty* pFunc, const WScriptableFunctionAttribute* pFuncAttr, WUInt32 uiArg);
  void Register_ExtraComponentFuncs();


  template <typename T>
  void RegisterPodValueType(asUINT additonalFlags = 0)
  {
    const WRTTI* pRtti = WGetStaticRTTI<T>();
    int typeId = m_pEngine->RegisterObjectType(pRtti->GetTypeName().GetStartPointer(), sizeof(T), asOBJ_VALUE | asOBJ_POD | asGetTypeTraits<T>() | additonalFlags);
    AS_CHECK(typeId);

    m_pEngine->GetTypeInfoById(typeId)->SetUserData((void*)pRtti, WAsUserData::RttiPtr);
  }

  template <typename T>
  void RegisterNonPodValueType()
  {
    const WRTTI* pRtti = WGetStaticRTTI<T>();
    int typeId = m_pEngine->RegisterObjectType(pRtti->GetTypeName().GetStartPointer(), sizeof(T), asOBJ_VALUE | asGetTypeTraits<T>());
    AS_CHECK(typeId);

    m_pEngine->GetTypeInfoById(typeId)->SetUserData((void*)pRtti, WAsUserData::RttiPtr);
  }

  template <typename T>
  void RegisterRefType()
  {
    const WRTTI* pRtti = WGetStaticRTTI<T>();
    int typeId = m_pEngine->RegisterObjectType(pRtti->GetTypeName().GetStartPointer(), 0, asOBJ_REF | asOBJ_NOCOUNT);
    AS_CHECK(typeId);

    m_pEngine->GetTypeInfoById(typeId)->SetUserData((void*)pRtti, WAsUserData::RttiPtr);

    AddForbiddenType(pRtti->GetTypeName().GetStartPointer());

    m_WhitelistedRefTypes.Insert(pRtti->GetTypeName());
  }

  asIScriptEngine* m_pEngine = nullptr;

  WSet<WString> m_WhitelistedRefTypes;

  WHybridArray<const asITypeInfo*, 16> m_ForbiddenTypes;

  WAsStringFactory* m_pStringFactory = nullptr;

  WSet<WString> m_NotRegistered;

  WMutex m_CompilerMutex;
  WStringView m_sCodeInCompilation;
};
