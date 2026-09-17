#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>

class WWorld;

class W_EDITORENGINEPROCESSFRAMEWORK_DLL WSceneExportModifier : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WSceneExportModifier, WReflectedClass);

public:
  static void CreateModifiers(WDynamicArray<WSceneExportModifier*>& ref_modifiers);
  static void DestroyModifiers(WDynamicArray<WSceneExportModifier*>& ref_modifiers);

  static void ApplyAllModifiers(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport);

  virtual void ModifyWorld(WWorld& ref_world, WStringView sDocumentType, const WUuid& documentGuid, bool bForExport) = 0;

  static void CleanUpWorld(WWorld& ref_world);
};
