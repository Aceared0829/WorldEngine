#pragma once

#include <Mcp/McpTool.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Mutex.h>

/// Writing to and reading back this process's log.
///
/// Reading matters more than writing: it is the cheapest feedback loop an agent has. It can trigger
/// something and then look at what the application said about it, instead of asking the user to copy
/// the log.
///
/// Host independent, hence concrete and living in the Mcp library rather than in a plugin: a ring
/// buffer over WGlobalLog is the same thing in the editor and in a game. Each process captures its
/// own log, which is what makes it worth running a server in both - the editor cannot see what the
/// engine process logged, and vice versa.
class WMcpLogTool : public WMcpToolProvider
{
  W_ADD_DYNAMIC_REFLECTION(WMcpLogTool, WMcpToolProvider);

public:
  WMcpLogTool();
  ~WMcpLogTool();

  virtual void OnActivate() override;
  virtual void OnDeactivate() override;

  virtual void GetSupportedTools(WDynamicArray<WMcpToolDesc>& out_tools) const override;
  virtual void Execute(WStringView sToolName, const WVariantDictionary& arguments, WMcpToolResult& out_result) override;

private:
  struct Entry
  {
    WUInt64 m_uiId = 0;
    WLogMsgType::Enum m_Type = WLogMsgType::None;
    WString m_sText;
  };

  void LogEventHandler(const WLoggingEventData& e);

  void ExecuteWrite(const WVariantDictionary& arguments, WMcpToolResult& out_result);
  void ExecuteRead(const WVariantDictionary& arguments, WMcpToolResult& out_result);

  /// How many messages are kept. The editor logs a lot, and a game logs every frame, so this is a
  /// compromise between covering a whole operation and not holding on to megabytes of text.
  static constexpr WUInt32 s_uiMaxEntries = 2000;

  /// Log events arrive from whatever thread produced them, everything else runs on the main thread.
  mutable WMutex m_Mutex;
  WDeque<Entry> m_Entries;
  WEventSubscriptionID m_LogSubscription = 0;

  /// Ids are assigned in order and never reused, even once their entry falls out of m_Entries, so a
  /// 'sinceId' from an earlier read remains meaningful no matter how much has been logged since.
  WUInt64 m_uiNextId = 1;
};
