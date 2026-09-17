#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <RendererCore/AnimationSystem/AnimGraph/Nodes/BoneWeights/BoneWeightsAnimNode.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

#include <ozz/animation/runtime/skeleton_utils.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WBoneWeightsAnimNode, 1, WRTTIDefaultAllocator<WBoneWeightsAnimNode>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("Weight", m_fWeight)->AddAttributes(new WDefaultValueAttribute(1.0f), new WClampValueAttribute(0.0f, 1.0f)),
    W_ARRAY_ACCESSOR_PROPERTY("RootBones", RootBones_GetCount, RootBones_GetValue, RootBones_SetValue, RootBones_Insert, RootBones_Remove),

    W_MEMBER_PROPERTY("Weights", m_WeightsPin)->AddAttributes(new WHiddenAttribute()),
    W_MEMBER_PROPERTY("InverseWeights", m_InverseWeightsPin)->AddAttributes(new WHiddenAttribute()),
  }
  W_END_PROPERTIES;
  W_BEGIN_ATTRIBUTES
  {
    new WCategoryAttribute("Weights"),
    new WColorAttribute(WColorScheme::DarkUI(WColorScheme::Teal)),
    new WTitleAttribute("Bone Weights '{RootBones[0]}' '{RootBones[1]}' '{RootBones[2]}'"),
  }
  W_END_ATTRIBUTES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

WBoneWeightsAnimNode::WBoneWeightsAnimNode() = default;
WBoneWeightsAnimNode::~WBoneWeightsAnimNode() = default;

WUInt32 WBoneWeightsAnimNode::RootBones_GetCount() const
{
  return m_RootBones.GetCount();
}

const char* WBoneWeightsAnimNode::RootBones_GetValue(WUInt32 uiIndex) const
{
  return m_RootBones[uiIndex].GetString();
}

void WBoneWeightsAnimNode::RootBones_SetValue(WUInt32 uiIndex, const char* value)
{
  m_RootBones[uiIndex].Assign(value);
}

void WBoneWeightsAnimNode::RootBones_Insert(WUInt32 uiIndex, const char* value)
{
  WHashedString tmp;
  tmp.Assign(value);
  m_RootBones.InsertAt(uiIndex, tmp);
}

void WBoneWeightsAnimNode::RootBones_Remove(WUInt32 uiIndex)
{
  m_RootBones.RemoveAtAndCopy(uiIndex);
}

WResult WBoneWeightsAnimNode::SerializeNode(WStreamWriter& stream) const
{
  stream.WriteVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  W_SUCCEED_OR_RETURN(stream.WriteArray(m_RootBones));

  stream << m_fWeight;

  W_SUCCEED_OR_RETURN(m_WeightsPin.Serialize(stream));
  W_SUCCEED_OR_RETURN(m_InverseWeightsPin.Serialize(stream));

  return W_SUCCESS;
}

WResult WBoneWeightsAnimNode::DeserializeNode(WStreamReader& stream)
{
  stream.ReadVersion(1);

  W_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  W_SUCCEED_OR_RETURN(stream.ReadArray(m_RootBones));

  stream >> m_fWeight;

  W_SUCCEED_OR_RETURN(m_WeightsPin.Deserialize(stream));
  W_SUCCEED_OR_RETURN(m_InverseWeightsPin.Deserialize(stream));

  return W_SUCCESS;
}

void WBoneWeightsAnimNode::Step(WAnimController& ref_controller, WAnimGraphInstance& ref_graph, WTime tDiff, const WSkeletonResource* pSkeleton, WGameObject* pTarget) const
{
  if (!m_WeightsPin.IsConnected() && !m_InverseWeightsPin.IsConnected())
    return;

  if (m_RootBones.IsEmpty())
  {
    WLog::Warning("No root-bones added to bone weight node in animation controller.");
    return;
  }

  InstanceData* pInstance = ref_graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_pSharedBoneWeights == nullptr && pInstance->m_pSharedInverseBoneWeights == nullptr)
  {
    const auto pOzzSkeleton = &pSkeleton->GetDescriptor().m_Skeleton.GetOzzSkeleton();

    WStringBuilder name;
    name.SetFormat("{}", pSkeleton->GetResourceIDHash());

    for (const auto& rootBone : m_RootBones)
    {
      name.AppendFormat("-{}", rootBone);
    }

    pInstance->m_pSharedBoneWeights = ref_controller.CreateBoneWeights(name, *pSkeleton, [this, pOzzSkeleton](WAnimGraphSharedBoneWeights& ref_bw)
      {
      for (const auto& rootBone : m_RootBones)
      {
        int iRootBone = -1;
        for (int iBone = 0; iBone < pOzzSkeleton->num_joints(); ++iBone)
        {
          if (WStringUtils::IsEqual(pOzzSkeleton->joint_names()[iBone], rootBone.GetData()))
          {
            iRootBone = iBone;
            break;
          }
        }

        const float fBoneWeight = 1.0f;

        auto setBoneWeight = [&](int iCurrentBone, int) {
          const int iJointIdx0 = iCurrentBone / 4;
          const int iJointIdx1 = iCurrentBone % 4;

          ozz::math::SimdFloat4& soa_weight = ref_bw.m_Weights[iJointIdx0];
          soa_weight = ozz::math::SetI(soa_weight, ozz::math::simd_float4::Load1(fBoneWeight), iJointIdx1);
        };

        ozz::animation::IterateJointsDF(*pOzzSkeleton, setBoneWeight, iRootBone);
      } });

    if (m_InverseWeightsPin.IsConnected())
    {
      name.Append("-inv");

      pInstance->m_pSharedInverseBoneWeights = ref_controller.CreateBoneWeights(name, *pSkeleton, [this, pInstance](WAnimGraphSharedBoneWeights& ref_bw)
        {
        const ozz::math::SimdFloat4 oneBone = ozz::math::simd_float4::one();

        for (WUInt32 b = 0; b < ref_bw.m_Weights.GetCount(); ++b)
        {
          ref_bw.m_Weights[b] = ozz::math::MSub(oneBone, oneBone, pInstance->m_pSharedBoneWeights->m_Weights[b]);
        } });
    }

    if (!m_WeightsPin.IsConnected())
    {
      pInstance->m_pSharedBoneWeights.Clear();
    }
  }

  if (m_WeightsPin.IsConnected())
  {
    WAnimGraphPinDataBoneWeights* pPinData = ref_controller.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = pInstance->m_pSharedBoneWeights.Borrow();

    m_WeightsPin.SetWeights(ref_graph, pPinData);
  }

  if (m_InverseWeightsPin.IsConnected())
  {
    WAnimGraphPinDataBoneWeights* pPinData = ref_controller.AddPinDataBoneWeights();
    pPinData->m_fOverallWeight = m_fWeight;
    pPinData->m_pSharedBoneWeights = pInstance->m_pSharedInverseBoneWeights.Borrow();

    m_InverseWeightsPin.SetWeights(ref_graph, pPinData);
  }
}

bool WBoneWeightsAnimNode::GetInstanceDataDesc(WInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

W_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_Nodes_BoneWeights_BoneWeightsAnimNode);
