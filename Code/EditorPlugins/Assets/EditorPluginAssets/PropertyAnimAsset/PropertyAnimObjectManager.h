#pragma once

#include <ToolsFoundation/Object/DocumentObjectManager.h>

class WPropertyAnimObjectManager : public WDocumentObjectManager
{
public:
  WPropertyAnimObjectManager();
  ~WPropertyAnimObjectManager();

  bool GetAllowStructureChangeOnTemporaries() const { return m_bAllowStructureChangeOnTemporaries; }
  void SetAllowStructureChangeOnTemporaries(bool bVal) { m_bAllowStructureChangeOnTemporaries = bVal; }

private:
  virtual WStatus InternalCanAdd(
    const WRTTI* pRtti, const WDocumentObject* pParent, WStringView sParentProperty, const WVariant& index) const override;
  virtual WStatus InternalCanRemove(const WDocumentObject* pObject) const override;
  virtual WStatus InternalCanMove(
    const WDocumentObject* pObject, const WDocumentObject* pNewParent, WStringView sParentProperty, const WVariant& index) const override;

private:
  bool m_bAllowStructureChangeOnTemporaries = false;
};
