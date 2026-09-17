#pragma once

#include <AngelScriptPlugin/AngelScriptPluginDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Types/VariantType.h>

class asIScriptEngine;
class WVariant;
class asIScriptGeneric;
class WAbstractFunctionProperty;
class asIScriptModule;
class asIScriptFunction;
class WWorld;

struct W_ANGELSCRIPTPLUGIN_DLL WAsInfos
{
  WSet<WString> m_Types;
  WSet<WString> m_Namespaces;
  WSet<WString> m_GlobalFunctions;
  WSet<WString> m_Methods;
  WSet<WString> m_AllDeclarations;
  WSet<WString> m_Properties;
  WSet<WString> m_EnumValues;
};

class W_ANGELSCRIPTPLUGIN_DLL WAngelScriptUtils
{
public:
  static void SetThreadLocalWorld(WWorld* pWorld);
  static WWorld* GetThreadLocalWorld();

  static void SaveByteCode(asIScriptModule* pModule, WDynamicArray<WUInt8>& out_byteCode);

  static const char* GetAsTypeName(asIScriptEngine* pEngine, int iAsTypeID);

  static WString GetNiceFunctionDeclaration(const asIScriptFunction* pFunc, bool bIncludeObjectName = false, bool bIncludeNamespace = false);

  static asIScriptModule* LoadFromByteCode(asIScriptEngine* pEngine, WStringView sModuleName, WArrayPtr<WUInt8> byteCode);

  static const WRTTI* MapToRTTI(int iAsTypeID, asIScriptEngine* pEngine);

  static WResult WriteToAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, const WVariant& value);
  static WResult ReadFromAsTypeAtLocation(asIScriptEngine* pEngine, int iAsTypeID, void* pMemoryLocation, WVariant& out_value);

  static const char* VariantTypeToString(WVariantType::Enum type);

  static WString DefaultValueToString(const WVariant& value, WVariantType::Enum expectedType);

  static void RetrieveArg(asIScriptGeneric* pGen, WUInt32 uiRealArg, WInt32& ref_iSkippedArg, const WAbstractFunctionProperty* pAbstractFuncProp, WVariant& out_arg);

  static void RetrieveVarArgs(asIScriptGeneric* pGen, WUInt32 uiStartArg, const WAbstractFunctionProperty* pAbstractFuncProp, WVariant& out_arg);

  static void MakeGenericFunctionCall(asIScriptGeneric* pGen);

  static void DefaultConstructInPlace(void* pPtr, const WRTTI* pRtti);

  static void RetrieveAsInfos(asIScriptEngine* pEngine, WAsInfos& out_infos);

  static void GenerateAsPredefinedFile(asIScriptEngine* pEngine, WStringBuilder& out_sContent);

  static WString RegisterEnumType(asIScriptEngine* pEngine, const WRTTI* pEnumType);

  static void RegisterTypeProperties(asIScriptEngine* pEngine, const char* szTypeName, const WRTTI* pRtti, bool bIsInherited);
};
