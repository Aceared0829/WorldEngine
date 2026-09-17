#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererCore/Textures/Texture3DResource.h>
#include <RendererCore/Textures/TextureCubeResource.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WInstanceableRenderData, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgExtractRenderData);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgExtractRenderData, 1, WRTTINoAllocator)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgExtractOccluderData);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgExtractOccluderData, 1, WRTTINoAllocator)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgCustomInstanceDataOffsetChanged);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgCustomInstanceDataOffsetChanged, 1, WRTTINoAllocator)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
static_assert(sizeof(WRenderData) == 48);
static_assert(sizeof(WInstanceableRenderData) == 72);
#else
static_assert(sizeof(WRenderData) == 40);
static_assert(sizeof(WInstanceableRenderData) == 64);
#endif

WHybridArray<WRenderData::CategoryData, 32> WRenderData::s_CategoryData;

// static
WRenderData::Category WRenderData::RegisterCategory(const char* szCategoryName, SortingKeyFunc sortingKeyFunc)
{
  WHashedString sCategoryName;
  sCategoryName.Assign(szCategoryName);

  Category oldCategory = FindCategory(sCategoryName);
  if (oldCategory != WInvalidRenderDataCategory)
    return oldCategory;

  Category newCategory = Category(static_cast<WUInt16>(s_CategoryData.GetCount()));

  auto& data = s_CategoryData.ExpandAndGetRef();
  data.m_sName = sCategoryName;
  data.m_sortingKeyFunc = sortingKeyFunc;

  return newCategory;
}

// static
WRenderData::Category WRenderData::RegisterDerivedCategory(const char* szCategoryName, Category baseCategory)
{
  auto& baseCategoryData = s_CategoryData[baseCategory.m_uiValue];

  Category derivedCategory = RegisterCategory(szCategoryName, baseCategoryData.m_sortingKeyFunc);
  s_CategoryData[derivedCategory.m_uiValue].m_baseCategory = baseCategory;

  return derivedCategory;
}

// static
WRenderData::Category WRenderData::RegisterRedirectedCategory(const char* szCategoryName, Category staticCategory, Category dynamicCategory)
{
  Category newCategory = RegisterCategory(szCategoryName, nullptr);
  s_CategoryData[newCategory.m_uiValue].m_staticCategory = staticCategory;
  s_CategoryData[newCategory.m_uiValue].m_dynamicCategory = dynamicCategory;

  return newCategory;
}

// static
WRenderData::Category WRenderData::FindCategory(WTempHashedString sCategoryName)
{
  for (WUInt32 uiCategoryIndex = 0; uiCategoryIndex < s_CategoryData.GetCount(); ++uiCategoryIndex)
  {
    if (s_CategoryData[uiCategoryIndex].m_sName == sCategoryName)
      return Category(static_cast<WUInt16>(uiCategoryIndex));
  }

  return WInvalidRenderDataCategory;
}

// static
WRenderData::Category WRenderData::ResolveCategory(Category category, bool bDynamic)
{
  auto& categoryData = s_CategoryData[category.m_uiValue];
  if (categoryData.m_staticCategory != WInvalidRenderDataCategory)
  {
    return bDynamic ? categoryData.m_dynamicCategory : categoryData.m_staticCategory;
  }

  return category;
}

// static
void WRenderData::GetAllCategoryNames(WDynamicArray<WHashedString>& out_categoryNames)
{
  out_categoryNames.Clear();

  for (auto& data : s_CategoryData)
  {
    out_categoryNames.PushBack(data.m_sName);
  }
}

//////////////////////////////////////////////////////////////////////////

