#include <RendererCore/RendererCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Textures/DynamicTextureAtlas.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/Device.h>

WDynamicTextureAtlas::WDynamicTextureAtlas() = default;
WDynamicTextureAtlas::~WDynamicTextureAtlas()
{
  Deinitialize();
}

WResult WDynamicTextureAtlas::Initialize(const WGALTextureCreationDescription& textureDesc, WUInt32 uiAlignment /*= 16*/)
{
  if (m_hTexture.IsInvalidated() == false)
  {
    return W_FAILURE;
  }

  W_ASSERT_DEV(WMath::IsPowerOf2(uiAlignment), "Alignment must be a power of 2.");
  W_ASSERT_DEV(textureDesc.m_uiWidth % uiAlignment == 0 && textureDesc.m_uiHeight % uiAlignment == 0, "Texture width and height must be a multiple of the alignment.");
  W_ASSERT_DEV(textureDesc.m_ResourceAccess.m_bImmutable == false, "Dynamic texture atlas must be mutable.");

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  m_hTexture = pDevice->CreateTexture(textureDesc);
  if (m_hTexture.IsInvalidated())
    return W_FAILURE;

  m_uiAlignment = uiAlignment;

  AddRootNode();

  return W_SUCCESS;
}

void WDynamicTextureAtlas::Deinitialize()
{
  // Check is needed since GetDefaultDevice will assert if there is no more device set.
  if (!m_hTexture.IsInvalidated())
  {
    WGALDevice::GetDefaultDevice()->DestroyTexture(m_hTexture);
  }

  m_uiAlignment = 0;

  ClearInternal();
}

