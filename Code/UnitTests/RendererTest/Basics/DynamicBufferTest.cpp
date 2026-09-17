#include <RendererTest/RendererTestPCH.h>

#include <RendererTest/Basics/DynamicBufferTest.h>

#include <RendererFoundation/Resources/DynamicBuffer.h>

void WRendererTestDynamicBuffer::SetupSubTests()
{
  AddSubTest("Allocations", SubTests::ST_Allocations);
  AddSubTest("Deallocations", SubTests::ST_Deallocations);
  AddSubTest("Compaction", SubTests::ST_Compaction);
  AddSubTest("Resize While Mapped", SubTests::ST_ResizeWhileMapped);
}

WResult WRendererTestDynamicBuffer::InitializeSubTest(WInt32 iIdentifier)
{
  W_SUCCEED_OR_RETURN(WGraphicsTest::InitializeSubTest(iIdentifier));

  m_iFrame = -1;

  WGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(WUInt64);
  desc.m_uiTotalSize = desc.m_uiStructSize * 16;
  desc.m_BufferFlags = WGALBufferUsageFlags::StructuredBuffer | WGALBufferUsageFlags::ShaderResource;

  m_hDynamicBuffer = WGALDevice::GetDefaultDevice()->CreateDynamicBuffer(desc, "Test buffer");

  return W_SUCCESS;
}

WResult WRendererTestDynamicBuffer::DeInitializeSubTest(WInt32 iIdentifier)
{
  WGALDevice::GetDefaultDevice()->DestroyDynamicBuffer(m_hDynamicBuffer);

  if (WGraphicsTest::DeInitializeSubTest(iIdentifier).Failed())
    return W_FAILURE;

  return W_SUCCESS;
}

