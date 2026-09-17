#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Foundation/SimdMath/SimdVec4i.h>
#include <ProcGenPlugin/Components/ProcVolumeComponent.h>
#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>
#include <ProcGenPlugin/Tasks/Utils.h>

namespace
{
  WSpatialData::Category s_ProcVolumeCategory = WSpatialData::RegisterCategory("ProcVolume", WSpatialData::Flags::None);
  static WHashedString s_sVolumes = WMakeHashedString("Volumes");

  static const WEnum<WExpression::RegisterType> s_ApplyVolumesTypes[] = {
    WExpression::RegisterType::Float, // PosX
    WExpression::RegisterType::Float, // PosY
    WExpression::RegisterType::Float, // PosZ
    WExpression::RegisterType::Float, // InitialValue
    WExpression::RegisterType::Int,   // TagSetIndex
    WExpression::RegisterType::Int,   // ImageMode
    WExpression::RegisterType::Float, // RefColorR
    WExpression::RegisterType::Float, // RefColorG
    WExpression::RegisterType::Float, // RefColorB
    WExpression::RegisterType::Float, // RefColorA
  };

  static void ApplyVolumes(WExpression::Inputs inputs, WExpression::Output output, const WExpression::GlobalData& globalData)
  {
    const WVariantArray& volumes = globalData.GetValue(s_sVolumes)->Get<WVariantArray>();
    if (volumes.IsEmpty())
      return;

    WUInt32 uiTagSetIndex = inputs[4].GetPtr()->i.x();
    auto pVolumeCollection = WDynamicCast<const WVolumeCollection*>(volumes[uiTagSetIndex].Get<WReflectedClass*>());
    if (pVolumeCollection == nullptr)
      return;

    const WExpression::Register* pPosX = inputs[0].GetPtr();
    const WExpression::Register* pPosY = inputs[1].GetPtr();
    const WExpression::Register* pPosZ = inputs[2].GetPtr();
    const WExpression::Register* pPosXEnd = inputs[0].GetEndPtr();

    const WExpression::Register* pInitialValues = inputs[3].GetPtr();

    WProcVolumeImageMode::Enum imgMode = WProcVolumeImageMode::Default;
    WColor refColor = WColor::White;
    if (inputs.GetCount() >= 10)
    {
      imgMode = static_cast<WProcVolumeImageMode::Enum>(inputs[5].GetPtr()->i.x());

      const float refColR = inputs[6].GetPtr()->f.x();
      const float refColG = inputs[7].GetPtr()->f.x();
      const float refColB = inputs[8].GetPtr()->f.x();
      const float refColA = inputs[9].GetPtr()->f.x();
      refColor = WColor(refColR, refColG, refColB, refColA);
    }

    WExpression::Register* pOutput = output.GetPtr();

    WSimdMat4f helperMat;
    while (pPosX < pPosXEnd)
    {
      helperMat.SetRows(pPosX->f, pPosY->f, pPosZ->f, WSimdVec4f::MakeZero());

      const float x = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col0, pInitialValues->f.x(), imgMode, refColor);
      const float y = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col1, pInitialValues->f.y(), imgMode, refColor);
      const float z = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col2, pInitialValues->f.z(), imgMode, refColor);
      const float w = pVolumeCollection->EvaluateAtGlobalPosition(helperMat.m_col3, pInitialValues->f.w(), imgMode, refColor);
      pOutput->f.Set(x, y, z, w);

      ++pPosX;
      ++pPosY;
      ++pPosZ;
      ++pInitialValues;
      ++pOutput;
    }
  }

  static WResult ApplyVolumesValidate(const WExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const WVariant* pValue = globalData.GetValue("Volumes"))
      {
        if (pValue->GetType() == WVariantType::VariantArray)
        {
          return W_SUCCESS;
        }
      }
    }

    return W_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////

  static WHashedString s_sInstanceSeed = WMakeHashedString("InstanceSeed");

  static void GetInstanceSeed(WExpression::Inputs inputs, WExpression::Output output, const WExpression::GlobalData& globalData)
  {
    int instanceSeed = globalData.GetValue(s_sInstanceSeed)->Get<int>();

    WExpression::Register* pOutput = output.GetPtr();
    WExpression::Register* pOutputEnd = output.GetEndPtr();

    while (pOutput < pOutputEnd)
    {
      pOutput->i.Set(instanceSeed);

      ++pOutput;
    }
  }

  static WResult GetInstanceSeedValidate(const WExpression::GlobalData& globalData)
  {
    if (!globalData.IsEmpty())
    {
      if (const WVariant* pValue = globalData.GetValue(s_sInstanceSeed))
      {
        if (pValue->GetType() == WVariantType::Int32)
        {
          return W_SUCCESS;
        }
      }
    }

    return W_FAILURE;
  }
} // namespace

