#include <Core/World/GameObject.h>

W_ALWAYS_INLINE WRenderData::Category::Category() = default;

W_ALWAYS_INLINE WRenderData::Category::Category(WUInt16 uiValue)
  : m_uiValue(uiValue)
{
}

W_ALWAYS_INLINE bool WRenderData::Category::operator==(const Category& other) const
{
  return m_uiValue == other.m_uiValue;
}

W_ALWAYS_INLINE bool WRenderData::Category::operator!=(const Category& other) const
{
  return m_uiValue != other.m_uiValue;
}

//////////////////////////////////////////////////////////////////////////

// static
W_FORCE_INLINE WHashedString WRenderData::GetCategoryName(Category category)
{
  if (category.m_uiValue < s_CategoryData.GetCount())
  {
    return s_CategoryData[category.m_uiValue].m_sName;
  }

  return WHashedString();
}

//////////////////////////////////////////////////////////////////////////

W_ALWAYS_INLINE bool WRenderData::IsDynamic() const
{
  return m_Flags.IsSet(Flags::Dynamic);
}

W_ALWAYS_INLINE bool WRenderData::IsStatic() const
{
  return !m_Flags.IsSet(Flags::Dynamic);
}

W_ALWAYS_INLINE bool WRenderData::FlipWinding() const
{
  return m_Flags.IsSet(Flags::FlipWinding);
}

W_FORCE_INLINE WUInt64 WRenderData::GetFinalSortingKey(Category category, const WCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, camera);
}

//////////////////////////////////////////////////////////////////////////

W_FORCE_INLINE bool WInstanceableRenderData::CanBatchByBaseValues(const WInstanceableRenderData& other) const
{
  return FlipWinding() == other.FlipWinding() && m_hInstanceDataBuffer == other.m_hInstanceDataBuffer;
}
