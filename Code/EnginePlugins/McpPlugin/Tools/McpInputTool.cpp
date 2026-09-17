#include <McpPlugin/McpPluginPCH.h>

#include <McpPlugin/McpEngineHost.h>
#include <McpPlugin/McpInputDevice.h>
#include <McpPlugin/Tools/McpInputTool.h>

#include <Core/Input/InputManager.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMcpInputTool, 1, WRTTIDefaultAllocator<WMcpInputTool>)
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WMcpInputTool::WMcpInputTool() = default;
WMcpInputTool::~WMcpInputTool() = default;

void WMcpInputTool::OnActivate()
{
  m_pDevice = W_DEFAULT_NEW(WMcpInputDevice);
}

void WMcpInputTool::OnDeactivate()
{
  // Destroying the device unregisters it, which also releases anything it was still holding down - a
  // key left pressed by a plugin that is going away would otherwise stay pressed forever.
  m_pDevice = nullptr;
}

void WMcpInputTool::GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const
{
  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "input_slots";
    desc.m_sDescription =
      "Lists the input slots this process knows about - the names input_set writes to. Call it before guessing a name: "
      "the spelling is 'keyboard_w', 'mouse_button_0', 'mouse_move_pos_x', and which slots exist at all depends on "
      "which input devices the platform registered.\n"
      "'flags' says what kind of thing a slot is, which decides what value makes sense: a button is 0 or 1, an analog "
      "axis is anything in between, and a relative slot like mouse movement is a per-frame delta rather than a "
      "position. 'value' is what the slot currently reads, merged across all devices including this one.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"contains":{"type":"string","description":"Only return slots whose name or display name contains this text, case insensitive. Omit for all of them."},"activeOnly":{"type":"boolean","description":"Only return slots whose current value is not zero. Useful to find out what is being pressed right now. Default false."}}})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "input_set";
    desc.m_sDescription =
      "Presses keys and moves the mouse, by writing input slot values that the game reads as if they came from real "
      "hardware. Get the slot names from input_slots.\n"
      "Values are merged with the real keyboard and mouse rather than replacing them, taking whichever is larger, so "
      "this never locks a human out of the process.\n"
      "'frames' is how long to hold the value. Omit it or pass 0 to hold indefinitely, which is what to use to press a "
      "key now and release it later with a second call setting the same slot to 0 - or with 'clearAll'. A key held for "
      "0 frames would never be seen, since the game reads input once per frame.\n"
      "The game only consumes input while it renders frames, so follow this with game_wait to let those frames happen; "
      "returning from this call only means the value is set, not that anything has reacted to it.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"slots":{"type":"object","description":"Slot name to value, e.g. {\"keyboard_w\":1.0,\"mouse_move_pos_x\":0.01}. A value of 0 stops writing that slot.","additionalProperties":{"type":"number"}},"frames":{"type":"number","description":"Hold the values for this many frames, then release them. 0 or omitted means until changed."},"clearAll":{"type":"boolean","description":"Release everything this tool is currently holding, before applying 'slots'. Pass it on its own to let go of everything."}}})";
  }

  {
    WMcpToolDesc& desc = out_tools.ExpandAndGetRef();
    desc.m_sName = "input_sequence";
    desc.m_sDescription =
      "Runs a chain of input steps in order, in a single call, instead of one input_set/game_wait round trip "
      "per step. Each step is exactly one of:\n"
      "  {\"set\": {...}} - writes slot values, same as input_set's 'slots' (merged with the real hardware, "
      "held until changed or cleared).\n"
      "  {\"wait\": {\"frames\": n}} - lets the game run for n frames before the next step, same as game_wait.\n"
      "  {\"text\": \"...\"} - queues typed text, delivered as one batch the next time the game reads input - "
      "not one character per frame, so it costs no extra frames by itself.\n"
      "  {\"clear\": true} - releases every slot this tool is currently holding.\n"
      "A chord (several keys in the same frame) is one 'set' step naming all of them, not several steps. "
      "The call returns once every step has run; a 'wait' step is the only thing that actually spends frames, "
      "so a sequence with none of those returns immediately, same as input_set.";

    desc.m_sInputSchema = R"({"type":"object","properties":{"steps":{"type":"array","description":"Executed in order. See the tool description for the four step shapes.","items":{"type":"object"}},"timeout":{"type":"number","description":"Give up on any individual 'wait' step after this many seconds. Default 30."}},"required":["steps"]})";
  }
}

