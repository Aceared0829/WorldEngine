#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/ShaderCompiler/PermutationGenerator.h>

void WPermutationGenerator::Clear()
{
  m_Permutations.Clear();
}


void WPermutationGenerator::RemovePermutations(const WHashedString& sPermVarName)
{
  m_Permutations.Remove(sPermVarName);
}

void WPermutationGenerator::AddPermutation(const WHashedString& sName, const WHashedString& sValue)
{
  W_ASSERT_DEV(!sName.IsEmpty(), "");
  W_ASSERT_DEV(!sValue.IsEmpty(), "");

  m_Permutations[sName].Insert(sValue);
}

WUInt32 WPermutationGenerator::GetPermutationCount() const
{
  WUInt32 uiPermutations = 1;

  for (auto it = m_Permutations.GetIterator(); it.IsValid(); ++it)
  {
    uiPermutations *= it.Value().GetCount();
  }

  return uiPermutations;
}

void WPermutationGenerator::GetPermutation(WUInt32 uiPerm, WDynamicArray<WPermutationVar>& out_permVars) const
{
  out_permVars.Clear();

  for (auto itVariable = m_Permutations.GetIterator(); itVariable.IsValid(); ++itVariable)
  {
    const WUInt32 uiValues = itVariable.Value().GetCount();
    WUInt32 uiUseValue = uiPerm % uiValues;

    uiPerm /= uiValues;

    auto itValue = itVariable.Value().GetIterator();

    for (; uiUseValue > 0; --uiUseValue)
    {
      ++itValue;
    }

    WPermutationVar& pv = out_permVars.ExpandAndGetRef();
    pv.m_sName = itVariable.Key();
    pv.m_sValue = itValue.Key();
  }
}
