#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

class WGameEngineTestApplication_Particles : public WGameEngineTestApplication
{
public:
  WGameEngineTestApplication_Particles();

  void SetupSceneSubTest(const char* szFile);
  void SetupParticleSubTest(const char* szFile);
  WTestAppRun ExecParticleSubTest(WInt32 iCurFrame);

  WUInt32 m_uiImageCompareThreshold = 110;
};

class WGameEngineTestParticles : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    // tests that reference particle assets directly
    BillboardRenderer,
    ColorGradientBehavior,
    FliesBehavior,
    GravityBehavior,
    LightRenderer,
    MeshRenderer,
    RaycastBehavior,
    SizeCurveBehavior,
    TrailRenderer,
    VelocityBehavior,
    EffectRenderer,
    BoxPositionInitializer,
    SpherePositionInitializer,
    CylinderPositionInitializer,
    RandomColorInitializer,
    RandomSizeInitializer,
    RotationSpeedInitializer,
    VelocityConeInitializer,
    BurstEmitter,
    ContinuousEmitter,
    OnEventEmitter,
    QuadRotatingOrtho,
    QuadFixedEmDir,
    QuadAxisEmDir,
    SphereBounds,
    Turbulence,
    Expression,

    // tests that use a dedicated scene
    Billboards,
    PullAlongBehavior,
    DistanceEmitter,
    SharedInstances,
    EventReactionEffect,
    LocalSpaceSim,
    Lighting,
    Attractors,
    Fog,
  };

  virtual void SetupSubTests() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;
  WUInt32 GetImageCompareThreshold(WInt32 iIdentifier);

  WInt32 m_iFrame = 0;
  WGameEngineTestApplication_Particles* m_pOwnApplication = nullptr;
};
