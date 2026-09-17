#pragma once

#include <EditorFramework/Document/GameObjectDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>

class WGameObjectDocument;
class WQtGameObjectDocumentWindow;
class WObjectAccessorBase;
class WEditorInputContext;

class W_EDITORFRAMEWORK_DLL WGameObjectGizmoInterface
{
public:
  virtual WObjectAccessorBase* GetObjectAccessor() = 0;
  virtual bool CanDuplicateSelection() const = 0;
  virtual void DuplicateSelection() = 0;
};

//////////////////////////////////////////////////////////////////////////

enum class WEditToolSupportedSpaces
{
  LocalSpaceOnly,
  WorldSpaceOnly,
  LocalAndWorldSpace,
};

class W_EDITORFRAMEWORK_DLL WGameObjectEditTool : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WGameObjectEditTool, WReflectedClass);

public:
  WGameObjectEditTool();

  void ConfigureTool(WGameObjectDocument* pDocument, WQtGameObjectDocumentWindow* pWindow, WGameObjectGizmoInterface* pInterface);

  WGameObjectDocument* GetDocument() const { return m_pDocument; }
  WQtGameObjectDocumentWindow* GetWindow() const { return m_pWindow; }
  WGameObjectGizmoInterface* GetGizmoInterface() const { return m_pInterface; }
  bool IsActive() const { return m_bIsActive; }
  void SetActive(bool bActive);

  virtual WEditorInputContext* GetEditorInputContextOverride() { return nullptr; }
  virtual WEditToolSupportedSpaces GetSupportedSpaces() const { return WEditToolSupportedSpaces::WorldSpaceOnly; }
  virtual bool GetSupportsMoveParentOnly() const { return false; }
  virtual void GetGridSettings(WGridSettingsMsgToEngine& out_gridSettings) {}

protected:
  virtual void OnConfigured() = 0;
  virtual void OnActiveChanged(bool bIsActive) {}

private:
  bool m_bIsActive = false;
  WGameObjectDocument* m_pDocument = nullptr;
  WQtGameObjectDocumentWindow* m_pWindow = nullptr;
  WGameObjectGizmoInterface* m_pInterface = nullptr;
};
