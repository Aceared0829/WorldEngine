#pragma once

#include <Core/Utils/CustomData.h>

// BEGIN-DOCS-CODE-SNIPPET: customdata-decl
class SampleCustomData : public WCustomData
{
  W_ADD_DYNAMIC_REFLECTION(SampleCustomData, WCustomData);

public:
  WString m_sText;
  WInt32 m_iSize = 42;
  WColor m_Color;
};

W_DECLARE_CUSTOM_DATA_RESOURCE(SampleCustomData);
// END-DOCS-CODE-SNIPPET
