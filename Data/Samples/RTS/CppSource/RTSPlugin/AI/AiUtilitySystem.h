#pragma once

#include <RTSPlugin/RTSPluginDLL.h>

class RtsAiUtility;

class RtsAiUtilitySystem
{
public:
  RtsAiUtilitySystem();
  ~RtsAiUtilitySystem();

  void AddUtility(WUniquePtr<RtsAiUtility>&& pUtility);

  void Reevaluate(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now, WTime frequency);
  bool Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now);

private:
  WTime m_LastUpdate;
  RtsAiUtility* m_pActiveUtility = nullptr;
  WHybridArray<WUniquePtr<RtsAiUtility>, 8> m_Utilities;
};

class RtsAiUtility
{
public:
  RtsAiUtility();
  virtual ~RtsAiUtility();

  virtual void Activate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) = 0;
  virtual void Deactivate(WGameObject* pOwnerObject, WComponent* pOwnerComponent) = 0;
  virtual void Execute(WGameObject* pOwnerObject, WComponent* pOwnerComponent, WTime now) = 0;
  virtual double ComputePriority(WGameObject* pOwnerObject, WComponent* pOwnerComponent) const = 0;
};

class RtsUnitComponentUtility : public RtsAiUtility
{
public:
};
