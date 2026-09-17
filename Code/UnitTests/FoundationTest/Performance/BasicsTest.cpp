#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Time/Time.h>

/* Performance Statistics:

  AMD E-350 Processor 1.6 GHz ('Fusion'), 32 Bit, Debug Mode
    Virtual Function Calls:   ~60 ns
    Simple Function Calls:    ~27 ns
    Fastcall Function Calls:  ~27 ns
    Integer Division:         52 ns
    Integer Multiplication:   23 ns
    Float Division:           25 ns
    Float Multiplication:     25 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 64 Bit, Debug Mode
    Virtual Function Calls:   ~80 ns
    Simple Function Calls:    ~55 ns
    Fastcall Function Calls:  ~55 ns
    Integer Division:         ~97 ns
    Integer Multiplication:   ~52 ns
    Float Division:           ~66 ns
    Float Multiplication:     ~58 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 32 Bit, Release Mode
    Virtual Function Calls:   ~9 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.78 ns
    Float Division:           10.7 ns
    Float Multiplication:     9.5 ns

  AMD E-350 Processor 1.6 GHz ('Fusion'), 64 Bit, Release Mode
    Virtual Function Calls:   ~10 ns
    Simple Function Calls:    ~5 ns
    Fastcall Function Calls:  ~5 ns
    Integer Division:         35 ns
    Integer Multiplication:   3.23 ns
    Float Division:           8.13 ns
    Float Multiplication:     4.13 ns

  Intel Core i7 3770 3.4 GHz, 64 Bit, Release Mode
    Virtual Function Calls:   ~3.8 ns
    Simple Function Calls:    ~4.4 ns
    Fastcall Function Calls:  ~4.0 ns
    Integer Division:         8.25 ns
    Integer Multiplication:   1.55 ns
    Float Division:           4.40 ns
    Float Multiplication:     1.87 ns

*/

W_CREATE_SIMPLE_TEST_GROUP(Performance);

struct WMsgTest : public WMessage
{
  W_DECLARE_MESSAGE_TYPE(WMsgTest, WMessage);
};

W_IMPLEMENT_MESSAGE_TYPE(WMsgTest);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(WMsgTest, 1, WRTTIDefaultAllocator<WMsgTest>)
W_END_DYNAMIC_REFLECTED_TYPE;


struct GetValueMessage : public WMsgTest
{
  W_DECLARE_MESSAGE_TYPE(GetValueMessage, WMsgTest);

  WInt32 m_iValue;
};
W_IMPLEMENT_MESSAGE_TYPE(GetValueMessage);
W_BEGIN_DYNAMIC_REFLECTED_TYPE(GetValueMessage, 1, WRTTIDefaultAllocator<GetValueMessage>)
W_END_DYNAMIC_REFLECTED_TYPE;



class Base : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(Base, WReflectedClass);

public:
  virtual ~Base() = default;

  virtual WInt32 Virtual() = 0;
};

