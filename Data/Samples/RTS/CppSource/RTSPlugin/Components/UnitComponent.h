#pragma once

#include <RTSPlugin/AI/AiUtilitySystem.h>
#include <RTSPlugin/Components/ComponentMessages.h>

class RtsUnitComponentManager : public WComponentManager<class RtsUnitComponent, WBlockStorageType::FreeList>
{
public:
  RtsUnitComponentManager(WWorld* pWorld);

  virtual void Initialize() override;

  void UnitUpdate(const WWorldModule::UpdateContext& context);
};

enum class RtsUnitMode
{
  GuardLocation,
  MoveToPosition,
  AttackUnit,
};

class RtsUnitComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(RtsUnitComponent, WComponent, RtsUnitComponentManager);

public:
  RtsUnitComponent();
  ~RtsUnitComponent();

  //////////////////////////////////////////////////////////////////////////
  // WComponent interface

  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  WUInt16 m_uiMaxHealth = 100;
  WUInt16 m_uiCurHealth = 0;

private:
  WPrefabResourceHandle m_hOnDestroyedPrefab;

  //////////////////////////////////////////////////////////////////////////
  // Message Handlers
  void OnMsgAssignPosition(RtsMsgAssignPosition& msg);
  void OnMsgSetTarget(RtsMsgSetTarget& msg);
  void OnMsgApplyDamage(RtsMsgApplyDamage& msg);
  void OnMsgGatherUnitStats(RtsMsgGatherUnitStats& msg);
  void OnMsgArrivedAtLocation(RtsMsgArrivedAtLocation& msg);

public:
  //////////////////////////////////////////////////////////////////////////
  //

  WGameObject* FindClosestEnemy(float fMaxRadius) const;
  void FireAt(WGameObjectHandle hUnit);
  WGameObject* AttackClosestEnemey(float fSearchRadius, float fIgnoreRadius);

protected:
  virtual void OnUnitDestroyed();

  friend class RtsAttackUnitAiUtility;
  friend class RtsGuardLocationAiUtility;
  friend class RtsMoveToPositionAiUtility;

  bool m_bModeChanged = true;
  RtsUnitMode m_UnitMode;

  WVec2 m_vAssignedPosition;

  WGameObjectHandle m_hAssignedUnitToAttack;
  WGameObjectHandle m_hCurrentUnitToAttack;

  WTime m_TimeLastShot;
  WUniquePtr<RtsAiUtilitySystem> m_pAiSystem; // has to be a pointer because RtsAiUtilitySystem isn't copyable

  void UpdateUnit();
};
