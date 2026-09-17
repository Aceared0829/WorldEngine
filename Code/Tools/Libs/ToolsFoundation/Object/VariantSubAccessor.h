#pragma once

#include <ToolsFoundation/Object/ObjectProxyAccessor.h>

class WDocumentObject;

/// Accessor for a sub-tree on an WVariant property.
/// The tools foundation code uses an WDocumentObject, one of its WAbstractProperty and an optional WVariant index to reference to properties. Any deeper hierarchies must be built from additional objects. This principle prevents the GUI to reference anything inside an WVariant that stores an VariantArray or VariantDictionary as WVariant is a pure value type and cannot store additional objects on the tool side. To work around this, this class creates a view one level deeper into an WVariant. This is done by calling SetSubItems which for each object in the map moves the view into the sub-tree referenced by the given value of the map.
class W_TOOLSFOUNDATION_DLL WVariantSubAccessor : public WObjectProxyAccessor
{
  W_ADD_DYNAMIC_REFLECTION(WVariantSubAccessor, WObjectProxyAccessor);

public:
  /// Constructor
  /// \param pSource The original accessor that is going to be proxied. By chaining this class an WVariant can be explored deeper and deeper.
  /// \param pProp The WVariant property that is going to be proxied. Only this property is allowed to be accessed by the accessor functions.
  WVariantSubAccessor(WObjectAccessorBase* pSource, const WAbstractProperty* pProp);
  /// Sets the sub-tree indices for the selected objects.
  /// \param subItemMap Object to index map. Note that as this is in the ToolsFoundation it cannot use the WPropertySelection class.
  void SetSubItems(const WMap<const WDocumentObject*, WVariant>& subItemMap);
  /// Returns the property this accessor wraps.
  const WAbstractProperty* GetRootProperty() const { return m_pProp; }
  /// How many level deep the view is inside the property.
  WInt32 GetDepth() const;
  /// Builds a path up the hierarchy of wrapped WVariantSubAccessor objects to determine the path to the current sub-tree of the WVariant.
  /// \param pObject The object for which the path should be computed
  /// \param out_path An array of indices that has to be followed from the root of the WVariant to each the current sub-tree view.
  /// \return Returns W_FAILURE if pObject is not known.
  WResult GetPath(const WDocumentObject* pObject, WDynamicArray<WVariant>& out_path) const;

  virtual WStatus GetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value, WVariant index = WVariant()) override;
  virtual WStatus SetValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus InsertValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& newValue, WVariant index = WVariant()) override;
  virtual WStatus RemoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) override;
  virtual WStatus MoveValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WVariant& oldIndex, const WVariant& newIndex) override;
  virtual WStatus GetCount(const WDocumentObject* pObject, const WAbstractProperty* pProp, WInt32& out_iCount) override;
  virtual WStatus GetKeys(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_keys) override;
  virtual WStatus GetValues(const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_values) override;

  virtual WObjectAccessorBase* ResolveProxy(const WDocumentObject*& ref_pObject, const WRTTI*& ref_pType, const WAbstractProperty*& ref_pProp, WDynamicArray<WVariant>& ref_indices) override;

private:
  WStatus GetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant& out_value);
  WStatus SetSubValue(const WDocumentObject* pObject, const WAbstractProperty* pProp, const WDelegate<WStatus(WVariant& subValue)>& func);

private:
  const WAbstractProperty* m_pProp = nullptr;
  WMap<const WDocumentObject*, WVariant> m_SubItemMap;
};
