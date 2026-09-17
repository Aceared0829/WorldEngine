#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/RefCounted.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

/// Describes the current meta state of a property for display purposes in the property grid
struct WPropertyUiState
{
  enum Visibility
  {
    Default,   ///< Displayed normally, for editing (unless the property is read-only)
    Invisible, ///< Hides the property entirely
    Disabled,  ///< The property is shown but disabled, when multiple objects are selected and in one the property is invisible, in the other it is
               ///< disabled, the disabled state takes precedence
  };

  WPropertyUiState()
  {
    m_Visibility = Visibility::Default;
  }

  Visibility m_Visibility;
  WString m_sNewLabelText;
};

/// Event that is broadcast whenever information about how to present properties is required
struct WPropertyMetaStateEvent
{
  /// The object for which the information is queried
  const WDocumentObject* m_pObject = nullptr;

  /// The map into which event handlers should write their information about the state of each property.
  /// The string is the property name that identifies the property in m_pObject.
  WMap<WString, WPropertyUiState>* m_pPropertyStates = nullptr;
};

/// Event that is broadcast whenever information about how to present elements in a container is required
struct WContainerElementMetaStateEvent
{
  /// The object for which the information is queried
  const WDocumentObject* m_pObject = nullptr;
  /// The Container property
  const char* m_szProperty = nullptr;
  /// The map into which event handlers should write their information about the state of each container element.
  /// The WVariant should be the key of the container element, either WUInt32 for arrays and sets or WString for maps.
  WHashTable<WVariant, WPropertyUiState>* m_pContainerElementStates = nullptr;
};

/// This class allows to query additional information about how to present properties in the property grid
///
/// The property grid calls GetTypePropertiesState() and GetContainerElementsState() with the current selection of WDocumentObject's.
/// This triggers the WPropertyMetaStateEvent to be broadcast, which allows for other code to determine additional
/// information for the properties and write it into the event data.
class W_GUIFOUNDATION_DLL WPropertyMetaState
{
  W_DECLARE_SINGLETON(WPropertyMetaState);

public:
  WPropertyMetaState();

  /// Queries the property meta state for a single WDocumentObject
  void GetTypePropertiesState(const WDocumentObject* pObject, WMap<WString, WPropertyUiState>& out_propertyStates);

  /// Queries the property meta state for a multi selection of WDocumentObject's
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetTypePropertiesState(const WArrayPtr<WPropertySelection>& items, WMap<WString, WPropertyUiState>& out_propertyStates);

  /// Queries the meta state for the elements of a single container property on one WDocumentObject.
  void GetContainerElementsState(const WDocumentObject* pObject, const char* szProperty, WHashTable<WVariant, WPropertyUiState>& out_propertyStates);

  /// Queries the meta state for the elements of a single container property on a multi selection of WDocumentObjects.
  ///
  /// This will query the information for every single selected object and then merge the result into one.
  void GetContainerElementsState(const WArrayPtr<WPropertySelection>& items, const char* szProperty, WHashTable<WVariant, WPropertyUiState>& out_propertyStates);

  /// Attach to this event to get notified of property state queries.
  /// Add information to WPropertyMetaStateEvent::m_pPropertyStates to return data.
  WEvent<WPropertyMetaStateEvent&> m_Events;
  /// Attach to this event to get notified of container element state queries.
  /// Add information to WContainerElementMetaStateEvent::m_pContainerElementStates to return data.
  WEvent<WContainerElementMetaStateEvent&> m_ContainerEvents;

private:
  WMap<WString, WPropertyUiState> m_Temp;
  WHashTable<WVariant, WPropertyUiState> m_Temp2;
};
