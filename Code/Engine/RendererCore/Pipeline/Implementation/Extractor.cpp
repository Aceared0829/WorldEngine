#include <RendererCore/RendererCorePCH.h>

#include <Core/World/SpatialSystem_RegularGrid.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/Extractor.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
WCVarBool cvar_SpatialVisBounds("Spatial.VisBounds", false, WCVarFlags::Default, "Enables debug visualization of object bounds");
WCVarBool cvar_SpatialVisLocalBBox("Spatial.VisLocalBBox", false, WCVarFlags::Default, "Enables debug visualization of object local bounding box");
WCVarBool cvar_SpatialVisData("Spatial.VisData.Enable", false, WCVarFlags::Default, "Enables debug visualization of the spatial data structure");
WCVarString cvar_SpatialVisDataOnlyCategory("Spatial.VisData.OnlyCategory", "", WCVarFlags::Default, "When set the debug visualization is only shown for the given spatial data category");
WCVarBool cvar_SpatialVisDataOnlySelected("Spatial.VisData.OnlySelected", false, WCVarFlags::Default, "When set the debug visualization is only shown for selected objects");
WCVarString cvar_SpatialVisDataOnlyObject("Spatial.VisData.OnlyObject", "", WCVarFlags::Default, "When set the debug visualization is only shown for objects with the given name");

WCVarBool cvar_SpatialExtractionShowStats("Spatial.Extraction.ShowStats", false, WCVarFlags::Default, "Display some stats of the render data extraction");
#endif

namespace
{
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  void VisualizeSpatialData(const WView& view)
  {
    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() && !cvar_SpatialVisDataOnlySelected)
    {
      const WSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = WDynamicCast<const WSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        WSpatialData::Category filterCategory = WSpatialData::FindCategory(cvar_SpatialVisDataOnlyCategory.GetValue());

        WTempHybridArray<WBoundingBox, 16> boxes;
        pSpatialSystemGrid->GetAllCellBoxes(boxes, filterCategory);

        for (auto& box : boxes)
        {
          WDebugRenderer::DrawLineBox(view.GetHandle(), box, WColor::Cyan);
        }
      }
    }
  }

  void VisualizeObject(const WView& view, const WGameObject* pObject)
  {
    if (!cvar_SpatialVisBounds && !cvar_SpatialVisLocalBBox && !cvar_SpatialVisData)
      return;

    if (cvar_SpatialVisLocalBBox)
    {
      const WBoundingBoxSphere& localBounds = pObject->GetLocalBounds();
      if (localBounds.IsValid())
      {
        WDebugRenderer::DrawLineBox(view.GetHandle(), localBounds.GetBox(), WColor::Yellow, pObject->GetGlobalTransform());
      }
    }

    if (cvar_SpatialVisBounds)
    {
      const WBoundingBoxSphere& globalBounds = pObject->GetGlobalBounds();
      if (globalBounds.IsValid())
      {
        WDebugRenderer::DrawLineBox(view.GetHandle(), globalBounds.GetBox(), WColor::Lime);
        WDebugRenderer::DrawLineSphere(view.GetHandle(), globalBounds.GetSphere(), WColor::Magenta);
      }
    }

    if (cvar_SpatialVisData && cvar_SpatialVisDataOnlyCategory.GetValue().IsEmpty())
    {
      const WSpatialSystem& spatialSystem = *view.GetWorld()->GetSpatialSystem();
      if (auto pSpatialSystemGrid = WDynamicCast<const WSpatialSystem_RegularGrid*>(&spatialSystem))
      {
        WBoundingBox box;
        if (pSpatialSystemGrid->GetCellBoxForSpatialData(pObject->GetSpatialData(), box).Succeeded())
        {
          WDebugRenderer::DrawLineBox(view.GetHandle(), box, WColor::Cyan);
        }
      }
    }
  }
#endif
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExtractor, 1, WRTTINoAllocator)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Active", m_bActive)->AddAttributes(new WDefaultValueAttribute(true)),
      W_ACCESSOR_PROPERTY("Name", GetName, SetName),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Red)),
      new WCategoryAttribute("Extractors")
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format om

WExtractor::WExtractor(const char* szName)
{
  m_bActive = true;
  m_sName.Assign(szName);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif
}

WExtractor::~WExtractor() = default;

void WExtractor::SetName(const char* szName)
{
  if (!WStringUtils::IsNullOrEmpty(szName))
  {
    m_sName.Assign(szName);
  }
}

const char* WExtractor::GetName() const
{
  return m_sName.GetData();
}

bool WExtractor::FilterByViewTags(const WView& view, const WGameObject* pObject) const
{
  if (!view.m_ExcludeTags.IsEmpty() && view.m_ExcludeTags.IsAnySet(pObject->GetTags()))
    return true;

  if (!view.m_IncludeTags.IsEmpty() && !view.m_IncludeTags.IsAnySet(pObject->GetTags()))
    return true;

  return false;
}

