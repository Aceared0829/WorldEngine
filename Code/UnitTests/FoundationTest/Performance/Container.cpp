#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>

#include <vector>

namespace
{
  enum constants
  {
#if W_ENABLED(W_COMPILE_FOR_DEBUG)
    NUM_SAMPLES = 128,
    NUM_APPENDS = 1024 * 32,
    NUM_RECUSRIVE_APPENDS = 128
#else
    NUM_SAMPLES = 1024,
    NUM_APPENDS = 1024 * 64,
    NUM_RECUSRIVE_APPENDS = 256
#endif
  };

  struct SomeBigObject
  {
    W_DECLARE_MEM_RELOCATABLE_TYPE();

    static WUInt32 constructionCount;
    static WUInt32 destructionCount;
    WUInt64 i1, i2, i3, i4, i5, i6, i7, i8;

    SomeBigObject(WUInt64 uiInit)
      : i1(uiInit)
      , i2(uiInit)
      , i3(uiInit)
      , i4(uiInit)
      , i5(uiInit)
      , i6(uiInit)
      , i7(uiInit)
      , i8(uiInit)
    {
      constructionCount++;
    }

    ~SomeBigObject() { destructionCount++; }

    SomeBigObject(const SomeBigObject& rh)
    {
      constructionCount++;
      this->i1 = rh.i1;
      this->i2 = rh.i2;
      this->i3 = rh.i3;
      this->i4 = rh.i4;
      this->i5 = rh.i5;
      this->i6 = rh.i6;
      this->i7 = rh.i7;
      this->i8 = rh.i8;
    }

    void operator=(const SomeBigObject& rh)
    {
      constructionCount++;
      this->i1 = rh.i1;
      this->i2 = rh.i2;
      this->i3 = rh.i3;
      this->i4 = rh.i4;
      this->i5 = rh.i5;
      this->i6 = rh.i6;
      this->i7 = rh.i7;
      this->i8 = rh.i8;
    }
  };

  WUInt32 SomeBigObject::constructionCount = 0;
  WUInt32 SomeBigObject::destructionCount = 0;
} // namespace

// Enable when needed
#define W_PERFORMANCE_TESTS_STATE WTestBlock::DisabledNoWarning

W_CREATE_SIMPLE_TEST(Performance, Container)
{
  const char* TestString = "There are 10 types of people in the world. Those who understand binary and those who don't.";
  const WUInt32 TestStringLength = (WUInt32)strlen(TestString);

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "POD Dynamic Array Appending")
  {
    WTime t0 = WTime::Now();
    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      WDynamicArray<int> a;
      for (WUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.PushBack(i);
      }

      for (WUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        sum += a[i];
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info("[test]POD Dynamic Array Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "POD std::vector Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<int> a;
      for (WUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.push_back(i);
      }

      for (WUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        sum += a[i];
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info("[test]POD std::vector Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "WDynamicArray<WDynamicArray<char>> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      WDynamicArray<WDynamicArray<char>> a;
      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        WUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        WDynamicArray<char>& cur = a[count];
        for (WUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.PushBack(TestString[j]);
        }
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetCount();
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info(
      "[test]WDynamicArray<WDynamicArray<char>> Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "WDynamicArray<WHybridArray<char, 64>> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      WDynamicArray<WHybridArray<char, 64>> a;
      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        WUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        WHybridArray<char, 64>& cur = a[count];
        for (WUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.PushBack(TestString[j]);
        }
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetCount();
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info("[test]WDynamicArray<WHybridArray<char, 64>> Appending {0}ms",
      WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "std::vector<std::vector<char>> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<std::vector<char>> a;
      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        WUInt32 count = (WUInt32)a.size();
        a.resize(count + 1);
        std::vector<char>& cur = a[count];
        for (WUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.push_back(TestString[j]);
        }
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (WUInt32)a[i].size();
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info(
      "[test]std::vector<std::vector<char>> Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "WDynamicArray<WString> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      WDynamicArray<WString> a;
      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        WUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        WString& cur = a[count];
        WStringBuilder b;
        for (WUInt32 j = 0; j < TestStringLength; j++)
        {
          b.Append(TestString[i]);
        }
        cur = std::move(b);
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetElementCount();
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info("[test]WDynamicArray<WString> Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "std::vector<std::string> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<std::string> a;
      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        std::string cur;
        for (WUInt32 j = 0; j < TestStringLength; j++)
        {
          cur += TestString[i];
        }
        a.push_back(std::move(cur));
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (WUInt32)a[i].length();
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info("[test]std::vector<std::string> Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "WDynamicArray<SomeBigObject> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      WDynamicArray<SomeBigObject> a;
      for (WUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.PushBack(SomeBigObject(i));
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (WUInt32)a[i].i1;
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info(
      "[test]WDynamicArray<SomeBigObject> Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(W_PERFORMANCE_TESTS_STATE, "std::vector<SomeBigObject> Appending")
  {
    WTime t0 = WTime::Now();

    WUInt32 sum = 0;
    for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<SomeBigObject> a;
      for (WUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.push_back(SomeBigObject(i));
      }

      for (WUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (WUInt32)a[i].i1;
      }
    }

    WTime t1 = WTime::Now();
    WLog::Info("[test]std::vector<SomeBigObject> Appending {0}ms", WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  W_TEST_BLOCK(WTestBlock::DisabledNoWarning, "WMap<void*, WUInt32>")
  {
    WUInt32 sum = 0;

    for (WUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      WMap<void*, WUInt32> map;

      for (WUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      WTime t0 = WTime::Now();
      for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
      {
        for (WUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (WUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        for (auto it = map.GetIterator(); it.IsValid(); ++it)
        {
          sum += it.Value();
        }
      }
      WTime t1 = WTime::Now();

      for (auto it = map.GetIterator(); it.IsValid(); ++it)
      {
        free(it.Key());
      }

      WLog::Info("[test]WMap<void*, WUInt32> size = {0} => {1}ms", size, WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
    }
  }

  W_TEST_BLOCK(WTestBlock::DisabledNoWarning, "WHashTable<void*, WUInt32>")
  {
    WUInt32 sum = 0;



    for (WUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      WHashTable<void*, WUInt32> map;

      for (WUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      WTime t0 = WTime::Now();
      for (WUInt32 n = 0; n < NUM_SAMPLES; n++)
      {

        for (WUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (WUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        for (auto it = map.GetIterator(); it.IsValid(); it.Next())
        {
          sum += it.Value();
        }
      }
      WTime t1 = WTime::Now();

      for (auto it = map.GetIterator(); it.IsValid(); it.Next())
      {
        free(it.Key());
      }

      WLog::Info("[test]WHashTable<void*, WUInt32> size = {0} => {1}ms", size,
        WArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
    }
  }
}