WRenderData::Category WDefaultRenderDataCategories::Light = WRenderData::RegisterCategory("Light", &WRenderSortingFunctions::BySortingKeyOnlyFunc);
WRenderData::Category WDefaultRenderDataCategories::Decal = WRenderData::RegisterCategory("Decal", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::ReflectionProbe = WRenderData::RegisterCategory("ReflectionProbe", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::Sky = WRenderData::RegisterCategory("Sky", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);

WRenderData::Category WDefaultRenderDataCategories::LitOpaqueStatic = WRenderData::RegisterCategory("LitOpaqueStatic", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::LitOpaqueDynamic = WRenderData::RegisterCategory("LitOpaqueDynamic", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::LitOpaque = WRenderData::RegisterRedirectedCategory("LitOpaque", WDefaultRenderDataCategories::LitOpaqueStatic, WDefaultRenderDataCategories::LitOpaqueDynamic);

WRenderData::Category WDefaultRenderDataCategories::LitMaskedStatic = WRenderData::RegisterCategory("LitMaskedStatic", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::LitMaskedDynamic = WRenderData::RegisterCategory("LitMaskedDynamic", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::LitMasked = WRenderData::RegisterRedirectedCategory("LitMasked", WDefaultRenderDataCategories::LitMaskedStatic, WDefaultRenderDataCategories::LitMaskedDynamic);

WRenderData::Category WDefaultRenderDataCategories::LitMeshDecal = WRenderData::RegisterCategory("LitMeshDecal", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::LitTransparent = WRenderData::RegisterCategory("LitTransparent", &WRenderSortingFunctions::BackToFrontThenByRenderDataFunc);
WRenderData::Category WDefaultRenderDataCategories::LitForeground = WRenderData::RegisterCategory("LitForeground", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);

WRenderData::Category WDefaultRenderDataCategories::LensEffects = WRenderData::RegisterCategory("LensEffects", &WRenderSortingFunctions::BackToFrontThenByRenderDataFunc);

WRenderData::Category WDefaultRenderDataCategories::SimpleOpaque = WRenderData::RegisterCategory("SimpleOpaque", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::SimpleTransparent = WRenderData::RegisterCategory("SimpleTransparent", &WRenderSortingFunctions::BackToFrontThenByRenderDataFunc);
WRenderData::Category WDefaultRenderDataCategories::SimpleForeground = WRenderData::RegisterCategory("SimpleForeground", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);

WRenderData::Category WDefaultRenderDataCategories::Selection = WRenderData::RegisterCategory("Selection", &WRenderSortingFunctions::ByRenderDataThenFrontToBackFunc);
WRenderData::Category WDefaultRenderDataCategories::GUI = WRenderData::RegisterCategory("GUI", &WRenderSortingFunctions::BackToFrontThenByRenderDataFunc);

//////////////////////////////////////////////////////////////////////////

void WMsgExtractRenderData::AddRenderData(const WRenderData* pRenderData, WRenderData::Category category, WRenderData::Caching::Enum cachingBehavior)
{
  auto& cached = m_ExtractedRenderData.ExpandAndGetRef();
  cached.m_pRenderData = pRenderData;
  cached.m_Category = WRenderData::ResolveCategory(category, pRenderData->m_Flags.IsSet(WRenderData::Flags::Dynamic));

  if (cachingBehavior == WRenderData::Caching::IfStatic)
  {
    ++m_uiNumCacheIfStatic;
  }
}

void WMsgExtractRenderData::AddDependency(WGALTextureHandle hTexture, WRenderData::Category category, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG(requiredState != WGALResourceState::Unknown, "The required state must be valid");
  if (hTexture.IsInvalidated())
    return;
  auto& dep = m_TextureDependencies.ExpandAndGetRef();
  dep.m_hTexture = hTexture;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
  dep.m_uiCategory = category.m_uiValue;
}

void WMsgExtractRenderData::AddDependency(WGALBufferHandle hBuffer, WRenderData::Category category, WBitflags<WGALResourceState> requiredState, WBitflags<WGALShaderStageFlags> stage)
{
  W_ASSERT_DEBUG(requiredState != WGALResourceState::Unknown, "The required state must be valid");
  if (hBuffer.IsInvalidated())
    return;
  auto& dep = m_BufferDependencies.ExpandAndGetRef();
  dep.m_hBuffer = hBuffer;
  dep.m_RequiredState = requiredState;
  dep.m_Stage = stage;
  dep.m_uiCategory = category.m_uiValue;
}

W_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_RenderData);
