#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamIterator.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Math/Float16.h>
#include <ParticlePlugin/Effect/ParticleEffectInstance.h>
#include <ParticlePlugin/Streams/DefaultParticleStreams.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

//////////////////////////////////////////////////////////////////////////
// ZERO-INIT STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_ZeroInit, 1, WRTTIDefaultAllocator<WParticleStream_ZeroInit>)
W_END_DYNAMIC_REFLECTED_TYPE;



//////////////////////////////////////////////////////////////////////////
// POSITION STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_Position, 1, WRTTIDefaultAllocator<WParticleStreamFactory_Position>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_Position, 1, WRTTIDefaultAllocator<WParticleStream_Position>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_Position::WParticleStreamFactory_Position()
  : WParticleStreamFactory("Position", WProcessingStream::DataType::Float4, WGetStaticRTTI<WParticleStream_Position>())
{
}

void WParticleStream_Position::Initialize(WParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void WParticleStream_Position::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WVec4> itData(m_pStream, uiNumElements, uiStartIndex);

  const WVec4 defValue = m_pOwner->GetTransform().m_vPosition.GetAsVec4(0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// SIZE STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_Size, 1, WRTTIDefaultAllocator<WParticleStreamFactory_Size>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_Size, 1, WRTTIDefaultAllocator<WParticleStream_Size>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_Size::WParticleStreamFactory_Size()
  : WParticleStreamFactory("Size", WProcessingStream::DataType::Half, WGetStaticRTTI<WParticleStream_Size>())
{
}

void WParticleStream_Size::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WFloat16> itData(m_pStream, uiNumElements, uiStartIndex);

  const float defValue = 1.0f;
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// COLOR STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_Color, 1, WRTTIDefaultAllocator<WParticleStreamFactory_Color>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_Color, 1, WRTTIDefaultAllocator<WParticleStream_Color>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_Color::WParticleStreamFactory_Color()
  : WParticleStreamFactory("Color", WProcessingStream::DataType::Half4, WGetStaticRTTI<WParticleStream_Color>())
{
}

void WParticleStream_Color::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WColorLinear16f> itData(m_pStream, uiNumElements, uiStartIndex);

  const WColorLinear16f defValue(1.0f, 1.0f, 1.0f, 1.0f);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// VELOCITY STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_Velocity, 1, WRTTIDefaultAllocator<WParticleStreamFactory_Velocity>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_Velocity, 1, WRTTIDefaultAllocator<WParticleStream_Velocity>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_Velocity::WParticleStreamFactory_Velocity()
  : WParticleStreamFactory("Velocity", WProcessingStream::DataType::Half4, WGetStaticRTTI<WParticleStream_Velocity>())
{
}

void WParticleStream_Velocity::Initialize(WParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void WParticleStream_Velocity::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WFloat16Vec4> itData(m_pStream, uiNumElements, uiStartIndex);

  const WVec3 startVel = m_pOwner->GetParticleStartVelocity();
  const float fSpeed = startVel.GetLength();
  const WVec3 dir = fSpeed > 0.0f ? startVel / fSpeed : WVec3(0, 0, 1);

  while (!itData.HasReachedEnd())
  {
    itData.Current() = WVec4(dir.x, dir.y, dir.z, fSpeed);
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// LAST POSITION STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_LastPosition, 1, WRTTIDefaultAllocator<WParticleStreamFactory_LastPosition>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_LastPosition::WParticleStreamFactory_LastPosition()
  : WParticleStreamFactory("LastPosition", WProcessingStream::DataType::Float3, WGetStaticRTTI<WParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION SPEED STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_RotationSpeed, 1, WRTTIDefaultAllocator<WParticleStreamFactory_RotationSpeed>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_RotationSpeed::WParticleStreamFactory_RotationSpeed()
  : WParticleStreamFactory("RotationSpeed", WProcessingStream::DataType::Half, WGetStaticRTTI<WParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ROTATION OFFSET STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_RotationOffset, 1, WRTTIDefaultAllocator<WParticleStreamFactory_RotationOffset>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_RotationOffset::WParticleStreamFactory_RotationOffset()
  : WParticleStreamFactory("RotationOffset", WProcessingStream::DataType::Half, WGetStaticRTTI<WParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// EFFECT ID STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_EffectID, 1, WRTTIDefaultAllocator<WParticleStreamFactory_EffectID>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_EffectID::WParticleStreamFactory_EffectID()
  : WParticleStreamFactory("EffectID", WProcessingStream::DataType::Int, WGetStaticRTTI<WParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// ON OFF STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_OnOff, 1, WRTTIDefaultAllocator<WParticleStreamFactory_OnOff>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_OnOff::WParticleStreamFactory_OnOff()
  : WParticleStreamFactory("OnOff", WProcessingStream::DataType::Byte, WGetStaticRTTI<WParticleStream_ZeroInit>())
{
}

//////////////////////////////////////////////////////////////////////////
// AXIS STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_Axis, 1, WRTTIDefaultAllocator<WParticleStreamFactory_Axis>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_Axis, 1, WRTTIDefaultAllocator<WParticleStream_Axis>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_Axis::WParticleStreamFactory_Axis()
  : WParticleStreamFactory("Axis", WProcessingStream::DataType::Float3, WGetStaticRTTI<WParticleStream_Axis>())
{
}

void WParticleStream_Axis::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WVec3> itData(m_pStream, uiNumElements, uiStartIndex);

  const WVec3 defValue(1, 0, 0);
  while (!itData.HasReachedEnd())
  {
    itData.Current() = defValue;
    itData.Advance();
  }
}

//////////////////////////////////////////////////////////////////////////
// TRAIL DATA STREAM
//////////////////////////////////////////////////////////////////////////

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_TrailData, 1, WRTTIDefaultAllocator<WParticleStreamFactory_TrailData>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_TrailData::WParticleStreamFactory_TrailData()
  : WParticleStreamFactory("TrailData", WProcessingStream::DataType::Short2, WGetStaticRTTI<WParticleStream_ZeroInit>())
{
}


//////////////////////////////////////////////////////////////////////////
// VARIATION STREAM
//////////////////////////////////////////////////////////////////////////


W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStreamFactory_Variation, 1, WRTTIDefaultAllocator<WParticleStreamFactory_Variation>)
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WParticleStream_Variation, 1, WRTTIDefaultAllocator<WParticleStream_Variation>)
W_END_DYNAMIC_REFLECTED_TYPE;

WParticleStreamFactory_Variation::WParticleStreamFactory_Variation()
  : WParticleStreamFactory("Variation", WProcessingStream::DataType::Int, WGetStaticRTTI<WParticleStream_Variation>())
{
}

void WParticleStream_Variation::Initialize(WParticleSystemInstance* pOwner)
{
  m_pOwner = pOwner;
}

void WParticleStream_Variation::InitializeElements(WUInt64 uiStartIndex, WUInt64 uiNumElements)
{
  WProcessingStreamIterator<WUInt32> itData(m_pStream, uiNumElements, uiStartIndex);

  WRandom& rng = m_pOwner->GetOwnerEffect()->GetRNG();

  while (!itData.HasReachedEnd())
  {
    itData.Current() = rng.UInt();
    itData.Advance();
  }
}



W_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Streams_DefaultParticleStreams);
