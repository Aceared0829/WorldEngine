#include <Core/CorePCH.h>

#include <Core/World/SpatialData.h>

WHybridArray<WSpatialData::CategoryData, 32>& WSpatialData::GetCategoryData()
{
  static WHybridArray<WSpatialData::CategoryData, 32> CategoryData;
  return CategoryData;
}

// static
WSpatialData::Category WSpatialData::RegisterCategory(WStringView sCategoryName, const WBitflags<Flags>& flags)
{
  if (sCategoryName.IsEmpty())
    return WInvalidSpatialDataCategory;

  Category oldCategory = FindCategory(sCategoryName);
  if (oldCategory != WInvalidSpatialDataCategory)
  {
    W_ASSERT_DEV(GetCategoryFlags(oldCategory) == flags, "Category registered with different flags");
    return oldCategory;
  }

  if (GetCategoryData().GetCount() == 32)
  {
    W_REPORT_FAILURE("Too many spatial data categories");
    return WInvalidSpatialDataCategory;
  }

  Category newCategory = Category(static_cast<WUInt16>(GetCategoryData().GetCount()));

  auto& data = GetCategoryData().ExpandAndGetRef();
  data.m_sName.Assign(sCategoryName);
  data.m_Flags = flags;

  return newCategory;
}

// static
WSpatialData::Category WSpatialData::FindCategory(WStringView sCategoryName)
{
  WTempHashedString categoryName(sCategoryName);

  for (WUInt32 uiCategoryIndex = 0; uiCategoryIndex < GetCategoryData().GetCount(); ++uiCategoryIndex)
  {
    if (GetCategoryData()[uiCategoryIndex].m_sName == categoryName)
      return Category(static_cast<WUInt16>(uiCategoryIndex));
  }

  return WInvalidSpatialDataCategory;
}

// static
const WHashedString& WSpatialData::GetCategoryName(Category category)
{
  if (category.m_uiValue < GetCategoryData().GetCount())
  {
    return GetCategoryData()[category.m_uiValue].m_sName;
  }

  static WHashedString sInvalidSpatialDataCategoryName;
  return sInvalidSpatialDataCategoryName;
}

// static
const WBitflags<WSpatialData::Flags>& WSpatialData::GetCategoryFlags(Category category)
{
  return GetCategoryData()[category.m_uiValue].m_Flags;
}

//////////////////////////////////////////////////////////////////////////

WSpatialData::Category WDefaultSpatialDataCategories::RenderStatic = WSpatialData::RegisterCategory("RenderStatic", WSpatialData::Flags::None);
WSpatialData::Category WDefaultSpatialDataCategories::RenderDynamic = WSpatialData::RegisterCategory("RenderDynamic", WSpatialData::Flags::FrequentChanges);
WSpatialData::Category WDefaultSpatialDataCategories::OcclusionStatic = WSpatialData::RegisterCategory("OcclusionStatic", WSpatialData::Flags::None);
WSpatialData::Category WDefaultSpatialDataCategories::OcclusionDynamic = WSpatialData::RegisterCategory("OcclusionDynamic", WSpatialData::Flags::FrequentChanges);
