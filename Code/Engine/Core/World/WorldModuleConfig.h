#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Basics.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

/// Configuration class for world module interface implementations.
///
/// Manages the mapping between world module interfaces and their specific implementations.
/// This is used when multiple implementations exist for the same interface, allowing
/// configuration of which implementation should be used by default.
class W_CORE_DLL WWorldModuleConfig
{
public:
  WResult Save();
  void Load();

  /// Applies the current configuration to the world module factory.
  void Apply();

  /// Adds a mapping from an interface to a specific implementation.
  void AddInterfaceImplementation(WStringView sInterfaceName, WStringView sImplementationName);

  /// Removes the implementation mapping for the given interface.
  void RemoveInterfaceImplementation(WStringView sInterfaceName);

  /// Represents a mapping between an interface and its implementation.
  struct InterfaceImpl
  {
    WString m_sInterfaceName;      ///< Name of the world module interface
    WString m_sImplementationName; ///< Name of the specific implementation to use

    bool operator<(const InterfaceImpl& rhs) const { return m_sInterfaceName < rhs.m_sInterfaceName; }
  };

  WHybridArray<InterfaceImpl, 8> m_InterfaceImpls; ///< List of interface to implementation mappings
};
