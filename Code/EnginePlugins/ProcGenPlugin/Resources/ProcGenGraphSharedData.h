#pragma once

#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Types/TagSet.h>
#include <ProcGenPlugin/Declarations.h>

namespace WProcGenInternal
{
  class W_PROCGENPLUGIN_DLL GraphSharedData : public GraphSharedDataBase
  {
  public:
    WUInt32 AddTagSet(const WTagSet& tagSet);
    WUInt32 AddCurve(WSampledCurve1D&& curve);

    const WTagSet& GetTagSet(WUInt32 uiIndex) const;
    const WSampledCurve1D& GetCurve(WUInt32 uiIndex) const;

    void Save(WStreamWriter& inout_stream) const;
    WResult Load(WStreamReader& inout_stream);

  private:
    WDynamicArray<WTagSet> m_TagSets;
    WDynamicArray<WSampledCurve1D> m_Curves;
  };

} // namespace WProcGenInternal
