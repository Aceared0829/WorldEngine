#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Communication/Message.h>

/// Message sent when a game object's parent relationship changes.
///
/// Notifies components when their object is linked to or unlinked from a parent object.
struct W_CORE_DLL WMsgParentChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgParentChanged, WMessage);

  enum class Type
  {
    ParentLinked,
    ParentUnlinked,
    Invalid
  };

  Type m_Type = Type::Invalid;
  WGameObjectHandle m_hParent; // previous or new parent, depending on m_Type
};

/// Message sent when a game object's children change.
///
/// Notifies parent objects when child objects are added or removed from their hierarchy.
struct W_CORE_DLL WMsgChildrenChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgChildrenChanged, WMessage);

  enum class Type
  {
    ChildAdded,
    ChildRemoved
  };

  Type m_Type;
  WGameObjectHandle m_hParent;
  WGameObjectHandle m_hChild;
};

/// Message sent when components are added to or removed from a game object.
///
/// Notifies interested parties when the component composition of an object changes.
struct W_CORE_DLL WMsgComponentsChanged : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgComponentsChanged, WMessage);

  enum class Type
  {
    ComponentAdded,
    ComponentRemoved,
    Invalid
  };

  Type m_Type = Type::Invalid;
  WGameObjectHandle m_hOwner;
  WComponentHandle m_hComponent;
};
