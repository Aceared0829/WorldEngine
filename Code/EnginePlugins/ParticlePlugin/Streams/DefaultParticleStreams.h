#pragma once

#include <ParticlePlugin/Streams/ParticleStream.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

/// Stream that initializes particle data to zero.
///
/// Uses the base class implementation which zero-fills all elements.
/// This stream type is used for data that should start at zero without custom initialization.
class W_PARTICLEPLUGIN_DLL WParticleStream_ZeroInit final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_ZeroInit, WParticleStream);

protected:
  // base class implementation already zero fills the stream data
  // virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating position streams (Float4 data type).
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_Position final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_Position, WParticleStreamFactory);

public:
  WParticleStreamFactory_Position();
};

/// Stream storing particle positions.
///
/// Initializes new particles at the particle system's transform position.
/// Stores positions as WVec4 (Float4 stream type).
class W_PARTICLEPLUGIN_DLL WParticleStream_Position final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_Position, WParticleStream);

protected:
  virtual void Initialize(WParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating size streams (Half data type).
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_Size final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_Size, WParticleStreamFactory);

public:
  WParticleStreamFactory_Size();
};

/// Stream storing particle sizes.
///
/// Initializes new particles with size 1.0.
/// Uses half-precision floats to reduce memory usage.
class W_PARTICLEPLUGIN_DLL WParticleStream_Size final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_Size, WParticleStream);

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating color streams (Half4 data type).
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_Color final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_Color, WParticleStreamFactory);

public:
  WParticleStreamFactory_Color();
};

/// Stream storing particle colors.
///
/// Initializes new particles with white color (1, 1, 1, 1).
/// Uses half-precision floats (WColorLinear16f) to reduce memory usage.
class W_PARTICLEPLUGIN_DLL WParticleStream_Color final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_Color, WParticleStream);

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating velocity streams (Half4 data type).
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_Velocity final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_Velocity, WParticleStreamFactory);

public:
  WParticleStreamFactory_Velocity();
};

/// Stream storing particle velocities.
///
/// Initializes new particles with the particle system's start velocity.
/// Stores velocity as direction (xyz) and speed (w) in an WVec4 using half-precision floats.
/// If the start velocity is zero, defaults to direction (0, 0, 1) with speed 0.
class W_PARTICLEPLUGIN_DLL WParticleStream_Velocity final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_Velocity, WParticleStream);

protected:
  virtual void Initialize(WParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WParticleSystemInstance* m_pOwner;
};

//////////////////////////////////////////////////////////////////////////
// LIFETIME STREAM
//////////////////////////////////////////////////////////////////////////

// always default initialized by the behavior

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating last position streams (Float3 data type).
///
/// Used for trail rendering and motion blur effects to track the previous frame's particle positions.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_LastPosition final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_LastPosition, WParticleStreamFactory);

public:
  WParticleStreamFactory_LastPosition();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating rotation speed streams (Half data type).
///
/// Stores the angular velocity for rotating billboard particles.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_RotationSpeed final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_RotationSpeed, WParticleStreamFactory);

public:
  WParticleStreamFactory_RotationSpeed();
};

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating rotation offset streams (Half data type).
///
/// Stores the initial rotation angle offset for particles.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_RotationOffset final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_RotationOffset, WParticleStreamFactory);

public:
  WParticleStreamFactory_RotationOffset();
};

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating effect ID streams (Int data type).
///
/// Used to track which effect instance spawned a particle, useful for event reactions and debugging.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_EffectID final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_EffectID, WParticleStreamFactory);

public:
  WParticleStreamFactory_EffectID();
};

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating on/off streams (Byte data type).
///
/// Used to enable or disable individual particles without removing them from the system.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_OnOff final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_OnOff, WParticleStreamFactory);

public:
  WParticleStreamFactory_OnOff();
};

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating axis streams (Float3 data type).
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_Axis final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_Axis, WParticleStreamFactory);

public:
  WParticleStreamFactory_Axis();
};

/// Stream storing particle orientation axes.
///
/// Initializes new particles with axis (1, 0, 0).
/// Used for oriented particle rendering where particles need a direction vector.
class W_PARTICLEPLUGIN_DLL WParticleStream_Axis final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_Axis, WParticleStream);

protected:
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;
};

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating trail data streams (Short2 data type).
///
/// Stores trail-specific data for trail renderers to connect particles into ribbons.
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_TrailData final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_TrailData, WParticleStreamFactory);

public:
  WParticleStreamFactory_TrailData();
};

//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////

/// Factory for creating variation streams (Int data type).
class W_PARTICLEPLUGIN_DLL WParticleStreamFactory_Variation final : public WParticleStreamFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStreamFactory_Variation, WParticleStreamFactory);

public:
  WParticleStreamFactory_Variation();
};

/// Stream storing particle variation values.
///
/// Initializes new particles with random unsigned integers.
/// Used for texture atlas variations, flipbook animations, or other per-particle randomization.
class W_PARTICLEPLUGIN_DLL WParticleStream_Variation final : public WParticleStream
{
  W_ADD_DYNAMIC_REFLECTION(WParticleStream_Variation, WParticleStream);

protected:
  virtual void Initialize(WParticleSystemInstance* pOwner) override;
  virtual void InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements) override;

  WParticleSystemInstance* m_pOwner;
};