void WMcpInputTool::Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (sToolName == "input_slots")
  {
    ExecuteSlots(arguments, out_result);
  }
  else if (sToolName == "input_set")
  {
    ExecuteSet(arguments, out_result);
  }
  else if (sToolName == "input_sequence")
  {
    ExecuteSequence(arguments, out_result);
  }
}

namespace
{
  /// The flags of one slot, as the names a caller would recognise.
  ///
  /// Only the ones that change how a value should be chosen. The rest describe implementation details of
  /// the hardware and would just be noise in every listing.
  void WriteSlotFlags(WMcpJsonWriter& ref_writer, WBitflags<WInputSlotFlags> flags)
  {
    ref_writer.BeginArray("flags");

    if (flags.IsSet(WInputSlotFlags::ValueBinaryZeroOrOne))
      ref_writer.WriteString("binary");
    if (flags.IsSet(WInputSlotFlags::ValueRangeZeroToOne))
      ref_writer.WriteString("analog");
    if (flags.IsSet(WInputSlotFlags::ReportsRelativeValues))
      ref_writer.WriteString("relative");
    if (flags.IsSet(WInputSlotFlags::Pressable))
      ref_writer.WriteString("pressable");
    if (flags.IsSet(WInputSlotFlags::Holdable))
      ref_writer.WriteString("holdable");

    ref_writer.EndArray();
  }

  /// What the device is currently holding, shared between input_set's and input_sequence's result.
  void WriteHeldSlots(WMcpJsonWriter& ref_writer, const WDynamicArray<WMcpInputDevice::HeldSlot>& held)
  {
    ref_writer.BeginArray("holding");

    for (const WMcpInputDevice::HeldSlot& slot : held)
    {
      ref_writer.BeginObject();
      ref_writer.AddVariableString("name", slot.m_sSlot);
      ref_writer.AddVariableFloat("value", slot.m_fValue);

      if (slot.m_uiFramesRemaining > 0)
      {
        ref_writer.AddVariableUInt32("framesRemaining", slot.m_uiFramesRemaining);
      }

      ref_writer.EndObject();
    }

    ref_writer.EndArray();
  }
} // namespace

