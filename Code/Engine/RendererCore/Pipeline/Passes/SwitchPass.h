#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>

/// Forwards one of several inputs to its output, depending on a value read from a blackboard.
///
/// The pin names are the string representations of the entries in m_Values, so changing the values invalidates existing connections. Only the branch that feeds the selected input is kept alive, everything that solely feeds the other branches is culled from the pipeline. Changing the selected value therefore requires a rebuild of the pass graph.
class W_RENDERERCORE_DLL WSwitchBasePass : public WRenderPipelinePass
{
  W_ADD_DYNAMIC_REFLECTION(WSwitchBasePass, WRenderPipelinePass);

public:
  /// Maximum number of inputs. Also limits how many entries m_Values may have.
  static constexpr WUInt32 s_uiMaxInputs = 8;

  /// Selects the input whose entry in m_Values equals iValue. Falls back to the first entry if the value is unknown.
  ///
  /// \return Whether the selection actually changed. If it did, the pass graph has to be rebuilt.
  bool SetSwitchValue(WInt32 iValue);

  virtual WStatus AddRenderPasses(const WViewData& viewData, const WCamera& camera, WRenderGraph& ref_graph, const WArrayPtr<const WRenderPipelinePinConnection> inputs, WArrayPtr<WRenderPipelinePinConnection> outputs) override;

  virtual WResult Serialize(WStreamWriter& inout_stream) const override;
  virtual WResult Deserialize(WStreamReader& inout_stream) override;

  WString m_sBlackboardProperty;     ///< Name of the blackboard entry that selects the active input. If it is empty or missing, the switch uses its first value.
  WDynamicArray<WInt32> m_Values;   ///< The value that selects each input pin. Must be unique and must not hold more than s_uiMaxInputs entries.
  WUInt8 m_uiSelectedValueIndex = 0; ///< Index into m_Values, and therefore also the index of the active input pin.

protected:
  WSwitchBasePass(const char* szName);

  void AddDynamicInputPins(WArrayPtr<const WRenderPipelineNodePin* const> pins, WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin);
};

/// WSwitchBasePass for texture connections.
class W_RENDERERCORE_DLL WTextureSwitchPass : public WSwitchBasePass
{
  W_ADD_DYNAMIC_REFLECTION(WTextureSwitchPass, WSwitchBasePass);

public:
  WTextureSwitchPass();
  virtual void AddDynamicPins(WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin) override;

  WRenderPipelineNodeOutputPin m_Output;
  WRenderPipelineNodeInputPin m_Input0;
  WRenderPipelineNodeInputPin m_Input1;
  WRenderPipelineNodeInputPin m_Input2;
  WRenderPipelineNodeInputPin m_Input3;
  WRenderPipelineNodeInputPin m_Input4;
  WRenderPipelineNodeInputPin m_Input5;
  WRenderPipelineNodeInputPin m_Input6;
  WRenderPipelineNodeInputPin m_Input7;
};

/// WSwitchBasePass for buffer connections.
class W_RENDERERCORE_DLL WBufferSwitchPass : public WSwitchBasePass
{
  W_ADD_DYNAMIC_REFLECTION(WBufferSwitchPass, WSwitchBasePass);

public:
  WBufferSwitchPass();
  virtual void AddDynamicPins(WHashTable<WHashedString, const WRenderPipelineNodePin*>& ref_nameToPin) override;

  WRenderPipelineNodeBufferOutputPin m_Output;
  WRenderPipelineNodeBufferInputPin m_Input0;
  WRenderPipelineNodeBufferInputPin m_Input1;
  WRenderPipelineNodeBufferInputPin m_Input2;
  WRenderPipelineNodeBufferInputPin m_Input3;
  WRenderPipelineNodeBufferInputPin m_Input4;
  WRenderPipelineNodeBufferInputPin m_Input5;
  WRenderPipelineNodeBufferInputPin m_Input6;
  WRenderPipelineNodeBufferInputPin m_Input7;
};
