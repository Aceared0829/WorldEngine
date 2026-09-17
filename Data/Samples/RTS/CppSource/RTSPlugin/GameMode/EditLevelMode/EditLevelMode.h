#pragma once

#include <RTSPlugin/GameMode/GameMode.h>

class WBlackboard;

class RtsEditLevelMode : public RtsGameMode
{
public:
  RtsEditLevelMode();
  ~RtsEditLevelMode();

protected:
  virtual void OnFirstActivation() override;
  virtual void OnActivateMode() override;
  virtual void OnDeactivateMode() override;
  virtual void OnProcessInput(const RtsMouseInputState& MouseInput, bool bUiWantsInput) override;
  virtual void OnBeforeWorldUpdate() override;

  //////////////////////////////////////////////////////////////////////////

private:
  void SetupEditUI();

  WComponentHandle m_hEditUIComponent;

  WSharedPtr<WBlackboard> m_pBlackboard;
};
