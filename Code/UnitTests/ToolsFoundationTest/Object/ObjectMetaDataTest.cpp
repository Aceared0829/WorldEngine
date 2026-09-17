#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Types/Uuid.h>
#include <ToolsFoundation/Object/ObjectMetaData.h>


W_CREATE_SIMPLE_TEST(DocumentObject, ObjectMetaData)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Pointers / int")
  {
    WObjectMetaData<void*, WInt32> meta;

    int a = 0, b = 1, c = 2, d = 3;

    W_TEST_BOOL(!meta.HasMetaData(&a));
    W_TEST_BOOL(!meta.HasMetaData(&b));
    W_TEST_BOOL(!meta.HasMetaData(&c));
    W_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pData = meta.BeginModifyMetaData(&a);
      *pData = a;
      meta.EndModifyMetaData();

      pData = meta.BeginModifyMetaData(&b);
      *pData = b;
      meta.EndModifyMetaData();

      pData = meta.BeginModifyMetaData(&c);
      *pData = c;
      meta.EndModifyMetaData();
    }

    W_TEST_BOOL(meta.HasMetaData(&a));
    W_TEST_BOOL(meta.HasMetaData(&b));
    W_TEST_BOOL(meta.HasMetaData(&c));
    W_TEST_BOOL(!meta.HasMetaData(&d));

    {
      auto pDataR = meta.BeginReadMetaData(&a);
      W_TEST_INT(*pDataR, a);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&b);
      W_TEST_INT(*pDataR, b);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&c);
      W_TEST_INT(*pDataR, c);
      meta.EndReadMetaData();

      pDataR = meta.BeginReadMetaData(&d);
      W_TEST_INT(*pDataR, 0);
      meta.EndReadMetaData();
    }
  }

  struct md
  {
    md() { b = false; }

    WString s;
    bool b;
  };

  W_TEST_BLOCK(WTestBlock::Enabled, "UUID / struct")
  {
    WObjectMetaData<WUuid, md> meta;

    const int num = 100;

    WDynamicArray<WUuid> obj;
    obj.SetCount(num);

    for (WUInt32 i = 0; i < num; ++i)
    {
      WUuid& uid = obj[i];
      uid = WUuid::MakeUuid();

      if (WMath::IsEven(i))
      {
        auto d1 = meta.BeginModifyMetaData(uid);
        d1->b = true;
        d1->s = "test";

        meta.EndModifyMetaData();
      }

      W_TEST_BOOL(meta.HasMetaData(uid) == WMath::IsEven(i));
    }

    for (WUInt32 i = 0; i < num; ++i)
    {
      const WUuid& uid = obj[i];

      auto p = meta.BeginReadMetaData(uid);

      W_TEST_BOOL(p->b == WMath::IsEven(i));

      if (WMath::IsEven(i))
      {
        W_TEST_STRING(p->s, "test");
      }
      else
      {
        W_TEST_BOOL(p->s.IsEmpty());
      }

      meta.EndReadMetaData();
      W_TEST_BOOL(meta.HasMetaData(uid) == WMath::IsEven(i));
    }
  }
}
