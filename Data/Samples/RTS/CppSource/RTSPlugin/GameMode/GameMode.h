#pragma once

class WWorld;
class WCamera;
class RTSGameState;
class WRmlUiContext;

struct RtsMouseInputState
{
  WVec2U32 m_MousePos;
  WVec2U32 m_MousePosLeftClick;
  WVec2U32 m_MousePosRightClick;
  WKeyState::Enum m_LeftClickState;
  WKeyState::Enum m_RightClickState;
  bool m_bLeftMouseMoved = false;
  bool m_bRightMouseMoved = false;

  static bool HasMouseMoved(WVec2U32 vStart, WVec2U32 vNow);
};

class RtsGameMode
{
public:
  RtsGameMode();
  virtual ~RtsGameMode();

  void ActivateMode(WWorld* pMainWorld, WViewHandle hView, WCamera* pMainCamera);
  void DeactivateMode();
  void ProcessInput(const RtsMouseInputState& mouseInput);
  void BeforeWorldUpdate();

  //////////////////////////////////////////////////////////////////////////
  // Game Mode Interface
public:
  virtual void AfterProcessInput() {}

protected:
  virtual void OnFirstActivation() {}
  virtual void OnActivateMode() {}
  virtual void OnDeactivateMode() {}
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput) {}
  virtual void OnBeforeWorldUpdate() {}

  RTSGameState* m_pGameState = nullptr;
  WWorld* m_pMainWorld = nullptr;
  WViewHandle m_hMainView;

private:
  bool m_bFirstActivation = true;

  //////////////////////////////////////////////////////////////////////////
  // Camera
protected:
  void DoDefaultCameraInput(const RtsMouseInputState& MouseInput);

  WCamera* m_pMainCamera = nullptr;

  //////////////////////////////////////////////////////////////////////////
  // User Interface
public:
  static WColor GetTeamColor(WUInt16 uiTeam);
  static WRmlUiContext* SetUiActive(WWorld* pWorld, WTempHashedString sName, bool bActive);

protected:
  void SetupSelectModeUI();

  WComponentHandle m_hSelectModeUIComponent;
};
