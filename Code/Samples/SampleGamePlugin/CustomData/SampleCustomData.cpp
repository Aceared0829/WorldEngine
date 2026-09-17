#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/CustomData/SampleCustomData.h>

// BEGIN-DOCS-CODE-SNIPPET: customdata-impl
// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(SampleCustomData, 1, WRTTIDefaultAllocator<SampleCustomData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Text", m_sText),
    W_MEMBER_PROPERTY("Size", m_iSize)->AddAttributes(new WDefaultValueAttribute(42), new WClampValueAttribute(16, 64)),
    W_MEMBER_PROPERTY("Color", m_Color)->AddAttributes(new WDefaultValueAttribute(WColor::CornflowerBlue)),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_DEFINE_CUSTOM_DATA_RESOURCE(SampleCustomData);
// END-DOCS-CODE-SNIPPET
