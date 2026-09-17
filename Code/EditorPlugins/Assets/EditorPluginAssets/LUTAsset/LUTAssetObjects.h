#pragma once

#include <EditorFramework/Assets/SimpleAssetDocument.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>


class WLUTAssetProperties : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WLUTAssetProperties, WReflectedClass);

public:
  static void PropertyMetaStateEventHandler(WPropertyMetaStateEvent& e);

  const char* GetInputFile() const { return m_sInput; }
  void SetInputFile(const char* szFile) { m_sInput = szFile; }

  WString GetAbsoluteInputFilePath() const;

private:
  WString m_sInput;
};
