#pragma once

#include <Foundation/Strings/HashedString.h>
#include <Foundation/Time/Clock.h>
#include <Foundation/Types/SharedPtr.h>
#include <Foundation/Types/UniquePtr.h>

#include <Core/Utils/Blackboard.h>
#include <Core/World/CoordinateSystem.h>
#include <Core/World/SpatialSystem.h>

/// Describes the initial state of a world.
struct WWorldDesc
{
  W_DECLARE_POD_TYPE();

  WWorldDesc(WStringView sWorldName) { m_sName.Assign(sWorldName); }

  WHashedString m_sName;                                                         ///< Name of the world for identification
  WUInt64 m_uiRandomNumberGeneratorSeed = 0;                                     ///< Seed for the world's random number generator (0 = use current time)

  WUniquePtr<WSpatialSystem> m_pSpatialSystem;                                  ///< Custom spatial system to use for this world
  bool m_bAutoCreateSpatialSystem = true;                                         ///< Automatically create a default spatial system if none is set

  bool m_bReportErrorWhenStaticObjectMoves = true;                                ///< Whether to log errors when objects marked as static change position

  WSharedPtr<WCoordinateSystemProvider> m_pCoordinateSystemProvider;            ///< Optional provider for position-dependent coordinate systems
  WUniquePtr<WTimeStepSmoothing> m_pTimeStepSmoothing;                          ///< Custom time step smoothing (if nullptr, WDefaultTimeStepSmoothing will be used)
  WSharedPtr<WBlackboard> m_pBlackboard;                                        ///< Custom blackboard to use for this world (if nullptr, a new blackboard will be created)

  WTime m_MaxComponentInitializationTimePerFrame = WTime::MakeFromHours(10000); ///< Maximum time to spend on component initialization per frame
};
