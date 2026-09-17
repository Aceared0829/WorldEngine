#include <RendererTest/RendererTestPCH.h>

#include <RendererFoundation/Utils/RingBufferTracker.h>
#include <RendererTest/TestClass/SimpleRendererTest.h>

W_CREATE_SIMPLE_RENDERER_TEST_GROUP(DataStructures)

W_CREATE_SIMPLE_RENDERER_TEST(DataStructures, RingBufferTracker)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Allocate entire buffer")
  {
    WRingBufferTracker tracker(64, 65472);
    W_TEST_INT(tracker.GetTotalMemory(), 65472);
    W_TEST_INT(tracker.GetUsedMemory(), 0);
    W_TEST_INT(tracker.GetFreeMemory(), 65472);

    WUInt32 uiOffset = 0;
    W_TEST_RESULT(tracker.CanAllocate(65472));
    W_TEST_RESULT(tracker.Allocate(65472, 1, uiOffset));
    W_TEST_INT(uiOffset, 0);
    W_TEST_INT(tracker.GetTotalMemory(), 65472);
    W_TEST_INT(tracker.GetUsedMemory(), 65472);
    W_TEST_INT(tracker.GetFreeMemory(), 0);
  }
  W_TEST_BLOCK(WTestBlock::Enabled, "Allocate, submit, free")
  {
    WRingBufferTracker tracker(64, 1024);
    WUInt32 uiOffset = 0;

    W_TEST_RESULT(tracker.CanAllocate(256));
    W_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    W_TEST_INT(uiOffset, 0);
    W_TEST_RESULT(tracker.CanAllocate(256));
    W_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    W_TEST_INT(uiOffset, 256);
    W_TEST_RESULT(tracker.CanAllocate(256));
    W_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    W_TEST_INT(uiOffset, 512);
    W_TEST_RESULT(tracker.CanAllocate(256));
    W_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    W_TEST_INT(uiOffset, 768);
    W_TEST_BOOL(tracker.CanAllocate(256).Failed());
    W_TEST_BOOL(tracker.Allocate(256, 1, uiOffset).Failed());
    W_TEST_INT(tracker.GetFreeMemory(), 0);
    W_TEST_INT(tracker.GetUsedMemory(), 1024);

    WTempHybridArray<WRingBufferTracker::FrameData, 2> frames;
    W_TEST_RESULT(tracker.SubmitFrame(1, frames));
    W_TEST_INT(frames.GetCount(), 1);
    W_TEST_INT(frames[0].m_uiFrame, 1);
    W_TEST_INT(frames[0].m_uiStartOffset, 0);
    W_TEST_INT(frames[0].m_uiSize, 1024);

    tracker.Free(1);
    W_TEST_INT(tracker.GetFreeMemory(), 1024);
    W_TEST_INT(tracker.GetUsedMemory(), 0);
    W_TEST_RESULT(tracker.CanAllocate(256));
    W_TEST_RESULT(tracker.Allocate(256, 1, uiOffset));
    W_TEST_INT(uiOffset, 0);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Loop through the ringbuffer")
  {
    WRingBufferTracker tracker(64, 1024);
    WUInt32 uiOffset = 0;
    for (size_t i = 4; i < 20; i++)
    {
      W_TEST_INT(tracker.GetTotalMemory(), 1024);
      tracker.Free(i - 4);
      W_TEST_RESULT(tracker.CanAllocate(256));
      W_TEST_RESULT(tracker.Allocate(256, i, uiOffset));
      W_TEST_INT(uiOffset, (i * 256) % 1024);
      WTempHybridArray<WRingBufferTracker::FrameData, 2> frames;
      W_TEST_RESULT(tracker.SubmitFrame(i, frames));
      W_TEST_INT(frames.GetCount(), 1);
      W_TEST_INT(frames[0].m_uiFrame, i);
      W_TEST_INT(frames[0].m_uiStartOffset, (i * 256) % 1024);
      W_TEST_INT(frames[0].m_uiSize, 256);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Buffer wrap around")
  {
    WRingBufferTracker tracker(2, 1000);
    WUInt32 uiOffset = 0;

    // Frame 1: Move a bit forward so we are at the end of the buffer in the next frame.
    W_TEST_RESULT(tracker.CanAllocate(800));
    W_TEST_RESULT(tracker.Allocate(800, 1, uiOffset));
    W_TEST_INT(uiOffset, 0);
    W_TEST_INT(tracker.GetUsedMemory(), 800);
    WTempHybridArray<WRingBufferTracker::FrameData, 2> frames;
    W_TEST_RESULT(tracker.SubmitFrame(1, frames));
    W_TEST_INT(frames.GetCount(), 1);
    W_TEST_INT(frames[0].m_uiFrame, 1);
    W_TEST_INT(frames[0].m_uiStartOffset, 0);
    W_TEST_INT(frames[0].m_uiSize, 800);
    tracker.Free(1);
    W_TEST_INT(tracker.GetUsedMemory(), 0);

    // Frame 2: We allocate memory and hit the end point exactly and then do another allocation.
    W_TEST_RESULT(tracker.CanAllocate(200));
    W_TEST_RESULT(tracker.Allocate(200, 2, uiOffset));
    W_TEST_INT(uiOffset, 800);
    W_TEST_INT(tracker.GetUsedMemory(), 200);

    // This allocation wraps around now.
    W_TEST_RESULT(tracker.CanAllocate(200));
    W_TEST_RESULT(tracker.Allocate(200, 2, uiOffset));
    W_TEST_INT(uiOffset, 0);
    W_TEST_INT(tracker.GetUsedMemory(), 400);

    W_TEST_RESULT(tracker.SubmitFrame(2, frames));
    W_TEST_INT(frames.GetCount(), 2);

    W_TEST_INT(frames[0].m_uiFrame, 2);
    W_TEST_INT(frames[0].m_uiStartOffset, 800);
    W_TEST_INT(frames[0].m_uiSize, 200);

    W_TEST_INT(frames[1].m_uiFrame, 2);
    W_TEST_INT(frames[1].m_uiStartOffset, 0);
    W_TEST_INT(frames[1].m_uiSize, 200);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Buffer wrap around, no continuous block")
  {
    WRingBufferTracker tracker(2, 1000);
    WUInt32 uiOffset = 0;

    WTempHybridArray<WRingBufferTracker::FrameData, 2> frames;
    // Frame 1: Allocate 100 bytes
    W_TEST_RESULT(tracker.CanAllocate(100));
    W_TEST_RESULT(tracker.Allocate(100, 1, uiOffset));
    W_TEST_RESULT(tracker.SubmitFrame(1, frames));
    W_TEST_INT(frames.GetCount(), 1);
    W_TEST_INT(frames[0].m_uiFrame, 1);
    W_TEST_INT(frames[0].m_uiStartOffset, 0);
    W_TEST_INT(frames[0].m_uiSize, 100);
    W_TEST_INT(tracker.GetUsedMemory(), 100);
    W_TEST_INT(tracker.GetFreeMemory(), 900);

    // Frame 2: Allocate 700 bytes
    W_TEST_RESULT(tracker.CanAllocate(700));
    W_TEST_RESULT(tracker.Allocate(700, 2, uiOffset));
    W_TEST_RESULT(tracker.SubmitFrame(2, frames));
    W_TEST_INT(frames.GetCount(), 1);
    W_TEST_INT(frames[0].m_uiFrame, 2);
    W_TEST_INT(frames[0].m_uiStartOffset, 100);
    W_TEST_INT(frames[0].m_uiSize, 700);

    W_TEST_INT(tracker.GetUsedMemory(), 800);
    W_TEST_INT(tracker.GetFreeMemory(), 200);

    // Frame 3: Free frame 1 memory (100 bytes at the start of the ring buffer)
    tracker.Free(1);

    W_TEST_INT(tracker.GetUsedMemory(), 700);
    W_TEST_INT(tracker.GetFreeMemory(), 300);

    W_TEST_BOOL(tracker.CanAllocate(200).Succeeded());
    W_TEST_BOOL(tracker.CanAllocate(300).Failed());

    W_TEST_BOOL(tracker.Allocate(300, 3, uiOffset).Failed());
    W_TEST_RESULT(tracker.CanAllocate(200));
    W_TEST_BOOL(tracker.Allocate(200, 3, uiOffset).Succeeded());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Allocation skip due to reaching the end of the buffer")
  {
    WRingBufferTracker tracker(2, 1000);
    WUInt32 uiOffset = 0;

    // Frame 1: Move a bit forward so we are at the end of the buffer in the next frame.
    W_TEST_RESULT(tracker.CanAllocate(800));
    W_TEST_RESULT(tracker.Allocate(800, 1, uiOffset));
    W_TEST_INT(uiOffset, 0);
    W_TEST_INT(tracker.GetUsedMemory(), 800);
    WTempHybridArray<WRingBufferTracker::FrameData, 2> frames;
    W_TEST_RESULT(tracker.SubmitFrame(1, frames));
    W_TEST_INT(frames.GetCount(), 1);
    W_TEST_INT(frames[0].m_uiFrame, 1);
    W_TEST_INT(frames[0].m_uiStartOffset, 0);
    W_TEST_INT(frames[0].m_uiSize, 800);
    tracker.Free(1);
    W_TEST_INT(tracker.GetUsedMemory(), 0);

    // Frame 2: We allocate memory but do not hit the end point yet.
    W_TEST_RESULT(tracker.CanAllocate(100));
    W_TEST_RESULT(tracker.Allocate(100, 2, uiOffset));
    W_TEST_INT(uiOffset, 800);
    W_TEST_INT(tracker.GetUsedMemory(), 100);

    // We allocate near the end of the buffer but it doesn't fit so 100 bytes are wasted as we skip to the start again.
    W_TEST_RESULT(tracker.CanAllocate(200));
    W_TEST_RESULT(tracker.Allocate(200, 2, uiOffset));
    W_TEST_INT(uiOffset, 0);
    W_TEST_INT(tracker.GetUsedMemory(), 400);

    W_TEST_RESULT(tracker.SubmitFrame(2, frames));
    W_TEST_INT(frames.GetCount(), 2);

    W_TEST_INT(frames[0].m_uiFrame, 2);
    W_TEST_INT(frames[0].m_uiStartOffset, 800);
    W_TEST_INT(frames[0].m_uiSize, 200);

    W_TEST_INT(frames[1].m_uiFrame, 2);
    W_TEST_INT(frames[1].m_uiStartOffset, 0);
    W_TEST_INT(frames[1].m_uiSize, 200);
  }
}