void WExtractor::ExtractRenderData(const WView& view, const WGameObject* pObject, WMsgExtractRenderData& msg, WExtractedRenderData& extractedRenderData) const
{
  auto AddRenderDataFromMessage = [&](const WMsgExtractRenderData& msg) {
    if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, msg.m_OverrideCategory);
      }
    }
    else
    {
      for (auto& data : msg.m_ExtractedRenderData)
      {
        extractedRenderData.AddRenderData(data.m_pRenderData, data.m_Category);
      }
    }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    m_uiNumUncachedRenderData += msg.m_ExtractedRenderData.GetCount();
#endif
  };

  // Forwards the barrier dependencies recorded on the message into the extracted render data. The stored category is the (possibly redirected) category passed to msg.AddDependency; it is resolved here to the static/dynamic variant the same way render data categories are resolved, so dependencies land in the same category as the render data they accompany.
  auto AddDependenciesFromMessage = [&](const WMsgExtractRenderData& msg, bool bDynamic) {
    for (WTextureDependency dep : msg.m_TextureDependencies)
    {
      dep.m_uiCategory = (msg.m_OverrideCategory != WInvalidRenderDataCategory)
                           ? msg.m_OverrideCategory.m_uiValue
                           : WRenderData::ResolveCategory(WRenderData::Category(dep.m_uiCategory), bDynamic).m_uiValue;
      extractedRenderData.AddDependency(dep);
    }

    for (WBufferDependency dep : msg.m_BufferDependencies)
    {
      dep.m_uiCategory = (msg.m_OverrideCategory != WInvalidRenderDataCategory)
                           ? msg.m_OverrideCategory.m_uiValue
                           : WRenderData::ResolveCategory(WRenderData::Category(dep.m_uiCategory), bDynamic).m_uiValue;
      extractedRenderData.AddDependency(dep);
    }

  };

  if (pObject->IsStatic())
  {
    WUInt16 uiComponentVersion = pObject->GetComponentVersion();

    WArrayPtr<const WTextureDependency> cachedTextureDependencies;
    WArrayPtr<const WBufferDependency> cachedBufferDependencies;
    auto cachedRenderData = WRenderWorld::GetCachedRenderData(view, pObject->GetHandle(), uiComponentVersion, cachedTextureDependencies, cachedBufferDependencies);

    // Apply per-object cached dependencies once. On cache-hit frames SendMessage is skipped for the owning components, so their dependencies must come from the cache here. On the frame the data is cached the dependencies are also applied through AddDependenciesFromMessage, but the per-object cache is still empty at that point, so there is no duplication.
    {
      for (WTextureDependency dep : cachedTextureDependencies)
      {
        if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
          dep.m_uiCategory = msg.m_OverrideCategory.m_uiValue;
        extractedRenderData.AddDependency(dep);
      }

      for (WBufferDependency dep : cachedBufferDependencies)
      {
        if (msg.m_OverrideCategory != WInvalidRenderDataCategory)
          dep.m_uiCategory = msg.m_OverrideCategory.m_uiValue;
        extractedRenderData.AddDependency(dep);
      }
    }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    for (WUInt32 i = 1; i < cachedRenderData.GetCount(); ++i)
    {
      W_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiComponentIndex <= cachedRenderData[i].m_uiComponentIndex, "Cached render data needs to be sorted");
      if (cachedRenderData[i - 1].m_uiComponentIndex == cachedRenderData[i].m_uiComponentIndex)
      {
        W_ASSERT_DEBUG(cachedRenderData[i - 1].m_uiPartIndex < cachedRenderData[i].m_uiPartIndex, "Cached render data needs to be sorted");
      }
    }
