#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/Debug/DebugAnimNodes.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogAnimNode, 1, WRTTINoAllocator)
  {
    W_BEGIN_PROPERTIES
    {
      W_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new WDefaultValueAttribute("Values: {0}/{1}-{3}/{4}")),

      W_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new WHiddenAttribute()),
      W_MEMBER_PROPERTY("NumberCount", m_uiNumberCount)->AddAttributes(new WNoTemporaryTransactionsAttribute(), new WDynamicPinAttribute(), new WDefaultValueAttribute(1)),
      W_ARRAY_MEMBER_PROPERTY("InNumbers", m_InNumbers)->AddAttributes(new WHiddenAttribute(), new WDynamicPinAttribute("NumberCount")),
    }
    W_END_PROPERTIES;
    W_BEGIN_ATTRIBUTES
    {
      new WCategoryAttribute("Debug"),
      new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Pink)),
      new WTitleAttribute("Log: '{Text}'"),
    }
    W_END_ATTRIBUTES;
  }
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WResult WLogAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sText;
  stream << m_uiNumberCount;

  W_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  W_SUCCEED_OR_RETURN(stream.WriteArray(m_InNumbers));

  return W_SUCCESS;
}

WResult WLogAnimNode::DeserializeNode(WStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  W_IGNORE_UNUSED(version);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;
  stream >> m_uiNumberCount;

  W_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  W_SUCCEED_OR_RETURN(stream.ReadArray(m_InNumbers));

  return W_SUCCESS;
}

static WStringView BuildFormattedText(WStringView sText, const WVariantArray& params, WStringBuilder& ref_sStorage)
{
  WTempHybridArray<WString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<WString>());
  }

  WTempHybridArray<WStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  WFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogInfoAnimNode, 1, WRTTIDefaultAllocator<WLogInfoAnimNode>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Log Info: '{Text}'"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WLogInfoAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  WVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  WStringBuilder sStorage;
  WLog::Info(BuildFormattedText(m_sText, params, sStorage));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WLogErrorAnimNode, 1, WRTTIDefaultAllocator<WLogErrorAnimNode>)
{
  W_BEGIN_ATTRIBUTES
  {
    new WTitleAttribute("Log Error: '{Text}'"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void WLogErrorAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  WVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  WStringBuilder sStorage;
  WLog::Error(BuildFormattedText(m_sText, params, sStorage));
}


W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_Debug_DebugAnimNodes);
