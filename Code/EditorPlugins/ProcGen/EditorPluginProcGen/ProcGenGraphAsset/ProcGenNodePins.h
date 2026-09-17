#pragma once

#include <Foundation/Reflection/Reflection.h>

/// Pin for connecting procedural generation nodes.
///
/// Pins represent input or output connections on ProcGen nodes. They are used to pass
/// values like density, position, color, or other data between nodes in the procedural generation graph.
struct WProcGenNodePin
{
  W_DECLARE_POD_TYPE();

  struct Type
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Input = W_BIT(0),  ///< Pin accepts input from other nodes.
      Output = W_BIT(1), ///< Pin provides output to other nodes.

      Default = 0
    };

    struct Bits
    {
      StorageType Input : 1;
      StorageType Output : 1;
    };
  };

  WBitflags<Type> m_Type;
  WUInt8 m_uiInputIndex = 0xFF;
  WUInt8 m_uiOutputIndex = 0xFF;
  class WProcGenNodeBase* m_pParent = nullptr;
};
W_DECLARE_FLAGS_OPERATORS(WProcGenNodePin::Type);

/// Input pin for receiving data from other procedural generation nodes.
struct WProcGenNodeInputPin : public WProcGenNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WProcGenNodeInputPin() { m_Type = Type::Input; }
};

/// Output pin for sending data to other procedural generation nodes.
struct WProcGenNodeOutputPin : public WProcGenNodePin
{
  W_DECLARE_POD_TYPE();

  W_ALWAYS_INLINE WProcGenNodeOutputPin() { m_Type = Type::Output; }
};

W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WProcGenNodePin);
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WProcGenNodeInputPin);
W_DECLARE_REFLECTABLE_TYPE(W_NO_LINKAGE, WProcGenNodeOutputPin);