void WMcpInputTool::ExecuteSlots(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  const WStringView sContains = WMcpJson::GetString(arguments, "contains");
  const bool bActiveOnly = WMcpJson::GetBool(arguments, "activeOnly", false);

  WDynamicArray<WStringView> slots;
  WInputManager::RetrieveAllKnownInputSlots(slots);

  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.BeginArray("slots");

  WUInt32 uiMatches = 0;
  WUInt32 uiReturned = 0;

  for (WStringView sSlot : slots)
  {
    const WStringView sDisplayName = WInputManager::GetInputSlotDisplayName(sSlot);

    if (!sContains.IsEmpty())
    {
      WStringBuilder sName = sSlot;
      WStringBuilder sDisplay = sDisplayName;

      if (sName.FindSubString_NoCase(sContains) == nullptr && sDisplay.FindSubString_NoCase(sContains) == nullptr)
        continue;
    }

    float fValue = 0.0f;
    WInputManager::GetInputSlotState(sSlot, &fValue);

    if (bActiveOnly && fValue == 0.0f)
      continue;

    ++uiMatches;

    if (uiReturned >= s_uiMaxResults)
      continue;

    writer.BeginObject();
    writer.AddVariableString("name", sSlot);

    // The display name is what a human calls the key. It differs from the slot name often enough to be
    // worth reporting - 'keyboard_left_ctrl' displays as 'Left Ctrl' - and it is also localised.
    if (!sDisplayName.IsEmpty() && sDisplayName != sSlot)
    {
      writer.AddVariableString("displayName", sDisplayName);
    }

    writer.AddVariableFloat("value", fValue);
    WriteSlotFlags(writer, WInputManager::GetInputSlotFlags(sSlot));
    writer.EndObject();

    ++uiReturned;
  }

  writer.EndArray();

  writer.AddVariableUInt32("returned", uiReturned);
  writer.AddVariableUInt32("totalMatches", uiMatches);
  writer.AddVariableBool("truncated", uiMatches > uiReturned);

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

void WMcpInputTool::ExecuteSet(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (m_pDevice == nullptr)
  {
    out_result.SetError("The synthetic input device does not exist, so no input can be written.");
    return;
  }

  const bool bClearAll = WMcpJson::GetBool(arguments, "clearAll", false);

  if (bClearAll)
  {
    m_pDevice->ClearAllSlots();
  }

  const WUInt32 uiFrames = static_cast<WUInt32>(WMath::Max<WInt64>(0, WMcpJson::GetInt(arguments, "frames", 0)));

  const WVariantDictionary* pSlots = WMcpJson::GetDict(arguments, "slots");

  if (pSlots == nullptr && !bClearAll)
  {
    out_result.SetError("Nothing to do: pass 'slots' with the values to write, or 'clearAll' to release everything.");
    return;
  }

  WUInt32 uiSet = 0;

  if (pSlots != nullptr)
  {
    if (!ApplySlots(*pSlots, uiFrames, out_result))
      return;

    uiSet = pSlots->GetCount();
  }

  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableUInt32("slotsWritten", uiSet);

  // What is being held afterwards, so that a caller never has to remember what it pressed. This is also
  // the answer to 'did my earlier call already expire'.
  WDynamicArray<WMcpInputDevice::HeldSlot> held;
  m_pDevice->GetHeldSlots(held);
  WriteHeldSlots(writer, held);

  writer.AddVariableString("note",
    "The game reads input once per frame. Call game_wait to let frames run before checking what happened.");

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}

bool WMcpInputTool::ApplySlots(const WVariantDictionary& slots, WUInt32 uiFrames, WMcpToolResult& out_result)
{
  for (auto it = slots.GetIterator(); it.IsValid(); ++it)
  {
    // Any convertible type, because a client sends 1, 1.0 and "1" for the same thing.
    WResult conversion = W_SUCCESS;
    const float fValue = it.Value().ConvertTo<float>(&conversion);

    if (conversion.Failed())
    {
      WStringBuilder sError;
      sError.SetFormat("The value for slot '{}' is not a number.", it.Key());
      out_result.SetError(sError);
      return false;
    }

    if (fValue == 0.0f)
    {
      m_pDevice->ClearSlot(it.Key());
    }
    else
    {
      m_pDevice->SetSlotValue(it.Key(), fValue, uiFrames);
    }
  }

  return true;
}

void WMcpInputTool::ExecuteSequence(const WVariantDictionary& arguments, WMcpToolResult& out_result)
{
  if (m_pDevice == nullptr)
  {
    out_result.SetError("The synthetic input device does not exist, so no input can be written.");
    return;
  }

  const WVariantArray* pSteps = WMcpJson::GetArray(arguments, "steps");

  if (pSteps == nullptr || pSteps->IsEmpty())
  {
    out_result.SetError("'steps' must be a non-empty array of step objects.");
    return;
  }

  if (pSteps->GetCount() > s_uiMaxSteps)
  {
    WStringBuilder sError;
    sError.SetFormat("'steps' has {} entries, which is more than the limit of {}.", pSteps->GetCount(), s_uiMaxSteps);
    out_result.SetError(sError);
    return;
  }

  const WInt64 iTimeout = WMcpJson::GetInt(arguments, "timeout", static_cast<WInt64>(s_DefaultWaitTimeout.GetSeconds()));
  const WTime waitTimeout = iTimeout > 0 ? WTime::MakeFromSeconds(static_cast<double>(iTimeout)) : s_DefaultWaitTimeout;

  if (!m_bSequenceActive)
  {
    // First entry. Later ones are re-entries from a 'wait' step's deferral, with the same 'arguments'
    // - see WMcpToolResult::m_bNotFinished - so steps before m_uiSequenceStep must not run again.
    m_bSequenceActive = true;
    m_uiSequenceStep = 0;
  }
  else if (m_bSequenceWaiting)
  {
    const WUInt64 uiNow = WMcpEngineHost::GetFrameCount();
    const bool bDone = uiNow >= m_uiSequenceWaitUntilFrame;
    const bool bTimedOut = !bDone && (WTime::Now() - m_SequenceWaitStarted >= m_SequenceWaitTimeout);

    if (!bDone && !bTimedOut)
    {
      out_result.m_bNotFinished = true;
      return;
    }

    m_bSequenceWaiting = false;

    if (bTimedOut)
    {
      WStringBuilder sError;
      sError.SetFormat("Sequence timed out on step {} (a 'wait') after {} seconds. The game is not producing "
                       "frames fast enough, or not at all - see game_wait's error for the usual cause.",
        m_uiSequenceStep, (WTime::Now() - m_SequenceWaitStarted).GetSeconds());
      out_result.SetError(sError);
      m_bSequenceActive = false;
      return;
    }

    // The 'wait' step that was in flight is now done; move on to whatever follows it.
    ++m_uiSequenceStep;
  }

  for (; m_uiSequenceStep < pSteps->GetCount(); ++m_uiSequenceStep)
  {
    const WVariant& step = (*pSteps)[m_uiSequenceStep];

    if (!step.IsA<WVariantDictionary>())
    {
      WStringBuilder sError;
      sError.SetFormat("Step {} is not an object.", m_uiSequenceStep);
      out_result.SetError(sError);
      m_bSequenceActive = false;
      return;
    }

    const WVariantDictionary& stepDict = step.Get<WVariantDictionary>();

    if (const WVariantDictionary* pSet = WMcpJson::GetDict(stepDict, "set"))
    {
      if (!ApplySlots(*pSet, 0, out_result))
      {
        m_bSequenceActive = false;
        return;
      }
    }
    else if (WMcpJson::GetBool(stepDict, "clear", false))
    {
      m_pDevice->ClearAllSlots();
    }
    else
    {
      const WVariant* pText = nullptr;
      const WVariantDictionary* pWait = WMcpJson::GetDict(stepDict, "wait");

      if (pWait != nullptr)
      {
        const WInt64 iFrames = WMcpJson::GetInt(*pWait, "frames", 1);

        if (iFrames < 1 || iFrames > s_uiMaxWaitFrames)
        {
          WStringBuilder sError;
          sError.SetFormat("Step {}: 'frames' must be between 1 and {}, not {}.", m_uiSequenceStep, s_uiMaxWaitFrames, iFrames);
          out_result.SetError(sError);
          m_bSequenceActive = false;
          return;
        }

        m_uiSequenceWaitUntilFrame = WMcpEngineHost::GetFrameCount() + static_cast<WUInt64>(iFrames);
        m_SequenceWaitStarted = WTime::Now();
        m_SequenceWaitTimeout = waitTimeout;
        m_bSequenceWaiting = true;

        out_result.m_bNotFinished = true;
        return;
      }
      else if (stepDict.TryGetValue("text", pText) && pText->CanConvertTo<WString>())
      {
        m_pDevice->QueueText(pText->ConvertTo<WString>());
      }
      else
      {
        WStringBuilder sError;
        sError.SetFormat("Step {} is none of 'set', 'wait', 'text' or 'clear'.", m_uiSequenceStep);
        out_result.SetError(sError);
        m_bSequenceActive = false;
        return;
      }
    }
  }

  m_bSequenceActive = false;

  WMcpJsonWriter writer;
  writer.BeginObject();
  writer.AddVariableUInt32("stepsRun", pSteps->GetCount());
  writer.AddVariableUInt64("frameCount", WMcpEngineHost::GetFrameCount());

  WDynamicArray<WMcpInputDevice::HeldSlot> held;
  m_pDevice->GetHeldSlots(held);
  WriteHeldSlots(writer, held);

  writer.EndObject();
  out_result.m_sText = writer.GetResult();
}
