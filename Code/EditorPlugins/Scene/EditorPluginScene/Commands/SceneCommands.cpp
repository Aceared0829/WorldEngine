#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Commands/SceneCommands.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WDuplicateObjectsCommand, 1, WRTTIDefaultAllocator<WDuplicateObjectsCommand>)
{
  W_BEGIN_PROPERTIES
  {
    W_MEMBER_PROPERTY("GraphText", m_sGraphTextFormat),
    W_MEMBER_PROPERTY("ParentNodes", m_sParentNodes),
    W_MEMBER_PROPERTY("NumCopies", m_uiNumberOfCopies),
    W_MEMBER_PROPERTY("Translate", m_vAccumulativeTranslation),
    W_MEMBER_PROPERTY("Rotate", m_vAccumulativeRotation),
    W_MEMBER_PROPERTY("RandomRotation", m_vRandomRotation),
    W_MEMBER_PROPERTY("RandomTranslation", m_vRandomTranslation),
    W_MEMBER_PROPERTY("Group", m_bGroupDuplicates),
    W_MEMBER_PROPERTY("RevolveAxis", m_iRevolveAxis),
    W_MEMBER_PROPERTY("RevoleStartAngle", m_RevolveStartAngle),
    W_MEMBER_PROPERTY("RevolveAngleStep", m_RevolveAngleStep),
    W_MEMBER_PROPERTY("RevolveRadius", m_fRevolveRadius),
    W_MEMBER_PROPERTY("InsertIndex", m_iInsertIndex),
  }
  W_END_PROPERTIES;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


////////////////////////////////////////////////////////////////////////
// WDuplicateObjectsCommand
////////////////////////////////////////////////////////////////////////

WDuplicateObjectsCommand::WDuplicateObjectsCommand()
{
  m_uiNumberOfCopies = 0;
  m_vAccumulativeTranslation.SetZero();
  m_vAccumulativeRotation.SetZero();
  m_vRandomRotation.SetZero();
  m_vRandomTranslation.SetZero();
  m_iRevolveAxis = 0;
  m_fRevolveRadius = 0.0f;
  m_bGroupDuplicates = false;
}

WStatus WDuplicateObjectsCommand::DoInternal(bool bRedo)
{
  WSceneDocument* pDocument = static_cast<WSceneDocument*>(GetDocument());

  if (!bRedo)
  {
    W_ASSERT_DEV(!m_bGroupDuplicates, "Not yet implemented");

    WAbstractObjectGraph graph;
    DeserializeGraph(graph);

    if (m_uiNumberOfCopies == 0)
    {
      WTempHybridArray<WDocument::PasteInfo, 16> ToBePasted;
      CreateOneDuplicate(graph, ToBePasted);
    }
    else
    {
      // store original selection
      m_OriginalSelection = m_pDocument->GetSelectionManager()->GetSelection();

      WTempHybridArray<WTempHybridArray<WDocument::PasteInfo, 16>, 8> ToBePasted;
      ToBePasted.SetCount(m_uiNumberOfCopies);

      for (WUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        CreateOneDuplicate(graph, ToBePasted[copies]);
      }

      WRandomGauss rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ;

      if (m_vRandomRotation.x > 0)
        rngRotX.Initialize((WUInt64)WTime::Now().GetNanoseconds(), (WUInt32)(m_vRandomRotation.x));
      if (m_vRandomRotation.y > 0)
        rngRotY.Initialize((WUInt64)WTime::Now().GetNanoseconds() + 1, (WUInt32)(m_vRandomRotation.y));
      if (m_vRandomRotation.z > 0)
        rngRotZ.Initialize((WUInt64)WTime::Now().GetNanoseconds() + 2, (WUInt32)(m_vRandomRotation.z));

      if (m_vRandomTranslation.x > 0)
        rngTransX.Initialize((WUInt64)WTime::Now().GetNanoseconds() + 3, (WUInt32)(m_vRandomTranslation.x * 100));
      if (m_vRandomTranslation.y > 0)
        rngTransY.Initialize((WUInt64)WTime::Now().GetNanoseconds() + 4, (WUInt32)(m_vRandomTranslation.y * 100));
      if (m_vRandomTranslation.z > 0)
        rngTransZ.Initialize((WUInt64)WTime::Now().GetNanoseconds() + 5, (WUInt32)(m_vRandomTranslation.z * 100));

      for (WUInt32 copies = 0; copies < m_uiNumberOfCopies; ++copies)
      {
        AdjustObjectPositions(ToBePasted[copies], copies, rngRotX, rngRotY, rngRotZ, rngTransX, rngTransY, rngTransZ);
      }
    }


    if (m_DuplicatedObjects.IsEmpty())
      return WStatus("Paste Objects: nothing was pasted!");
  }
  else
  {
    // Re-add at recorded place.
    for (auto& po : m_DuplicatedObjects)
    {
      pDocument->GetObjectManager()->AddObject(po.m_pObject, po.m_pParent, po.m_sParentProperty, po.m_Index);
    }
  }

  SetAsSelection();

  return WStatus(W_SUCCESS);
}

void WDuplicateObjectsCommand::SetAsSelection()
{
  if (!m_DuplicatedObjects.IsEmpty())
  {
    m_DuplicatedObjects.Sort([](const auto& lhs, const auto& rhs)
      { return lhs.m_uiSelectionOrder < rhs.m_uiSelectionOrder; });

    auto pSelMan = m_pDocument->GetSelectionManager();

    WDeque<const WDocumentObject*> NewSelection = m_OriginalSelection;

    for (const DuplicatedObject& pi : m_DuplicatedObjects)
    {
      NewSelection.PushBack(pi.m_pObject);
    }

    pSelMan->SetSelection(NewSelection);
  }
}

void WDuplicateObjectsCommand::DeserializeGraph(WAbstractObjectGraph& graph)
{
  WRawMemoryStreamReader memoryReader(m_sGraphTextFormat.GetData(), m_sGraphTextFormat.GetElementCount());
  WAbstractGraphDdlSerializer::Read(memoryReader, &graph).IgnoreResult();
}

void WDuplicateObjectsCommand::CreateOneDuplicate(WAbstractObjectGraph& graph, WDynamicArray<WDocument::PasteInfo>& out_toBePasted)
{
  WSceneDocument* pDocument = static_cast<WSceneDocument*>(GetDocument());

  // Remap
  const WUuid seed = WUuid::MakeUuid();
  graph.ReMapNodeGuids(seed);

  WDocumentObjectConverterReader reader(&graph, pDocument->GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateOnly);


  WStringBuilder sParentGuids = m_sParentNodes;
  WStringBuilder sNextParentGuid;

  WMap<WUuid, WUuid> ParentGuids;

  while (!sParentGuids.IsEmpty())
  {
    sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
    sParentGuids.Shrink(41, 0);

    WUuid guidObj = WConversionUtils::ConvertStringToUuid(sNextParentGuid);
    guidObj.CombineWithSeed(seed);

    sNextParentGuid.SetSubString_ElementCount(sParentGuids, 40);
    sParentGuids.Shrink(41, 0);

    ParentGuids[guidObj] = WConversionUtils::ConvertStringToUuid(sNextParentGuid);
  }

  WTempHybridArray<WUInt32, 32> selectionOrder;

  auto& nodes = graph.GetAllNodes();
  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();

    if (pNode->GetNodeName() == "root")
    {
      auto* pNewObject = reader.CreateObjectFromNode(pNode);

      if (pNewObject)
      {
        reader.ApplyPropertiesToObject(pNode, pNewObject);

        selectionOrder.PushBack(0);

        if (auto* pProperty = pNode->FindProperty("__SelectionOrder"))
        {
          selectionOrder.PeekBack() = pProperty->m_Value.ConvertTo<WUInt32>();
        }

        auto& ref = out_toBePasted.ExpandAndGetRef();
        ref.m_pObject = pNewObject;
        ref.m_pParent = nullptr;
        if (m_uiNumberOfCopies == 0 && m_iInsertIndex >= 0)
          ref.m_Index = m_iInsertIndex;

        const WUuid guidParent = ParentGuids[pNode->GetGuid()];

        if (guidParent.IsValid())
          ref.m_pParent = pDocument->GetObjectManager()->GetObject(guidParent);
      }
    }
  }

  if (pDocument->DuplicateSelectedObjects(out_toBePasted, graph, false))
  {
    for (WUInt32 i = 0; i < out_toBePasted.GetCount(); ++i)
    {
      const auto& item = out_toBePasted[i];
      auto& po = m_DuplicatedObjects.ExpandAndGetRef();
      po.m_pObject = item.m_pObject;
      po.m_Index = item.m_pObject->GetPropertyIndex();
      po.m_pParent = item.m_pParent;
      po.m_sParentProperty = item.m_pObject->GetParentProperty();
      po.m_uiSelectionOrder = selectionOrder[i];
    }
  }
  else
  {
    for (const auto& item : out_toBePasted)
    {
      pDocument->GetObjectManager()->DestroyObject(item.m_pObject);
    }
  }

  // undo uuid changes, so that we can do this again with another seed
  graph.ReMapNodeGuids(seed, true);
}