W_BEGIN_DYNAMIC_REFLECTED_TYPE(Base, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

#if W_ENABLED(W_COMPILER_MSVC_PURE)
#  define W_FASTCALL __fastcall
#  define W_NO_INLINE __declspec(noinline)
#else
#  if W_ENABLED(W_PLATFORM_ARCH_X86) && W_ENABLED(W_PLATFORM_32BIT)
#    define W_FASTCALL __attribute((fastcall)) // Fastcall only relevant on x86-32 and would otherwise generate warnings
#  else
#    define W_FASTCALL
#  endif
#  define W_NO_INLINE __attribute__((noinline))
#endif

class Derived1 : public Base
{
  W_ADD_DYNAMIC_REFLECTION(Derived1, Base);

public:
  W_NO_INLINE WInt32 W_FASTCALL FastCall() { return 1; }
  W_NO_INLINE WInt32 NonVirtual() { return 1; }
  W_NO_INLINE virtual WInt32 Virtual() override { return 1; }
  W_NO_INLINE void OnGetValueMessage(GetValueMessage& ref_msg) { ref_msg.m_iValue = 1; }
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived1, 1, WRTTINoAllocator)
{
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class Derived2 : public Base
{
  W_ADD_DYNAMIC_REFLECTION(Derived2, Base);

public:
  W_NO_INLINE WInt32 W_FASTCALL FastCall() { return 2; }
  W_NO_INLINE WInt32 NonVirtual() { return 2; }
  W_NO_INLINE virtual WInt32 Virtual() override { return 2; }
  W_NO_INLINE void OnGetValueMessage(GetValueMessage& ref_msg) { ref_msg.m_iValue = 2; }
};

// clang-format off
W_BEGIN_DYNAMIC_REFLECTED_TYPE(Derived2, 1, WRTTINoAllocator)
{
  W_BEGIN_MESSAGEHANDLERS
  {
    W_MESSAGE_HANDLER(GetValueMessage, OnGetValueMessage),
  }
  W_END_MESSAGEHANDLERS;
}
W_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

W_CREATE_SIMPLE_TEST(Performance, Basics)
{
  const WInt32 iNumObjects = 1000000;
  const float fNumObjects = (float)iNumObjects;

  WDynamicArray<Derived1> Der1;
  Der1.SetCount(iNumObjects / 2);

  WDynamicArray<Derived2> Der2;
  Der2.SetCount(iNumObjects / 2);

  WDynamicArray<Base*> Objects;
  Objects.SetCount(iNumObjects);

  for (WInt32 i = 0; i < iNumObjects; i += 2)
  {
    Objects[i] = &Der1[i / 2];
    Objects[i + 1] = &Der2[i / 2];
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Dispatch Message")
  {
    WInt32 iResult = 0;

    // warm up
    for (WUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    WTime t0 = WTime::Now();

    for (WUInt32 i = 0; i < iNumObjects; ++i)
    {
      GetValueMessage msg;
      Objects[i]->GetDynamicRTTI()->DispatchMessage(Objects[i], msg);
      iResult += msg.m_iValue;
    }

    WTime t1 = WTime::Now();

    W_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    WTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    WLog::Info("[test]Dispatch Message: {0}ns", WArgF(tFC, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Virtual")
  {
    WInt32 iResult = 0;

    // warm up
    for (WUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    WTime t0 = WTime::Now();

    for (WUInt32 i = 0; i < iNumObjects; ++i)
      iResult += Objects[i]->Virtual();

    WTime t1 = WTime::Now();

    W_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    WTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    WLog::Info("[test]Virtual Function Calls: {0}ns", WArgF(tFC, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "NonVirtual")
  {
    WInt32 iResult = 0;

    // warm up
    for (WUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->NonVirtual();
      iResult += ((Derived2*)Objects[i])->NonVirtual();
    }

    WTime t0 = WTime::Now();

    for (WUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->NonVirtual();
      iResult += ((Derived2*)Objects[i])->NonVirtual();
    }

    WTime t1 = WTime::Now();

    W_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    WTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    WLog::Info("[test]Non-Virtual Function Calls: {0}ns", WArgF(tFC, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "FastCall")
  {
    WInt32 iResult = 0;

    // warm up
    for (WUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->FastCall();
      iResult += ((Derived2*)Objects[i])->FastCall();
    }

    WTime t0 = WTime::Now();

    for (WUInt32 i = 0; i < iNumObjects; i += 2)
    {
      iResult += ((Derived1*)Objects[i])->FastCall();
      iResult += ((Derived2*)Objects[i])->FastCall();
    }

    WTime t1 = WTime::Now();

    W_TEST_INT(iResult, iNumObjects * 1 + iNumObjects * 2);

    WTime tdiff = t1 - t0;
    double tFC = tdiff.GetNanoseconds() / (double)iNumObjects;

    WLog::Info("[test]FastCall Function Calls: {0}ns", WArgF(tFC, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "32 Bit Integer Division")
  {
    WDynamicArray<WInt32> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100;

    WTime t0 = WTime::Now();

    WInt32 iResult = 0;

    for (WInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / i;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects - 1);

    WLog::Info("[test]32 Bit Integer Division: {0}ns", WArgF(t, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "32 Bit Integer Multiplication")
  {
    WDynamicArray<WInt32> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    WTime t0 = WTime::Now();

    WInt32 iResult = 0;

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * i;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    WLog::Info("[test]32 Bit Integer Multiplication: {0}ns", WArgF(t, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "64 Bit Integer Division")
  {
    WDynamicArray<WInt64> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = (WInt64)i * (WInt64)100;

    WTime t0 = WTime::Now();

    WInt64 iResult = 0;

    for (WInt32 i = 1; i < iNumObjects; i += 1)
      iResult += Ints[i] / (WInt64)i;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects - 1);

    WLog::Info("[test]64 Bit Integer Division: {0}ns", WArgF(t, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "64 Bit Integer Multiplication")
  {
    WDynamicArray<WInt64> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = iNumObjects - i;

    WTime t0 = WTime::Now();

    WInt64 iResult = 0;

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      iResult += Ints[i] * (WInt64)i;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    WLog::Info("[test]64 Bit Integer Multiplication: {0}ns", WArgF(t, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "32 Bit Float Division")
  {
    WDynamicArray<float> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0f;

    WTime t0 = WTime::Now();

    float fResult = 0;

    float d = 1.0f;
    for (WInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    WLog::Info("[test]32 Bit Float Division: {0}ns", WArgF(t, 2), fResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "32 Bit Float Multiplication")
  {
    WDynamicArray<float> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (float)(fNumObjects) - (float)(i);

    WTime t0 = WTime::Now();

    float iResult = 0;

    float d = 1.0f;
    for (WInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      iResult += Ints[i] * d;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    WLog::Info("[test]32 Bit Float Multiplication: {0}ns", WArgF(t, 2), iResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "64 Bit Double Division")
  {
    WDynamicArray<double> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i += 1)
      Ints[i] = i * 100.0;

    WTime t0 = WTime::Now();

    double fResult = 0;

    double d = 1.0;
    for (WInt32 i = 0; i < iNumObjects; i++, d += 1.0f)
      fResult += Ints[i] / d;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    WLog::Info("[test]64 Bit Double Division: {0}ns", WArgF(t, 2), fResult);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "64 Bit Double Multiplication")
  {
    WDynamicArray<double> Ints;
    Ints.SetCountUninitialized(iNumObjects);

    for (WInt32 i = 0; i < iNumObjects; i++)
      Ints[i] = (double)(fNumObjects) - (double)(i);

    WTime t0 = WTime::Now();

    double iResult = 0;

    double d = 1.0;
    for (WInt32 i = 0; i < iNumObjects; i++, d += 1.0)
      iResult += Ints[i] * d;

    WTime t1 = WTime::Now();

    WTime tdiff = t1 - t0;
    double t = tdiff.GetNanoseconds() / (double)(iNumObjects);

    WLog::Info("[test]64 Bit Double Multiplication: {0}ns", WArgF(t, 2), iResult);
  }
}
