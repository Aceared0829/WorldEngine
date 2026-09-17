#pragma once

#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <TestFramework/Framework/TestBaseClass.h>
#include <Texture/Image/Image.h>

class WGameEngineTestGameState : public WGameState
{
  W_ADD_DYNAMIC_REFLECTION(WGameEngineTestGameState, WGameState);

public:
  virtual void ProcessInput() override;
  virtual void ConfigureInputActions() override;
};

class WGameEngineTestApplication : public WGameApplication
{
public:
  using SUPER = WGameApplication;

  WGameEngineTestApplication(const char* szProjectDirName);

  virtual WString FindProjectDirectory() const final override;
  virtual WString GetProjectDataDirectoryPath() const final override;
  const WImage& GetLastScreenshot() { return m_LastScreenshot; }
  /// Camera must have a global key named "Camera4" if `uiCameraNumber` was 4.
  void SwitchToCamera(WUInt32 uiCameraNumber);

  WResult LoadScene(const char* szSceneFile);
  WWorld* GetWorld() const { return m_pWorld.Borrow(); }

protected:
  virtual WResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeHighLevelSystemsShutdown() override;
  virtual void StoreScreenshot(WImage&& image, WStringView sContext) override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual WUniquePtr<WGameStateBase> CreateGameState() override;

  WString m_sProjectDirName;
  WUniquePtr<WWorld> m_pWorld;
  WImage m_LastScreenshot;
};

class WGameEngineTest : public WTestBaseClass
{
  using SUPER = WTestBaseClass;

public:
  WGameEngineTest();
  ~WGameEngineTest();

  virtual WResult GetImage(WImage& ref_img, const WSubTestEntry& subTest, WUInt32 uiImageNumber) override;
  virtual WGameEngineTestApplication* CreateApplication() = 0;

protected:
  virtual WResult InitializeTest() override;
  virtual WResult DeInitializeTest() override;
  virtual WResult InitializeSubTest(WInt32 iIdentifier) override;

  WGameEngineTestApplication* m_pApplication = nullptr;
};
