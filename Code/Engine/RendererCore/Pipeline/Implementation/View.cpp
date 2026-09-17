#include <RendererCore/RendererCorePCH.h>

#include <Core/Utils/Blackboard.h>
#include <Core/World/World.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WCameraUsageHint, 1)
  W_ENUM_CONSTANT(WCameraUsageHint::None),
  W_ENUM_CONSTANT(WCameraUsageHint::MainView),
  W_ENUM_CONSTANT(WCameraUsageHint::EditorView),
  W_ENUM_CONSTANT(WCameraUsageHint::RenderTarget),
  W_ENUM_CONSTANT(WCameraUsageHint::Culling),
  W_ENUM_CONSTANT(WCameraUsageHint::Thumbnail),
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WView, 1, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("RenderTarget0", m_PinRenderTarget0),
    W_MEMBER_PROPERTY("RenderTarget1", m_PinRenderTarget1),
    W_MEMBER_PROPERTY("RenderTarget2", m_PinRenderTarget2),
    W_MEMBER_PROPERTY("RenderTarget3", m_PinRenderTarget3),
    W_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WView::WView()
{
  m_pExtractTask = W_DEFAULT_NEW(WDelegateTask<void>, "", WTaskNesting::Never, WMakeDelegate(&WView::ExtractData, this));
}

WView::~WView() = default;

void WView::SetName(WStringView sName)
{
  m_Data.m_sName.Assign(sName);

  WStringBuilder sb = sName;
  sb.Append(".ExtractData");
  m_pExtractTask->ConfigureTask(sb, WTaskNesting::Maybe);
}

void WView::SetWorld(WWorld* pWorld)
{
  if (m_pWorld != pWorld)
  {
    m_pWorld = pWorld;
    m_Data.m_uiSkyIrradianceIndex = pWorld == nullptr ? 0 : pWorld->GetIndex();
    WRenderWorld::ResetRenderDataCache(*this);

    m_pWorldBlackboard = pWorld != nullptr ? pWorld->GetBlackboard() : nullptr;
    m_BlackboardChangeCounter[SourceBlackboard::World] = {};
    m_bBlackboardMappingsDirty = true;
  }
}