void WDuplicateObjectsCommand::AdjustObjectPositions(const WArrayPtr<WDocument::PasteInfo>& duplicates, WUInt32 uiNumDuplicate, WRandomGauss& rngRotX, WRandomGauss& rngRotY, WRandomGauss& rngRotZ, WRandomGauss& rngTransX, WRandomGauss& rngTransY, WRandomGauss& rngTransZ)
{
  WSceneDocument* pScene = static_cast<WSceneDocument*>(m_pDocument);

  const float fStep = uiNumDuplicate;

  WVec3 vRandT(0.0f);
  WVec3 vRandR(0.0f);

  if (m_vRandomRotation.x != 0)
    vRandR.x = rngRotX.SignedValue();
  if (m_vRandomRotation.y != 0)
    vRandR.y = rngRotY.SignedValue();
  if (m_vRandomRotation.z != 0)
    vRandR.z = rngRotZ.SignedValue();

  if (m_vRandomTranslation.x != 0)
    vRandT.x = rngTransX.SignedValue() / 100.0f;
  if (m_vRandomTranslation.y != 0)
    vRandT.y = rngTransY.SignedValue() / 100.0f;
  if (m_vRandomTranslation.z != 0)
    vRandT.z = rngTransZ.SignedValue() / 100.0f;

  WVec3 vPosOffset(0.0f);

  if (m_iRevolveAxis > 0 && m_fRevolveRadius != 0.0f && m_RevolveAngleStep != WAngle())
  {
    WVec3 vRevolveAxis(0.0f);
    WAngle revolve = m_RevolveStartAngle;

    switch (m_iRevolveAxis)
    {
      case 1:
        vRevolveAxis.Set(1, 0, 0);
        vPosOffset.Set(0, 0, m_fRevolveRadius);
        break;
      case 2:
        vRevolveAxis.Set(0, 1, 0);
        vPosOffset.Set(m_fRevolveRadius, 0, 0);
        break;
      case 3:
        vRevolveAxis.Set(0, 0, 1);
        vPosOffset.Set(0, m_fRevolveRadius, 0);
        break;
    }

    revolve += fStep * m_RevolveAngleStep;

    WMat3 mRevolve = WMat3::MakeAxisRotation(vRevolveAxis, revolve);

    vPosOffset = mRevolve * vPosOffset;
  }

  WQuat qRot = WQuat::MakeFromEulerAngles(WAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.x + vRandR.x), WAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.y + vRandR.y), WAngle::MakeFromDegree(fStep * m_vAccumulativeRotation.z + vRandR.z));

  for (const auto& pi : duplicates)
  {
    WTransform tGlobal = pScene->GetGlobalTransform(pi.m_pObject);

    tGlobal.m_vScale.Set(1.0f);
    tGlobal.m_vPosition += vPosOffset + (1.0f + fStep) * m_vAccumulativeTranslation + vRandT;
    tGlobal.m_qRotation = qRot * tGlobal.m_qRotation;

    /// \todo Christopher: Modifying the position through a command after creating the object seems to destroy the undo-ability of this
    /// operation Duplicating multiple objects (with some translation) and then undoing that will crash the editor process

    pScene->SetGlobalTransform(pi.m_pObject, tGlobal, TransformationChanges::Translation | TransformationChanges::Rotation);
  }
}

WStatus WDuplicateObjectsCommand::UndoInternal(bool bFireEvents)
{
  W_ASSERT_DEV(bFireEvents, "This command does not support temporary commands");
  WDocument* pDocument = GetDocument();

  for (auto& po : m_DuplicatedObjects)
  {
    W_SUCCEED_OR_RETURN(pDocument->GetObjectManager()->CanRemove(po.m_pObject));

    pDocument->GetObjectManager()->RemoveObject(po.m_pObject);
  }

  return WStatus(W_SUCCESS);
}

void WDuplicateObjectsCommand::CleanupInternal(CommandState state)
{
  if (state == CommandState::WasUndone)
  {
    for (auto& po : m_DuplicatedObjects)
    {
      GetDocument()->GetObjectManager()->DestroyObject(po.m_pObject);
    }
    m_DuplicatedObjects.Clear();
  }
}
