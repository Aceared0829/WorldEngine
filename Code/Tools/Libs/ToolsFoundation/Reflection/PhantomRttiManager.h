#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/IdTable.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class WPhantomRTTI;

struct WPhantomRttiManagerEvent
{
  enum class Type
  {
    TypeAdded,
    TypeRemoved,
    TypeChanged,
  };

  WPhantomRttiManagerEvent()

    = default;

  Type m_Type = Type::TypeAdded;
  const WRTTI* m_pChangedType = nullptr;
};

/// Manages all WPhantomRTTI types that have been added to him.
///
/// A WPhantomRTTI cannot be created directly but must be created via this managers
/// RegisterType function with a given WReflectedTypeDescriptor.
class W_TOOLSFOUNDATION_DLL WPhantomRttiManager
{
public:
  /// Adds a reflected type to the list of accessible types.
  ///
  /// Types must be added in the correct order, any type must be added before
  /// it can be referenced in other types. Any base class must be added before
  /// any class deriving from it can be added.
  /// Call the function again if a type has changed during the run of the
  /// program. If the type actually differs the last known class layout the
  /// m_TypeChangedEvent event will be called with the old and new WRTTI.
  ///
  /// \sa WReflectionUtils::GetReflectedTypeDescriptorFromRtti
  static const WRTTI* RegisterType(WReflectedTypeDescriptor& ref_desc);

  /// Removes a type from the list of accessible types.
  ///
  /// No instance of the given type or storage must still exist when this function is called.
  static bool UnregisterType(const WRTTI* pRtti);

private:
  W_MAKE_SUBSYSTEM_STARTUP_FRIEND(ToolsFoundation, ReflectedTypeManager);

  static void Startup();
  static void Shutdown();
  static void PluginEventHandler(const WPluginEvent& e);

public:
  static WCopyOnBroadcastEvent<const WPhantomRttiManagerEvent&> s_Events;

private:
  static WSet<const WRTTI*> s_RegisteredConcreteTypes;
  static WHashTable<WStringView, WPhantomRTTI*> s_NameToPhantom;
};
