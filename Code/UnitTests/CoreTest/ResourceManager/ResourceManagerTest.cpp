#include <CoreTest/CoreTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Threading/Thread.h>
#include <Foundation/Types/ScopeExit.h>

W_CREATE_SIMPLE_TEST_GROUP(ResourceManager);

namespace
{
  using TestResourceHandle = WTypedResourceHandle<class TestResource>;

  struct TestResourceDescriptor
  {
    WUInt32 m_uiData = 0;
  };

  class TestResource : public WResource
  {
    W_ADD_DYNAMIC_REFLECTION(TestResource, WResource);
    W_RESOURCE_DECLARE_COMMON_CODE(TestResource);
    W_RESOURCE_DECLARE_CREATEABLE(TestResource, TestResourceDescriptor);

  public:
    TestResource()
      : WResource(WResource::DoUpdate::OnAnyThread, 1)
    {
    }

  protected:
    virtual WResourceLoadDesc UnloadData(Unload WhatToUnload) override
    {
      WResourceLoadDesc ld;
      ld.m_State = WResourceState::Unloaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      return ld;
    }

    virtual WResourceLoadDesc UpdateContent(WStreamReader* Stream) override
    {
      WResourceLoadDesc ld;
      ld.m_State = WResourceState::Loaded;
      ld.m_uiQualityLevelsDiscardable = 0;
      ld.m_uiQualityLevelsLoadable = 0;

      WStreamReader& s = *Stream;

      WUInt32 uiNumElements = 0;
      s >> uiNumElements;

      if (GetResourceID().StartsWith("NonBlockingLevel1-"))
      {
        m_hNested = WResourceManager::LoadResource<TestResource>("Level0-0");
      }

      if (GetResourceID().StartsWith("BlockingLevel1-"))
      {
        m_hNested = WResourceManager::LoadResource<TestResource>("Level0-0");

        WResourceLock<TestResource> pTestResource(m_hNested, WResourceAcquireMode::BlockTillLoaded_NeverFail);

        W_ASSERT_ALWAYS(pTestResource.GetAcquireResult() == WResourceAcquireResult::Final, "");
      }

      m_Data.SetCountUninitialized(uiNumElements);

      for (WUInt32 i = 0; i < uiNumElements; ++i)
      {
        s >> m_Data[i];
      }

      return ld;
    }

    virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override
    {
      out_NewMemoryUsage.m_uiMemoryCPU = sizeof(TestResource);
      out_NewMemoryUsage.m_uiMemoryGPU = 0;
    }

  public:
    void Test() { W_TEST_BOOL(!m_Data.IsEmpty()); }

  private:
    TestResourceHandle m_hNested;
    WDynamicArray<WUInt32> m_Data;
  };

  W_RESOURCE_IMPLEMENT_CREATEABLE(TestResource, TestResourceDescriptor)
  {
    WResourceLoadDesc res;
    res.m_State = WResourceState::Loaded;
    res.m_uiQualityLevelsDiscardable = 0;
    res.m_uiQualityLevelsLoadable = 0;

    W_TEST_INT(m_Data.GetCount(), 0);
    m_Data.PushBack(descriptor.m_uiData);
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(50));
    W_TEST_INT(m_Data.GetCount(), 1);

    return res;
  }

  class TestResourceTypeLoader : public WResourceTypeLoader
  {
  public:
    struct LoadedData
    {
      WDefaultMemoryStreamStorage m_StreamData;
      WMemoryStreamReader m_Reader;
    };

    virtual WResourceLoadData OpenDataStream(const WResource* pResource) override
    {
      LoadedData* pData = W_DEFAULT_NEW(LoadedData);

      const WUInt32 uiNumElements = 1024 * 10;
      pData->m_StreamData.Reserve(uiNumElements * sizeof(WUInt32) + 1);

      WMemoryStreamWriter writer(&pData->m_StreamData);
      pData->m_Reader.SetStorage(&pData->m_StreamData);

      writer << uiNumElements;

      for (WUInt32 i = 0; i < uiNumElements; ++i)
      {
        writer << i;
      }

      WResourceLoadData ld;
      ld.m_pCustomLoaderData = pData;
      ld.m_pDataStream = &pData->m_Reader;
      ld.m_sResourceDescription = pResource->GetResourceID();

      return ld;
    }

    virtual void CloseDataStream(const WResource* pResource, const WResourceLoadData& loaderData) override
    {
      LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);
      W_DEFAULT_DELETE(pData);
    }
  };

  W_RESOURCE_IMPLEMENT_COMMON_CODE(TestResource);
  W_BEGIN_DYNAMIC_REFLECTED_TYPE(TestResource, 1, WRTTIDefaultAllocator<TestResource>)
  W_END_DYNAMIC_REFLECTED_TYPE;


  class ResourceTestThread : public WThread
  {
  public:
    WDelegate<int()> m_Lambda;

    ResourceTestThread()
      : WThread("Resource Test Thread")
    {
    }

    virtual WUInt32 Run()
    {
      if (m_Lambda.IsValid())
      {
        return m_Lambda();
      }
      return 0;
    }
  };
} // namespace

