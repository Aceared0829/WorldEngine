#pragma once

#include <Core/Scripting/ScriptClassResource.h>
#include <Foundation/Types/RangeView.h>
#include <GameEngine/StateMachine/StateMachine.h>

/// A state machine state implementation that can be scripted using e.g. visual scripting.
class W_GAMEENGINE_DLL WStateMachineState_Script : public WStateMachineState
{
  W_ADD_DYNAMIC_REFLECTION(WStateMachineState_Script, WStateMachineState);

public:
  WStateMachineState_Script(WStringView sName = WStringView());
  ~WStateMachineState_Script();

  virtual void OnEnter(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pFromState) const override;
  virtual void OnExit(WStateMachineInstance& ref_instance, void* pInstanceData, const WStateMachineState* pToState) const override;
  virtual void Update(WStateMachineInstance& ref_instance, void* pInstanceData, WTime deltaTime) const override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  virtual bool GetInstanceDataDesc(WInstanceDataDesc& out_desc) override;

  void SetScriptClassFile(const char* szFile); // [ property ]
  const char* GetScriptClassFile() const;      // [ property ]

  // Exposed Parameters
  const WRangeView<const char*, WUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const WVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, WVariant& out_value) const;

private:
  WArrayMap<WHashedString, WVariant> m_Parameters;

  WString m_sScriptClassFile;
};
