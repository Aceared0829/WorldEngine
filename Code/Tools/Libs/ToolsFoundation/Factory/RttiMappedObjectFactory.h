#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// A factory that creates the closest matching objects according to the passed type.
///
/// Creators can be registered at the factory for a specific type.
/// When the create function is called for a type, the parent type hierarchy is traversed until
/// the first type is found for which a creator is registered.
template <typename Object>
class WRttiMappedObjectFactory
{
  W_DISALLOW_COPY_AND_ASSIGN(WRttiMappedObjectFactory);

public:
  WRttiMappedObjectFactory();
  ~WRttiMappedObjectFactory();

  using CreateObjectFunc = Object* (*)(const WRTTI*);

  void RegisterCreator(const WRTTI* pType, CreateObjectFunc creator);
  void UnregisterCreator(const WRTTI* pType);
  Object* CreateObject(const WRTTI* pType);

  struct Event
  {
    enum class Type
    {
      CreatorAdded,
      CreatorRemoved
    };

    Type m_Type;
    const WRTTI* m_pRttiType;
  };

  WEvent<const Event&> m_Events;

private:
  WHashTable<const WRTTI*, CreateObjectFunc> m_Creators;
};

#include <ToolsFoundation/Factory/Implementation/RttiMappedObjectFactory_inl.h>
