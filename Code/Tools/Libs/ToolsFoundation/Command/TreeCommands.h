#pragma once

#include <ToolsFoundation/Command/Command.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WAddObjectCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WAddObjectCommand, WCommand);

public:
  WAddObjectCommand();

public: // Properties
  void SetType(WStringView sType);
  WStringView GetType() const;

  const WRTTI* m_pType = nullptr;
  WUuid m_Parent;
  WString m_sParentProperty;
  WVariant m_Index;
  WUuid m_NewObjectGuid; ///< This is optional. If not filled out, a new guid is assigned automatically.

private:
  virtual bool HasReturnValues() const override { return true; }
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  WDocumentObject* m_pObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


class W_TOOLSFOUNDATION_DLL WPasteObjectsCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WPasteObjectsCommand, WCommand);

public:
  WPasteObjectsCommand();

public: // Properties
  WUuid m_Parent;
  WString m_sGraphTextFormat;
  WString m_sMimeType;
  bool m_bAllowPickedPosition = true;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    WDocumentObject* m_pObject;
    WDocumentObject* m_pParent;
    WString m_sParentProperty;
    WVariant m_Index;
  };

  WHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WInstantiatePrefabCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WInstantiatePrefabCommand, WCommand);

public:
  WInstantiatePrefabCommand();

public: // Properties
  WUuid m_Parent;
  WInt32 m_Index = -1;
  WUuid m_CreateFromPrefab;
  WUuid m_RemapGuid;
  WString m_sBasePrefabGraph;
  WString m_sObjectGraph;
  WUuid m_CreatedRootObject;
  bool m_bAllowPickedPosition;

private:
  virtual bool HasReturnValues() const override { return true; }
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  struct PastedObject
  {
    WDocumentObject* m_pObject;
    WDocumentObject* m_pParent;
    WString m_sParentProperty;
    WVariant m_Index;
  };

  // at the moment this array always only holds a single item, the group node for the prefab
  WHybridArray<PastedObject, 4> m_PastedObjects;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WUnlinkPrefabCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WUnlinkPrefabCommand, WCommand);

public:
  WUnlinkPrefabCommand() = default;

  WUuid m_Object;

private:
  virtual bool HasReturnValues() const override { return false; }
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WUuid m_OldCreateFromPrefab;
  WUuid m_OldRemapGuid;
  WString m_sOldGraphTextFormat;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WRemoveObjectCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WRemoveObjectCommand, WCommand);

public:
  WRemoveObjectCommand();

public: // Properties
  WUuid m_Object;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  WDocumentObject* m_pParent = nullptr;
  WString m_sParentProperty;
  WVariant m_Index;
  WDocumentObject* m_pObject = nullptr;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WMoveObjectCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WMoveObjectCommand, WCommand);

public:
  WMoveObjectCommand();

public: // Properties
  WUuid m_Object;
  WUuid m_NewParent;
  WString m_sParentProperty;
  WVariant m_Index;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pObject;
  WDocumentObject* m_pOldParent;
  WDocumentObject* m_pNewParent;
  WString m_sOldParentProperty;
  WVariant m_OldIndex;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WSetObjectPropertyCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WSetObjectPropertyCommand, WCommand);

public:
  WSetObjectPropertyCommand();

public: // Properties
  WUuid m_Object;
  WVariant m_NewValue;
  WVariant m_Index;
  WString m_sProperty;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pObject;
  WVariant m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WResizeAndSetObjectPropertyCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WResizeAndSetObjectPropertyCommand, WCommand);

public:
  WResizeAndSetObjectPropertyCommand();

public: // Properties
  WUuid m_Object;
  WVariant m_NewValue;
  WVariant m_Index;
  WString m_sProperty;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override { return W_SUCCESS; }
  virtual void CleanupInternal(CommandState state) override {}

  WDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WInsertObjectPropertyCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WInsertObjectPropertyCommand, WCommand);

public:
  WInsertObjectPropertyCommand();

public: // Properties
  WUuid m_Object;
  WVariant m_NewValue;
  WVariant m_Index;
  WString m_sProperty;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pObject;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WRemoveObjectPropertyCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WRemoveObjectPropertyCommand, WCommand);

public:
  WRemoveObjectPropertyCommand();

public: // Properties
  WUuid m_Object;
  WVariant m_Index;
  WString m_sProperty;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pObject;
  WVariant m_OldValue;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class W_TOOLSFOUNDATION_DLL WMoveObjectPropertyCommand : public WCommand
{
  W_ADD_DYNAMIC_REFLECTION(WMoveObjectPropertyCommand, WCommand);

public:
  WMoveObjectPropertyCommand();

public: // Properties
  WUuid m_Object;
  WVariant m_OldIndex;
  WVariant m_NewIndex;
  WString m_sProperty;

private:
  virtual WStatus DoInternal(bool bRedo) override;
  virtual WStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  WDocumentObject* m_pObject;
};
