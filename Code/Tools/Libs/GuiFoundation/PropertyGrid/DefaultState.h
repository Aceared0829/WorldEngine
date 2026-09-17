#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Basics.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/Status.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

class WDefaultStateProvider;
class WObjectAccessorBase;
class WDocumentObject;
class WAbstractProperty;

/// Registry for all WDefaultStateProvider factory functions.
class W_GUIFOUNDATION_DLL WDefaultState
{
public:
  /// The functor interface for the WDefaultStateProvider factory function
  ///
  /// The return value is a sharedPtr as each implementation can decide whether to provide the same instance for all objects or whether a custom instance should be created for each object to allow for state caching (e.g. prefab root information). Returning nullptr is also valid for objects / containers for which the factory has no use (e.g. prefab default state provider on an object that does not belong to a prefab).
  /// The function is called for WDefaultObjectState usage with the pProp field left blank.
  /// For WDefaultContainerState usage pProp will point to the container property.
  using CreateStateProviderFunc = WSharedPtr<WDefaultStateProvider> (*)(WObjectAccessorBase*, const WDocumentObject*, const WAbstractProperty*);

  /// Registers a WDefaultStateProvider factory method. It is safe to register / unregister factories at any time.
  static void RegisterDefaultStateProvider(CreateStateProviderFunc func);
  /// Unregisters a WDefaultStateProvider factory method.
  static void UnregisterDefaultStateProvider(CreateStateProviderFunc func);

private:
  friend class WDefaultObjectState;
  friend class WDefaultContainerState;
  static WDynamicArray<CreateStateProviderFunc> s_Factories;
};

/// Object used to query and revert to the default state of all properties of an object.
///
/// This class should not be persisted in memory and just used on the stack to query all property states and then destroyed. It should also not be used across hierarchical changes of any kind (deleting objects etc).
class W_GUIFOUNDATION_DLL WDefaultObjectState
{
  W_DISALLOW_COPY_AND_ASSIGN(WDefaultObjectState);

public:
  /// Constructor. Will collect the appropriate WDefaultStateProviders to query the states.
  /// \param pType The common base type of the selection.
  /// \param pAccessor Used to revert properties and query their current value.
  /// \param selection For which objects the default state should be queried. The WPropertySelection::m_Index should be invalid.
  WDefaultObjectState(const WRTTI* pType, WObjectAccessorBase* pAccessor, const WArrayPtr<WPropertySelection> selection);

  /// Returns the color of the top-most WDefaultStateProvider of the first element of the selection.
  WColorGammaUB GetBackgroundColor() const;
  /// Returns the name of the top-most WDefaultStateProvider of the first element of the selection.
  WString GetStateProviderName() const;

  bool IsDefaultValue(const char* szProperty) const;
  bool IsDefaultValue(const WAbstractProperty* pProp) const;
  WStatus RevertProperty(const char* szProperty);
  WStatus RevertProperty(const WAbstractProperty* pProp);
  WStatus RevertObject();
  WVariant GetDefaultValue(const char* szProperty, WUInt32 uiSelectionIndex = 0) const;
  WVariant GetDefaultValue(const WAbstractProperty* pProp, WUInt32 uiSelectionIndex = 0) const;


private:
  const WRTTI* m_pType = nullptr;
  WObjectAccessorBase* m_pAccessor = nullptr;
  WArrayPtr<WPropertySelection> m_Selection;
  WHybridArray<WHybridArray<WSharedPtr<WDefaultStateProvider>, 4>, 1> m_Providers;
};

/// Object used to query and revert to the default state of all elements of a container of an object.
///
/// This class should not be persisted in memory and just used on the stack to query all element states and then destroyed. It should also not be used across hierarchical changes of any kind (deleting objects etc).
class W_GUIFOUNDATION_DLL WDefaultContainerState
{
  W_DISALLOW_COPY_AND_ASSIGN(WDefaultContainerState);

public:
  /// Constructor. Will collect the appropriate WDefaultStateProviders to query the states.
  /// \param pType The common base type of the selection.
  /// \param pAccessor Used to revert properties and query their current value.
  /// \param selection For which objects the default state should be queried. If WPropertySelection::m_Index is set, IsDefaultElement and RevertElement will query the value under that index if the passed in index is invalid.
  /// \param szProperty The name of the container for which default states should be queried.
  WDefaultContainerState(const WRTTI* pType, WObjectAccessorBase* pAccessor, const WArrayPtr<WPropertySelection> selection, const char* szProperty);

  /// Returns the color of the top-most WDefaultStateProvider of the first element of the selection.
  /// \sa WDefaultStateProvider::GetBackgroundColor
  WColorGammaUB GetBackgroundColor() const;
  /// Returns the name of the top-most WDefaultStateProvider of the first element of the selection.
  /// \sa WDefaultStateProvider::GetStateProviderName
  WString GetStateProviderName() const;

