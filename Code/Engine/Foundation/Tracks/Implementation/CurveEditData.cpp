#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Math.h>
#include <Foundation/Tracks/Curve1D.h>
#include <Foundation/Tracks/CurveEditData.h>

// clang-format off
W_BEGIN_STATIC_REFLECTED_ENUM(WCurveTangentMode, 1)
W_ENUM_CONSTANTS(WCurveTangentMode::Bezier, WCurveTangentMode::FixedLength, WCurveTangentMode::Linear, WCurveTangentMode::Auto)
W_END_STATIC_REFLECTED_ENUM;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCurveControlPointData, 5, WRTTIDefaultAllocator<WCurveControlPointData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Tick", m_iTick),
    W_MEMBER_PROPERTY("Value", m_fValue),
    W_MEMBER_PROPERTY("LeftTangent", m_LeftTangent)->AddAttributes(new WDefaultValueAttribute(WVec2(-0.1f, 0))),
    W_MEMBER_PROPERTY("RightTangent", m_RightTangent)->AddAttributes(new WDefaultValueAttribute(WVec2(+0.1f, 0))),
    W_MEMBER_PROPERTY("Linked", m_bTangentsLinked)->AddAttributes(new WDefaultValueAttribute(true)),
    W_ENUM_MEMBER_PROPERTY("LeftTangentMode", WCurveTangentMode, m_LeftTangentMode),
    W_ENUM_MEMBER_PROPERTY("RightTangentMode", WCurveTangentMode, m_RightTangentMode),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WSingleCurveData, 3, WRTTIDefaultAllocator<WSingleCurveData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Color", m_CurveColor)->AddAttributes(new WDefaultValueAttribute(WColorScheme::LightUI(WColorScheme::Lime))),
    W_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCurveGroupData, 2, WRTTIDefaultAllocator<WCurveGroupData>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("FPS", m_uiFramesPerSecond)->AddAttributes(new WDefaultValueAttribute(60)),
    W_ARRAY_MEMBER_PROPERTY("Curves", m_Curves)->AddFlags(WPropertyFlags::PointerOwner),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WCurveExtentsAttribute, 1, WRTTIDefaultAllocator<WCurveExtentsAttribute>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("LowerExtent", m_fLowerExtent),
    W_MEMBER_PROPERTY("UpperExtent", m_fUpperExtent),
    W_MEMBER_PROPERTY("LowerExtentFixed", m_bLowerExtentFixed),
    W_MEMBER_PROPERTY("UpperExtentFixed", m_bUpperExtentFixed),
  }
  W_END_PROPERTIES;
  W_BEGIN_FUNCTIONS
  {
    W_CONSTRUCTOR_PROPERTY(float, bool, float, bool),
  }
  W_END_FUNCTIONS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WCurveExtentsAttribute::WCurveExtentsAttribute(double fLowerExtent, bool bLowerExtentFixed, double fUpperExtent, bool bUpperExtentFixed)
{
  m_fLowerExtent = fLowerExtent;
  m_fUpperExtent = fUpperExtent;
  m_bLowerExtentFixed = bLowerExtentFixed;
  m_bUpperExtentFixed = bUpperExtentFixed;
}

void WCurveControlPointData::SetTickFromTime(WTime time, WInt64 iFps)
{
  const WInt64 iTicksPerStep = 4800 / iFps;
  m_iTick = (WInt64)WMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)iTicksPerStep);
}

WCurveGroupData::~WCurveGroupData()
{
  Clear();
}

void WCurveGroupData::CloneFrom(const WCurveGroupData& rhs)
{
  Clear();

  m_bOwnsData = true;
  m_uiFramesPerSecond = rhs.m_uiFramesPerSecond;
  m_Curves.SetCount(rhs.m_Curves.GetCount());

  for (WUInt32 i = 0; i < m_Curves.GetCount(); ++i)
  {
    m_Curves[i] = W_DEFAULT_NEW(WSingleCurveData);
    *m_Curves[i] = *(rhs.m_Curves[i]);
  }
}

void WCurveGroupData::Clear()
{
  m_uiFramesPerSecond = 60;

  if (m_bOwnsData)
  {
    m_bOwnsData = false;

    for (WUInt32 i = 0; i < m_Curves.GetCount(); ++i)
    {
      W_DEFAULT_DELETE(m_Curves[i]);
    }
  }

  m_Curves.Clear();
}

WInt64 WCurveGroupData::TickFromTime(WTime time) const
{
  const WUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (WInt64)WMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

static void ConvertControlPoint(const WCurveControlPointData& cp, WCurve1D& out_result)
{
  auto& ccp = out_result.AddControlPoint(cp.GetTickAsTime().GetSeconds());
  ccp.m_Position.y = cp.m_fValue;
  ccp.m_LeftTangent = cp.m_LeftTangent;
  ccp.m_RightTangent = cp.m_RightTangent;
  ccp.m_TangentModeLeft = cp.m_LeftTangentMode;
  ccp.m_TangentModeRight = cp.m_RightTangentMode;
}

void WSingleCurveData::ConvertToRuntimeData(WCurve1D& out_result) const
{
  out_result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    ConvertControlPoint(cp, out_result);
  }
}

