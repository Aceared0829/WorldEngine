#pragma once

#include <Foundation/Containers/Deque.h>
#include <Foundation/Math/Math.h>
#include <Utilities/PathFinding/PathState.h>
#include <Utilities/UtilitiesDLL.h>

/// Implements a directed breadth-first search through a graph (A*).
///
/// You can search for a path to a specific location using FindPath() or to the closest node that fulfills some arbitrary criteria
/// using FindClosest().
///
/// PathStateType must be derived from WPathState and can be used for keeping track of certain state along a path and to modify
/// the path search dynamically.
template <typename PathStateType>
class WPathSearch
{
public:
  /// Used by FindClosest() to query whether the currently visited node fulfills the termination criteria.
  using IsSearchedObjectCallback = bool (*)(WInt64 iCurrentNodeIndex, const PathStateType& CurrentState, WPathStateGenerator<PathStateType>* pGenerator);

  /// FindPath() and FindClosest() return an array of these objects as the path result.
  struct PathResultData
  {
    W_DECLARE_POD_TYPE();

    /// The index of the node that was visited.
    WInt64 m_iNodeIndex;

    /// Pointer to the path state that was active at that step along the path.
    const PathStateType* m_pPathState;
  };

  /// Sets the WPathStateGenerator that should be used by this WPathSearch object.
  void SetPathStateGenerator(WPathStateGenerator<PathStateType>* pStateGenerator) { m_pStateGenerator = pStateGenerator; }

  /// Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when the graph node \a iTargetNodeIndex was reached.
  ///
  /// Returns W_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  WResult FindPath(WInt64 iStartNodeIndex, const PathStateType& StartState, WInt64 iTargetNodeIndex, WDeque<PathResultData>& out_Path,
    float fMaxPathCost = WMath::Infinity<float>());

  /// Searches for a path that starts at the graph node \a iStartNodeIndex with the start state \a StartState and shall terminate
  /// when a graph node is reached for which \a Callback return true.
  ///
  /// Returns W_FAILURE if no path could be found.
  /// Returns the path result as a list of PathResultData objects in \a out_Path.
  ///
  /// The path search is stopped (and thus fails) if the path reaches costs of \a fMaxPathCost or higher.
  WResult FindClosest(WInt64 iStartNodeIndex, const PathStateType& StartState, IsSearchedObjectCallback Callback, WDeque<PathResultData>& out_Path,
    float fMaxPathCost = WMath::Infinity<float>());

  /// Needs to be called by the used WPathStateGenerator to add nodes to evaluate.
  void AddPathNode(WInt64 iNodeIndex, const PathStateType& NewState);

private:
  void ClearPathStates();
  WInt64 FindBestNodeToExpand(PathStateType*& out_pPathState);
  void FillOutPathResult(WInt64 iEndNodeIndex, WDeque<PathResultData>& out_Path);

  WPathStateGenerator<PathStateType>* m_pStateGenerator;

  WHashTable<WInt64, PathStateType> m_PathStates;

  WDeque<WInt64> m_StateQueue;

  WInt64 m_iCurNodeIndex;
  PathStateType m_CurState;
};



#include <Utilities/PathFinding/Implementation/GraphSearch_inl.h>
