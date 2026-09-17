#pragma once

#include <GameComponentsPlugin/GameComponentsDLL.h>

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>

using WLineToComponentManager = WComponentManagerSimple<class WLineToComponent, WComponentUpdateType::Always, WBlockStorageType::FreeList, WWorldUpdatePhase::PostTransform>;

/// Draws a line from its own position to the target object position
class W_GAMECOMPONENTS_DLL WLineToComponent : public WComponent
{
  W_DECLARE_COMPONENT_TYPE(WLineToComponent, WComponent, WLineToComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

public:
  virtual void SerializeComponent(WWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(WWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // WLineToComponent

public:
  WLineToComponent();
  ~WLineToComponent();

  const char* GetLineToTargetGuid() const;                                      // [ property ]
  void SetLineToTargetGuid(const char* szTargetGuid);                           // [ property ]

  void SetLineToTarget(const WGameObjectHandle& hTargetObject);                // [ property ]
  const WGameObjectHandle& GetLineToTarget() const { return m_hTargetObject; } // [ property ]

  WColor m_LineColor;                                                          // [ property ]

  void Update();

protected:
  WGameObjectHandle m_hTargetObject;
};
