#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WExposedParameter, WNoBase, 3, WRTTIDefaultAllocator<WExposedParameter>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName),
    W_MEMBER_PROPERTY("Type", m_sType),
    W_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    W_ENUM_MEMBER_PROPERTY("Category", WPropertyCategory, m_Category)->AddAttributes(new WDefaultValueAttribute(WPropertyCategory::Member)),
    W_ARRAY_MEMBER_PROPERTY("Attributes", m_Attributes)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_STATIC_REFLECTED_TYPE;

WExposedParameter::WExposedParameter()
= default;

WExposedParameter::~WExposedParameter()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
  }
}

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WExposedParameters, 3, WRTTIDefaultAllocator<WExposedParameters>)
{
  W_BEGIN_PROPERTIES
  {
    W_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WExposedParameters::WExposedParameters() = default;

WExposedParameters::~WExposedParameters()
{
  for (auto pAttr : m_Parameters)
  {
    WGetStaticRTTI<WExposedParameter>()->GetAllocator()->Deallocate(pAttr);
  }
}

const WExposedParameter* WExposedParameters::Find(const char* szParamName) const
{
  const WExposedParameter* const* pParam =
    std::find_if(cbegin(m_Parameters), cend(m_Parameters), [szParamName](const WExposedParameter* pParam)
      { return pParam->m_sName == szParamName; });
  return pParam != cend(m_Parameters) ? *pParam : nullptr;
}
