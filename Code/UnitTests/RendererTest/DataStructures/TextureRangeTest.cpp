#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

W_CREATE_SIMPLE_RENDERER_TEST(DataStructures, WGALTextureRange)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Default constructed range represents all")
  {
    WGALTextureRange range;
    W_TEST_INT(range.m_uiBaseArraySlice, 0);
    W_TEST_INT(range.m_uiArraySlices, W_GAL_ALL_ARRAY_SLICES);
    W_TEST_INT(range.m_uiBaseMipLevel, 0);
    W_TEST_INT(range.m_uiMipLevels, W_GAL_ALL_MIP_LEVELS);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromMipRange")
  {
    WGALTextureRange range = WGALTextureRange::MakeFromMipRange(2, 3);
    W_TEST_INT(range.m_uiBaseArraySlice, 0);
    W_TEST_INT(range.m_uiArraySlices, 1);
    W_TEST_INT(range.m_uiBaseMipLevel, 2);
    W_TEST_INT(range.m_uiMipLevels, 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromRenderTargetRange")
  {
    WGALRenderTargetRange rtRange;
    rtRange.m_uiBaseArraySlice = 3;
    rtRange.m_uiArraySlices = 2;
    rtRange.m_uiBaseMipLevel = 1;

    WGALTextureRange range = WGALTextureRange::MakeFromRenderTargetRange(rtRange);
    W_TEST_INT(range.m_uiBaseArraySlice, 3);
    W_TEST_INT(range.m_uiArraySlices, 2);
    W_TEST_INT(range.m_uiBaseMipLevel, 1);
    W_TEST_INT(range.m_uiMipLevels, 1);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "MakeFromMipLevel for RenderTargetRange")
  {
    WGALRenderTargetRange rtRange = WGALRenderTargetRange::MakeFromMipLevel(3);
    W_TEST_INT(rtRange.m_uiBaseArraySlice, 0);
    W_TEST_INT(rtRange.m_uiArraySlices, W_GAL_ALL_ARRAY_SLICES);
    W_TEST_INT(rtRange.m_uiBaseMipLevel, 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Equality operators")
  {
    WGALTextureRange a = {0, 1, 0, 1};
    WGALTextureRange b = {0, 1, 0, 1};
    WGALTextureRange c = {1, 1, 0, 1};

    W_TEST_BOOL(a == b);
    W_TEST_BOOL(!(a != b));
    W_TEST_BOOL(a != c);
    W_TEST_BOOL(!(a == c));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Multi-mip multi-layer")
  {
    WGALTextureRange fullRange = {0, 3, 0, 4};

    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(0, 0, fullRange), 0);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(1, 0, fullRange), 1);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(3, 0, fullRange), 3);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(0, 1, fullRange), 4);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(2, 1, fullRange), 6);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(0, 2, fullRange), 8);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(3, 2, fullRange), 11);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Single-mip texture")
  {
    WGALTextureRange singleMip = {0, 5, 0, 1};
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(0, 0, singleMip), 0);
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(0, 3, singleMip), 3);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Single-layer texture")
  {
    WGALTextureRange singleLayer = {0, 1, 0, 8};
    W_TEST_INT(WGALTextureRange::ComputeSubResourceIndex(5, 0, singleLayer), 5);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Identical ranges")
  {
    WGALTextureRange a = {0, 2, 0, 3};
    W_TEST_BOOL(a.Overlaps(a));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Fully contained")
  {
    WGALTextureRange outer = {0, 4, 0, 4};
    WGALTextureRange inner = {1, 2, 1, 2};
    W_TEST_BOOL(outer.Overlaps(inner));
    W_TEST_BOOL(inner.Overlaps(outer));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Partial overlap in both dimensions")
  {
    WGALTextureRange a = {0, 3, 0, 3};
    WGALTextureRange b = {2, 3, 2, 3};
    W_TEST_BOOL(a.Overlaps(b));
    W_TEST_BOOL(b.Overlaps(a));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Adjacent slices do not overlap")
  {
    WGALTextureRange a = {0, 2, 0, 4};
    WGALTextureRange b = {2, 2, 0, 4};
    W_TEST_BOOL(!a.Overlaps(b));
    W_TEST_BOOL(!b.Overlaps(a));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Adjacent mips do not overlap")
  {
    WGALTextureRange a = {0, 4, 0, 2};
    WGALTextureRange b = {0, 4, 2, 2};
    W_TEST_BOOL(!a.Overlaps(b));
    W_TEST_BOOL(!b.Overlaps(a));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Disjoint slices with overlapping mips")
  {
    WGALTextureRange a = {0, 2, 0, 4};
    WGALTextureRange b = {3, 2, 1, 2};
    W_TEST_BOOL(!a.Overlaps(b));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Overlapping slices with disjoint mips")
  {
    WGALTextureRange a = {0, 4, 0, 2};
    WGALTextureRange b = {1, 2, 3, 2};
    W_TEST_BOOL(!a.Overlaps(b));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Single sub-resource vs single sub-resource")
  {
    WGALTextureRange a = {1, 1, 2, 1};
    WGALTextureRange same = {1, 1, 2, 1};
    WGALTextureRange diffSlice = {2, 1, 2, 1};
    WGALTextureRange diffMip = {1, 1, 3, 1};

    W_TEST_BOOL(a.Overlaps(same));
    W_TEST_BOOL(!a.Overlaps(diffSlice));
    W_TEST_BOOL(!a.Overlaps(diffMip));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Single sub-resource vs wide range")
  {
    WGALTextureRange wide = {0, 8, 0, 8};
    WGALTextureRange single = {3, 1, 5, 1};
    W_TEST_BOOL(wide.Overlaps(single));
    W_TEST_BOOL(single.Overlaps(wide));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Default range overlaps with everything")
  {
    WGALTextureRange fullRange;
    W_TEST_BOOL(fullRange.Overlaps(fullRange));

    WGALTextureRange origin = {0, 1, 0, 1};
    W_TEST_BOOL(fullRange.Overlaps(origin));
    W_TEST_BOOL(origin.Overlaps(fullRange));

    WGALTextureRange high = {100, 1, 50, 1};
    W_TEST_BOOL(fullRange.Overlaps(high));
    W_TEST_BOOL(high.Overlaps(fullRange));

    WGALTextureRange maxSingle = {W_GAL_ALL_ARRAY_SLICES - 1, 1, W_GAL_ALL_MIP_LEVELS - 1, 1};
    W_TEST_BOOL(fullRange.Overlaps(maxSingle));
    W_TEST_BOOL(maxSingle.Overlaps(fullRange));

    WGALTextureRange shiftedFull = {5, W_GAL_ALL_ARRAY_SLICES, 3, W_GAL_ALL_MIP_LEVELS};
    W_TEST_BOOL(fullRange.Overlaps(shiftedFull));
    W_TEST_BOOL(shiftedFull.Overlaps(fullRange));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Sentinel values with non-zero base (overflow edge cases)")
  {
    WGALTextureRange a = {1, W_GAL_ALL_ARRAY_SLICES, 0, 1};
    WGALTextureRange b = {0, 1, 0, 1};
    W_TEST_BOOL(!a.Overlaps(b));
    W_TEST_BOOL(!b.Overlaps(a));

    WGALTextureRange c = {1, 1, 0, 1};
    W_TEST_BOOL(a.Overlaps(c));

    WGALTextureRange d = {0, 1, 1, W_GAL_ALL_MIP_LEVELS};
    WGALTextureRange e = {0, 1, 0, 1};
    W_TEST_BOOL(!d.Overlaps(e));
    W_TEST_BOOL(!e.Overlaps(d));

    WGALTextureRange f = {0, 1, 1, 1};
    W_TEST_BOOL(d.Overlaps(f));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Two shifted sentinel ranges overlap")
  {
    WGALTextureRange a = {10, W_GAL_ALL_ARRAY_SLICES, 5, W_GAL_ALL_MIP_LEVELS};
    WGALTextureRange b = {20, W_GAL_ALL_ARRAY_SLICES, 10, W_GAL_ALL_MIP_LEVELS};
    W_TEST_BOOL(a.Overlaps(b));
    W_TEST_BOOL(b.Overlaps(a));
  }
}
