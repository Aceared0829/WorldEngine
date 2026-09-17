#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

class WOpenDdlWriter;
class WOpenDdlReaderElement;

class W_GAMEENGINE_DLL WGameAppInputConfig
{
public:
  constexpr static WUInt32 MaxInputSlotAlternatives = 3;

  static constexpr const WStringView s_sConfigFile = ":project/RuntimeConfigs/InputConfig.ddl"_wsv;

  WGameAppInputConfig();

  void Apply() const;
  void WriteToDDL(WOpenDdlWriter& ref_writer) const;
  void ReadFromDDL(const WOpenDdlReaderElement* pAction);

  static void ApplyAll(const WArrayPtr<WGameAppInputConfig>& actions);
  static void WriteToDDL(WStreamWriter& inout_stream, const WArrayPtr<WGameAppInputConfig>& actions);
  static void ReadFromDDL(WStreamReader& inout_stream, WDynamicArray<WGameAppInputConfig>& out_actions);

  WString m_sInputSet;
  WString m_sInputAction;

  WString m_sInputSlotTrigger[MaxInputSlotAlternatives];

  float m_fInputSlotScale[MaxInputSlotAlternatives];

  bool m_bApplyTimeScaling = true;
};