  bool IsDefaultElement(WVariant index) const;
  bool IsDefaultContainer() const;
  WStatus RevertElement(WVariant index);
  WStatus RevertContainer();
  WVariant GetDefaultElement(WVariant index, WUInt32 uiSelectionIndex = 0) const;
  WVariant GetDefaultContainer(WUInt32 uiSelectionIndex = 0) const;

private:
  const WRTTI* m_pType = nullptr;
  WObjectAccessorBase* m_pAccessor = nullptr;
  const WAbstractProperty* m_pProp = nullptr;
  WArrayPtr<WPropertySelection> m_Selection;
  WHybridArray<WHybridArray<WSharedPtr<WDefaultStateProvider>, 4>, 1> m_Providers;
};

/// Interface for querying and restoring the default state of objects and containers.
///
/// The high level functions IsDefaultValue, RevertProperty, RevertObjectContainer don't need to be overwritten in most cases. Instead, just implementing the pure virtual methods is enough.
class W_GUIFOUNDATION_DLL WDefaultStateProvider : public WRefCounted
{
public:
  /// Parent hierarchy of state providers.
  ///
  /// WDefaultContainerState and WDefaultObjectState will build a hierarchy of parent default state providers depending on the root depth of all available providers (this is like virtual function overrides but with dynamic parent classes). If a provider can't handle a request, it should forward it to the first element in the superPtr array and pass in superPtr.GetSubArray(1) to that function call. Note that generally you don't need to check for validity of the ptr as the WAttributeDefaultStateProvider has root depth of -1 and will thus always be the last one in line.
  using SuperArray = const WArrayPtr<const WSharedPtr<WDefaultStateProvider>>;

  /// Returns the root depth of this provider instance.
  ///
  /// This is through how many properties and objects we needed to pass through from the object and property passed into the factory method to find the root object / property that this provider represents.
  /// For example if we have this object hierarchy:
  /// A
  /// |-children- B
  ///             |-elements- C
  ///
  /// If A is a prefab and the factory method was called for C (with no property) then we need to walk up the hierarchy via elements container, the B object, the children container and then finally A. Thus, we need 4 hops to get the the prefab root which means the root depth for this provider instance is 4.
  virtual WInt32 GetRootDepth() const = 0;

  /// Returns a color to be used in the property grid. Only the hue of the color is used. If alpha is 0, the color is ignored and no tinting of the property grid takes place.
  virtual WColorGammaUB GetBackgroundColor() const = 0;

  /// Returns the name of this state provider. Can be used to check what the outer most provider is for GUI purposes.
  virtual WString GetStateProviderName() const = 0;

  /// Returns the default value of an object's property at a given index.
  /// \param superPtr Parent hierarchy of inner providers that should be called of this instance cannot handle the request. See SuperArray definition for details.
  /// \param pAccessor Accessor to be used for querying object values if necessary. Always valid.
  /// \param pObject The object for which the default value should be queried. Always valid.
  /// \param pProp The property for which the default value should be queried. Always valid.
  /// \param index For containers: If the index is valid, the container element's default value is requested. If not, the entire container (either array or dictionary) is requested.
  /// \return The default value. WReflectionUtils::GetDefaultValue is a good example what is expected to be returned.
  /// \sa WReflectionUtils::GetDefaultValue, WDefaultStateProvider::DoesVariantMatchProperty
  virtual WVariant GetDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant()) = 0;

  /// Queries an array of diff operations that can be executed to revert the object container.
  /// \param superPtr superPtr Parent hierarchy of inner providers that should be called of this instance cannot handle the request. See SuperArray definition for details.
  /// \param pAccessor pAccessor Accessor to be used for querying object values if necessary. Always valid.
  /// \param pObject pObject The object which is to be reverted. Always valid.
  /// \param pProp pProp The container property which is to be reverted. Always valid.
  /// \param out_diff An array of diff operations that should be executed via WDocumentObjectConverterReader::ApplyDiffToObject to revert the object / container to its default state.
  /// \return If failure is returned, the operation failed and the undo transaction should be canceled.
  /// \sa WDocumentObjectConverterReader::ApplyDiffToObject
  virtual WStatus CreateRevertContainerDiff(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDeque<WAbstractGraphDiffOperation>& out_diff) = 0;

public:
  virtual bool IsDefaultValue(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant());
  virtual WStatus RevertProperty(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp, WVariant index = WVariant());
  virtual WStatus RevertObjectContainer(SuperArray superPtr, WObjectAccessorBase* pAccessor, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  /// A sanity check function that verifies that a given variant's value matches that expected of the property at the given index. If index is invalid and the property a container, the value must be an array or dictionary of the property's type.
  static bool DoesVariantMatchProperty(const WVariant& value, const WAbstractProperty* pProp, WVariant index = WVariant());
};
