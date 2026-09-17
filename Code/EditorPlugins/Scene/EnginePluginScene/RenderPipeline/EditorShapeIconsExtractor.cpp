#include <EnginePluginScene/EnginePluginScenePCH.h>

#include <EnginePluginScene/RenderPipeline/EditorShapeIconsExtractor.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditorShapeIconsExtractor, 1, WRTTIDefaultAllocator<WEditorShapeIconsExtractor>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(1.0f), new WSuffixAttribute(" m")),
    W_MEMBER_PROPERTY("MaxScreenSize", m_fMaxScreenSize)->AddAttributes(new WClampValueAttribute(0.0f, WVariant()), new WDefaultValueAttribute(64.0f), new WSuffixAttribute(" px")),
    W_ACCESSOR_PROPERTY("SceneContext", GetSceneContext, SetSceneContext),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WEditorShapeIconsExtractor::WEditorShapeIconsExtractor(const char* szName)
  : WExtractor(szName)
{
  m_fSize = 1.0f;
  m_fMaxScreenSize = 64.0f;
  m_pSceneContext = nullptr;

  FillShapeIconInfo();
}

WEditorShapeIconsExtractor::~WEditorShapeIconsExtractor() = default;

void WEditorShapeIconsExtractor::Extract(const WView& view, const WDynamicArray<const WGameObject*>& visibleObjects, WExtractedRenderData& ref_extractedRenderData)
{
  WFrustum frustum;
  view.ComputeCullingFrustum(frustum);

  W_LOCK(view.GetWorld()->GetReadMarker());
  auto pRenderDataManager = view.GetWorld()->GetModuleReadOnly<WRenderDataManager>();

  /// \todo Once we have a solution for objects that only have a shape icon we can switch this loop to use visibleObjects instead.
  for (auto it = view.GetWorld()->GetObjects(); it.IsValid(); ++it)
  {
    const WGameObject* pObject = it;
    if (!pObject->IsActive() || FilterByViewTags(view, pObject))
      continue;

    WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(pObject->GetGlobalPosition(), 0.1f);
    if (frustum.GetObjectPosition(sphere) == WVolumePosition::Outside)
      continue;

    ExtractShapeIcon(pObject, view, pRenderDataManager, ref_extractedRenderData, WDefaultRenderDataCategories::SimpleOpaque);
  }

  if (m_pSceneContext != nullptr)
  {
    auto objects = m_pSceneContext->GetSelectionWithChildren();

    for (const auto& hObject : objects)
    {
      const WGameObject* pObject = nullptr;
      if (view.GetWorld()->TryGetObject(hObject, pObject))
      {
        if (!pObject->IsActive() || FilterByViewTags(view, pObject))
          continue;

        WBoundingSphere sphere = WBoundingSphere::MakeFromCenterAndRadius(pObject->GetGlobalPosition(), 0.1f);
        if (frustum.GetObjectPosition(sphere) == WVolumePosition::Outside)
          continue;

        ExtractShapeIcon(pObject, view, pRenderDataManager, ref_extractedRenderData, WDefaultRenderDataCategories::Selection);
      }
    }
  }
}

WResult WEditorShapeIconsExtractor::Serialize(WStreamWriter& inout_stream) const
{
  W_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fSize;
  inout_stream << m_fMaxScreenSize;
  return W_SUCCESS;
}

WResult WEditorShapeIconsExtractor::Deserialize(WStreamReader& inout_stream)
{
  W_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const WUInt32 uiVersion = WTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  W_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fSize;
  inout_stream >> m_fMaxScreenSize;
  return W_SUCCESS;
}