W_CREATE_SIMPLE_TEST(ResourceManager, Basics)
{
  TestResourceTypeLoader TypeLoader;
  WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  WResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  W_SCOPE_EXIT(WResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  W_TEST_BLOCK(WTestBlock::Enabled, "Main")
  {
    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const WUInt32 uiNumResources = 200;

    WDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    WStringBuilder sResourceID;
    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("Level0-{}", i);
      hResources.PushBack(WResourceManager::LoadResource<TestResource>(sResourceID));
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      WResourceManager::PreloadResource(hResources[i]);
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      WResourceLock<TestResource> pTestResource(hResources[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);

      W_TEST_BOOL(pTestResource.GetAcquireResult() == WResourceAcquireResult::Final);

      pTestResource->Test();
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    hResources.Clear();

    WUInt32 uiUnloaded = 0;

    for (WUInt32 tries = 0; tries < 3; ++tries)
    {
      // if a resource is in a loading queue, unloading it can actually 'fail' for a short time
      uiUnloaded += WResourceManager::FreeAllUnusedResources();

      if (uiUnloaded == uiNumResources)
        break;

      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}


W_CREATE_SIMPLE_TEST(ResourceManager, ThreadedCreateResource)
{
  TestResourceTypeLoader TypeLoader;
  WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  WResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  W_SCOPE_EXIT(WResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  auto testLambda = []() -> int
  {
    TestResourceDescriptor desc;
    TestResourceHandle hTest = WResourceManager::GetOrCreateResource<TestResource>("test1", std::move(desc));
    {
      WResourceLock<TestResource> pTest(hTest, WResourceAcquireMode::PointerOnly);
      W_TEST_INT((int)pTest.GetAcquireResult(), (int)WResourceAcquireResult::Final);
    }
    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 1);
    return 0;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "ThreadedCreateResource")
  {
    WTempHybridArray<WUniquePtr<ResourceTestThread>, 8> threads;
    for (WUInt32 i = 0; i < 8; ++i)
    {
      WUniquePtr<ResourceTestThread> thread = W_DEFAULT_NEW(ResourceTestThread);
      thread->m_Lambda = testLambda;
      threads.PushBack(std::move(thread));
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    for (WUInt32 i = 0; i < 8; ++i)
    {
      threads[i]->Start();
    }

    for (WUInt32 i = 0; i < 8; ++i)
    {
      threads[i]->Join();
    }
    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 1);

    threads.Clear();
  }
}


W_CREATE_SIMPLE_TEST(ResourceManager, NestedLoading)
{
  TestResourceTypeLoader TypeLoader;
  WResourceManager::AllowResourceTypeAcquireDuringUpdateContent<TestResource, TestResource>();
  WResourceManager::SetResourceTypeLoader<TestResource>(&TypeLoader);
  W_SCOPE_EXIT(WResourceManager::SetResourceTypeLoader<TestResource>(nullptr));

  W_TEST_BLOCK(WTestBlock::Enabled, "NonBlocking")
  {
    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const WUInt32 uiNumResources = 200;

    WDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    WStringBuilder sResourceID;
    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("NonBlockingLevel1-{}", i);
      hResources.PushBack(WResourceManager::LoadResource<TestResource>(sResourceID));
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      WResourceManager::PreloadResource(hResources[i]);
    }

    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      WResourceLock<TestResource> pTestResource(hResources[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);

      W_TEST_BOOL(pTestResource.GetAcquireResult() == WResourceAcquireResult::Final);

      pTestResource->Test();
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (WResourceManager::IsAnyLoadingInProgress())
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    }

    WResourceManager::FreeAllUnusedResources();
    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }

  // Test disabled as it deadlocks
  W_TEST_BLOCK(WTestBlock::Enabled, "Blocking")
  {
    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);

    const WUInt32 uiNumResources = 500;

    WDynamicArray<TestResourceHandle> hResources;
    hResources.Reserve(uiNumResources);

    WStringBuilder sResourceID;
    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      sResourceID.SetFormat("BlockingLevel1-{}", i);
      hResources.PushBack(WResourceManager::LoadResource<TestResource>(sResourceID));
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources);

    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      WResourceManager::PreloadResource(hResources[i]);
    }

    for (WUInt32 i = 0; i < uiNumResources; ++i)
    {
      WResourceLock<TestResource> pTestResource(hResources[i], WResourceAcquireMode::BlockTillLoaded_NeverFail);

      W_TEST_BOOL(pTestResource.GetAcquireResult() == WResourceAcquireResult::Final);

      pTestResource->Test();
    }

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), uiNumResources + 1);

    hResources.Clear();

    while (WResourceManager::IsAnyLoadingInProgress())
    {
      WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    }

    WResourceManager::FreeAllUnusedResources();
    WThreadUtils::Sleep(WTime::MakeFromMilliseconds(100));
    WResourceManager::FreeAllUnusedResources();

    W_TEST_INT(WResourceManager::GetAllResourcesOfType<TestResource>()->GetCount(), 0);
  }
}
