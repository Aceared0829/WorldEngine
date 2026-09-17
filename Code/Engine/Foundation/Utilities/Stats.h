#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Variant.h>

/// This class holds a simple map that maps strings (keys) to strings (values), which represent certain stats.
///
/// This can be used by a game to store (and continuously update) information about the internal game state. Other tools can then
/// display this information in a convenient manner. For example the stats can be shown on screen. The data is also transmitted through
/// WTelemetry, and the WInspector tool will display the information.
class W_FOUNDATION_DLL WStats
{
public:
  using MapType = WMap<WString, WVariant>;

  /// Removes the stat with the given name.
  ///
  /// This will also send a 'remove' message through WTelemetry, such that external tools can remove it from their list.
  static void RemoveStat(WStringView sStatName);

  /// Sets the value of the given stat, adds it if it did not exist before.
  ///
  /// szStatName may contain slashes (but not backslashes) to define groups and subgroups, which can be used by tools such as WInspector
  /// to display the stats in a hierarchical way.
  /// This function will also send the name and value of the stat through WTelemetry, such that tools like WInspector will show the
  /// changed value.
  static void SetStat(WStringView sStatName, const WVariant& value);

  /// Returns the value of the given stat. Returns an invalid WVariant, if the stat did not exist before.
  static const WVariant& GetStat(WStringView sStatName) { return s_Stats[sStatName]; }

  /// Returns the entire map of stats, can be used to display them.
  static const MapType& GetAllStats() { return s_Stats; }

  /// The event data that is broadcast whenever a stat is changed.
  struct StatsEventData
  {
    /// Which type of event this is.
    enum EventType
    {
      Add,   ///< A variable has been set for the first time.
      Set,   ///< A variable has been changed.
      Remove ///< A variable that existed has been removed.
    };

    EventType m_EventType;
    WStringView m_sStatName;
    WVariant m_NewStatValue;
  };

  using WEventStats = WEvent<const StatsEventData&, WMutex>;

  /// Adds an event handler that is called every time a stat is changed.
  static void AddEventHandler(WEventStats::Handler handler) { s_StatsEvents.AddEventHandler(handler); }

  /// Removes a previously added event handler.
  static void RemoveEventHandler(WEventStats::Handler handler) { s_StatsEvents.RemoveEventHandler(handler); }

private:
  static WMutex s_Mutex;
  static MapType s_Stats;
  static WEventStats s_StatsEvents;
};
