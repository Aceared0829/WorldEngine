#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScript/source/add_on/scriptarray/scriptarray.h>
#include <AngelScript/source/add_on/scriptdictionary/scriptdictionary.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsInstance.h>
#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/Scripting/ScriptComponent.h>
#include <Core/World/Component.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/Variant.h>

static WAsAllocatorType* g_pAsAllocator = nullptr;
static void OnThreadEvent(const WThreadEvent& e);

W_IMPLEMENT_SINGLETON(WAngelScriptEngineSingleton);

// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(AngelScriptPlugin, AngelScriptEngineSingleton)

BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation"
END_SUBSYSTEM_DEPENDENCIES

ON_CORESYSTEMS_SHUTDOWN
{
  if (g_pAsAllocator)
  {
    WThread::s_ThreadEvents.RemoveEventHandler(OnThreadEvent);
    W_DEFAULT_DELETE(g_pAsAllocator);
  }
}

ON_HIGHLEVELSYSTEMS_STARTUP
{
  g_pAsAllocator = W_DEFAULT_NEW(WAsAllocatorType, "AngelScript", WFoundation::GetDefaultAllocator());
  WThread::s_ThreadEvents.AddEventHandler(OnThreadEvent);
  W_DEFAULT_NEW(WAngelScriptEngineSingleton);
}

ON_HIGHLEVELSYSTEMS_SHUTDOWN
{
  WAngelScriptEngineSingleton* pDummy = WAngelScriptEngineSingleton::GetSingleton();
  W_DEFAULT_DELETE(pDummy);
}

W_END_SUBSYSTEM_DECLARATION;
// clang-format on



static void* WAsMalloc(size_t uiSize)
{
  return g_pAsAllocator->Allocate(uiSize, 8);
}

static void WAsFree(void* pPtr)
{
  g_pAsAllocator->Deallocate(pPtr);
}

static void AsThrow(WStringView sMsg)
{
  if (asIScriptContext* ctx = asGetActiveContext())
  {
    WStringBuilder tmp;
    ctx->SetException(sMsg.GetData(tmp), false);
  }
}

static void OnThreadEvent(const WThreadEvent& e)
{
  if (e.m_Type == WThreadEvent::Type::ClearThreadLocals)
  {
    asThreadCleanup();
  }
}

WAngelScriptEngineSingleton::WAngelScriptEngineSingleton()
  : m_SingletonRegistrar(this)
{
  W_LOG_BLOCK("WAngelScriptEngineSingleton");

  asSetGlobalMemoryFunctions(WAsMalloc, WAsFree);

  m_pEngine = asCreateScriptEngine();
  // m_pEngine->SetEngineProperty(asEP_DISALLOW_VALUE_ASSIGN_FOR_REF_TYPE, 1); // means we can't copy messages during PostMessage
  m_pEngine->SetEngineProperty(asEP_REQUIRE_ENUM_SCOPE, 1);
  m_pEngine->SetEngineProperty(asEP_DISALLOW_GLOBAL_VARS, 1);

  AS_CHECK(m_pEngine->SetMessageCallback(asMETHOD(WAngelScriptEngineSingleton, CompilerMessageCallback), this, asCALL_THISCALL));
  AS_CHECK(m_pEngine->SetTranslateAppExceptionCallback(asMETHOD(WAngelScriptEngineSingleton, ExceptionCallback), this, asCALL_THISCALL));

  m_pStringFactory = W_DEFAULT_NEW(WAsStringFactory);

  AS_CHECK(m_pEngine->RegisterInterface("WAngelScriptMessage"));

  RegisterScriptArray(m_pEngine, true);
  // RegisterScriptDictionary(m_pEngine);

  RegisterStandardTypes();

  AS_CHECK(m_pEngine->RegisterGlobalFunction("void throw(WStringView)", asFUNCTION(AsThrow), asCALL_CDECL));

  m_pEngine->RegisterStringFactory("WStringView", m_pStringFactory);

  Register_ReflectedTypes();
  Register_GlobalReflectedFunctions();

  Register_WAngelScriptClass();

  AddForbiddenType("WStringView");
  AddForbiddenType("WStringBuilder");
}

WAngelScriptEngineSingleton::~WAngelScriptEngineSingleton()
{
  m_pEngine->ShutDownAndRelease();

  if (m_pStringFactory)
  {
    WAsStringFactory* pFactor = (WAsStringFactory*)m_pStringFactory;
    W_DEFAULT_DELETE(pFactor);
  }
}

void WAngelScriptEngineSingleton::AddForbiddenType(const char* szTypeName)
{
  asITypeInfo* pTypeInfo = m_pEngine->GetTypeInfoByName(szTypeName);
  W_ASSERT_DEV(pTypeInfo != nullptr, "Type '{}' not found", szTypeName);

  m_ForbiddenTypes.PushBack(pTypeInfo);
}

bool WAngelScriptEngineSingleton::IsTypeForbidden(const asITypeInfo* pType) const
{
  return m_ForbiddenTypes.Contains(pType);
}

void WAngelScriptEngineSingleton::ExceptionCallback(asIScriptContext* pContext)
{
  WLog::Error("AngelScript: App-Exception: {}", pContext->GetExceptionString());
}

