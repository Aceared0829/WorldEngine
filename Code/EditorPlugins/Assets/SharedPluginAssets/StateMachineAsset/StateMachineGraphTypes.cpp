#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineConnection, 1, WRTTIDefaultAllocator<WStateMachineConnection>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Type", m_pType)->AddFlags(WPropertyFlags::PointerOwner)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineNodeBase, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineNode, 1, WRTTIDefaultAllocator<WStateMachineNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new WDefaultValueAttribute(WStringView("State"))), // wrap in WStringView to prevent a memory leak report
    W_MEMBER_PROPERTY("Type", m_pType)->AddFlags(WPropertyFlags::PointerOwner),
    W_MEMBER_PROPERTY("IsInitialState", m_bIsInitialState)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WStateMachineNodeAny, 1, WRTTIDefaultAllocator<WStateMachineNodeAny>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
