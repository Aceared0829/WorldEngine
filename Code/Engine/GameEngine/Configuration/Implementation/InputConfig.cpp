#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <GameEngine/Configuration/InputConfig.h>

static_assert(WGameAppInputConfig::MaxInputSlotAlternatives == WInputActionConfig::MaxInputSlotAlternatives, "Max values should be kept in sync");

WGameAppInputConfig::WGameAppInputConfig()
{
  for (WUInt16 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    m_fInputSlotScale[i] = 1.0f;
    m_sInputSlotTrigger[i] = WInputSlot_None;
  }
}

void WGameAppInputConfig::Apply() const
{
  WInputActionConfig cfg;
  cfg.m_bApplyTimeScaling = m_bApplyTimeScaling;

  for (WUInt32 i = 0; i < MaxInputSlotAlternatives; ++i)
  {
    cfg.m_sInputSlotTrigger[i] = m_sInputSlotTrigger[i];
    cfg.m_fInputSlotScale[i] = m_fInputSlotScale[i];
  }

  WInputManager::SetInputActionConfig(m_sInputSet, m_sInputAction, cfg, true);
}

void WGameAppInputConfig::WriteToDDL(WStreamWriter& inout_stream, const WArrayPtr<WGameAppInputConfig>& actions)
{
  WOpenDdlWriter writer;
  writer.SetCompactMode(false);
  writer.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);
  writer.SetPrimitiveTypeStringMode(WOpenDdlWriter::TypeStringMode::Compliant);
  writer.SetOutputStream(&inout_stream);

  for (const WGameAppInputConfig& config : actions)
  {
    config.WriteToDDL(writer);
  }
}

void WGameAppInputConfig::WriteToDDL(WOpenDdlWriter& ref_writer) const
{
  ref_writer.BeginObject("InputAction");
  {
    WOpenDdlUtils::StoreString(ref_writer, m_sInputSet, "Set");
    WOpenDdlUtils::StoreString(ref_writer, m_sInputAction, "Action");
    WOpenDdlUtils::StoreBool(ref_writer, m_bApplyTimeScaling, "TimeScale");

    for (int i = 0; i < 3; ++i)
    {
      if (!m_sInputSlotTrigger[i].IsEmpty())
      {
        ref_writer.BeginObject("Slot");
        {
          WOpenDdlUtils::StoreString(ref_writer, m_sInputSlotTrigger[i], "Key");
          WOpenDdlUtils::StoreFloat(ref_writer, m_fInputSlotScale[i], "Scale");
        }
        ref_writer.EndObject();
      }
    }
  }
  ref_writer.EndObject();
}

void WGameAppInputConfig::ReadFromDDL(WStreamReader& inout_stream, WDynamicArray<WGameAppInputConfig>& out_actions)
{
  WOpenDdlReader reader;

  if (reader.ParseDocument(inout_stream, 0, WLog::GetThreadLocalLogSystem()).Failed())
    return;

  const WOpenDdlReaderElement* pRoot = reader.GetRootElement();

  for (const WOpenDdlReaderElement* pAction = pRoot->GetFirstChild(); pAction != nullptr; pAction = pAction->GetSibling())
  {
    if (!pAction->IsCustomType("InputAction"))
      continue;

    WGameAppInputConfig& cfg = out_actions.ExpandAndGetRef();

    cfg.ReadFromDDL(pAction);
  }
}

void WGameAppInputConfig::ReadFromDDL(const WOpenDdlReaderElement* pInput)
{
  const WOpenDdlReaderElement* pSet = pInput->FindChildOfType(WOpenDdlPrimitiveType::String, "Set");
  const WOpenDdlReaderElement* pAction = pInput->FindChildOfType(WOpenDdlPrimitiveType::String, "Action");
  const WOpenDdlReaderElement* pTimeScale = pInput->FindChildOfType(WOpenDdlPrimitiveType::Bool, "TimeScale");


  if (pSet)
    m_sInputSet = pSet->GetPrimitivesString()[0];

  if (pAction)
    m_sInputAction = pAction->GetPrimitivesString()[0];

  if (pTimeScale)
    m_bApplyTimeScaling = pTimeScale->GetPrimitivesBool()[0];

  WInt32 iSlot = 0;
  for (const WOpenDdlReaderElement* pSlot = pInput->GetFirstChild(); pSlot != nullptr; pSlot = pSlot->GetSibling())
  {
    if (!pSlot->IsCustomType("Slot"))
      continue;

    const WOpenDdlReaderElement* pKey = pSlot->FindChildOfType(WOpenDdlPrimitiveType::String, "Key");
    const WOpenDdlReaderElement* pScale = pSlot->FindChildOfType(WOpenDdlPrimitiveType::Float, "Scale");

    if (pKey)
      m_sInputSlotTrigger[iSlot] = pKey->GetPrimitivesString()[0];

    if (pScale)
      m_fInputSlotScale[iSlot] = pScale->GetPrimitivesFloat()[0];

    ++iSlot;

    if (iSlot >= MaxInputSlotAlternatives)
      break;
  }
}

void WGameAppInputConfig::ApplyAll(const WArrayPtr<WGameAppInputConfig>& actions)
{
  for (const WGameAppInputConfig& config : actions)
  {
    config.Apply();
  }
}