WDynamicTextureAtlas::AllocationId WDynamicTextureAtlas::Allocate(WUInt32 uiWidth, WUInt32 uiHeight, WStringView sName, WRectU16* out_pRect /*= nullptr*/)
{
  if (uiWidth == 0 || uiHeight == 0)
    return AllocationId();

  uiWidth = WMemoryUtils::AlignSize(uiWidth, m_uiAlignment);
  uiHeight = WMemoryUtils::AlignSize(uiHeight, m_uiAlignment);

  AllocationId nodeId = FindSuitableNode(uiWidth, uiHeight);
  if (nodeId.IsInvalidated())
    return AllocationId();

  Node* pNode = &m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex);
  W_ASSERT_DEV(pNode->GetType() == NodeType::Free, "Implementation error");
  const WRectU16 allocationRect = WRectU16(pNode->m_Rect.x, pNode->m_Rect.y, uiWidth, uiHeight);
  const auto currentOrientation = pNode->GetOrientation();
  const auto flippedOrientation = FlipOrientation(currentOrientation);
  const WUInt16 uiParentIndex = pNode->m_uiParentIndex;
  const WUInt16 uiNextSiblingIndex = pNode->m_uiNextSiblingIndex;

  WRectU16 splitRect, leftoverRect;
  const auto orientation = GuillotineRect(pNode->m_Rect, uiWidth, uiHeight, currentOrientation, splitRect, leftoverRect);

  pNode = nullptr;

  // Update the tree
  AllocationId allocationId;
  AllocationId splitId;
  AllocationId leftoverId;

  if (orientation == currentOrientation)
  {
    if (splitRect.HasNonZeroArea())
    {
      splitId = AllocateNode(NodeType::Free, currentOrientation, uiParentIndex, uiNextSiblingIndex, nodeId.m_InstanceIndex, splitRect);

      m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex).m_uiNextSiblingIndex = splitId.m_InstanceIndex;
      if (uiNextSiblingIndex != WSmallInvalidIndex)
      {
        m_Nodes.GetValueUnchecked(uiNextSiblingIndex).m_uiPrevSiblingIndex = splitId.m_InstanceIndex;
      }
    }

    if (leftoverRect.HasNonZeroArea())
    {
      m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex).m_uiType = NodeType::Container;

      allocationId = AllocateNode(NodeType::Allocation, flippedOrientation, nodeId.m_InstanceIndex, WSmallInvalidIndex, WSmallInvalidIndex, allocationRect);
      leftoverId = AllocateNode(NodeType::Free, flippedOrientation, nodeId.m_InstanceIndex, WSmallInvalidIndex, allocationId.m_InstanceIndex, leftoverRect);

      auto& allocationNode = m_Nodes.GetValueUnchecked(allocationId.m_InstanceIndex);
      allocationNode.m_uiNextSiblingIndex = leftoverId.m_InstanceIndex;
    }
    else
    {
      // No need to split for the leftover area, we can allocate directly in the chosen node
      allocationId = nodeId;

      auto& node = m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex);
      node.m_uiType = NodeType::Allocation;
      node.m_Rect = allocationRect;
    }
  }
  else
  {
    m_Nodes.GetValueUnchecked(nodeId.m_InstanceIndex).m_uiType = NodeType::Container;

    if (splitRect.HasNonZeroArea())
    {
      splitId = AllocateNode(NodeType::Free, flippedOrientation, nodeId.m_InstanceIndex, WSmallInvalidIndex, WSmallInvalidIndex, splitRect);
    }

    if (leftoverRect.HasNonZeroArea())
    {
      AllocationId containerId = AllocateNode(NodeType::Container, flippedOrientation, nodeId.m_InstanceIndex, splitId.m_InstanceIndex, WSmallInvalidIndex, WRectU16::MakeZero());

      auto& splitNode = m_Nodes.GetValueUnchecked(splitId.m_InstanceIndex);
      splitNode.m_uiPrevSiblingIndex = containerId.m_InstanceIndex;

      allocationId = AllocateNode(NodeType::Allocation, currentOrientation, containerId.m_InstanceIndex, WSmallInvalidIndex, WSmallInvalidIndex, allocationRect);
      leftoverId = AllocateNode(NodeType::Free, currentOrientation, containerId.m_InstanceIndex, WSmallInvalidIndex, allocationId.m_InstanceIndex, leftoverRect);

      auto& allocationNode = m_Nodes.GetValueUnchecked(allocationId.m_InstanceIndex);
      allocationNode.m_uiNextSiblingIndex = leftoverId.m_InstanceIndex;
    }
    else
    {
      allocationId = AllocateNode(NodeType::Allocation, flippedOrientation, nodeId.m_InstanceIndex, splitId.m_InstanceIndex, WSmallInvalidIndex, allocationRect);

      auto& splitNode = m_Nodes.GetValueUnchecked(splitId.m_InstanceIndex);
      splitNode.m_uiPrevSiblingIndex = allocationId.m_InstanceIndex;
    }
  }

  if (!splitId.IsInvalidated())
  {
    m_FreeList.PushBack(splitId);
  }

  if (!leftoverId.IsInvalidated())
  {
    m_FreeList.PushBack(leftoverId);
  }

  {
    auto& allocationNode = m_Nodes.GetValueUnchecked(allocationId.m_InstanceIndex);
    W_ASSERT_DEV(allocationNode.GetType() == NodeType::Allocation, "Implementation error");
    allocationNode.m_sName.Assign(sName);

    if (out_pRect != nullptr)
    {
      *out_pRect = allocationRect;
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  CheckTree();
#endif

  return allocationId;
}

void WDynamicTextureAtlas::Deallocate(AllocationId& ref_allocationId)
{
  Node* pNode = nullptr;
  if (!m_Nodes.TryGetValue(ref_allocationId, pNode))
  {
    ref_allocationId.Invalidate();
    return;
  }

  WUInt16 uiNodeIndex = ref_allocationId.m_InstanceIndex;

  W_ASSERT_DEV(pNode->GetType() == NodeType::Allocation, "Implementation error");
  pNode->m_uiType = NodeType::Free;
  pNode->m_sName.Clear();

  while (true)
  {
    // Try to merge with the next node
    {
      const WUInt16 uiNextIndex = pNode->m_uiNextSiblingIndex;
      if (uiNextIndex != WSmallInvalidIndex && m_Nodes.GetValueUnchecked(uiNextIndex).GetType() == NodeType::Free)
      {
        MergeSiblingNodes(uiNodeIndex, uiNextIndex);
      }
    }

    {
      // Try to merge with the previous node
      const WUInt16 uiPrevIndex = pNode->m_uiPrevSiblingIndex;
      if (uiPrevIndex != WSmallInvalidIndex && m_Nodes.GetValueUnchecked(uiPrevIndex).GetType() == NodeType::Free)
      {
        MergeSiblingNodes(uiPrevIndex, uiNodeIndex);
        uiNodeIndex = uiPrevIndex;
        pNode = &m_Nodes.GetValueUnchecked(uiNodeIndex);
      }
    }

    // If this node is now the only child left we collapse it into its parent and try to merge again at the parent level
    const WUInt16 uiParentIndex = pNode->m_uiParentIndex;
    if (uiParentIndex != WSmallInvalidIndex &&
        pNode->m_uiNextSiblingIndex == WSmallInvalidIndex &&
        pNode->m_uiPrevSiblingIndex == WSmallInvalidIndex)
    {
      auto& parentNode = m_Nodes.GetValueUnchecked(uiParentIndex);
      W_ASSERT_DEV(parentNode.GetType() == NodeType::Container, "Implementation error");

      // Replace the parent container with a free node
      parentNode.m_uiType = NodeType::Free;
      parentNode.m_Rect = pNode->m_Rect;

      AllocationId nodeId = m_Nodes.GetIdUnchecked(uiNodeIndex);
      W_VERIFY(m_Nodes.Remove(nodeId), "Implementation error");

      // Start again at the parent level
      uiNodeIndex = uiParentIndex;
      pNode = &m_Nodes.GetValueUnchecked(uiNodeIndex);
    }
    else
    {
      // No more merging possible, we are done
      AllocationId nodeId = m_Nodes.GetIdUnchecked(uiNodeIndex);
      if (m_FreeList.Contains(nodeId) == false)
      {
        m_FreeList.PushBack(nodeId);
      }
      break;
    }
  }

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  CheckTree();
#endif

  ref_allocationId.Invalidate();
}

void WDynamicTextureAtlas::Clear()
{
  ClearInternal();
  AddRootNode();
}

WRectU16 WDynamicTextureAtlas::GetAllocationRect(AllocationId id) const
{
  Node* pNode = nullptr;
  if (m_Nodes.TryGetValue(id, pNode))
  {
    W_ASSERT_DEV(pNode->GetType() == NodeType::Allocation, "Implementation error");
    return pNode->m_Rect;
  }

  return WRectU16::MakeZero();
}

void WDynamicTextureAtlas::DebugDraw(const WDebugRendererContext& debugContext, float fViewWidth, float fViewHeight, bool bBlackOutFreeAreas /*= true*/) const
{
  if (m_hTexture.IsInvalidated())
    return;

  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  auto& textureDesc = pDevice->GetTexture(m_hTexture)->GetDescription();

  const float fOffset = 10.0f;
  const float fScale = WMath::Min((fViewWidth * 0.75f) / textureDesc.m_uiWidth, (fViewHeight * 0.5f) / textureDesc.m_uiHeight);

  const WRectFloat atlasRect = WRectFloat(fOffset, fOffset, WMath::Round(textureDesc.m_uiWidth * fScale), WMath::Round(textureDesc.m_uiHeight * fScale));
  WDebugRenderer::Draw2DRectangle(debugContext, atlasRect, 0.0f, WColor::White, m_hTexture);

  double fUsedPixel = 0.0;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto& node = it.Value();
    if (node.GetType() == NodeType::Container)
      continue;

    const float fColorIndex = (it.Id().m_InstanceIndex & 0xF) / 16.0f;
    const WColor color = node.GetType() == NodeType::Free ? WColor(0.5f, 0.5f, 0.5f, 0.5f) : WColorScheme::LightUI(fColorIndex);
    const WRectFloat nodeRect = WRectFloat(
      WMath::Round(node.m_Rect.x * fScale + fOffset),
      WMath::Round(node.m_Rect.y * fScale + fOffset),
      WMath::Round(node.m_Rect.width * fScale) - 1.0f,
      WMath::Round(node.m_Rect.height * fScale) - 1.0f);
    WDebugRenderer::Draw2DLineRectangle(debugContext, nodeRect, 0.0f, color);

    if (bBlackOutFreeAreas && node.GetType() == NodeType::Free)
    {
      WDebugRenderer::Draw2DRectangle(debugContext, nodeRect, 0.0f, WColor::Black, WResourceManager::LoadResource<WTexture2DResource>("White.color"));
    }

    if (node.GetType() == NodeType::Allocation)
    {
      fUsedPixel += double(node.m_Rect.width) * node.m_Rect.height;
    }

    if (node.m_sName.IsEmpty() == false)
    {
      const WVec2 nodeRectCenter = nodeRect.GetCenter();
      const WVec2I32 nodeRectCenterI32 = WVec2I32(int(WMath::Round(nodeRectCenter.x)), int(WMath::Round(nodeRectCenter.y)));

      const float fTextWidth = (node.m_sName.GetView().GetElementCount() + 3) * 8.0f * WDebugRenderer::GetTextScale();
      const WUInt32 uiTextSize = WUInt32(WMath::Round(WMath::Clamp((nodeRect.width / fTextWidth) * 16.0f, 6.0f, 16.0f)));

      WDebugRenderer::Draw2DText(debugContext, node.m_sName.GetView(), nodeRectCenterI32, color, uiTextSize, WDebugTextHAlign::Center, WDebugTextVAlign::Center);
    }
  }

  const double fUtilization = fUsedPixel / (double(textureDesc.m_uiWidth) * textureDesc.m_uiHeight) * 100.0;
  const WVec2I32 pos = WVec2I32(int(fOffset), int(atlasRect.GetY2() + 8));
  WDebugRenderer::Draw2DText(debugContext, WFmt("Atlas Size: {}x{} | Utilization: {}%%", textureDesc.m_uiWidth, textureDesc.m_uiHeight, WArgF(fUtilization, 2)), pos, WColor::White);
}

