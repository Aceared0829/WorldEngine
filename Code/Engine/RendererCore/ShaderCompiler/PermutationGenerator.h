#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererCore/Declarations.h>

/// A helper class to iterate over all possible permutations.
///
/// Just add all permutation variables and their possible values.
/// Then the number of possible permutations and each permutation
/// can be queried.
class W_RENDERERCORE_DLL WPermutationGenerator
{
public:
  /// Resets everything.
  void Clear();

  /// Removes all permutations for the given variable
  void RemovePermutations(const WHashedString& sPermVarName);

  /// Adds the name and one of the possible values of a permutation variable.
  void AddPermutation(const WHashedString& sName, const WHashedString& sValue);

  /// Returns how many permutations are possible.
  WUInt32 GetPermutationCount() const;

  /// Returns the n-th permutation.
  void GetPermutation(WUInt32 uiPerm, WDynamicArray<WPermutationVar>& out_permVars) const;

private:
  WMap<WHashedString, WHashSet<WHashedString>> m_Permutations;
};
