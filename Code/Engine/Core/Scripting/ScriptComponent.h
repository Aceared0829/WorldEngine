#pragma once

#include <Core/Messages/EventMessageSender.h>
#include <Core/Scripting/ScriptClassResource.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Foundation/Types/RangeView.h>

using WScriptComponentManager = WComponentManager<class WScriptComponent, WBlockStorageType::FreeList>;

/// Component that hosts and executes a script class instance on a game object.
///
/// Manages script execution lifecycle, variable access, parameter exposure, and event handling.
/// Supports configurable update intervals and simulation-only updates. Provides integration
/// between game objects and scripting systems through the WScriptClassResource.
class W_CORE_DLL WScriptComponent : public WEventMessageHandlerComponent
{
  W_DECLARE_COMPONENT_TYPE(WScriptComponent, WEventMessageHandlerComponent, WScriptComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // WComponent

protected:
  virtual void SerializeComponent(WWorldWriter& stream) const override;
  virtual void DeserializeComponent(WWorldReader& stream) override;
  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // WScriptComponent
public:
  WScriptComponent();
  ~WScriptComponent();

  void SetScriptVariable(const WHashedString& sName, const WVariant& value);         // [ scriptable ]
  WVariant GetScriptVariable(const WHashedString& sName) const;                      // [ scriptable ]

  void SetScriptClass(const WScriptClassResourceHandle& hScript);                     // [ property ]
  const WScriptClassResourceHandle& GetScriptClass() const { return m_hScriptClass; } // [ property ]

  void SetUpdateInterval(WTime interval);                                             // [ property ]
  WTime GetUpdateInterval() const { return m_UpdateInterval; }                        // [ property ]

  void SetUpdateOnlyWhenSimulating(bool bUpdate);                                      // [ property ]
  bool GetUpdateOnlyWhenSimulating() const { return m_bUpdateOnlyWhenSimulating; }     // [ property ]

  void BroadcastEventMsg(WMessage& ref_msg);

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
  const WRangeView<const char*, WUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const WVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, WVariant& out_value) const;

  W_ALWAYS_INLINE WScriptInstance* GetScriptInstance() { return m_pInstance.Borrow(); }

private:
  void InstantiateScript(bool bActivate);
  void ClearInstance(bool bDeactivate);
  void AddUpdateFunctionToSchedule();
  void RemoveUpdateFunctionToSchedule();

  const WAbstractFunctionProperty* GetScriptFunction(WUInt32 uiFunctionIndex);
  void CallScriptFunction(WUInt32 uiFunctionIndex);

  void ReloadScript();

  WArrayMap<WHashedString, WVariant> m_Parameters;

  WScriptClassResourceHandle m_hScriptClass;
  WTime m_UpdateInterval = WTime::MakeZero();
  bool m_bUpdateOnlyWhenSimulating = true;

  WSharedPtr<WScriptRTTI> m_pScriptType;
  WUniquePtr<WScriptInstance> m_pInstance;

private:
  struct EventSender
  {
    const WRTTI* m_pMsgType = nullptr;
    WEventMessageSender<WMessage> m_Sender;
  };

  WSmallArray<EventSender, 1> m_EventSenders;
};
