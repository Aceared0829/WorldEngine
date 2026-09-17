#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Factory.h>
#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Jolt/Core/IssueReporting.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/RegisterTypes.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltJobSystem.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <stdarg.h>

#ifdef JPH_DEBUG_RENDERER
std::unique_ptr<WJoltDebugRenderer> WJoltCore::s_pDebugRenderer;
#endif

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WJoltSteppingMode, 1)
  W_ENUM_CONSTANTS(WJoltSteppingMode::Variable, WJoltSteppingMode::Fixed, WJoltSteppingMode::SemiFixed)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WOnJoltContact, 1)
  // W_BITFLAGS_CONSTANT(WOnJoltContact::SendContactMsg), // do not expose in the UI
  W_BITFLAGS_CONSTANT(WOnJoltContact::ImpactReactions),
  W_BITFLAGS_CONSTANT(WOnJoltContact::SlideReactions),
  W_BITFLAGS_CONSTANT(WOnJoltContact::RollXReactions),
  W_BITFLAGS_CONSTANT(WOnJoltContact::RollYReactions),
  W_BITFLAGS_CONSTANT(WOnJoltContact::RollZReactions),
W_END_STATIC_REFLECTED_BITFLAGS;
// clang-format on

WJoltMaterial* WJoltCore::s_pDefaultMaterial = nullptr;
WUniquePtr<JPH::JobSystem> WJoltCore::s_pJobSystemEZ;
std::unique_ptr<JPH::JobSystem> WJoltCore::s_pJobSystemJolt;
WUniquePtr<WProxyAllocator> WJoltCore::s_pAllocator;
WUniquePtr<WProxyAllocator> WJoltCore::s_pAllocatorAligned;
WUniquePtr<WCollisionFilterConfig> WJoltCore::s_pCollisionFilterConfig;
WUniquePtr<WWeightCategoryConfig> WJoltCore::s_pWeightCategoryConfig;
WUniquePtr<WImpulseTypeConfig> WJoltCore::s_pImpulseTypeConfig;

WJoltMaterial::WJoltMaterial() = default;
WJoltMaterial::~WJoltMaterial() = default;

static void JoltTraceFunc(const char* szText, ...)
{
  WStringBuilder tmp;

  va_list args;
  va_start(args, szText);
  tmp.SetPrintfArgs(szText, args);
  va_end(args);

  WLog::Dev("Jolt: {}", tmp);
}

#ifdef JPH_ENABLE_ASSERTS

static bool JoltAssertFailed(const char* szInExpression, const char* szInMessage, const char* szInFile, uint32_t inLine)
{
  return WFailedCheck(szInFile, inLine, "Jolt", szInExpression, szInMessage);
};

#endif // JPH_ENABLE_ASSERTS

WCVarBool cvar_JoltUseEzTaskSystem("Jolt.UseEzTaskSystem", true, WCVarFlags::Save, "Use the W TaskSystem for Jolt updates");

JPH::JobSystem* WJoltCore::GetJoltJobSystem()
{
  if (cvar_JoltUseEzTaskSystem)
  {
    if (s_pJobSystemEZ == nullptr)
    {
      s_pJobSystemEZ = W_NEW(WFoundation::GetAlignedAllocator(), WJoltJobSystem, JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers);
    }

    return s_pJobSystemEZ.Borrow();
  }
  else
  {
    if (s_pJobSystemJolt == nullptr)
    {
      s_pJobSystemJolt = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
    }

    return s_pJobSystemJolt.get();
  }
}

void WJoltCore::DebugDraw(WWorld* pWorld)
{
#ifdef JPH_DEBUG_RENDERER
  if (s_pDebugRenderer == nullptr)
    return;

  WDebugRenderer::DrawSolidTriangles(pWorld, s_pDebugRenderer->m_Triangles, WColor::White);
  WDebugRenderer::DrawLines(pWorld, s_pDebugRenderer->m_Lines, WColor::White);

  s_pDebugRenderer->m_Triangles.Clear();
  s_pDebugRenderer->m_Lines.Clear();
#endif
}

void* WJoltCore::JoltMalloc(size_t inSize)
{
  return WJoltCore::s_pAllocator->Allocate(inSize, 8);
}

void WJoltCore::JoltFree(void* inBlock)
{
  if (inBlock)
  {
    WJoltCore::s_pAllocator->Deallocate(inBlock);
  }
}

void* WJoltCore::JoltReallocate(void* inBlock, size_t inOldSize, size_t inNewSize)
{
  if (inBlock == nullptr)
  {
    return JoltMalloc(inNewSize);
  }
  else
  {
    return WJoltCore::s_pAllocator->Reallocate(inBlock, inOldSize, inNewSize, 8);
  }
}

void* WJoltCore::JoltAlignedMalloc(size_t inSize, size_t inAlignment)
{
  return WJoltCore::s_pAllocatorAligned->Allocate(inSize, inAlignment);
}

void WJoltCore::JoltAlignedFree(void* inBlock)
{
  WJoltCore::s_pAllocatorAligned->Deallocate(inBlock);
}

const WCollisionFilterConfig& WJoltCore::GetCollisionFilterConfig()
{
  return *s_pCollisionFilterConfig;
}

const WWeightCategoryConfig& WJoltCore::GetWeightCategoryConfig()
{
  return *s_pWeightCategoryConfig;
}

const WImpulseTypeConfig& WJoltCore::GetImpulseTypeConfig()
{
  return *s_pImpulseTypeConfig;
}