WExpressionFunction WProcGenExpressionFunctions::s_ApplyVolumesFunc = {
  {WMakeHashedString("applyVolumes"), WExpression::FunctionDesc::TypeList(s_ApplyVolumesTypes), 5, WExpression::RegisterType::Float},
  &ApplyVolumes,
  &ApplyVolumesValidate,
};

WExpressionFunction WProcGenExpressionFunctions::s_GetInstanceSeedFunc = {
  {WMakeHashedString("getInstanceSeed"), WExpression::FunctionDesc::TypeList(), 0, WExpression::RegisterType::Int},
  &GetInstanceSeed,
  &GetInstanceSeedValidate,
};

//////////////////////////////////////////////////////////////////////////

// static
void WProcGenGlobalData::ExtractVolumeCollections(const WWorld& world, const WBoundingBox& box, const WProcGenInternal::Output& output, WDeque<WVolumeCollection>& ref_volumeCollections, WExpression::GlobalData& ref_globalData)
{
  auto& volumeTagSetIndices = output.m_VolumeTagSetIndices;
  if (volumeTagSetIndices.IsEmpty())
    return;

  WVariantArray volumes;
  if (WVariant* volumesVar = ref_globalData.GetValue(s_sVolumes))
  {
    volumes = volumesVar->Get<WVariantArray>();
  }

  for (WUInt8 tagSetIndex : volumeTagSetIndices)
  {
    if (tagSetIndex < volumes.GetCount() && volumes[tagSetIndex].IsValid())
    {
      continue;
    }

    auto pGraphSharedData = static_cast<const WProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& includeTags = pGraphSharedData->GetTagSet(tagSetIndex);

    auto& volumeCollection = ref_volumeCollections.ExpandAndGetRef();
    WVolumeCollection::ExtractVolumesInBox(world, box, s_ProcVolumeCategory, includeTags, volumeCollection, WGetStaticRTTI<WProcVolumeComponent>());

    volumes.EnsureCount(tagSetIndex + 1);
    volumes[tagSetIndex] = WVariant(&volumeCollection);
  }

  ref_globalData.Insert(s_sVolumes, volumes);
}

// static
void WProcGenGlobalData::SetInstanceSeed(WUInt32 uiSeed, WExpression::GlobalData& ref_globalData)
{
  ref_globalData.Insert(s_sInstanceSeed, (int)uiSeed);
}

// static
void WProcGenGlobalData::SetCurves(const WProcGenInternal::Output& output, WExpression::GlobalData& ref_globalData)
{
  auto& curveIndices = output.m_CurveIndices;
  if (curveIndices.IsEmpty())
    return;

  const WHashedString sCurves = WMakeHashedString("Curves");

  WVariantArray curves;
  if (WVariant* curvesVar = ref_globalData.GetValue(sCurves))
  {
    curves = curvesVar->Get<WVariantArray>();
  }

  for (WUInt8 curveIndex : curveIndices)
  {
    if (curveIndex < curves.GetCount() && curves[curveIndex].IsValid())
    {
      continue;
    }

    auto pGraphSharedData = static_cast<const WProcGenInternal::GraphSharedData*>(output.m_pGraphSharedData.Borrow());
    auto& curveData = pGraphSharedData->GetCurve(curveIndex);

    curves.EnsureCount(curveIndex + 1);
    curves[curveIndex] = WVariant(&curveData);
  }

  ref_globalData.Insert(sCurves, curves);
}
