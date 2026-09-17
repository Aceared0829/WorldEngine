#include <RendererTest/RendererTestPCH.h>

#include "DynamicTextureAtlasTest.h"
#include <Foundation/SimdMath/SimdRandom.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

static WGameEngineTestDynamicTextureAtlas g_DynamicTextureAtlasTest;

const char* WGameEngineTestDynamicTextureAtlas::GetTestName() const
{
  return "DynamicTextureAtlas Tests";
}

WGameEngineTestApplication* WGameEngineTestDynamicTextureAtlas::CreateApplication()
{
  m_pOwnApplication = W_DEFAULT_NEW(WGameEngineTestApplication, "DynamicTextureAtlas");
  return m_pOwnApplication;
}

void WGameEngineTestDynamicTextureAtlas::SetupSubTests()
{
  AddSubTest("Allocations (Small)", SubTests::ST_AllocationsSmall);
  AddSubTest("Allocations (Large)", SubTests::ST_AllocationsLarge);
  AddSubTest("Allocations (Mixed)", SubTests::ST_AllocationsMixed);
  AddSubTest("Deallocations", SubTests::ST_Deallocations);
  AddSubTest("Deallocations 2", SubTests::ST_Deallocations2);
}

WResult WGameEngineTestDynamicTextureAtlas::InitializeSubTest(WInt32 iIdentifier)
{
  struct AllocInfo
  {
    WUInt32 m_uiWidth;
    WUInt32 m_uiHeight;
    const char* m_szName;
  };

  m_iFrame = -1;

  WGALTextureCreationDescription desc;
  desc.m_Format = WGALResourceFormat::RUByteNormalized;
  desc.m_ResourceAccess.m_bImmutable = false;

  if (iIdentifier == SubTests::ST_AllocationsSmall)
  {
    {
      desc.m_uiWidth = 512 + 256;
      desc.m_uiHeight = 512;

      W_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr WUInt32 uiSize = 32;

    WStringBuilder sb;
    const WUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);
    for (WUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      W_TEST_BOOL(!id.IsInvalidated());
    }

    // Should be full now
    {
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, "Invalid");
      W_TEST_BOOL(id.IsInvalidated());
    }
  }
  else if (iIdentifier == SubTests::ST_AllocationsLarge)
  {
    {
      desc.m_uiWidth = 512 + 256;
      desc.m_uiHeight = 512 + 256;

      W_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr WUInt32 uiSize = 256;

    WStringBuilder sb;
    const WUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);
    for (WUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      W_TEST_BOOL(!id.IsInvalidated());
    }

    // Should be full now
    {
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, "Invalid");
      W_TEST_BOOL(id.IsInvalidated());
    }
  }
  else if (iIdentifier == SubTests::ST_AllocationsMixed)
  {
    {
      desc.m_uiWidth = 512;
      desc.m_uiHeight = 512;

      W_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    {
      auto id = m_TextureAtlas.Allocate(1024, 256, "Invalid");
      W_TEST_BOOL(id.IsInvalidated());
    }

    AllocInfo allocInfos[] = {
      {256, 256, "L1"},
      {128, 128, "M1"},
      {128, 128, "M2"},
      {128, 128, "M3"},
      {128, 64, "HM"},
      {64, 128, "VM"},
      {64, 64, "S1"},
      {64, 64, "S2"},
      {64, 64, "S3"},
      {64, 256, "VVS"},
      {256, 256, "L2"},
    };

    for (auto& a : allocInfos)
    {
      auto id = m_TextureAtlas.Allocate(a.m_uiWidth, a.m_uiHeight, a.m_szName);
      W_TEST_BOOL(!id.IsInvalidated());
    }

    {
      auto id = m_TextureAtlas.Allocate(256, 256, "Invalid");
      W_TEST_BOOL(id.IsInvalidated());
    }
  }
  else if (iIdentifier == SubTests::ST_Deallocations)
  {
    {
      desc.m_uiWidth = 512;
      desc.m_uiHeight = 512;

      W_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr WUInt32 uiSize = 128;

    WStringBuilder sb;
    const WUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);

    WDynamicArray<WDynamicTextureAtlas::AllocationId> allocations;
    allocations.Reserve(uiNumAllocations);

    for (WUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      W_TEST_BOOL(!id.IsInvalidated());
      allocations.PushBack(id);
    }

    const WUInt32 uiNumDeallocations = uiNumAllocations;

    for (WUInt32 i = 0; i < uiNumDeallocations; ++i)
    {
      WUInt32 allocationIndex = WSimdRandom::UInt(WSimdVec4i(i)).x() % allocations.GetCount();
      auto& id = allocations[allocationIndex];

      m_TextureAtlas.Deallocate(id);
      W_TEST_BOOL(id.IsInvalidated());

      allocations.RemoveAtAndSwap(allocationIndex);
    }

    // Atlas should be empty again at this point
    auto id = m_TextureAtlas.Allocate(512, 512, "A2");
    W_TEST_BOOL(!id.IsInvalidated());
  }
  else if (iIdentifier == SubTests::ST_Deallocations2)
  {
    {
      desc.m_uiWidth = 512;
      desc.m_uiHeight = 512;

      W_SUCCEED_OR_RETURN(m_TextureAtlas.Initialize(desc));
    }

    constexpr WUInt32 uiSize = 32;

    WStringBuilder sb;
    const WUInt32 uiNumAllocations = (desc.m_uiWidth / uiSize) * (desc.m_uiHeight / uiSize);

    WDynamicArray<WDynamicTextureAtlas::AllocationId> allocations;
    allocations.Reserve(uiNumAllocations);

    for (WUInt32 i = 0; i < uiNumAllocations; ++i)
    {
      sb.SetFormat("A{0}", i);
      auto id = m_TextureAtlas.Allocate(uiSize, uiSize, sb);
      W_TEST_BOOL(!id.IsInvalidated());
      allocations.PushBack(id);
    }

    const WUInt32 uiNumDeallocations = uiNumAllocations / 2;

    for (WUInt32 i = 0; i < uiNumDeallocations; ++i)
    {
      WUInt32 allocationIndex = WSimdRandom::UInt(WSimdVec4i(i)).x() % allocations.GetCount();
      auto& id = allocations[allocationIndex];

      m_TextureAtlas.Deallocate(id);
      W_TEST_BOOL(id.IsInvalidated());

      allocations.RemoveAtAndSwap(allocationIndex);
    }

    AllocInfo allocInfos[] = {
      {64, 32, "N1"},
      {64, 32, "N2"},
      {32, 64, "N3"},
      {128, 32, "N4"},
    };

    for (auto& a : allocInfos)
    {
      auto id = m_TextureAtlas.Allocate(a.m_uiWidth, a.m_uiHeight, a.m_szName);
      W_TEST_BOOL(!id.IsInvalidated());
    }
  }

  return W_SUCCESS;
}

WResult WGameEngineTestDynamicTextureAtlas::DeInitializeSubTest(WInt32 iIdentifier)
{
  m_TextureAtlas.Deinitialize();

  return W_SUCCESS;
}

WTestAppRun WGameEngineTestDynamicTextureAtlas::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  const bool bVulkan = WGameApplication::GetActiveRenderer().IsEqual_NoCase("Vulkan");
  ++m_iFrame;

  WView* pView = WRenderWorld::GetViewByUsageHint(WCameraUsageHint::MainView);
  auto& viewport = pView->GetViewport();

  m_TextureAtlas.DebugDraw(pView->GetHandle(), viewport.width, viewport.height);

  m_pOwnApplication->Run();
  if (m_pOwnApplication->ShouldApplicationQuit())
  {
    return WTestAppRun::Quit;
  }

  if (m_iFrame == 1)
  {
    W_TEST_IMAGE(0, bVulkan ? 300 : 250);

    return WTestAppRun::Quit;
  }

  return WTestAppRun::Continue;
}
