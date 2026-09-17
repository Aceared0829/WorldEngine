#pragma once

#include <Foundation/Containers/IdTable.h>
#include <Foundation/Math/Rect.h>
#include <RendererCore/Debug/DebugRendererContext.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct WGALTextureCreationDescription;

/// Manages dynamic allocation of rectangular regions within a GPU texture.
///
/// Uses a binary tree structure with guillotine algorithm for efficient 2D bin packing.
/// Useful for atlasing frequently updated textures like runtime generated decals, fonts or UI elements.
/// Supports alignment requirements and can visualize allocations for debugging.
class W_RENDERERCORE_DLL WDynamicTextureAtlas
{
public:
  using AllocationId = WGenericId<16, 8>;

  WDynamicTextureAtlas();
  ~WDynamicTextureAtlas();

  /// Initializes the atlas with the given texture description and alignment.
  ///
  /// The texture must not be immutable. Width and height must be multiples of alignment.
  /// Alignment must be a power of 2. Returns failure if already initialized.
  WResult Initialize(const WGALTextureCreationDescription& textureDesc, WUInt32 uiAlignment = 16);

  /// Destroys the GPU texture and clears all allocations.
  void Deinitialize();

  bool IsInitialized() const { return m_hTexture.IsInvalidated() == false; }

  /// Allocates a rectangular region of the specified size.
  ///
  /// Dimensions are aligned up to the atlas alignment. Returns invalid ID if the allocation fails
  /// due to insufficient space. The name is used for debugging visualization.
  AllocationId Allocate(WUInt32 uiWidth, WUInt32 uiHeight, WStringView sName, WRectU16* out_pRect = nullptr);

  /// Deallocates a previously allocated region and invalidates the ID.
  void Deallocate(AllocationId& ref_allocationId);

  /// Clears all allocations without destroying the texture.
  void Clear();

  WGALTextureHandle GetTexture() const { return m_hTexture; }

  /// Returns the rectangle for a given allocation ID.
  WRectU16 GetAllocationRect(AllocationId id) const;

  /// Renders a debug visualization of the atlas layout.
  ///
  /// Shows allocated regions with their names.
  /// If bBlackOutFreeAreas is true, free space is rendered as black rectangles.
  void DebugDraw(const WDebugRendererContext& debugContext, float fViewWidth, float fViewHeight, bool bBlackOutFreeAreas = true) const;

private:
  struct NodeType
  {
    enum Enum
    {
      Container,
      Allocation,
      Free,

      Count,
    };
  };

  struct Orientation
  {
    enum Enum
    {
      Horizontal,
      Vertical,

      Count,
    };
  };

  W_ALWAYS_INLINE static Orientation::Enum FlipOrientation(Orientation::Enum orientation)
  {
    return (orientation == Orientation::Horizontal) ? Orientation::Vertical : Orientation::Horizontal;
  }

  AllocationId AllocateNode(NodeType::Enum type, Orientation::Enum orientation, WUInt16 uiParentIndex, WUInt16 uiNextSiblingIndex, WUInt16 uiPrevSiblingIndex, const WRectU16& rect);
  AllocationId FindSuitableNode(WUInt32 uiWidth, WUInt32 uiHeight);
  void MergeSiblingNodes(WUInt16 uiNodeIndex, WUInt16 uiNextSiblingIndex);
  Orientation::Enum GuillotineRect(const WRectU16& nodeRect, WUInt32 uiAllocatedWidth, WUInt32 uiAllocatedHeight, Orientation::Enum defaultOrientation, WRectU16& out_splitRect, WRectU16& out_leftoverRect);

  void ClearInternal();
  void AddRootNode();

  WGALTextureHandle m_hTexture;
  WUInt32 m_uiAlignment = 0;

  struct Node
  {
    WHashedString m_sName;
    WUInt8 m_uiType : 2;
    WUInt8 m_uiOrientation : 1;
    WUInt8 m_uiChannelMask = 0; // Not used yet
    WUInt16 m_uiParentIndex = WSmallInvalidIndex;
    WUInt16 m_uiNextSiblingIndex = WSmallInvalidIndex;
    WUInt16 m_uiPrevSiblingIndex = WSmallInvalidIndex;
    WRectU16 m_Rect;

    W_ALWAYS_INLINE NodeType::Enum GetType() const { return static_cast<NodeType::Enum>(m_uiType); }
    W_ALWAYS_INLINE Orientation::Enum GetOrientation() const { return static_cast<Orientation::Enum>(m_uiOrientation); }
  };

  WIdTable<AllocationId, Node> m_Nodes;
  WDynamicArray<AllocationId> m_FreeList;

#if W_ENABLED(W_COMPILE_FOR_DEBUG)
  void CheckTree() const;
  void CheckNode(WUInt16 uiNodeIndex, Orientation::Enum parentOrientation) const;
#endif
};