WDynamicTextureAtlas::AllocationId WDynamicTextureAtlas::AllocateNode(NodeType::Enum type, Orientation::Enum orientation, WUInt16 uiParentIndex, WUInt16 uiNextSiblingIndex, WUInt16 uiPrevSiblingIndex, const WRectU16& rect)
{
  static_assert(NodeType::Count <= W_BIT(2));
  static_assert(Orientation::Count <= W_BIT(1));

  Node node;
  node.m_uiType = type;
  node.m_uiOrientation = orientation;
  node.m_uiChannelMask = 0;
  node.m_uiParentIndex = uiParentIndex;
  node.m_uiNextSiblingIndex = uiNextSiblingIndex;
  node.m_uiPrevSiblingIndex = uiPrevSiblingIndex;
  node.m_Rect = rect;

  return m_Nodes.Insert(node);
}

WDynamicTextureAtlas::AllocationId WDynamicTextureAtlas::FindSuitableNode(WUInt32 uiWidth, WUInt32 uiHeight)
{
  WUInt32 uiBestScore = WInvalidIndex;
  AllocationId bestNodeId;
  WUInt32 uiBestFreeListIndex = WInvalidIndex;

  WUInt32 i = 0;
  while (i < m_FreeList.GetCount())
  {
    const AllocationId freeId = m_FreeList[i];
    Node* pNode = nullptr;
    if (!m_Nodes.TryGetValue(freeId, pNode))
    {
      // Node was removed in the meantime, remove it from the freelist as well
      m_FreeList.RemoveAtAndSwap(i);
      continue;
    }

    if (pNode->m_Rect.width >= uiWidth && pNode->m_Rect.height >= uiHeight)
    {
      WUInt32 dx = pNode->m_Rect.width - uiWidth;
      WUInt32 dy = pNode->m_Rect.height - uiHeight;
      if (dx == 0 && dy == 0)
      {
        // perfect fit
        bestNodeId = freeId;
        uiBestFreeListIndex = i;
        break;
      }

      WUInt32 uiScore = WMath::Min(dx, dy);
      if (uiScore < uiBestScore)
      {
        uiBestScore = uiScore;
        bestNodeId = freeId;
        uiBestFreeListIndex = i;
      }
    }

    ++i;
  }

  if (!bestNodeId.IsInvalidated())
  {
    m_FreeList.RemoveAtAndSwap(uiBestFreeListIndex);
    return bestNodeId;
  }

  return AllocationId();
}

