#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class WDocumentObjectManager;

class W_TOOLSFOUNDATION_DLL WDocumentObject
{
public:
  WDocumentObject() = default;
  virtual ~WDocumentObject() = default;

  // Accessors
  const WUuid& GetGuid() const { return m_Guid; }
  /// Returns the RTTI type of the object that is represented by this WDocumentObject.
  const WRTTI* GetType() const { return GetTypeAccessor().GetType(); }

  const WDocumentObjectManager* GetDocumentObjectManager() const { return m_pDocumentObjectManager; }
  WDocumentObjectManager* GetDocumentObjectManager() { return m_pDocumentObjectManager; }

  virtual const WIReflectedTypeAccessor& GetTypeAccessor() const = 0;
  WIReflectedTypeAccessor& GetTypeAccessor();

  // Ownership
  const WDocumentObject* GetParent() const { return m_pParent; }

  virtual void InsertSubObject(WDocumentObject* pObject, WStringView sProperty, const WVariant& index);
  virtual void RemoveSubObject(WDocumentObject* pObject);

  // Helper
  void ComputeObjectHash(WUInt64& ref_uiHash) const;
  const WHybridArray<WDocumentObject*, 8>& GetChildren() const { return m_Children; }
  WDocumentObject* GetChild(const WUuid& guid);
  const WDocumentObject* GetChild(const WUuid& guid) const;
  WStringView GetParentProperty() const { return m_sParentProperty; }
  const WAbstractProperty* GetParentPropertyType() const;
  WVariant GetPropertyIndex() const;
  bool IsOnHeap() const;
  WUInt32 GetChildIndex(const WDocumentObject* pChild) const;

private:
  friend class WDocumentObjectManager;
  void HashPropertiesRecursive(const WIReflectedTypeAccessor& acc, WUInt64& uiHash, const WRTTI* pType) const;

protected:
  WUuid m_Guid;
  WDocumentObjectManager* m_pDocumentObjectManager = nullptr;

  WDocumentObject* m_pParent = nullptr;
  WHybridArray<WDocumentObject*, 8> m_Children;

  // Sub object data
  WString m_sParentProperty;
};

class W_TOOLSFOUNDATION_DLL WDocumentStorageObject : public WDocumentObject
{
public:
  WDocumentStorageObject(const WRTTI* pType)
    : WDocumentObject()
    , m_ObjectPropertiesAccessor(pType, this)
  {
  }

  virtual ~WDocumentStorageObject() = default;

  virtual const WIReflectedTypeAccessor& GetTypeAccessor() const override { return m_ObjectPropertiesAccessor; }

protected:
  WReflectedTypeStorageAccessor m_ObjectPropertiesAccessor;
};
