#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/DeduplicationReadContext.h>
#include <Foundation/IO/DeduplicationWriteContext.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Types/UniquePtr.h>

namespace
{
  struct RefCountedVec3 : public WRefCounted
  {
    RefCountedVec3() = default;
    RefCountedVec3(const WVec3& v)
      : m_v(v)
    {
    }

    WResult Serialize(WStreamWriter& inout_stream) const
    {
      inout_stream << m_v;
      return W_SUCCESS;
    }

    WResult Deserialize(WStreamReader& inout_stream)
    {
      inout_stream >> m_v;
      return W_SUCCESS;
    }

    WVec3 m_v;
  };

  struct ComplexComponent
  {
    WTransform* m_pTransform = nullptr;
    WVec3* m_pPosition = nullptr;
    WSharedPtr<RefCountedVec3> m_pScale;
    WUInt32 m_uiIndex = WInvalidIndex;

    WResult Serialize(WStreamWriter& inout_stream) const
    {
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pTransform));
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pPosition));
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteObject(inout_stream, m_pScale));

      inout_stream << m_uiIndex;
      return W_SUCCESS;
    }

    WResult Deserialize(WStreamReader& inout_stream)
    {
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pTransform));
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pPosition));
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadObject(inout_stream, m_pScale));

      inout_stream >> m_uiIndex;
      return W_SUCCESS;
    }
  };

  struct ComplexObject
  {
    WDynamicArray<WUniquePtr<WTransform>> m_Transforms;
    WDynamicArray<WVec3> m_Positions;
    WDynamicArray<WSharedPtr<RefCountedVec3>> m_Scales;

    WDynamicArray<ComplexComponent> m_Components;

    WMap<WUInt32, WTransform*> m_TransformMap;
    WSet<WVec3*> m_UniquePositions;

    WResult Serialize(WStreamWriter& inout_stream) const
    {
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Transforms));
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Positions));
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteArray(inout_stream, m_Scales));
      W_SUCCEED_OR_RETURN(
        WDeduplicationWriteContext::GetContext()->WriteMap(inout_stream, m_TransformMap, WDeduplicationWriteContext::WriteMapMode::DedupValue));
      W_SUCCEED_OR_RETURN(WDeduplicationWriteContext::GetContext()->WriteSet(inout_stream, m_UniquePositions));
      W_SUCCEED_OR_RETURN(inout_stream.WriteArray(m_Components));
      return W_SUCCESS;
    }

    WResult Deserialize(WStreamReader& inout_stream)
    {
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Transforms));
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Positions,
        nullptr));                                                                                                       // should not allocate anything
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadArray(inout_stream, m_Scales));
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadMap(
        inout_stream, m_TransformMap, WDeduplicationReadContext::ReadMapMode::DedupValue, nullptr, nullptr));           // should not allocate anything
      W_SUCCEED_OR_RETURN(WDeduplicationReadContext::GetContext()->ReadSet(inout_stream, m_UniquePositions, nullptr)); // should not allocate anything
      W_SUCCEED_OR_RETURN(inout_stream.ReadArray(m_Components));
      return W_SUCCESS;
    }
  };
} // namespace

W_CREATE_SIMPLE_TEST(IO, DeduplicationContext)
{
  WDefaultMemoryStreamStorage streamStorage;

  W_TEST_BLOCK(WTestBlock::Enabled, "Writer")
  {
    WMemoryStreamWriter writer(&streamStorage);

    WDeduplicationWriteContext dedupWriteContext;

    ComplexObject obj;
    for (WUInt32 i = 0; i < 20; ++i)
    {
      obj.m_Transforms.ExpandAndGetRef() = W_DEFAULT_NEW(WTransform, WVec3(static_cast<float>(i), 0, 0));
      obj.m_Positions.ExpandAndGetRef() = WVec3(1, 2, static_cast<float>(i));
      obj.m_Scales.ExpandAndGetRef() = W_DEFAULT_NEW(RefCountedVec3, WVec3(0, static_cast<float>(i), 0));
    }

    for (WUInt32 i = 0; i < 10; ++i)
    {
      auto& component = obj.m_Components.ExpandAndGetRef();
      component.m_uiIndex = i * 2;
      component.m_pTransform = obj.m_Transforms[component.m_uiIndex].Borrow();
      component.m_pPosition = &obj.m_Positions[component.m_uiIndex];
      component.m_pScale = obj.m_Scales[component.m_uiIndex];

      obj.m_TransformMap.Insert(i, obj.m_Transforms[i].Borrow());
      obj.m_UniquePositions.Insert(&obj.m_Positions[i]);
    }



    W_TEST_BOOL(obj.Serialize(writer).Succeeded());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Reader")
  {
    WMemoryStreamReader reader(&streamStorage);

    WDeduplicationReadContext dedupReadContext;

    ComplexObject obj;
    W_TEST_BOOL(obj.Deserialize(reader).Succeeded());

    W_TEST_INT(obj.m_Transforms.GetCount(), 20);
    W_TEST_INT(obj.m_Positions.GetCount(), 20);
    W_TEST_INT(obj.m_Scales.GetCount(), 20);
    W_TEST_INT(obj.m_TransformMap.GetCount(), 10);
    W_TEST_INT(obj.m_UniquePositions.GetCount(), 10);
    W_TEST_INT(obj.m_Components.GetCount(), 10);

    for (WUInt32 i = 0; i < obj.m_Components.GetCount(); ++i)
    {
      auto& component = obj.m_Components[i];

      W_TEST_BOOL(component.m_pTransform == obj.m_Transforms[component.m_uiIndex].Borrow());
      W_TEST_BOOL(component.m_pPosition == &obj.m_Positions[component.m_uiIndex]);
      W_TEST_BOOL(component.m_pScale == obj.m_Scales[component.m_uiIndex]);

      W_TEST_BOOL(component.m_pTransform->m_vPosition == WVec3(static_cast<float>(i) * 2, 0, 0));
      W_TEST_BOOL(*component.m_pPosition == WVec3(1, 2, static_cast<float>(i) * 2));
      W_TEST_BOOL(component.m_pScale->m_v == WVec3(0, static_cast<float>(i) * 2, 0));
    }

    for (WUInt32 i = 0; i < 10; ++i)
    {
      if (W_TEST_BOOL(obj.m_TransformMap.GetValue(i) != nullptr))
      {
        W_TEST_BOOL(*obj.m_TransformMap.GetValue(i) == obj.m_Transforms[i].Borrow());
      }

      W_TEST_BOOL(obj.m_UniquePositions.Contains(&obj.m_Positions[i]));
    }
  }
}
