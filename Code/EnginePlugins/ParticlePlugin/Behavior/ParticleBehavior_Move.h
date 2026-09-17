#pragma once

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class WPhysicsWorldModuleInterface;

/// How Move behavior changes movement speed
struct W_PARTICLEPLUGIN_DLL WMovementMode
{
  using StorageType = WUInt8;

  enum Enum
  {
    Constant,    ///< Constant movement speed
    CustomCurve, ///< Use embedded curve data
    SharedCurve, ///< Reference shared curve resource

    Default = Constant
  };
};

W_DECLARE_REFLECTABLE_TYPE(W_PARTICLEPLUGIN_DLL, WMovementMode);

/// Behavior that moves particles along world axes
///
/// Moves particles at constant speed along the X, Y, or Z axes.
/// Movement direction is in world space.
class W_PARTICLEPLUGIN_DLL WParticleBehaviorFactory_Move final : public WParticleBehaviorFactory
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehaviorFactory_Move, WParticleBehaviorFactory);

public:
  WParticleBehaviorFactory_Move();
  ~WParticleBehaviorFactory_Move();

  virtual const WRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(WParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(WStreamWriter& inout_stream) const override;
  virtual void Load(WStreamReader& inout_stream, const WParticleEffectDescriptor& ownerEffectDescriptor, const WParticleSystemDescriptor& ownerSystemDescriptor) override;

  WEnum<WMovementMode> m_MoveX_Mode;
  float m_fMoveX_Speed = 0.0f;
  WSingleCurveData m_MoveX_Curve;
  WCurve1DResourceHandle m_hMoveX_SharedCurve;
  float m_fMoveX_CurveOffset = 0.0f;
  float m_fMoveX_CurveScale = 1.0f;
  mutable WCurve1D m_RuntimeMoveX_Curve;

  WEnum<WMovementMode> m_MoveY_Mode;
  float m_fMoveY_Speed = 0.0f;
  WSingleCurveData m_MoveY_Curve;
  WCurve1DResourceHandle m_hMoveY_SharedCurve;
  float m_fMoveY_CurveOffset = 0.0f;
  float m_fMoveY_CurveScale = 1.0f;
  mutable WCurve1D m_RuntimeMoveY_Curve;

  WEnum<WMovementMode> m_MoveZ_Mode;
  float m_fMoveZ_Speed = 0.0f;
  WSingleCurveData m_MoveZ_Curve;
  WCurve1DResourceHandle m_hMoveZ_SharedCurve;
  float m_fMoveZ_CurveOffset = 0.0f;
  float m_fMoveZ_CurveScale = 1.0f;
  mutable WCurve1D m_RuntimeMoveZ_Curve;
};


class W_PARTICLEPLUGIN_DLL WParticleBehavior_Move final : public WParticleBehavior
{
  W_ADD_DYNAMIC_REFLECTION(WParticleBehavior_Move, WParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  WEnum<WMovementMode> m_MoveX_Mode;
  const WCurve1D* m_pMoveX_Curve = nullptr;
  float m_fMoveX_CurveOffset = 0;
  float m_fMoveX_CurveScale = 1;
  float m_fMoveX_Speed = 0.0f;

  WEnum<WMovementMode> m_MoveY_Mode;
  const WCurve1D* m_pMoveY_Curve = nullptr;
  float m_fMoveY_CurveOffset = 0;
  float m_fMoveY_CurveScale = 1;
  float m_fMoveY_Speed = 0.0f;

  WEnum<WMovementMode> m_MoveZ_Mode;
  const WCurve1D* m_pMoveZ_Curve = nullptr;
  float m_fMoveZ_CurveOffset = 0;
  float m_fMoveZ_CurveScale = 1;
  float m_fMoveZ_Speed = 0.0f;

protected:
  friend class WParticleBehaviorFactory_Move;

  virtual void Process(WUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(WParticleWorldModule* pParticleModule) override;

  // used to get the gravity vector for Z-axis direction
  WPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

  WProcessingStream* m_pStreamPosition = nullptr;
  WProcessingStream* m_pStreamLifeTime = nullptr;
};
