#include <Core/CorePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPhysicsWorldModuleInterface, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_STATIC_REFLECTED_BITFLAGS(WPhysicsShapeType, 1)
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Static),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Dynamic),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Query),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Trigger),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Character),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Ragdoll),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Rope),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Cloth),
  W_BITFLAGS_CONSTANT(WPhysicsShapeType::Debris),
W_END_STATIC_REFLECTED_BITFLAGS;

W_IMPLEMENT_MESSAGE_TYPE(WMsgPhysicsAddImpulse);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgPhysicsAddImpulse, 1, WRTTIDefaultAllocator<WMsgPhysicsAddImpulse>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    W_MEMBER_PROPERTY("Impulse", m_vImpulse),
    W_MEMBER_PROPERTY("ObjectFilterID", m_uiObjectFilterID),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgPhysicCharacterContact);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgPhysicCharacterContact, 1, WRTTIDefaultAllocator<WMsgPhysicCharacterContact>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Character", m_hCharacter),
    W_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    W_MEMBER_PROPERTY("Normal", m_vNormal),
    W_MEMBER_PROPERTY("CharacterVelocity", m_vCharacterVelocity),
    W_MEMBER_PROPERTY("Impact", m_fImpact),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgPhysicContact);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgPhysicContact, 1, WRTTIDefaultAllocator<WMsgPhysicContact>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("OtherObject", m_hOtherObject),
    W_MEMBER_PROPERTY("GlobalPosition", m_vGlobalPosition),
    W_MEMBER_PROPERTY("Normal", m_vNormal),
    W_MEMBER_PROPERTY("ImpactSqr", m_fImpactSqr),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgPhysicsJointBroke);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgPhysicsJointBroke, 1, WRTTIDefaultAllocator<WMsgPhysicsJointBroke>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("JointObject", m_hJointObject)
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE

W_IMPLEMENT_MESSAGE_TYPE(WMsgObjectGrabbed);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgObjectGrabbed, 1, WRTTIDefaultAllocator<WMsgObjectGrabbed>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GrabbedBy", m_hGrabbedBy),
    W_MEMBER_PROPERTY("GotGrabbed", m_bGotGrabbed),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgPhysicsMakeTemporarilyDynamic);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgPhysicsMakeTemporarilyDynamic, 1, WRTTIDefaultAllocator<WMsgPhysicsMakeTemporarilyDynamic>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_IMPLEMENT_MESSAGE_TYPE(WMsgReleaseObjectGrab);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgReleaseObjectGrab, 1, WRTTIDefaultAllocator<WMsgReleaseObjectGrab>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GrabbedObjectToRelease", m_hGrabbedObjectToRelease),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

W_IMPLEMENT_MESSAGE_TYPE(WMsgBuildStaticMesh);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgBuildStaticMesh, 1, WRTTIDefaultAllocator<WMsgBuildStaticMesh>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WExcludeFromScript()
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


W_STATICLINK_FILE(Core, Core_Interfaces_PhysicsWorldModule);
