#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Status.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocument;
class WCommandTransaction;

/// Interface for a command
///
/// Commands are the only objects that have non-const access to any data structures (contexts, documents etc.).
/// Thus, any modification must go through a command and the WCommandHistory is the only class capable of executing commands.
class W_TOOLSFOUNDATION_DLL WCommand : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WCommand, WReflectedClass);

public:
  WCommand();
  ~WCommand();

  bool IsUndoable() const { return m_bUndoable; };
  bool HasChildActions() const { return !m_ChildActions.IsEmpty(); }
  bool HasModifiedDocument() const;

  enum class CommandState
  {
    WasDone,
    WasUndone
  };

protected:
  WStatus Do(bool bRedo);
  WStatus Undo(bool bFireEvents);
  void Cleanup(CommandState state);

  WStatus AddSubCommand(WCommand& command);
  WDocument* GetDocument() { return m_pDocument; };

private:
  virtual bool HasReturnValues() const { return false; }
  virtual WStatus DoInternal(bool bRedo) = 0;
  virtual WStatus UndoInternal(bool bFireEvents) = 0;
  virtual void CleanupInternal(CommandState state) = 0;

protected:
  friend class WCommandHistory;
  friend class WCommandTransaction;

  WString m_sDescription;
  bool m_bUndoable = true;
  bool m_bModifiedDocument = true;
  WHybridArray<WCommand*, 8> m_ChildActions;
  WDocument* m_pDocument = nullptr;
};