void WJoltCore::ReloadConfigs()
{
  LoadCollisionFilters();
  LoadWeightCategories();
  LoadImpulseTypes();
}

void WJoltCore::LoadCollisionFilters()
{
  W_LOG_BLOCK("WJoltCore::LoadCollisionFilters");

  if (s_pCollisionFilterConfig->Load().Failed())
  {
    WLog::Info("Collision filter config file could not be found ('{}'). Using default values.", WCollisionFilterConfig::s_sConfigFile);

    // setup some default config

    s_pCollisionFilterConfig->SetGroupName(0, "Default");
    s_pCollisionFilterConfig->EnableCollision(0, 0);
  }
}

void WJoltCore::LoadWeightCategories()
{
  W_LOG_BLOCK("WJoltCore::LoadWeightCategories");

  if (s_pWeightCategoryConfig->Load().Failed())
  {
    WLog::Info("Weight category config file could not be found ('{}').", WWeightCategoryConfig::s_sConfigFile);
  }
}

void WJoltCore::LoadImpulseTypes()
{
  W_LOG_BLOCK("WJoltCore::LoadImpulseTypes");

  if (s_pImpulseTypeConfig->Load().Failed())
  {
    WLog::Info("Impulse Types config file could not be found ('{}').", WImpulseTypeConfig::s_sConfigFile);
  }
}

void WJoltCore::Startup()
{
  s_pAllocator = W_DEFAULT_NEW(WProxyAllocator, "Jolt-Core", WFoundation::GetDefaultAllocator());
  s_pAllocatorAligned = W_DEFAULT_NEW(WProxyAllocator, "Jolt-Core-Aligned", WFoundation::GetAlignedAllocator());

  s_pCollisionFilterConfig = W_DEFAULT_NEW(WCollisionFilterConfig);
  s_pWeightCategoryConfig = W_DEFAULT_NEW(WWeightCategoryConfig);
  s_pImpulseTypeConfig = W_DEFAULT_NEW(WImpulseTypeConfig);

  JPH::Trace = JoltTraceFunc;
  JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed);
  JPH::Allocate = WJoltCore::JoltMalloc;
  JPH::Free = WJoltCore::JoltFree;
  JPH::Reallocate = WJoltCore::JoltReallocate;
  JPH::AlignedAllocate = WJoltCore::JoltAlignedMalloc;
  JPH::AlignedFree = WJoltCore::JoltAlignedFree;

  JPH::Factory::sInstance = new JPH::Factory();

  JPH::RegisterTypes();

  WJoltCustomShapeInfo::sRegister();

  s_pDefaultMaterial = new WJoltMaterial;
  s_pDefaultMaterial->AddRef();
  s_pDefaultMaterial->m_DebugColor = WColor::DimGrey;
  JPH::PhysicsMaterial::sDefault = s_pDefaultMaterial;

#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = std::make_unique<WJoltDebugRenderer>();
#endif

  // do this once here to prevent a race condition later
  GetJoltJobSystem();

  WSurfaceResource::s_Events.AddEventHandler(&WJoltCore::SurfaceResourceEventHandler);
}

void WJoltCore::Shutdown()
{
#ifdef JPH_DEBUG_RENDERER
  s_pDebugRenderer = nullptr;
#endif

  JPH::PhysicsMaterial::sDefault = nullptr;

  s_pDefaultMaterial->Release();
  s_pDefaultMaterial = nullptr;

  s_pJobSystemEZ = nullptr;
  s_pJobSystemJolt = nullptr;

  delete JPH::Factory::sInstance;
  JPH::Factory::sInstance = nullptr;

  JPH::Trace = nullptr;

  s_pCollisionFilterConfig.Clear();
  s_pWeightCategoryConfig.Clear();
  s_pImpulseTypeConfig.Clear();

  s_pAllocator.Clear();
  s_pAllocatorAligned.Clear();

  WSurfaceResource::s_Events.RemoveEventHandler(&WJoltCore::SurfaceResourceEventHandler);
}

void WJoltCore::SurfaceResourceEventHandler(const WSurfaceResourceEvent& e)
{
  if (e.m_Type == WSurfaceResourceEvent::Type::Created)
  {
    const auto& desc = e.m_pSurface->GetDescriptor();

    WJoltMaterial* pJoltMat = static_cast<WJoltMaterial*>(e.m_pSurface->m_pPhysicsMaterialJolt);

    if (pJoltMat == nullptr)
    {
      pJoltMat = new WJoltMaterial;
      pJoltMat->AddRef();
      pJoltMat->m_pSurface = e.m_pSurface;
    }
    else
    {
      W_ASSERT_DEV(pJoltMat->m_pSurface == e.m_pSurface, "Invalid surface");
    }

    pJoltMat->m_DebugColor = desc.m_DebugColor;
    pJoltMat->m_fRestitution = desc.m_fPhysicsRestitution;
    pJoltMat->m_fFriction = WMath::Lerp(desc.m_fPhysicsFrictionStatic, desc.m_fPhysicsFrictionDynamic, 0.5f);

    e.m_pSurface->m_pPhysicsMaterialJolt = pJoltMat;
  }
  else if (e.m_Type == WSurfaceResourceEvent::Type::Destroyed)
  {
    if (e.m_pSurface->m_pPhysicsMaterialJolt != nullptr)
    {
      WJoltMaterial* pMaterial = static_cast<WJoltMaterial*>(e.m_pSurface->m_pPhysicsMaterialJolt);
      pMaterial->Release();

      e.m_pSurface->m_pPhysicsMaterialJolt = nullptr;
    }
  }
}


W_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltCore);