void WDynamicTextureAtlas::MergeSiblingNodes(WUInt16 uiNodeIndex, WUInt16 uiNextSiblingIndex)
{
  auto& node = m_Nodes.GetValueUnchecked(uiNodeIndex);
  auto& next = m_Nodes.GetValueUnchecked(uiNextSiblingIndex);
  W_ASSERT_DEV(node.GetType() == NodeType::Free && next.GetType() == NodeType::Free, "Implementation error");

  // Merge the two nodes into one
  node.m_Rect = WRectU16::MakeUnion(node.m_Rect, next.m_Rect);

  WUInt16 uiNextNextIndex = next.m_uiNextSiblingIndex;
  node.m_uiNextSiblingIndex = uiNextNextIndex;
  if (uiNextNextIndex != WSmallInvalidIndex)
  {
    m_Nodes.GetValueUnchecked(uiNextNextIndex).m_uiPrevSiblingIndex = uiNodeIndex;
  }

  // Remove the next node
  AllocationId nextId = m_Nodes.GetIdUnchecked(uiNextSiblingIndex);
  W_VERIFY(m_Nodes.Remove(nextId), "Implementation error");
}

WDynamicTextureAtlas::Orientation::Enum WDynamicTextureAtlas::GuillotineRect(const WRectU16& nodeRect, WUInt32 uiAllocatedWidth, WUInt32 uiAllocatedHeight, Orientation::Enum defaultOrientation, WRectU16& out_splitRect, WRectU16& out_leftoverRect)
{
  if (nodeRect.width == uiAllocatedWidth && nodeRect.height == uiAllocatedHeight)
  {
    out_splitRect = WRectU16::MakeZero();
    out_leftoverRect = WRectU16::MakeZero();
    return defaultOrientation;
  }

  // +-----------+-------------+
  // |///////////|             |
  // |/allocated/|             |
  // |///////////|    split    |
  // +-----------+             |
  // |           |             |
  // | leftover  |             |
  // |           |             |
  // +-----------+-------------+
  const WRectU16 candidateLeftoverRectToBottom = WRectU16(nodeRect.x, nodeRect.y + uiAllocatedHeight, uiAllocatedWidth, nodeRect.height - uiAllocatedHeight);
  const WUInt32 uiLeftoverAreaToBottom = WUInt32(candidateLeftoverRectToBottom.width) * candidateLeftoverRectToBottom.height;

  // +-----------+-------------+
  // |///////////|             |
  // |/allocated/|  leftover   |
  // |///////////|             |
  // +-----------+-------------+
  // |                         |
  // |          split          |
  // |                         |
  // +-------------------------+
  const WRectU16 candidateLeftoverRectToRight = WRectU16(nodeRect.x + uiAllocatedWidth, nodeRect.y, nodeRect.width - uiAllocatedWidth, uiAllocatedHeight);
  const WUInt32 uiLeftoverAreaToRight = WUInt32(candidateLeftoverRectToRight.width) * candidateLeftoverRectToRight.height;

  if (uiLeftoverAreaToRight > uiLeftoverAreaToBottom)
  {
    out_splitRect = WRectU16(nodeRect.x + uiAllocatedWidth, nodeRect.y, nodeRect.width - uiAllocatedWidth, nodeRect.height);
    out_leftoverRect = candidateLeftoverRectToBottom;
    return Orientation::Horizontal;
  }
  else
  {
    out_splitRect = WRectU16(nodeRect.x, nodeRect.y + uiAllocatedHeight, nodeRect.width, nodeRect.height - uiAllocatedHeight);
    out_leftoverRect = candidateLeftoverRectToRight;
    return Orientation::Vertical;
  }
}

