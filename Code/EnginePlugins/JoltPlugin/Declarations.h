#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <JoltPlugin/JoltPluginDLL.h>

class WJoltActorComponent;

struct WJoltSteppingMode
{
  using StorageType = WUInt32;

  enum Enum
  {
    Variable,
    Fixed,
    SemiFixed,

    Default = SemiFixed
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WJoltSteppingMode);

//////////////////////////////////////////////////////////////////////////

/// Flags for what should happen when two physical bodies touch.
///
/// The reactions need to be set up through WSurface's.
/// For most objects only some reactions make sense.
/// For example a box may hit another object as well as slide, but it cannot roll.
/// A barrel can impact and slide on some sides, but roll around its up axis (Z).
/// A sphere can impact and roll around all its axis, but never slide.
/// A soft object may not have any impact reactions.
struct WOnJoltContact
{
  using StorageType = WUInt32;

  enum Enum
  {
    None = 0,
    SendContactMsg = W_BIT(0),  ///< On contact, send a WMsgPhysicContact msg directly to the component.
    ImpactReactions = W_BIT(1), ///< Spawn prefabs for impacts (two objects hit each other with enough force).
    SlideReactions = W_BIT(2),  ///< Spawn prefabs for sliding (one object slides along the surface of another).
    RollXReactions = W_BIT(3),  ///< Spawn prefabs for rolling (one object rotates around its X axis while touching another).
    RollYReactions = W_BIT(4),  ///< Spawn prefabs for rolling (one object rotates around its Y axis while touching another).
    RollZReactions = W_BIT(5),  ///< Spawn prefabs for rolling (one object rotates around its Z axis while touching another).

    AllRollReactions = RollXReactions | RollYReactions | RollZReactions,
    SlideAndRollReactions = AllRollReactions | SlideReactions,
    AllReactions = ImpactReactions | AllRollReactions | SlideReactions | SendContactMsg,

    Default = None
  };

  struct Bits
  {
    StorageType SendContactMsg : 1;
    StorageType ImpactReactions : 1;
    StorageType SlideReactions : 1;
    StorageType RollXReactions : 1;
    StorageType RollYReactions : 1;
    StorageType RollZReactions : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WOnJoltContact);
W_DECLARE_REFLECTABLE_TYPE(W_JOLTPLUGIN_DLL, WOnJoltContact);

//////////////////////////////////////////////////////////////////////////

struct WJoltSettings
{
  WVec3 m_vObjectGravity = WVec3(0, 0, -9.81f);
  WVec3 m_vCharacterGravity = WVec3(0, 0, -12.0f);

  WEnum<WJoltSteppingMode> m_SteppingMode = WJoltSteppingMode::SemiFixed;
  float m_fFixedFrameRate = 60.0f;
  WUInt32 m_uiMaxSubSteps = 4;

  WUInt32 m_uiMaxBodies = 1000 * 10;

  float m_fSleepVelocityThreshold = 0.03f;
};

//////////////////////////////////////////////////////////////////////////

/// This message can be sent to a constraint component to break the constraint.
struct W_JOLTPLUGIN_DLL WJoltMsgDisconnectConstraints : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WJoltMsgDisconnectConstraints, WMessage);

  /// The actor that is being deleted. All constraints that are linked to it must be removed for Jolt not to crash.
  WJoltActorComponent* m_pActor = nullptr;

  /// The ID of the Jolt body that is being removed. If an actor were to have multiple bodies, this message may be sent multiple times.
  WUInt32 m_uiJoltBodyID = WInvalidIndex;
};
