#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleSystemDescriptor, 2, WRTTIDefaultAllocator<WParticleSystemDescriptor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new WDefaultValueAttribute(WVarianceTypeTime(WTime::MakeFromSeconds(2))), new WClampValueAttribute(WTime::MakeFromSeconds(0.0), WVariant())),
    W_MEMBER_PROPERTY("LifeScaleParam", m_sLifeScaleParameter),
    W_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent)->AddAttributes(new WDynamicStringEnumAttribute("ParticleEventNamesEnum")),
    W_ARRAY_MEMBER_PROPERTY("Emitters", m_EmitterFactories)->AddFlags(WPropertyFlags::PointerOwner)->AddAttributes(new WMaxArraySizeAttribute(1)),
    W_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(WPropertyFlags::PointerOwner)->AddAttributes(new WPreventDuplicatesAttribute()),
    W_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(WPropertyFlags::PointerOwner),
    W_SET_ACCESSOR_PROPERTY("Types", GetTypeFactories, AddTypeFactory, RemoveTypeFactory)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WParticleSystemDescriptor::WParticleSystemDescriptor()
{
  m_bVisible = true;
}

WParticleSystemDescriptor::~WParticleSystemDescriptor()
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();
}

void WParticleSystemDescriptor::ClearEmitters()
{
  for (auto pFactory : m_EmitterFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_EmitterFactories.Clear();
}

void WParticleSystemDescriptor::ClearInitializers()
{
  for (auto pFactory : m_InitializerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_InitializerFactories.Clear();
}

void WParticleSystemDescriptor::ClearBehaviors()
{
  for (auto pFactory : m_BehaviorFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_BehaviorFactories.Clear();
}

void WParticleSystemDescriptor::ClearTypes()
{
  for (auto pFactory : m_TypeFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_TypeFactories.Clear();
}

void WParticleSystemDescriptor::ClearFinalizers()
{
  for (auto pFactory : m_FinalizerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_FinalizerFactories.Clear();
}

void WParticleSystemDescriptor::SetupDefaultProcessors()
{
  // Age Behavior
  {
    WParticleFinalizerFactory_Age* pFactory =
      WParticleFinalizerFactory_Age::GetStaticRTTI()->GetAllocator()->Allocate<WParticleFinalizerFactory_Age>();
    pFactory->m_LifeTime = m_LifeTime;
    pFactory->m_sOnDeathEvent = m_sOnDeathEvent;
    pFactory->m_sLifeScaleParameter = m_sLifeScaleParameter;
    m_FinalizerFactories.PushBack(pFactory);
  }

  // Bounding Volume Update Behavior
  {
    WParticleFinalizerFactory_Volume* pFactory =
      WParticleFinalizerFactory_Volume::GetStaticRTTI()->GetAllocator()->Allocate<WParticleFinalizerFactory_Volume>();
    m_FinalizerFactories.PushBack(pFactory);
  }

  if (m_TypeFactories.IsEmpty())
  {
    WParticleTypePointFactory* pFactory = WParticleTypePointFactory::GetStaticRTTI()->GetAllocator()->Allocate<WParticleTypePointFactory>();
    m_TypeFactories.PushBack(pFactory);
  }

  WSet<const WRTTI*> finalizers;
  for (const auto* pFactory : m_InitializerFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_BehaviorFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_TypeFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const WRTTI* pRtti : finalizers)
  {
    W_ASSERT_DEBUG(
      pRtti->IsDerivedFrom<WParticleFinalizerFactory>(), "Invalid finalizer factory added as a dependency: '{0}'", pRtti->GetTypeName());
    W_ASSERT_DEBUG(pRtti->GetAllocator()->CanAllocate(), "Finalizer factory cannot be allocated: '{0}'", pRtti->GetTypeName());

    m_FinalizerFactories.PushBack(pRtti->GetAllocator()->Allocate<WParticleFinalizerFactory>());
  }
}

enum class ParticleSystemVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added Types
  Version_5, // added default processors
  Version_6, // changed lifetime variance
  Version_7, // added life scale param

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


WTime WParticleSystemDescriptor::GetAvgLifetime() const
{
  WTime time = m_LifeTime.m_Value + m_LifeTime.m_Value * (m_LifeTime.m_fVariance * 2.0f / 3.0f);

  // we actively prevent values outside the [0;2] range for the life-time scale parameter, when it is applied
  // so this is the accurate worst case value
  // effects should be authored with the maximum lifetime, and at runtime the lifetime should only be scaled down
  if (!m_sLifeScaleParameter.IsEmpty())
  {
    time = time * 2.0f;
  }

  return time;
}

void WParticleSystemDescriptor::Save(WStreamWriter& inout_stream) const
{
  const WUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  inout_stream << uiVersion;

  const WUInt32 uiNumEmitters = m_EmitterFactories.GetCount();
  const WUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const WUInt32 uiNumBehaviors = m_BehaviorFactories.GetCount();
  const WUInt32 uiNumTypes = m_TypeFactories.GetCount();

  WUInt32 uiMaxParticles = 0;
  inout_stream << m_bVisible;
  inout_stream << uiMaxParticles;
  inout_stream << m_LifeTime.m_Value;
  inout_stream << m_LifeTime.m_fVariance;
  inout_stream << m_sOnDeathEvent;
  inout_stream << m_sLifeScaleParameter;
  inout_stream << uiNumEmitters;
  inout_stream << uiNumInitializers;
  inout_stream << uiNumBehaviors;
  inout_stream << uiNumTypes;

  for (auto pEmitter : m_EmitterFactories)
  {
    inout_stream << pEmitter->GetDynamicRTTI()->GetTypeName();

    pEmitter->Save(inout_stream);
  }

  for (auto pInitializer : m_InitializerFactories)
  {
    inout_stream << pInitializer->GetDynamicRTTI()->GetTypeName();

    pInitializer->Save(inout_stream);
  }

  for (auto pBehavior : m_BehaviorFactories)
  {
    inout_stream << pBehavior->GetDynamicRTTI()->GetTypeName();

    pBehavior->Save(inout_stream);
  }

  for (auto pType : m_TypeFactories)
  {
    inout_stream << pType->GetDynamicRTTI()->GetTypeName();

    pType->Save(inout_stream);
  }
}


void WParticleSystemDescriptor::Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();

  WUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  W_ASSERT_DEV(uiVersion <= (int)ParticleSystemVersion::Version_Current, "Unknown particle template version {0}", uiVersion);

  WUInt32 uiNumEmitters = 0;
  WUInt32 uiNumInitializers = 0;
  WUInt32 uiNumBehaviors = 0;
  WUInt32 uiNumTypes = 0;

  if (uiVersion >= 3)
  {
    inout_stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    // now unused
    WUInt32 uiMaxParticles = 0;
    inout_stream >> uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_LifeTime.m_Value;
    inout_stream >> m_LifeTime.m_fVariance;
    inout_stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sLifeScaleParameter;
  }

  inout_stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    inout_stream >> uiNumInitializers;
  }

  inout_stream >> uiNumBehaviors;

  if (uiVersion >= 4)
  {
    inout_stream >> uiNumTypes;
  }

  m_EmitterFactories.SetCountUninitialized(uiNumEmitters);
  m_InitializerFactories.SetCountUninitialized(uiNumInitializers);
  m_BehaviorFactories.SetCountUninitialized(uiNumBehaviors);
  m_TypeFactories.SetCountUninitialized(uiNumTypes);

  WStringBuilder sType;

  for (auto& pEmitter : m_EmitterFactories)
  {
    inout_stream >> sType;

    const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
    W_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '{0}'", sType);

    pEmitter = pRtti->GetAllocator()->Allocate<WParticleEmitterFactory>();

    pEmitter->Load(inout_stream, ownerEffectDescriptor, *this);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      inout_stream >> sType;

      const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
      W_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '{0}'", sType);

      pInitializer = pRtti->GetAllocator()->Allocate<WParticleInitializerFactory>();

      pInitializer->Load(inout_stream, ownerEffectDescriptor, *this);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    inout_stream >> sType;

    const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
    W_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '{0}'", sType);

    pBehavior = pRtti->GetAllocator()->Allocate<WParticleBehaviorFactory>();

    pBehavior->Load(inout_stream, ownerEffectDescriptor, *this);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_TypeFactories)
    {
      inout_stream >> sType;

      const WRTTI* pRtti = WRTTI::FindTypeByName(sType);
      W_ASSERT_DEBUG(pRtti != nullptr, "Unknown type factory type '{0}'", sType);

      pType = pRtti->GetAllocator()->Allocate<WParticleTypeFactory>();

      pType->Load(inout_stream, ownerEffectDescriptor, *this);
    }
  }

  SetupDefaultProcessors();
}

//////////////////////////////////////////////////////////////////////////

class WParticleSystemDescriptor_1_2 : public WGraphPatch
{
public:
  WParticleSystemDescriptor_1_2()
    : WGraphPatch("WParticleSystemDescriptor", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("LifeTime").IgnoreResult();
  }
};

WParticleSystemDescriptor_1_2 g_WParticleSystemDescriptor_1_2;


W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);