double WSingleCurveData::Evaluate(WInt64 iTick) const
{
  WCurve1D temp;
  const WCurveControlPointData* llhs = nullptr;
  const WCurveControlPointData* lhs = nullptr;
  const WCurveControlPointData* rhs = nullptr;
  const WCurveControlPointData* rrhs = nullptr;
  FindNearestControlPoints(m_ControlPoints.GetArrayPtr(), iTick, llhs, lhs, rhs, rrhs);

  if (llhs)
    ConvertControlPoint(*llhs, temp);
  if (lhs)
    ConvertControlPoint(*lhs, temp);
  if (rhs)
    ConvertControlPoint(*rhs, temp);
  if (rrhs)
    ConvertControlPoint(*rrhs, temp);

  // #TODO: This is rather slow as we eval lots of points but only need one
  temp.CreateLinearApproximation();
  return temp.Evaluate(iTick / 4800.0);
}

void WCurveGroupData::ConvertToRuntimeData(WUInt32 uiCurveIdx, WCurve1D& out_result) const
{
  m_Curves[uiCurveIdx]->ConvertToRuntimeData(out_result);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class WCurve1DControlPoint_2_3 : public WGraphPatch
{
public:
  WCurve1DControlPoint_2_3()
    : WGraphPatch("WCurve1DControlPoint", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& /*ref_context*/, WAbstractObjectGraph* /*pGraph*/, WAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Point");
    if (pPoint && pPoint->m_Value.IsA<WVec2>())
    {
      WVec2 pt = pPoint->m_Value.Get<WVec2>();
      pNode->AddProperty("Time", (double)WMath::Max(0.0f, pt.x));
      pNode->AddProperty("Value", (double)pt.y);
      pNode->AddProperty("LeftTangentMode", (WUInt32)WCurveTangentMode::Bezier);
      pNode->AddProperty("RightTangentMode", (WUInt32)WCurveTangentMode::Bezier);
    }
  }
};

WCurve1DControlPoint_2_3 g_WCurve1DControlPoint_2_3;

//////////////////////////////////////////////////////////////////////////

class WCurve1DControlPoint_3_4 : public WGraphPatch
{
public:
  WCurve1DControlPoint_3_4()
    : WGraphPatch("WCurve1DControlPoint", 4)
  {
  }

  virtual void Patch(WGraphPatchContext& /*ref_context*/, WAbstractObjectGraph* /*pGraph*/, WAbstractObjectNode* pNode) const override
  {
    auto* pPoint = pNode->FindProperty("Time");
    if (pPoint && pPoint->m_Value.IsA<double>())
    {
      const double fTime = pPoint->m_Value.Get<double>();
      pNode->AddProperty("Tick", (WInt64)WMath::RoundToMultiple(fTime * 4800.0, 4800.0 / 60.0));
    }
  }
};

WCurve1DControlPoint_3_4 g_WCurve1DControlPoint_3_4;

//////////////////////////////////////////////////////////////////////////

class WCurve1DControlPoint_4_5 : public WGraphPatch
{
public:
  WCurve1DControlPoint_4_5()
    : WGraphPatch("WCurve1DControlPoint", 5)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* /*pGraph*/, WAbstractObjectNode* /*pNode*/) const override
  {
    ref_context.RenameClass("WCurveControlPointData");
  }
};

WCurve1DControlPoint_4_5 g_WCurve1DControlPoint_4_5;

//////////////////////////////////////////////////////////////////////////

class WCurve1DData_2_3 : public WGraphPatch
{
public:
  WCurve1DData_2_3()
    : WGraphPatch("WCurve1DData", 3)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* /*pGraph*/, WAbstractObjectNode* /*pNode*/) const override
  {
    ref_context.RenameClass("WSingleCurveData");
  }
};

WCurve1DData_2_3 g_WCurve1DData_2_3;

//////////////////////////////////////////////////////////////////////////

class WCurve1DAssetData_1_2 : public WGraphPatch
{
public:
  WCurve1DAssetData_1_2()
    : WGraphPatch("WCurve1DAssetData", 2)
  {
  }

  virtual void Patch(WGraphPatchContext& ref_context, WAbstractObjectGraph* /*pGraph*/, WAbstractObjectNode* /*pNode*/) const override
  {
    ref_context.RenameClass("WCurveGroupData");
  }
};

WCurve1DAssetData_1_2 g_WCurve1DAssetData_1_2;


W_STATICLINK_FILE(Foundation, Foundation_Tracks_Implementation_CurveEditData);
