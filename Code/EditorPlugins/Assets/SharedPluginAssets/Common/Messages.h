#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <SharedPluginAssets/SharedPluginAssetsDLL.h>

class W_SHAREDPLUGINASSETS_DLL WEditorEngineRestartSimulationMsg : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineRestartSimulationMsg, WEditorEngineDocumentMsg);

public:
};

class W_SHAREDPLUGINASSETS_DLL WEditorEngineLoopAnimationMsg : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineLoopAnimationMsg, WEditorEngineDocumentMsg);

public:
  bool m_bLoop;
};

class W_SHAREDPLUGINASSETS_DLL WEditorEngineSetMaterialsMsg : public WEditorEngineDocumentMsg
{
  W_ADD_DYNAMIC_REFLECTION(WEditorEngineSetMaterialsMsg, WEditorEngineDocumentMsg);

public:
  WHybridArray<WString, 16> m_Materials;

  /// Human-readable display name for each material slot (e.g. the material asset filename).
  /// Used by asset previews to show which material is under the cursor.
  WHybridArray<WString, 16> m_SlotNames;
};