#endif

    WUInt32 uiCacheIndex = 0;

    auto components = pObject->GetComponents();
    const WUInt32 uiNumComponents = components.GetCount();
    for (WUInt32 uiComponentIndex = 0; uiComponentIndex < uiNumComponents; ++uiComponentIndex)
    {
      bool bCacheFound = false;
      while (uiCacheIndex < cachedRenderData.GetCount() && cachedRenderData[uiCacheIndex].m_uiComponentIndex == uiComponentIndex)
      {
        const WInternal::RenderDataCacheEntry& cacheEntry = cachedRenderData[uiCacheIndex];
        if (cacheEntry.m_pRenderData != nullptr)
        {
          extractedRenderData.AddRenderData(cacheEntry.m_pRenderData, msg.m_OverrideCategory != WInvalidRenderDataCategory ? msg.m_OverrideCategory : WRenderData::Category(cacheEntry.m_uiCategory));

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
          ++m_uiNumCachedRenderData;
#endif
        }
        ++uiCacheIndex;

        bCacheFound = true;
      }

      if (bCacheFound)
      {
        continue;
      }

      const WComponent* pComponent = components[uiComponentIndex];

      msg.m_ExtractedRenderData.Clear();
      msg.m_TextureDependencies.Clear();
      msg.m_BufferDependencies.Clear();
      msg.m_uiNumCacheIfStatic = 0;

      if (pComponent->SendMessage(msg))
      {
        // Only cache render data if all parts should be cached otherwise the cache is incomplete and we won't call SendMessage again
        if (msg.m_uiNumCacheIfStatic > 0 && msg.m_ExtractedRenderData.GetCount() == msg.m_uiNumCacheIfStatic)
        {
          WTempHybridArray<WInternal::RenderDataCacheEntry, 16> newCacheEntries;

          for (WUInt32 uiPartIndex = 0; uiPartIndex < msg.m_ExtractedRenderData.GetCount(); ++uiPartIndex)
          {
            auto& newCacheEntry = newCacheEntries.ExpandAndGetRef();
            newCacheEntry.m_pRenderData = msg.m_ExtractedRenderData[uiPartIndex].m_pRenderData;
            newCacheEntry.m_uiCategory = msg.m_ExtractedRenderData[uiPartIndex].m_Category.m_uiValue;
            newCacheEntry.m_uiComponentIndex = static_cast<WUInt16>(uiComponentIndex);
            newCacheEntry.m_uiPartIndex = static_cast<WUInt16>(uiPartIndex);
          }

          // Cache the dependencies with their resolved (static) category, matching how render data categories are cached without the override applied. The override is re-applied on read.
          for (WTextureDependency& dep : msg.m_TextureDependencies)
          {
            dep.m_uiCategory = WRenderData::ResolveCategory(WRenderData::Category(dep.m_uiCategory), false).m_uiValue;
          }

          for (WBufferDependency& dep : msg.m_BufferDependencies)
          {
            dep.m_uiCategory = WRenderData::ResolveCategory(WRenderData::Category(dep.m_uiCategory), false).m_uiValue;
          }

          WRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, newCacheEntries, msg.m_TextureDependencies, msg.m_BufferDependencies);
        }

        AddRenderDataFromMessage(msg);
        AddDependenciesFromMessage(msg, false);
      }
      else if (pComponent->IsActiveAndInitialized()) // component does not handle extract message at all
      {
        W_ASSERT_DEV(pComponent->GetDynamicRTTI()->CanHandleMessage<WMsgExtractRenderData>() == false, "");

        // Create a dummy cache entry so we don't call send message next time
        WInternal::RenderDataCacheEntry dummyEntry;
        dummyEntry.m_pRenderData = nullptr;
        dummyEntry.m_uiCategory = WInvalidRenderDataCategory.m_uiValue;
        dummyEntry.m_uiComponentIndex = static_cast<WUInt16>(uiComponentIndex);

        WRenderWorld::CacheRenderData(view, pObject->GetHandle(), pComponent->GetHandle(), uiComponentVersion, WMakeArrayPtr(&dummyEntry, 1));
      }
    }
  }
  else
  {
    msg.m_ExtractedRenderData.Clear();
    msg.m_TextureDependencies.Clear();
    msg.m_BufferDependencies.Clear();
    pObject->SendMessage(msg);

    AddRenderDataFromMessage(msg);
    AddDependenciesFromMessage(msg, true);
  }
}

WResult WExtractor::Serialize(WStreamWriter& inout_stream) const
{
  inout_stream << m_bActive;
  inout_stream << m_sName;
  return W_SUCCESS;
}


WResult WExtractor::Deserialize(WStreamReader& inout_stream)
{
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_ASSERT_DEBUG(uiVersion == 1, "Unknown version encountered");

  inout_stream >> m_bActive;
  inout_stream >> m_sName;
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WVisibleObjectsExtractor, 1, WRTTIDefaultAllocator<WVisibleObjectsExtractor>)
W_END_DYNAMIC_REFLECTED_TYPE;

WVisibleObjectsExtractor::WVisibleObjectsExtractor(const char* szName)
  : WExtractor(szName)
{
}

WVisibleObjectsExtractor::~WVisibleObjectsExtractor() = default;

void WVisibleObjectsExtractor::Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  WMsgExtractRenderData msg;
  msg.m_pView = &view;

  W_LOCK(view.GetWorld()->GetReadMarker());
  msg.m_pRenderDataManager = view.GetWorld()->GetModuleReadOnly<WRenderDataManager>();

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  VisualizeSpatialData(view);

  m_uiNumCachedRenderData = 0;
  m_uiNumUncachedRenderData = 0;
