#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <RendererCore/Components/CameraComponent.h>

class W_GAMECOMPONENTS_DLL WThirdPersonViewComponentManager : public WComponentManager<class WThirdPersonViewComponent, WBlockStorageType::Compact>
{
public:
  WThirdPersonViewComponentManager(WWorld* pWorld);
  ~WThirdPersonViewComponentManager();

  virtual void Initialize() override;

private:
  void Update(const WWorldModule::UpdateContext& context);

  friend class WThirdPersonViewComponent;
};

/// The third-person View component is used to place an object, typically a camera, relative to another object with clear line of sight.
///
/// The component will make the owner object look at the target point and place it at a certain distance.
/// When there are physical obstacles between the camera and the target, it moves the owner object closer.
class W_GAMECOMPONENTS_DLL WThirdPersonViewComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WThirdPersonViewComponent, WComponent, WThirdPersonViewComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WThirdPersonViewComponent

public:
  WThirdPersonViewComponent();
  ~WThirdPersonViewComponent();

  /// Changes the object that the view should focus on.
  void SetTargetObject(const char* szTargetObject);
  const char* GetTargetObject() const;

  /// Makes the camera rotate up or down by the given angle within the defined boundaries.
  void RotateUp(WAngle angle); // [ scriptable ]

protected:
  void Update();
  void OnSimulationStarted() override;

private:
  // properties
  WString m_sTargetObject;                               // [ property ]
  WVec3 m_vTargetOffsetHigh = WVec3::MakeZero();        // [ property ]
  WVec3 m_vTargetOffsetLow = WVec3::MakeZero();         // [ property ]
  float m_fMinDistance = 0.25f;                           // [ property ]
  float m_fMaxDistance = 3.0f;                            // [ property ]
  float m_fMaxDistanceUp = 3.0f;                          // [ property ]
  float m_fMaxDistanceDown = 1.0f;                        // [ property ]
  WAngle m_MinUpRotation = WAngle::MakeFromDegree(-70); // [ property ]
  WAngle m_MaxUpRotation = WAngle::MakeFromDegree(+80); // [ property ]
  WUInt8 m_uiCollisionLayer = 0;                         // [ property ]
  float m_fSweepWidth = 0.2f;                             // [ property ]
  float m_fZoomInSpeed = 10.0f;                           // [ property ]
  float m_fZoomOutSpeed = 1.0f;                           // [ property ]

  // runtime state
  WAngle m_RotateUp;
  WAngle m_CurUpRotation;
  WGameObjectHandle m_hTargetObject;
  float m_fCurDistance = 1.0f;
};
