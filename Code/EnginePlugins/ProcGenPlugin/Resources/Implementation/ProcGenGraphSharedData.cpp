#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace WProcGenInternal
{
  WUInt32 GraphSharedData::AddTagSet(const WTagSet& tagSet)
  {
    WUInt32 uiIndex = m_TagSets.IndexOf(tagSet);
    if (uiIndex == WInvalidIndex)
    {
      uiIndex = m_TagSets.GetCount();
      m_TagSets.PushBack(tagSet);
    }
    return uiIndex;
  }

  WUInt32 GraphSharedData::AddCurve(WSampledCurve1D&& curve)
  {
    WUInt32 uiIndex = m_Curves.IndexOf(curve);
    if (uiIndex == WInvalidIndex)
    {
      uiIndex = m_Curves.GetCount();
      m_Curves.PushBack(std::move(curve));
    }

    return uiIndex;
  }

  const WTagSet& GraphSharedData::GetTagSet(WUInt32 uiIndex) const
  {
    return m_TagSets[uiIndex];
  }

  const WSampledCurve1D& GraphSharedData::GetCurve(WUInt32 uiIndex) const
  {
    return m_Curves[uiIndex];
  }

  static WTypeVersion s_GraphSharedDataVersion = 2;

  void GraphSharedData::Save(WStreamWriter& inout_stream) const
  {
    inout_stream.WriteVersion(s_GraphSharedDataVersion);

    {
      const WUInt32 uiCount = m_TagSets.GetCount();
      inout_stream << uiCount;

      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets[i].Save(inout_stream);
      }
    }

    {
      const WUInt32 uiCount = m_Curves.GetCount();
      inout_stream << uiCount;

      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        m_Curves[i].Save(inout_stream);
      }
    }
  }

  WResult GraphSharedData::Load(WStreamReader& inout_stream)
  {
    auto version = inout_stream.ReadVersion(s_GraphSharedDataVersion);

    {
      WUInt32 uiCount = 0;
      inout_stream >> uiCount;

      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        m_TagSets.ExpandAndGetRef().Load(inout_stream, WTagRegistry::GetGlobalRegistry());
      }
    }

    if (version >= 2)
    {
      WUInt32 uiCount = 0;
      inout_stream >> uiCount;

      for (WUInt32 i = 0; i < uiCount; ++i)
      {
        WSampledCurve1D curveData;
        W_SUCCEED_OR_RETURN(curveData.Load(inout_stream));
        m_Curves.PushBack(std::move(curveData));
      }
    }

    return W_SUCCESS;
  }

} // namespace WProcGenInternal