#endif

  for (auto pObject : visibleObjects)
  {
    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if ((cvar_SpatialVisDataOnlyObject.GetValue().IsEmpty() ||
            pObject->GetName().FindSubString_NoCase(cvar_SpatialVisDataOnlyObject.GetValue()) != nullptr) &&
          !cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  const bool bIsMainView = (view.GetCameraUsageHint() == WCameraUsageHint::MainView || view.GetCameraUsageHint() == WCameraUsageHint::EditorView);

  if (cvar_SpatialExtractionShowStats && bIsMainView)
  {
    WViewHandle hView = view.GetHandle();

    WStringBuilder sb;

    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "ExtractionStats", "Extraction Stats:");

    sb.SetFormat("Num Cached Render Data: {0}", m_uiNumCachedRenderData);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "ExtractionStats", sb);

    sb.SetFormat("Num Uncached Render Data: {0}", m_uiNumUncachedRenderData);
    WDebugRenderer::DrawInfoText(hView, WDebugTextPlacement::TopLeft, "ExtractionStats", sb);
  }
#endif
}

WResult WVisibleObjectsExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}

WResult WVisibleObjectsExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSelectedObjectsExtractorBase, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WSelectedObjectsExtractorBase::WSelectedObjectsExtractorBase(const char* szName)
  : WExtractor(szName)
  , m_OverrideCategory(WDefaultRenderDataCategories::Selection)
{
}

WSelectedObjectsExtractorBase::~WSelectedObjectsExtractorBase() = default;

void WSelectedObjectsExtractorBase::Extract(
  const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  const WDeque<WGameObjectHandle>* pSelection = GetSelection();
  if (pSelection == nullptr)
    return;

  WMsgExtractRenderData msg;
  msg.m_pView = &view;
  msg.m_OverrideCategory = m_OverrideCategory;

  W_LOCK(view.GetWorld()->GetReadMarker());
  msg.m_pRenderDataManager = view.GetWorld()->GetModuleReadOnly<WRenderDataManager>();

  for (const auto& hObj : *pSelection)
  {
    const WGameObject* pObject = nullptr;
    if (!view.GetWorld()->TryGetObject(hObj, pObject))
      continue;

    ExtractRenderData(view, pObject, msg, ref_extractedRenderData);

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    if (cvar_SpatialVisBounds || cvar_SpatialVisLocalBBox || cvar_SpatialVisData)
    {
      if (cvar_SpatialVisDataOnlySelected)
      {
        VisualizeObject(view, pObject);
      }
    }
#endif
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSelectedObjectsContext, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSelectedObjectsExtractor, 1, WRTTIDefaultAllocator<WSelectedObjectsExtractor>)
{
  W_BEGIN_PROPERTIES
  {
    W_ACCESSOR_PROPERTY("SelectionContext", GetSelectionContext, SetSelectionContext),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WSelectedObjectsContext::WSelectedObjectsContext() = default;
WSelectedObjectsContext::~WSelectedObjectsContext() = default;

void WSelectedObjectsContext::RemoveDeadObjects(const WWorld& world)
{
  for (WUInt32 i = 0; i < m_Objects.GetCount();)
  {
    const WGameObject* pObj;
    if (world.TryGetObject(m_Objects[i], pObj) == false)
    {
      m_Objects.RemoveAtAndSwap(i);
    }
    else
      ++i;
  }
}

void WSelectedObjectsContext::AddObjectAndChildren(const WWorld& world, const WGameObjectHandle& hObject)
{
  const WGameObject* pObj;
  if (world.TryGetObject(hObject, pObj))
  {
    m_Objects.PushBack(hObject);

    for (auto it = pObj->GetChildren(); it.IsValid(); ++it)
    {
      AddObjectAndChildren(world, it);
    }
  }
}

void WSelectedObjectsContext::AddObjectAndChildren(const WWorld& world, const WGameObject* pObject)
{
  m_Objects.PushBack(pObject->GetHandle());

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    AddObjectAndChildren(world, it);
  }
}

WSelectedObjectsExtractor::WSelectedObjectsExtractor(const char* szName /*= "ExplicitlySelectedObjectsExtractor"*/)
  : WSelectedObjectsExtractorBase(szName)
{
}

WSelectedObjectsExtractor::~WSelectedObjectsExtractor() = default;

const WDeque<WGameObjectHandle>* WSelectedObjectsExtractor::GetSelection()
{
  if (m_pSelectionContext && m_pSelectionContext->m_bEnabled)
  {
    return &m_pSelectionContext->m_Objects;
  }

  return nullptr;
}

WResult WSelectedObjectsExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  return W_SUCCESS;
}

WResult WSelectedObjectsExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  return W_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Extractor);