void WView::SetSwapChain(WGALSwapChainHandle hSwapChain)
{
  if (m_Data.m_hSwapChain != hSwapChain)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = hSwapChain;
    m_Data.m_RenderTargets = WGALRenderTargets();
    if (m_pRenderPipeline)
    {
      WRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

void WView::SetRenderTargets(const WGALRenderTargets& renderTargets)
{
  if (m_Data.m_RenderTargets != renderTargets)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_hSwapChain = WGALSwapChainHandle();
    m_Data.m_RenderTargets = renderTargets;
    if (m_pRenderPipeline)
    {
      WRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

const WGALRenderTargets& WViewData::GetActiveRenderTargets() const
{
  if (const WGALSwapChain* pSwapChain = WGALDevice::GetDefaultDevice()->GetSwapChain(m_hSwapChain))
  {
    return pSwapChain->GetRenderTargets();
  }
  return m_RenderTargets;
}

const WGALRenderTargets& WView::GetActiveRenderTargets() const
{
  return m_Data.GetActiveRenderTargets();
}

void WView::SetRenderPipelineResource(WRenderPipelineResourceHandle hPipeline)
{
  if (hPipeline == m_hRenderPipeline)
  {
    return;
  }

  m_uiRenderPipelineResourceDescriptionCounter = 0;
  m_hRenderPipeline = hPipeline;

  if (m_pRenderPipeline == nullptr)
  {
    EnsureUpToDate();
  }
}

WRenderPipelineResourceHandle WView::GetRenderPipelineResource() const
{
  return m_hRenderPipeline;
}

void WView::SetCameraUsageHint(WEnum<WCameraUsageHint> val)
{
  m_Data.m_CameraUsageHint = val;
}

void WView::SetViewRenderMode(WEnum<WViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

void WView::SetViewport(const WRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;

  UpdateViewData(WRenderWorld::GetDataIndexForExtraction());
}

void WView::ForceUpdate()
{
  if (m_pRenderPipeline)
  {
    WRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

void WView::ExtractData()
{
  W_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  m_pRenderPipeline->m_sName = m_Data.m_sName;
  m_pRenderPipeline->ExtractData(*this);
}

void WView::ComputeCullingFrustum(WFrustum& out_frustum) const
{
  const WCamera* pCamera = GetCullingCamera();
  const float fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;

  WMat4 viewMatrix = pCamera->GetViewMatrix();

  WMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  out_frustum = WFrustum::MakeFromMVP(projectionMatrix * viewMatrix);
}

void WView::SetShaderPermutationVariable(const char* szName, const char* szValue)
{
  WHashedString sName;
  sName.Assign(szName);

  for (auto& var : m_PermutationVars)
  {
    if (var.m_sName == sName)
    {
      if (var.m_sValue != szValue)
      {
        var.m_sValue.Assign(szValue);
        m_bPermutationVarsDirty = true;
      }
      return;
    }
  }

  auto& var = m_PermutationVars.ExpandAndGetRef();
  var.m_sName = sName;
  var.m_sValue.Assign(szValue);
  m_bPermutationVarsDirty = true;
}

void WView::SetBlackboard(const WSharedPtr<WBlackboard>& pBlackboard)
{
  if (m_pViewBlackboard != pBlackboard)
  {
    m_pViewBlackboard = pBlackboard;
    m_BlackboardChangeCounter[SourceBlackboard::View] = {};
    m_bBlackboardMappingsDirty = true;
  }
}

const WSharedPtr<WBlackboard>& WView::GetBlackboard() const
{
  return m_pViewBlackboard;
}

void WView::UpdateViewData(WUInt32 uiDataIndex)
{
  if (m_pRenderPipeline != nullptr)
  {
    m_pRenderPipeline->UpdateViewData(*this, uiDataIndex);
  }
}

void WView::UpdateCachedMatrices() const
{
  const WCamera* pCamera = GetCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = pCamera->GetViewMatrix(WCameraEye::Left);
    m_Data.m_ViewMatrix[1] = pCamera->GetViewMatrix(WCameraEye::Right);

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = m_Data.m_ViewMatrix[0].GetInverse(0.0f);
    m_Data.m_InverseViewMatrix[1] = m_Data.m_ViewMatrix[1].GetInverse(0.0f);
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio = fViewportAspectRatio;


    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[0], WCameraEye::Left);
    m_Data.m_InverseProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0].GetInverse(0.0f);

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[1], WCameraEye::Right);
    m_Data.m_InverseProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1].GetInverse(0.0f);
  }

  if (bUpdateVP)
  {
    for (int i = 0; i < 2; ++i)
    {
      m_Data.m_ViewProjectionMatrix[i] = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

void WView::EnsureUpToDate()
{
  if (m_hRenderPipeline.IsValid())
  {
    WResourceLock<WRenderPipelineResource> pPipeline(m_hRenderPipeline, WResourceAcquireMode::BlockTillLoaded);

    WUInt32 uiCounter = pPipeline->GetCurrentResourceChangeCounter();

    if (m_uiRenderPipelineResourceDescriptionCounter != uiCounter)
    {
      m_uiRenderPipelineResourceDescriptionCounter = uiCounter;

      m_pRenderPipeline = pPipeline->CreateRenderPipeline();
      if (m_pRenderPipeline != nullptr)
      {
        WRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
      }

      m_bPermutationVarsDirty = true;

      // Re-evaluate all blackboards since the render pipeline has changed and the property mappings are not valid anymore.
      m_BlackboardChangeCounter[SourceBlackboard::World] = {};
      m_BlackboardChangeCounter[SourceBlackboard::View] = {};
      m_PropertyMappings.Clear();
      m_bBlackboardMappingsDirty = true;
    }

    ApplyPermutationVars();
    ApplyPropertiesFromBlackboard();
  }
}

void WView::ReadBackPassProperties()
{
  W_PROFILE_SCOPE("ViewReadBackPassProperties");

  WTempHybridArray<WRenderPipelinePass*, 32> passes;
  m_pRenderPipeline->GetPasses(passes);

  for (auto pPass : passes)
  {
    W_PROFILE_SCOPE(pPass->GetName());

    pPass->ReadBackProperties(this);
  }
}

void WView::ApplyPermutationVars()
{
  if (!m_bPermutationVarsDirty)
    return;

  if (m_pRenderPipeline == nullptr)
    return;

  m_pRenderPipeline->m_PermutationVars = m_PermutationVars;
  m_bPermutationVarsDirty = false;
}

void WView::ApplyPropertiesFromBlackboard()
{
  if (m_pRenderPipeline == nullptr)
    return;

  const WBlackboard* pBlackboards[SourceBlackboard::COUNT];
  pBlackboards[SourceBlackboard::World] = m_pWorldBlackboard.Borrow();
  pBlackboards[SourceBlackboard::View] = m_pViewBlackboard.Borrow();

  bool bAnyStructureChanged = false;
  bool bAnyValuesChanged = false;
  bool blackboardValuesChanged[SourceBlackboard::COUNT] = {};

  static_assert(W_ARRAY_SIZE(m_BlackboardChangeCounter) == W_ARRAY_SIZE(pBlackboards));
  for (WUInt32 i = 0; i < W_ARRAY_SIZE(pBlackboards); ++i)
  {
    const WBlackboard* pBlackboard = pBlackboards[i];
    if (pBlackboard == nullptr)
      continue;

    ChangeCounter& changeCounter = m_BlackboardChangeCounter[i];

    bAnyStructureChanged |= changeCounter.m_uiStructure != pBlackboard->GetBlackboardChangeCounter();
    changeCounter.m_uiStructure = pBlackboard->GetBlackboardChangeCounter();

    blackboardValuesChanged[i] = changeCounter.m_uiValue != pBlackboard->GetBlackboardEntryChangeCounter();
    bAnyValuesChanged |= blackboardValuesChanged[i];

    changeCounter.m_uiValue = pBlackboard->GetBlackboardEntryChangeCounter();
  }

  // Adding or removing an entry can change which blackboard provides a mapping, so both have to be resolved together.
  bool bSwitchChanged = false;
  if (m_bBlackboardMappingsDirty || bAnyStructureChanged)
  {
    RebuildPropertyMappings(pBlackboards);
    bSwitchChanged = RebuildSwitchMappings(pBlackboards);

    m_bBlackboardMappingsDirty = false;
  }
  else if (bAnyValuesChanged)
  {
    UpdatePropertyMappings(blackboardValuesChanged);
    bSwitchChanged = UpdateSwitchValues(blackboardValuesChanged);
  }

  if (bSwitchChanged)
  {
    WRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

bool WView::RebuildSwitchMappings(const WBlackboard* const* pBlackboards)
{
  const WArrayPtr<const WRenderPipelinePassGraph::SwitchInfo> switches = m_pRenderPipeline->GetSwitches();
  m_SwitchMappings.SetCount(switches.GetCount());

  bool bSwitchChanged = false;
  for (WUInt32 i = 0; i < switches.GetCount(); ++i)
  {
    SwitchMapping& mapping = m_SwitchMappings[i];
    mapping = {};

    // The view blackboard overrides the world blackboard when both provide the same entry.
    for (WUInt32 uiSource = SourceBlackboard::COUNT; uiSource-- > 0;)
    {
      const WBlackboard* pBlackboard = pBlackboards[uiSource];
      if (pBlackboard == nullptr)
        continue;

      if (const WBlackboard::Entry* pEntry = pBlackboard->GetEntry(switches[i].m_sBlackboardProperty))
      {
        mapping.m_pEntry = pEntry;
        mapping.m_uiEntryChangeCounter = pEntry->m_uiChangeCounter;
        mapping.m_SourceIndex = static_cast<SourceBlackboard>(uiSource);
        break;
      }
    }

    if (mapping.m_pEntry != nullptr && mapping.m_pEntry->m_Value.CanConvertTo<WInt32>())
    {
      bSwitchChanged |= m_pRenderPipeline->SetSwitchValue(i, mapping.m_pEntry->m_Value.ConvertTo<WInt32>());
    }
    else
    {
      if (mapping.m_pEntry != nullptr)
      {
        WLog::Warning("Blackboard entry '{}' for switch '{}' is not an integer.", switches[i].m_sBlackboardProperty, switches[i].m_pSwitch->GetName());
      }
      bSwitchChanged |= m_pRenderPipeline->SetSwitchToDefault(i);
    }
  }

  return bSwitchChanged;
}

bool WView::UpdateSwitchValues(const bool* pBlackboardValuesChanged)
{
  bool bSwitchChanged = false;
  for (WUInt32 i = 0; i < m_SwitchMappings.GetCount(); ++i)
  {
    SwitchMapping& mapping = m_SwitchMappings[i];
    if (mapping.m_pEntry == nullptr || !pBlackboardValuesChanged[mapping.m_SourceIndex] || mapping.m_uiEntryChangeCounter == mapping.m_pEntry->m_uiChangeCounter)
      continue;

    mapping.m_uiEntryChangeCounter = mapping.m_pEntry->m_uiChangeCounter;
    if (mapping.m_pEntry->m_Value.CanConvertTo<WInt32>())
    {
      bSwitchChanged |= m_pRenderPipeline->SetSwitchValue(i, mapping.m_pEntry->m_Value.ConvertTo<WInt32>());
    }
    else
    {
      bSwitchChanged |= m_pRenderPipeline->SetSwitchToDefault(i);
    }
  }

  return bSwitchChanged;
}

void WView::RebuildPropertyMappings(const WBlackboard* const* pBlackboards)
{
  W_ASSERT_DEV(m_pRenderPipeline != nullptr, "Can only update mappings with a valid render pipeline");

  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_pEntry = nullptr;
  }

  // Ascending priority, so that entries of the view blackboard replace those of the world blackboard.
  for (WUInt32 uiSource = 0; uiSource < SourceBlackboard::COUNT; ++uiSource)
  {
    const WBlackboard* pBlackboard = pBlackboards[uiSource];
    if (pBlackboard == nullptr)
      continue;

    for (auto it = pBlackboard->GetAllEntries().GetIterator(); it.IsValid(); ++it)
    {
      const WStringView sName = it.Key();
      const char* szDot = sName.FindSubString(".");
      if (szDot == nullptr)
        continue;

      const WStringView sObjectName = WStringView(sName.GetStartPointer(), szDot);

      WReflectedClass* pObject = m_pRenderPipeline->GetPassByName(sObjectName);
      if (pObject == nullptr)
      {
        pObject = m_pRenderPipeline->GetExtractorByName(sObjectName);
      }

      if (pObject == nullptr)
        continue;

      const WStringView sPropertyName = WStringView(szDot + 1, sName.GetEndPointer());
      const WAbstractProperty* pAbstractProperty = pObject->GetDynamicRTTI()->FindPropertyByName(sPropertyName);
      if (pAbstractProperty == nullptr || pAbstractProperty->GetCategory() != WPropertyCategory::Member)
        continue;

      bool bExisted = false;
      PropertyMapping& mapping = m_PropertyMappings.FindOrAdd(it.Key(), &bExisted);
      if (!bExisted)
      {
        mapping.m_pObject = pObject;
        mapping.m_pProperty = static_cast<const WAbstractMemberProperty*>(pAbstractProperty);
        // Read before any blackboard value was applied, so that the property can be restored once no blackboard provides this entry anymore.
        mapping.m_DefaultValue = WReflectionUtils::GetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject);
      }

      mapping.m_pEntry = &it.Value();
      mapping.m_uiEntryChangeCounter = it.Value().m_uiChangeCounter;
      mapping.m_SourceIndex = static_cast<SourceBlackboard>(uiSource);
    }
  }

  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid();)
  {
    PropertyMapping& mapping = it.Value();

    if (mapping.m_pEntry != nullptr)
    {
      WReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_pEntry->m_Value);
      ++it;
      continue;
    }

    WReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_DefaultValue);
    it = m_PropertyMappings.Remove(it);
  }
}

void WView::UpdatePropertyMappings(const bool* pBlackboardValuesChanged)
{
  for (auto it = m_PropertyMappings.GetIterator(); it.IsValid(); ++it)
  {
    PropertyMapping& mapping = it.Value();
    if (!pBlackboardValuesChanged[mapping.m_SourceIndex] || mapping.m_uiEntryChangeCounter == mapping.m_pEntry->m_uiChangeCounter)
      continue;

    mapping.m_uiEntryChangeCounter = mapping.m_pEntry->m_uiChangeCounter;
    WReflectionUtils::SetMemberPropertyValue(mapping.m_pProperty, mapping.m_pObject, mapping.m_pEntry->m_Value);
  }
}


W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_View);
