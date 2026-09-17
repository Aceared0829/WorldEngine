#pragma once

#include <GameEngineTest/GameEngineTestPCH.h>

#include "../TestClass/TestClass.h"

/// Verifies that SerializeComponent() and DeserializeComponent() of every known component type
/// agree about the amount of data they write and read.
///
/// Runs inside a game application because creating a component of an arbitrary type also creates its
/// component manager, and some managers allocate GPU resources, which requires a graphics device.
class WGameEngineTestComponentSerialization : public WGameEngineTest
{
  using SUPER = WGameEngineTest;

public:
  virtual const char* GetTestName() const override;
  virtual WGameEngineTestApplication* CreateApplication() override;

private:
  enum SubTests
  {
    SerializeDeserializeRoundtrip,
  };

  virtual void SetupSubTests() override;
  virtual WTestAppRun RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount) override;

  WGameEngineTestApplication* m_pOwnApplication = nullptr;
};
