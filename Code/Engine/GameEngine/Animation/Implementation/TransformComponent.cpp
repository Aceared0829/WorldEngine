#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/TransformComponent.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WTransformComponent, 3, WRTTINoAllocator)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    W_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new WDefaultValueAttribute(true)), // Whether the animation should start right away.
    W_ACCESSOR_PROPERTY("ReverseAtEnd", GetReverseAtEnd, SetReverseAtEnd)->AddAttributes(new WDefaultValueAttribute(true)), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    W_ACCESSOR_PROPERTY("ReverseAtStart", GetReverseAtStart, SetReverseAtStart)->AddAttributes(new WDefaultValueAttribute(true)), // If true, it will not stop at the end, but turn around and continue.
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Animation"),
  }
  W_END_ATTRIBUTES;
  W_BEGIN_FUNCTIONS
  {
    W_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    W_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    W_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WTransformComponent::SerializeComponent(WWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream() << m_Flags.GetValue();
  inout_stream.GetStream() << m_AnimationTime;
  inout_stream.GetStream() << m_fAnimationSpeed;
}


void WTransformComponent::DeserializeComponent(WWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  WTransformComponentFlags::StorageType flags;
  inout_stream.GetStream() >> flags;
  m_Flags.SetValue(flags);

  inout_stream.GetStream() >> m_AnimationTime;
  inout_stream.GetStream() >> m_fAnimationSpeed;
}

void WTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_AnimationTime = WTime::MakeZero();
  m_Flags.AddOrRemove(WTransformComponentFlags::CurrentlyRunning, m_Flags.IsSet(WTransformComponentFlags::Running));
  m_Flags.Remove(WTransformComponentFlags::AnimationReversed);
}

bool WTransformComponent::IsRunning(void) const
{
  return m_Flags.IsAnySet(WTransformComponentFlags::CurrentlyRunning);
}

void WTransformComponent::SetRunning(bool b)
{
  m_Flags.AddOrRemove(WTransformComponentFlags::Running, b);
  m_Flags.AddOrRemove(WTransformComponentFlags::CurrentlyRunning, b);
}

bool WTransformComponent::GetReverseAtStart(void) const
{
  return (m_Flags.IsAnySet(WTransformComponentFlags::AutoReturnStart));
}

void WTransformComponent::SetReverseAtStart(bool b)
{
  m_Flags.AddOrRemove(WTransformComponentFlags::AutoReturnStart, b);
}

bool WTransformComponent::GetReverseAtEnd(void) const
{
  return (m_Flags.IsAnySet(WTransformComponentFlags::AutoReturnEnd));
}

void WTransformComponent::SetReverseAtEnd(bool b)
{
  m_Flags.AddOrRemove(WTransformComponentFlags::AutoReturnEnd, b);
}

WTransformComponent::WTransformComponent() = default;
WTransformComponent::~WTransformComponent() = default;

void WTransformComponent::SetDirectionForwards(bool bForwards)
{
  m_Flags.AddOrRemove(WTransformComponentFlags::AnimationReversed, !bForwards);
}

void WTransformComponent::ToggleDirection()
{
  m_Flags.AddOrRemove(WTransformComponentFlags::AnimationReversed, !m_Flags.IsAnySet(WTransformComponentFlags::AnimationReversed));
}

bool WTransformComponent::IsDirectionForwards() const
{
  return !m_Flags.IsAnySet(WTransformComponentFlags::AnimationReversed);
}

/*! Distance should be given in meters, but can be anything else, too. E.g. "angles" or "radians". All other values need to use the same
units. For example, when distance is given in angles, acceleration has to be in "angles per square seconds". Deceleration can be positive or
negative, internally the absolute value is used. Distance, acceleration, max velocity and time need to be positive. Time is expected to be
"in seconds". The returned value is 0, if time is negative. It is clamped to fDistanceInMeters, if time is too big.
*/

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, WTime& ref_timeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
  {
    const float fDist = fMaxVelocity * (float)ref_timeSinceStartInSec.GetSeconds();

    if (fDist > fDistanceInMeters)
    {
      ref_timeSinceStartInSec = WTime::MakeFromSeconds(fDistanceInMeters / fMaxVelocity);
      return fDistanceInMeters;
    }

    return WMath::Max(0.0f, fDist);
  }

  // do some sanity-checks
  if ((ref_timeSinceStartInSec.GetSeconds() <= 0.0) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
    return 0.0f;

  // calculate the duration and distance of accelerated movement
  double fAccTime = 0.0;
  if (fAcceleration > 0.0)
    fAccTime = fMaxVelocity / fAcceleration;
  double fAccDist = fMaxVelocity * fAccTime * 0.5;

  // calculate the duration and distance of decelerated movement
  double fDecTime = 0.0f;
  if (fDeceleration > 0.0f)
    fDecTime = fMaxVelocity / fDeceleration;
  double fDecDist = fMaxVelocity * fDecTime * 0.5f;

  // if acceleration and deceleration take longer, than the whole path is long
  if (fAccDist + fDecDist > fDistanceInMeters)
  {
    double fFactor = fDistanceInMeters / (fAccDist + fDecDist);

    // shorten the acceleration path
    if (fAcceleration > 0.0f)
    {
      fAccDist *= fFactor;
      fAccTime = WMath::Sqrt(2 * fAccDist / fAcceleration);
    }

    // shorten the deceleration path
    if (fDeceleration > 0.0f)
    {
      fDecDist *= fFactor;
      fDecTime = WMath::Sqrt(2 * fDecDist / fDeceleration);
    }
  }

  // if the time is still within the acceleration phase, return accelerated distance
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime)
    return static_cast<float>(0.5 * fAcceleration * WMath::Square(ref_timeSinceStartInSec.GetSeconds()));

  // calculate duration and length of the path, that has maximum velocity
  const double fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const double fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return static_cast<float>(fAccDist + (ref_timeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity);

  // if the time is, however, outside the whole path, just return the upper end
  if (ref_timeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    ref_timeSinceStartInSec = WTime::MakeFromSeconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const double fDecTime2 = ref_timeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return static_cast<float>(fDistanceInMeters - 0.5 * fDeceleration * WMath::Square(fDecTime - fDecTime2));
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class WTransformComponentPatch_1_2 : public WGraphPatch
{
public:
  WTransformComponentPatch_1_2()
    : WGraphPatch("WTransformComponent", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Run at Startup", "RunAtStartup");
    pNode->RenameProperty("Reverse at Start", "ReverseAtStart");
    pNode->RenameProperty("Reverse at End", "ReverseAtEnd");
  }
};

WTransformComponentPatch_1_2 g_WTransformComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class WTransformComponentPatch_2_3 : public WGraphPatch
{
public:
  WTransformComponentPatch_2_3()
    : WGraphPatch("WTransformComponent", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* pGraph, WAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("RunAtStartup", "Running");
  }
};

WTransformComponentPatch_2_3 g_WTransformComponentPatch_2_3;

W_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_TransformComponent);