WTestAppRun WRendererTestDynamicBuffer::RunSubTest(WInt32 iIdentifier, WUInt32 uiInvocationCount)
{
  if (iIdentifier == SubTests::ST_Allocations)
  {
    constexpr WUInt32 uiAllocationSizes[] = {11, 23, 4, 5, 6, 7, 8, 9};

    auto pDevice = WGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    WUInt32 uiTotalOffset = 0;
    auto AllocateAndMap = [&](WUInt32 uiIndex)
    {
      WUInt32 uiOffset = pDynamicBuffer->Allocate(uiIndex, uiAllocationSizes[uiIndex]);
      W_TEST_INT(uiOffset, uiTotalOffset);

      auto pMappedData = pDynamicBuffer->MapForWriting<WUInt64>(uiOffset);
      W_TEST_INT(pMappedData.GetCount(), uiAllocationSizes[uiIndex]);

      uiTotalOffset += uiAllocationSizes[uiIndex];
    };

    AllocateAndMap(0);

    pDynamicBuffer->UploadChangesForNextFrame();

    // Internal buffer should have been created after upload but GetBufferForRendering should still return the old handle until the next frame.
    WGALBufferHandle hBuffer = pDynamicBuffer->GetBufferForRendering();
    W_TEST_BOOL(hBuffer.IsInvalidated());

    {
      BeginFrame();

      // After one frame the buffer should be created
      WGALBufferHandle hNewBuffer = pDynamicBuffer->GetBufferForRendering();
      W_TEST_BOOL(hNewBuffer.IsInvalidated() == false);
      hBuffer = hNewBuffer;

      EndFrame();
    }

    for (WUInt32 i = 1; i < W_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      AllocateAndMap(i);
    }

    pDynamicBuffer->UploadChangesForNextFrame();

    WGALBufferHandle hNewBuffer = pDynamicBuffer->GetBufferForRendering();
    W_TEST_BOOL(hNewBuffer.IsInvalidated() == false);

    // The internal buffer has been re-allocated but GetBufferForRendering should still return the old handle until the next frame.
    W_TEST_BOOL(hNewBuffer == hBuffer);

    {
      BeginFrame();

      // Now the new buffer should be returned
      hNewBuffer = pDynamicBuffer->GetBufferForRendering();
      W_TEST_BOOL(hNewBuffer != hBuffer);

      EndFrame();
    }
  }
  else if (iIdentifier == SubTests::ST_Deallocations)
  {
    constexpr WUInt32 uiAllocationSizes[] = {3, 4, 5, 6, 7, 9, 11, 13};

    auto pDevice = WGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    WTempHybridArray<WUInt32, 16> offsets;
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      offsets.PushBack(pDynamicBuffer->Allocate(i, uiAllocationSizes[i]));
    }

    // Create some holes out of order
    pDynamicBuffer->Deallocate(offsets[1]);
    pDynamicBuffer->Deallocate(offsets[5]);
    pDynamicBuffer->Deallocate(offsets[3]);

    // Should try to waste as little space as possible so this allocation should go into the middle hole with size 6.
    WUInt32 uiNewOffset = pDynamicBuffer->Allocate(100, 5);
    W_TEST_INT(uiNewOffset, offsets[3]);

    pDynamicBuffer->Deallocate(uiNewOffset);

    // Should merge all holes into one big hole
    pDynamicBuffer->Deallocate(offsets[4]);
    pDynamicBuffer->Deallocate(offsets[2]);

    // Test whether the merging was done correctly by allocation something almost as big as the hole
    uiNewOffset = pDynamicBuffer->Allocate(101, 30);
    W_TEST_INT(uiNewOffset, offsets[1]);

    // Deallocate the last allocation
    pDynamicBuffer->Deallocate(offsets[7]);

    uiNewOffset = pDynamicBuffer->Allocate(102, 100);
    W_TEST_INT(uiNewOffset, offsets[7]);
  }
  else if (iIdentifier == SubTests::ST_Compaction)
  {
    constexpr WUInt32 uiAllocationSizes[] = {15, 14, 13, 11, 7, 9, 15, 14};

    auto pDevice = WGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    WTempHybridArray<WUInt32, 16> offsets;
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      offsets.PushBack(pDynamicBuffer->Allocate(i, uiAllocationSizes[i]));
    }

    // Upload changes to get rid of any temporary data
    pDynamicBuffer->UploadChangesForNextFrame();

    // Create some holes out of order
    pDynamicBuffer->Deallocate(offsets[5]);
    pDynamicBuffer->Deallocate(offsets[1]);
    pDynamicBuffer->Deallocate(offsets[3]);

    WTempHybridArray<WGALDynamicBuffer::ChangedAllocation, 16> changedAllocations;

    // The first compaction step should move the last allocation into the first hole
    {
      pDynamicBuffer->RunCompactionSteps(changedAllocations, 1);

      W_TEST_INT(changedAllocations.GetCount(), 1);
      W_TEST_INT(changedAllocations[0].m_uiUserData, 7);
      W_TEST_INT(changedAllocations[0].m_uiNewOffset, offsets[1]);
    }

    // For the next steps the last allocation does not match the hole size so it should start to move allocations forward to close the hole
    {
      pDynamicBuffer->RunCompactionSteps(changedAllocations, 16);

      W_TEST_INT(changedAllocations.GetCount(), 2);
      W_TEST_INT(changedAllocations[0].m_uiUserData, 4);
      W_TEST_INT(changedAllocations[0].m_uiNewOffset, offsets[3]);
      W_TEST_INT(changedAllocations[1].m_uiUserData, 6);
      W_TEST_INT(changedAllocations[1].m_uiNewOffset, offsets[3] + uiAllocationSizes[4]);
    }

    pDynamicBuffer->Clear();
    offsets.Clear();

    for (WUInt32 i = 0; i < W_ARRAY_SIZE(uiAllocationSizes); ++i)
    {
      offsets.PushBack(pDynamicBuffer->Allocate(i, uiAllocationSizes[i]));
    }

    pDynamicBuffer->Deallocate(offsets[6]);
    pDynamicBuffer->Deallocate(offsets[7]);

    // Compaction should have already happened in deallocate
    {
      pDynamicBuffer->RunCompactionSteps(changedAllocations, 16);
      W_TEST_INT(changedAllocations.GetCount(), 0);
    }

    WUInt32 uiOffset = pDynamicBuffer->Allocate(400, 20);
    W_TEST_INT(uiOffset, offsets[6]);
  }
  else if (iIdentifier == SubTests::ST_ResizeWhileMapped)
  {
    auto pDevice = WGALDevice::GetDefaultDevice();
    auto pDynamicBuffer = pDevice->GetDynamicBuffer(m_hDynamicBuffer);

    WTempHybridArray<WUInt32, 16> offsets;
    WTempHybridArray<WArrayPtr<WUInt64>, 16> mappings;
    offsets.SetCount(6);
    mappings.SetCount(6);

    offsets[0] = pDynamicBuffer->Allocate(0, 7);
    offsets[1] = pDynamicBuffer->Allocate(1, 7);
    mappings[0] = pDynamicBuffer->MapForWriting<WUInt64>(offsets[0]);
    mappings[1] = pDynamicBuffer->MapForWriting<WUInt64>(offsets[1]);

    // The next allocation will resize the internal buffer
    offsets[2] = pDynamicBuffer->Allocate(2, 120);
    offsets[3] = pDynamicBuffer->Allocate(3, 120);
    mappings[2] = pDynamicBuffer->MapForWriting<WUInt64>(offsets[2]);
    mappings[3] = pDynamicBuffer->MapForWriting<WUInt64>(offsets[3]);

    // The next allocation will resize the internal buffer again
    offsets[4] = pDynamicBuffer->Allocate(4, 120);
    offsets[5] = pDynamicBuffer->Allocate(5, 138);
    mappings[4] = pDynamicBuffer->MapForWriting<WUInt64>(offsets[4]);
    mappings[5] = pDynamicBuffer->MapForWriting<WUInt64>(offsets[5]);

    for (WUInt32 i = 0; i < mappings.GetCount(); ++i)
    {
      for (auto& v : mappings[i])
      {
        v = i + 1;
      }
    }

    for (WUInt32 i = 0; i < mappings.GetCount(); ++i)
    {
      // Reading will give us temp memory until the next upload
      auto data = pDynamicBuffer->MapForReading<WUInt64>(offsets[i]);
      W_TEST_INT(data.GetCount(), mappings[i].GetCount());
      for (auto& v : data)
      {
        W_TEST_INT(v, i + 1);
      }
    }

    pDynamicBuffer->UploadChangesForNextFrame();

    const WUInt64* pBasePtr = nullptr;
    for (WUInt32 i = 0; i < mappings.GetCount(); ++i)
    {
      auto data = pDynamicBuffer->MapForReading<WUInt64>(offsets[i]);
      W_TEST_INT(data.GetCount(), mappings[i].GetCount());

      if (i == 0)
      {
        W_TEST_INT(offsets[i], 0);
        pBasePtr = data.GetPtr();
      }
      else
      {
        // Memory should be contiguous now
        W_TEST_BOOL(data.GetPtr() == pBasePtr + offsets[i]);
      }

      for (auto& v : data)
      {
        W_TEST_INT(v, i + 1);
      }
    }
  }

  return WTestAppRun::Quit;
}

static WRendererTestDynamicBuffer g_DynamicBufferTest;
