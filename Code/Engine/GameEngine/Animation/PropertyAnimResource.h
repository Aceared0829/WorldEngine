#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Tracks/ColorGradient.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/EventTrack.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/GameEngineDLL.h>

/// What data type an animation modifies.
struct W_GAMEENGINE_DLL WPropertyAnimTarget
{
  using StorageType = WUInt8;

  enum Enum
  {
    Number,    ///< A single value.
    VectorX,   ///< The x coordinate of a vector.
    VectorY,   ///< The y coordinate of a vector.
    VectorZ,   ///< The z coordinate of a vector.
    VectorW,   ///< The w coordinate of a vector.
    RotationX, ///< The x coordinate of a rotation.
    RotationY, ///< The y coordinate of a rotation.
    RotationZ, ///< The z coordinate of a rotation.
    Color,     ///< A color.

    Default = Number,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WPropertyAnimTarget);

//////////////////////////////////////////////////////////////////////////

/// Describes how an animation should be played back.
struct W_GAMEENGINE_DLL WPropertyAnimMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Once,         ///< Play the animation once from start to end and then stop.
    Loop,         ///< Play the animation from start to end, then loop back to the start and repeat indefinitely.
    BackAndForth, ///< Play the animation from start to end, then reverse direction and play from end to start, then repeat indefinitely.

    Default = Loop,
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMEENGINE_DLL, WPropertyAnimMode);

//////////////////////////////////////////////////////////////////////////

struct W_GAMEENGINE_DLL WPropertyAnimEntry
{
  WString m_sObjectSearchSequence; ///< Sequence of named objects to search for the target
  WString m_sComponentType;        ///< Empty to reference the game object properties (position etc.)
  WString m_sPropertyPath;
  WEnum<WPropertyAnimTarget> m_Target;
  const WRTTI* m_pComponentRtti = nullptr;
};

struct W_GAMEENGINE_DLL WFloatPropertyAnimEntry : public WPropertyAnimEntry
{
  WCurve1D m_Curve;
};

struct W_GAMEENGINE_DLL WColorPropertyAnimEntry : public WPropertyAnimEntry
{
  WColorGradient m_Gradient;
};

//////////////////////////////////////////////////////////////////////////

// this class is actually ref counted and used with WSharedPtr to allow to work on the same data, even when the resource was reloaded
struct W_GAMEENGINE_DLL WPropertyAnimResourceDescriptor : public WRefCounted
{
  WTime m_AnimationDuration;
  WDynamicArray<WFloatPropertyAnimEntry> m_FloatAnimations;
  WDynamicArray<WColorPropertyAnimEntry> m_ColorAnimations;
  WEventTrack m_EventTrack;

  void Save(WStreamWriter& inout_stream) const;
  void Load(WStreamReader& inout_stream);
};

//////////////////////////////////////////////////////////////////////////

using WPropertyAnimResourceHandle = WTypedResourceHandle<class WPropertyAnimResource>;

class W_GAMEENGINE_DLL WPropertyAnimResource : public WResource
{
  W_ADD_DYNAMIC_REFLECTION(WPropertyAnimResource, WResource);

  W_RESOURCE_DECLARE_COMMON_CODE(WPropertyAnimResource);
  W_RESOURCE_DECLARE_CREATEABLE(WPropertyAnimResource, WPropertyAnimResourceDescriptor);

public:
  WPropertyAnimResource();

  WSharedPtr<WPropertyAnimResourceDescriptor> GetDescriptor() const { return m_pDescriptor; }

private:
  virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  WSharedPtr<WPropertyAnimResourceDescriptor> m_pDescriptor;
};