void WDynamicTextureAtlas::ClearInternal()
{
  m_Nodes.Clear();
  m_FreeList.Clear();
}

void WDynamicTextureAtlas::AddRootNode()
{
  WGALDevice* pDevice = WGALDevice::GetDefaultDevice();
  auto& textureDesc = pDevice->GetTexture(m_hTexture)->GetDescription();

  AllocationId rootId = AllocateNode(NodeType::Free, Orientation::Vertical, WSmallInvalidIndex, WSmallInvalidIndex, WSmallInvalidIndex, WRectU16(0, 0, textureDesc.m_uiWidth, textureDesc.m_uiHeight));
  m_FreeList.PushBack(rootId);
}

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
void WDynamicTextureAtlas::CheckTree() const
{
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    auto& node = it.Value();

    WUInt16 uiNextSiblingIndex = node.m_uiNextSiblingIndex;
    while (uiNextSiblingIndex != WSmallInvalidIndex)
    {
      auto& siblingNode = m_Nodes.GetValueUnchecked(uiNextSiblingIndex);
      W_ASSERT_DEBUG(siblingNode.m_uiOrientation == node.m_uiOrientation, "Implementation error");
      W_ASSERT_DEBUG(siblingNode.m_uiParentIndex == node.m_uiParentIndex, "Implementation error");

      CheckNode(uiNextSiblingIndex, node.GetOrientation());

      uiNextSiblingIndex = siblingNode.m_uiNextSiblingIndex;
    }

    if (node.m_uiParentIndex != WSmallInvalidIndex)
    {
      auto& parentNode = m_Nodes.GetValueUnchecked(node.m_uiParentIndex);
      W_ASSERT_DEBUG(parentNode.GetType() == NodeType::Container, "Implementation error");
      W_ASSERT_DEBUG(parentNode.GetOrientation() == FlipOrientation(node.GetOrientation()), "Implementation error");
    }
  }
}

void WDynamicTextureAtlas::CheckNode(WUInt16 uiNodexIndex, Orientation::Enum parentOrientation) const
{
  auto& node = m_Nodes.GetValueUnchecked(uiNodexIndex);
  if (node.m_uiNextSiblingIndex == WSmallInvalidIndex)
    return;

  auto& nextNode = m_Nodes.GetValueUnchecked(node.m_uiNextSiblingIndex);
  W_ASSERT_DEBUG(nextNode.m_uiPrevSiblingIndex == uiNodexIndex, "Implementation error");

  if (node.GetType() == NodeType::Container)
    return;

  auto& r1 = node.m_Rect;
  auto& r2 = nextNode.m_Rect;
  if (parentOrientation == Orientation::Horizontal)
  {
    W_ASSERT_DEBUG(r1.y == r2.y, "Implementation error");
    W_ASSERT_DEBUG(r1.GetY2() == r2.GetY2(), "Implementation error");
  }
  else
  {
    W_ASSERT_DEBUG(r1.x == r2.x, "Implementation error");
    W_ASSERT_DEBUG(r1.GetX2() == r2.GetX2(), "Implementation error");
  }
}
#endif
