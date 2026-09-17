#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <TestFramework/TestFrameworkDLL.h>

struct WConstructionCounter
{
  /// Dummy m_iData, such that one can test the constructor with initialization
  WInt32 m_iData;
  bool m_valid;

  /// Default Constructor
  WConstructionCounter()
    : m_iData(0)
    , m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Constructor with initialization
  WConstructionCounter(WInt32 d)
    : m_iData(d)
    , m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Copy Constructor
  WConstructionCounter(const WConstructionCounter& cc)
    : m_iData(cc.m_iData)
    , m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Move construction counts as a construction as well.
  WConstructionCounter(WConstructionCounter&& cc) noexcept
    : m_iData(cc.m_iData)
    , m_valid(true)
  {
    cc.m_iData = 0; // data has been moved, so "destroy" it.
    ++s_iConstructions;
  }

  /// Destructor
  ~WConstructionCounter()
  {
    W_ASSERT_ALWAYS(m_valid, "Destroying object twice");
    m_valid = false;
    ++s_iDestructions;
  }

  /// Assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator=(const WConstructionCounter& cc) { m_iData = cc.m_iData; }
  /// Move assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator=(const WConstructionCounter&& cc) noexcept { m_iData = cc.m_iData; }

  bool operator==(const WConstructionCounter& cc) const { return m_iData == cc.m_iData; }
  W_ADD_DEFAULT_OPERATOR_NOTEQUAL(const WConstructionCounter&);

  bool operator<(const WConstructionCounter& rhs) const { return m_iData < rhs.m_iData; }

  /// Checks whether n constructions have been done since the last check.
  static bool HasConstructed(WInt32 iCons)
  {
    const bool b = s_iConstructions == s_iConstructionsLast + iCons;
    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    if (!b)
      PrintStats();

    return (b);
  }

  /// Checks whether n destructions have been done since the last check.
  static bool HasDestructed(WInt32 iCons)
  {
    const bool b = s_iDestructions == s_iDestructionsLast + iCons;
    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    if (!b)
      PrintStats();

    return (b);
  }

  /// Checks whether n constructions and destructions have been done since the last check.
  static bool HasDone(WInt32 iCons, WInt32 iDes)
  {
    const bool bc = (s_iConstructions == (s_iConstructionsLast + iCons));
    const bool bd = (s_iDestructions == (s_iDestructionsLast + iDes));

    if (!(bc && bd))
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (bc && bd);
  }

  /// For debugging and getting tests right: Prints out the current number of constructions and destructions
  static void PrintStats()
  {
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast, s_iDestructions,
      s_iDestructions - s_iDestructionsLast);
  }

  /// Checks that all instances have been destructed.
  static bool HasAllDestructed()
  {
    if (s_iConstructions != s_iDestructions)
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (s_iConstructions == s_iDestructions);
  }

  static void Reset()
  {
    s_iConstructions = 0;
    s_iConstructionsLast = 0;
    s_iDestructions = 0;
    s_iDestructionsLast = 0;
  }

  static WInt32 s_iConstructions;
  static WInt32 s_iConstructionsLast;
  static WInt32 s_iDestructions;
  static WInt32 s_iDestructionsLast;
};

struct WConstructionCounterRelocatable
{
  W_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Dummy m_iData, such that one can test the constructor with initialization
  WInt32 m_iData;

  /// Bool to track if the element was default constructed or received valid data.
  bool m_valid = false;

  WConstructionCounterRelocatable() = default;

  WConstructionCounterRelocatable(WInt32 d)
    : m_iData(d)
    , m_valid(true)
  {
    s_iConstructions++;
  }

  WConstructionCounterRelocatable(const WConstructionCounterRelocatable& other) = delete;

  WConstructionCounterRelocatable(WConstructionCounterRelocatable&& other) noexcept
  {
    m_iData = other.m_iData;
    m_valid = other.m_valid;

    other.m_valid = false;
  }

  ~WConstructionCounterRelocatable()
  {
    if (m_valid)
      s_iDestructions++;
  }

  void operator=(WConstructionCounterRelocatable&& other) noexcept
  {
    m_iData = other.m_iData;
    m_valid = other.m_valid;

    other.m_valid = false;
    ;
  }

  /// For debugging and getting tests right: Prints out the current number of constructions and destructions
  static void PrintStats()
  {
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast, s_iDestructions,
      s_iDestructions - s_iDestructionsLast);
  }

  /// Checks whether n constructions and destructions have been done since the last check.
  static bool HasDone(WInt32 iCons, WInt32 iDes)
  {
    const bool bc = (s_iConstructions == (s_iConstructionsLast + iCons));
    const bool bd = (s_iDestructions == (s_iDestructionsLast + iDes));

    if (!(bc && bd))
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (bc && bd);
  }

  /// Checks that all instances have been destructed.
  static bool HasAllDestructed()
  {
    if (s_iConstructions != s_iDestructions)
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast = s_iDestructions;

    return (s_iConstructions == s_iDestructions);
  }

  static void Reset()
  {
    s_iConstructions = 0;
    s_iConstructionsLast = 0;
    s_iDestructions = 0;
    s_iDestructionsLast = 0;
  }

  static WInt32 s_iConstructions;
  static WInt32 s_iConstructionsLast;
  static WInt32 s_iDestructions;
  static WInt32 s_iDestructionsLast;
};

template <>
struct WHashHelper<WConstructionCounter>
{
  static WUInt32 Hash(const WConstructionCounter& value) { return WHashHelper<WInt32>::Hash(value.m_iData); }

  W_ALWAYS_INLINE static bool Equal(const WConstructionCounter& a, const WConstructionCounter& b) { return a == b; }
};
