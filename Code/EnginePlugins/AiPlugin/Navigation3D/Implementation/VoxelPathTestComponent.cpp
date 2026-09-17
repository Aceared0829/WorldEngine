#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/Navigation3D/VoxelGridComponent.h>
#include <AiPlugin/Navigation3D/VoxelPathTestComponent.h>
#include <AiPlugin/Navigation3D/VoxelWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
W_BEGIN_COMPONENT_TYPE(WAiVoxelPathTestComponent, 1, WComponentMode::Static)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("PathEnd", DummyGetter, SetPathEndReference)->AddAttributes(new WGameObjectReferenceAttribute()),

    W_MEMBER_PROPERTY("VisualizeSmoothedPath", m_bVisualizeSmoothedPath)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new WDefaultValueAttribute(true)),

    W_MEMBER_PROPERTY("SearchMargin", m_fSearchMargin)->AddAttributes(new WDefaultValueAttribute(5.0f)),
    W_MEMBER_PROPERTY("MaxIterationsPerHop", m_uiMaxIterationsPerHop)->AddAttributes(new WDefaultValueAttribute(10000)),
    W_MEMBER_PROPERTY("MaxHops", m_uiMaxHops)->AddAttributes(new WDefaultValueAttribute(16)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("AI/Navigation"),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WAiVoxelPathTestComponent::WAiVoxelPathTestComponent() = default;
WAiVoxelPathTestComponent::~WAiVoxelPathTestComponent() = default;

void WAiVoxelPathTestComponent::SetPathEndReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetPathEnd(resolver(szReference, GetHandle(), "PathEnd"));
}

void WAiVoxelPathTestComponent::SetPathEnd(WGameObjectHandle hObject)
{
  m_hPathEnd = hObject;
}

void WAiVoxelPathTestComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  WStreamWriter& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hPathEnd);
  s << m_bVisualizeSmoothedPath;
  s << m_bVisualizePathState;
  s << m_fSearchMargin;
  s << m_uiMaxIterationsPerHop;
  s << m_uiMaxHops;
}

void WAiVoxelPathTestComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  WStreamReader& s = inout_stream.GetStream();
  const WUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  m_hPathEnd = inout_stream.ReadGameObjectHandle();
  s >> m_bVisualizeSmoothedPath;
  s >> m_bVisualizePathState;
  s >> m_fSearchMargin;
  s >> m_uiMaxIterationsPerHop;
  s >> m_uiMaxHops;
}

void WAiVoxelPathTestComponent::Update()
{
  if (m_hPathEnd.IsInvalidated())
    return;

  WGameObject* pEnd = nullptr;
  if (!GetWorld()->TryGetObject(m_hPathEnd, pEnd))
    return;

  auto* pVoxelModule = GetWorld()->GetOrCreateModule<WAiVoxelWorldModule>();
  if (pVoxelModule == nullptr || !pVoxelModule->IsReady())
    return;

  const WVec3 vStart = GetOwner()->GetGlobalPosition();
  const WVec3 vTarget = pEnd->GetGlobalPosition();

  const WAiVoxelGridFinder gridFinder = [pVoxelModule](const WBoundingBox& searchBox, WDynamicArray<const WVoxelGrid*>& out_grids)
  {
    WHybridArray<WAiVoxelGridComponent*, 4> gridComponents;
    pVoxelModule->FindGridsInBox(searchBox, gridComponents);

    out_grids.Clear();
    for (const WAiVoxelGridComponent* pGridComponent : gridComponents)
      out_grids.PushBack(&pGridComponent->GetStaticVoxelGrid());
  };

  const auto state = m_Navigation.FindPath(vStart, vTarget, gridFinder, m_fSearchMargin, m_uiMaxIterationsPerHop, m_uiMaxHops);

  if (m_bVisualizeSmoothedPath)
  {
    m_Navigation.DebugDrawPathSegments(GetWorld(), WColor::DeepSkyBlue, WColor::Yellow);
  }

  if (m_bVisualizePathState)
  {
    const WVec3 vTextPos = GetOwner()->GetGlobalPosition() + WVec3(0, 0, 0.5f);

    switch (state)
    {
      case WAiVoxelNavigation::State::Idle:
        WDebugRenderer::Draw3DText(GetWorld(), "Idle", vTextPos, WColor::Grey);
        break;
      case WAiVoxelNavigation::State::PathFound:
        WDebugRenderer::Draw3DText(GetWorld(), "Path Found", vTextPos, WColor::LawnGreen);
        break;
      case WAiVoxelNavigation::State::NoPathFound:
        WDebugRenderer::Draw3DText(GetWorld(), "No Path Found", vTextPos, WColor::IndianRed);
        break;
      case WAiVoxelNavigation::State::InvalidStartPosition:
        WDebugRenderer::Draw3DText(GetWorld(), "Invalid Start Position", vTextPos, WColor::IndianRed);
        break;
      case WAiVoxelNavigation::State::InvalidTargetPosition:
        WDebugRenderer::Draw3DText(GetWorld(), "Invalid Target Position", vTextPos, WColor::IndianRed);
        break;
    }
  }
}

W_STATICLINK_FILE(AiPlugin, AiPlugin_Navigation3D_Implementation_VoxelPathTestComponent);
