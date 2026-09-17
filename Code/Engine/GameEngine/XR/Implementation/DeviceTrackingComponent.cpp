#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/DeviceTrackingComponent.h>
#include <GameEngine/XR/StageSpaceComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WXRPoseLocation, 1)
W_BITFLAGS_CONSTANTS(WXRPoseLocation::Grip, WXRPoseLocation::Aim)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_COMPONENT_TYPE(WDeviceTrackingComponent, 3, WComponentMode::Dynamic)
{
  W_BEGIN_PROPERTIES
  {
    W_ENUM_ACCESSOR_PROPERTY("DeviceType", WXRDeviceType, GetDeviceType, SetDeviceType),
    W_ENUM_ACCESSOR_PROPERTY("PoseLocation", WXRPoseLocation, GetPoseLocation, SetPoseLocation),
    W_ENUM_ACCESSOR_PROPERTY("TransformSpace", WXRTransformSpace, GetTransformSpace, SetTransformSpace),
    W_MEMBER_PROPERTY("Rotation", m_bRotation)->AddAttributes(new WDefaultValueAttribute(true)),
    W_MEMBER_PROPERTY("Scale", m_bScale)->AddAttributes(new WDefaultValueAttribute(true)),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("XR"),
    new WInDevelopmentAttribute(WInDevelopmentAttribute::Phase::Alpha),
  }
  W_END_ATTRIBUTES;
}
W_END_COMPONENT_TYPE
// clang-format on

WDeviceTrackingComponent::WDeviceTrackingComponent() = default;
WDeviceTrackingComponent::~WDeviceTrackingComponent() = default;

void WDeviceTrackingComponent::SetDeviceType(WEnum<WXRDeviceType> type)
{
  m_DeviceType = type;
}

WEnum<WXRDeviceType> WDeviceTrackingComponent::GetDeviceType() const
{
  return m_DeviceType;
}

void WDeviceTrackingComponent::SetPoseLocation(WEnum<WXRPoseLocation> poseLocation)
{
  m_PoseLocation = poseLocation;
}

WEnum<WXRPoseLocation> WDeviceTrackingComponent::GetPoseLocation() const
{
  return m_PoseLocation;
}

void WDeviceTrackingComponent::SetTransformSpace(WEnum<WXRTransformSpace> space)
{
  m_Space = space;
}

WEnum<WXRTransformSpace> WDeviceTrackingComponent::GetTransformSpace() const
{
  return m_Space;
}

void WDeviceTrackingComponent::SerializeComponent(WWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  WStreamWriter& s = stream.GetStream();

  s << m_DeviceType;
  s << m_PoseLocation;
  s << m_Space;
  s << m_bRotation;
  s << m_bScale;
}

void WDeviceTrackingComponent::DeserializeComponent(WWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const WUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  WStreamReader& s = stream.GetStream();

  s >> m_DeviceType;
  if (uiVersion >= 2)
  {
    s >> m_PoseLocation;
  }
  s >> m_Space;
  if (uiVersion >= 3)
  {
    s >> m_bRotation;
    s >> m_bScale;
  }
}

void WDeviceTrackingComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (WXRInterface* pXRInterface = WSingletonRegistry::GetSingletonInstance<WXRInterface>())
  {
    if (!pXRInterface->IsInitialized())
      return;

    WXRDeviceID deviceID = pXRInterface->GetXRInput().GetDeviceIDByType(m_DeviceType);
    if (deviceID != -1)
    {
      const WXRDeviceState& state = pXRInterface->GetXRInput().GetDeviceState(deviceID);
      WVec3 vPosition;
      WQuat qRotation;
      if (m_PoseLocation == WXRPoseLocation::Grip && state.m_bGripPoseIsValid)
      {
        vPosition = state.m_vGripPosition;
        qRotation = state.m_qGripRotation;
      }
      else if (m_PoseLocation == WXRPoseLocation::Aim && state.m_bAimPoseIsValid)
      {
        vPosition = state.m_vAimPosition;
        qRotation = state.m_qAimRotation;
      }
      else
      {
        return;
      }
      if (m_Space == WXRTransformSpace::Local)
      {
        GetOwner()->SetLocalPosition(vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(qRotation);
      }
      else
      {
        WTransform add;
        add.SetIdentity();
        if (const WStageSpaceComponentManager* pStageMan = GetWorld()->GetComponentManager<WStageSpaceComponentManager>())
        {
          if (const WStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
          {
            add = pStage->GetOwner()->GetGlobalTransform();
          }
        }

        const WTransform global(add * WTransform(vPosition, qRotation));
        WTransform local;
        if (GetOwner()->GetParent() != nullptr)
        {
          local = WTransform::MakeLocalTransform(GetOwner()->GetParent()->GetGlobalTransform(), global);
        }
        else
        {
          local = global;
        }
        GetOwner()->SetLocalPosition(local.m_vPosition);
        if (m_bRotation)
          GetOwner()->SetLocalRotation(local.m_qRotation);
        if (m_bScale)
          GetOwner()->SetLocalScaling(local.m_vScale);
      }
    }
  }
}

W_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_DeviceTrackingComponent);