void WEditorShapeIconsExtractor::ExtractShapeIcon(const WGameObject* pObject, const WView& view, const WRenderDataManager* pRenderDataManager, WExtractedRenderData& extractedRenderData, WRenderData::Category category)
{
  static const WTag& tagHidden = WTagRegistry::GetGlobalRegistry().RegisterTag("EditorHidden");
  static const WTag& tagEditor = WTagRegistry::GetGlobalRegistry().RegisterTag("Editor");

  if (pObject->GetTags().IsSet(tagEditor) || pObject->GetTags().IsSet(tagHidden))
    return;

  if (pObject->IsShapeIconHidden())
    return;

  if (pObject->GetComponents().IsEmpty())
    return;

  const WComponent* pComponent = nullptr;
  for (auto it : pObject->GetComponents())
  {
    if (it->IsActive())
    {
      pComponent = it;
      break;
    }
  }

  if (pComponent == nullptr)
    return;

  const WRTTI* pRtti = pComponent->GetDynamicRTTI();

  ShapeIconInfo* pShapeIconInfo = nullptr;
  if (m_ShapeIconInfos.TryGetValue(pRtti, pShapeIconInfo))
  {
    WSpriteRenderData* pRenderData = pRenderDataManager->CreateRenderDataForThisFrame<WSpriteRenderData>(pObject);
    {
      pRenderData->m_hTexture = pShapeIconInfo->m_hTexture;
      pRenderData->m_fSize = m_fSize;
      pRenderData->m_fMaxScreenSize = m_fMaxScreenSize * WDebugRenderer::GetTextScale() * WSpriteRenderer::s_fShapeIconScale;
      pRenderData->m_fAspectRatio = 1.0f;
      pRenderData->m_BlendMode = WSpriteBlendMode::ShapeIcon;
      pRenderData->m_texCoordScale = WVec2(1.0f);
      pRenderData->m_texCoordOffset = WVec2(0.0f);
      pRenderData->m_uiUniqueID = WRenderComponent::GetUniqueIdForRendering(*pComponent);

      // prefer color gamma properties
      if (pShapeIconInfo->m_pColorGammaProperty != nullptr)
      {
        pRenderData->m_color = WColor(pShapeIconInfo->m_pColorGammaProperty->GetValue(pComponent));
      }
      else if (pShapeIconInfo->m_pColorProperty != nullptr)
      {
        pRenderData->m_color = pShapeIconInfo->m_pColorProperty->GetValue(pComponent);
      }
      else
      {
        pRenderData->m_color = pShapeIconInfo->m_FallbackColor;
      }

      pRenderData->m_color.a = 1.0f;

      pRenderData->FillSortingKey();
    }

    WRenderData::Category effectiveCategory = category;
    if (pShapeIconInfo->m_bAlwaysVisible && category == WDefaultRenderDataCategories::SimpleOpaque)
    {
      effectiveCategory = WDefaultRenderDataCategories::SimpleForeground;
    }

    extractedRenderData.AddRenderData(pRenderData, effectiveCategory);
  }
}

const WTypedMemberProperty<WColor>* WEditorShapeIconsExtractor::FindColorProperty(const WRTTI* pRtti) const
{
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const WAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == WPropertyCategory::Member && pProperty->GetSpecificType() == WGetStaticRTTI<WColor>())
    {
      return static_cast<const WTypedMemberProperty<WColor>*>(pProperty);
    }
  }

  return nullptr;
}

const WTypedMemberProperty<WColorGammaUB>* WEditorShapeIconsExtractor::FindColorGammaProperty(const WRTTI* pRtti) const
{
  WTempHybridArray<const WAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (const WAbstractProperty* pProperty : properties)
  {
    if (pProperty->GetCategory() == WPropertyCategory::Member && pProperty->GetSpecificType() == WGetStaticRTTI<WColorGammaUB>())
    {
      return static_cast<const WTypedMemberProperty<WColorGammaUB>*>(pProperty);
    }
  }

  return nullptr;
}

void WEditorShapeIconsExtractor::FillShapeIconInfo()
{
  W_LOG_BLOCK("LoadShapeIconTextures");

  WStringBuilder sPath;

  WRTTI::ForEachDerivedType<WComponent>(
    [&](const WRTTI* pRtti)
    {
      sPath.Set("Editor/ShapeIcons/", pRtti->GetTypeName(), ".dds");

      if (WFileSystem::ExistsFile(sPath))
      {
        auto& shapeIconInfo = m_ShapeIconInfos[pRtti];
        shapeIconInfo.m_hTexture = WResourceManager::LoadResource<WTexture2DResource>(sPath);
        shapeIconInfo.m_pColorProperty = FindColorProperty(pRtti);
        shapeIconInfo.m_pColorGammaProperty = FindColorGammaProperty(pRtti);

        if (auto pCatAttribute = pRtti->GetAttributeByType<WCategoryAttribute>())
        {
          shapeIconInfo.m_FallbackColor = WColorScheme::GetCategoryColor(pCatAttribute->GetCategory(), WColorScheme::CategoryColorUsage::ViewportIcon);
        }

        if (pRtti->GetAttributeByType<WShapeIconAlwaysVisibleAttribute>() != nullptr)
        {
          shapeIconInfo.m_bAlwaysVisible = true;
        }
      }
    });
}
