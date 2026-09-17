#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

struct W_EDITORFRAMEWORK_DLL WExposedParameter
{
  WExposedParameter();
  virtual ~WExposedParameter();

  WString m_sName;
  WString m_sType;
  WVariant m_DefaultValue;
  WEnum<WPropertyCategory> m_Category;
  WHybridArray<WPropertyAttribute*, 2> m_Attributes;
};
W_DECLARE_REFLECTABLE_TYPE(W_EDITORFRAMEWORK_DLL, WExposedParameter)

class W_EDITORFRAMEWORK_DLL WExposedParameters : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WExposedParameters, WReflectedClass);

public:
  WExposedParameters();
  virtual ~WExposedParameters();

  const WExposedParameter* Find(const char* szParamName) const;

  WDynamicArray<WExposedParameter*> m_Parameters;
};