void WAngelScriptEngineSingleton::RegisterStandardTypes()
{
  W_LOG_BLOCK("AS::RegisterStandardTypes");

  AS_CHECK(m_pEngine->RegisterTypedef("WInt8", "int8"));
  AS_CHECK(m_pEngine->RegisterTypedef("WInt16", "int16"));
  AS_CHECK(m_pEngine->RegisterTypedef("WInt32", "int32"));
  AS_CHECK(m_pEngine->RegisterTypedef("WInt64", "int64"));
  AS_CHECK(m_pEngine->RegisterTypedef("WUInt8", "uint8"));
  AS_CHECK(m_pEngine->RegisterTypedef("WUInt16", "uint16"));
  AS_CHECK(m_pEngine->RegisterTypedef("WUInt32", "uint32"));
  AS_CHECK(m_pEngine->RegisterTypedef("WUInt64", "uint64"));

  AS_CHECK(m_pEngine->RegisterObjectType("WRTTI", 0, asOBJ_REF | asOBJ_NOCOUNT));

  // TODO AngelScript: WResult ?

  RegisterPodValueType<WVec2>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WVec3>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WVec4>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WAngle>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WQuat>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WMat3>(asOBJ_APP_CLASS_ALLFLOATS);
  RegisterPodValueType<WMat4>(asOBJ_APP_CLASS_ALLFLOATS);
  RegisterPodValueType<WTransform>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WTime>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_ALIGN8);
  RegisterPodValueType<WColor>(asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WColorGammaUB>(asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WStringView>(asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WGameObjectHandle>(asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WComponentHandle>(asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WTempHashedString>(asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);
  RegisterPodValueType<WHashedString>(asOBJ_APP_CLASS_ALLINTS | asOBJ_APP_CLASS_MORE_CONSTRUCTORS);

  RegisterNonPodValueType<WString>();
  RegisterNonPodValueType<WStringBuilder>();

  RegisterRefType<WGameObject>();
  RegisterRefType<WComponent>();
  RegisterRefType<WWorld>();
  RegisterRefType<WMessage>();
  RegisterRefType<WClock>();

  {
    AS_CHECK(m_pEngine->RegisterObjectType("WRandom", 0, asOBJ_REF | asOBJ_NOCOUNT));
    AddForbiddenType("WRandom");
  }

  Register_RTTI();
  Register_Vec2();
  Register_Vec3();
  Register_Vec4();
  Register_Angle();
  Register_Quat();
  Register_Transform();
  Register_GameObject();
  Register_Time();
  Register_Mat3();
  Register_Mat4();
  Register_World();
  Register_Clock();
  Register_StringView();
  Register_String();
  Register_StringBuilder();
  Register_HashedString();
  Register_TempHashedString();
  Register_Color();
  Register_ColorGammaUB();
  Register_Random();
  Register_Math();
  Register_Spatial();

  // TODO AngelScript: register these standard types
  // WBoundingBox
  // WBoundingSphere
  // WPlane
}



void WAngelScriptEngineSingleton::Register_ReflectedTypes()
{
  W_LOG_BLOCK("Register_ReflectedTypes");

  Register_ReflectedType(WGetStaticRTTI<WComponent>(), false);
  Register_ReflectedType(WGetStaticRTTI<WMessage>(), true);

  Register_ExtraComponentFuncs();
}

void WAngelScriptEngineSingleton::Register_ExtraComponentFuncs()
{
  WRTTI::ForEachDerivedType(WGetStaticRTTI<WComponent>(), [&](const WRTTI* pRtti)
    {
      if (pRtti->GetAttributeByType<WHiddenAttribute>() != nullptr || pRtti->GetAttributeByType<WExcludeFromScript>() != nullptr)
        return;

      intptr_t flags = 0;

      if (pRtti != WGetStaticRTTI<WComponent>())
      {
        // derived type
        flags = 0x01;
      }

      const char* compName = pRtti->GetTypeName().GetStartPointer();

      int funcID;

      funcID = m_pEngine->RegisterObjectMethod(compName, "bool SendMessage(WMessage& inout ref_msg)", asMETHODPR(WComponent, SendMessage, (WMessage&), bool), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);

      funcID = m_pEngine->RegisterObjectMethod(compName, "bool SendMessage(WMessage& inout ref_msg) const", asMETHODPR(WComponent, SendMessage, (WMessage&) const, bool), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);

      funcID = m_pEngine->RegisterObjectMethod(compName, "void PostMessage(const WMessage& in msg, WTime delay = WTime::MakeZero(), WObjectMsgQueueType queueType = WObjectMsgQueueType::NextFrame) const", asMETHOD(WComponent, PostMessage), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);

      funcID = m_pEngine->RegisterObjectMethod(compName, "WComponentHandle GetHandle() const", asMETHOD(WComponent, GetHandle), asCALL_THISCALL);
      AS_CHECK(funcID);
      m_pEngine->GetFunctionById(funcID)->SetUserData(reinterpret_cast<void*>(flags), WAsUserData::FuncFlags);
      //
    });
}


W_STATICLINK_FILE(AngelScriptPlugin, AngelScriptPlugin_Runtime_AsEngineSingleton);
