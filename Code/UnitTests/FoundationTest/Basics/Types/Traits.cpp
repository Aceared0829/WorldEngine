#include <FoundationTest/FoundationTestPCH.h>

// This test does not actually run, it tests compile time stuff

namespace
{
  struct AggregatePod
  {
    int m_1;
    float m_2;

    W_DETECT_TYPE_CLASS(int, float);
  };

  struct AggregatePod2
  {
    int m_1;
    float m_2;
    AggregatePod m_3;

    W_DETECT_TYPE_CLASS(int, float, AggregatePod);
  };

  struct MemRelocateable
  {
    W_DECLARE_MEM_RELOCATABLE_TYPE();
  };

  struct AggregateMemRelocateable
  {
    int m_1;
    float m_2;
    AggregatePod m_3;
    MemRelocateable m_4;

    W_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable);
  };

  class ClassType
  {
  };

  struct AggregateClass
  {
    int m_1;
    float m_2;
    AggregatePod m_3;
    MemRelocateable m_4;
    ClassType m_5;

    W_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable, ClassType);
  };

  static_assert(WGetTypeClass<AggregatePod>::value == WTypeIsPod::value);
  static_assert(WGetTypeClass<AggregatePod2>::value == WTypeIsPod::value);
  static_assert(WGetTypeClass<MemRelocateable>::value == WTypeIsMemRelocatable::value);
  static_assert(WGetTypeClass<AggregateMemRelocateable>::value == WTypeIsMemRelocatable::value);
  static_assert(WGetTypeClass<ClassType>::value == WTypeIsClass::value);
  static_assert(WGetTypeClass<AggregateClass>::value == WTypeIsClass::value);
} // namespace
