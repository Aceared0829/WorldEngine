#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/ComponentManager.h>

class WPhysicsWorldModuleInterface;

using WCreatureCrawlComponentManager = WComponentManagerSimple<class WCreatureCrawlComponent, WComponentUpdateType::WhenSimulating>;

struct WCreatureLeg
{
  WHashedString m_sLegObject;
  WUInt8 m_uiStepGroup = 0;

  WVec3 m_vRestPositionRelative;
  WGameObjectHandle m_hLegObject;
  WVec3 m_vCurTargetPosAbs;
  float m_fMoveLegFactor;
};

W_DECLARE_REFLECTABLE_TYPE(W_GAMECOMPONENTS_DLL, WCreatureLeg);

class W_GAMECOMPONENTS_DLL WCreatureCrawlComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WCreatureCrawlComponent, WComponent, WCreatureCrawlComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WCreatureCrawlComponent

protected:
  void Update();

  void OnSimulationStarted() override;

public:
  WCreatureCrawlComponent();
  ~WCreatureCrawlComponent();

  void SetBodyReference(const char* szReference); // [ property ]

  float m_fCastUp = 0.3f;
  float m_fCastDown = 1.0f;
  float m_fStepDistance = 0.4f;
  float m_fMinLegDistance = 0.5f;

protected:
  WGameObjectHandle m_hBody;             // [ property ]
  WHybridArray<WCreatureLeg, 4> m_Legs; // [ property ]

  const WPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  WTime m_LastMove;
  WQuat m_qBodyTilt = WQuat::MakeIdentity();

private:
  const char* DummyGetter() const { return nullptr; }
};
