#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Math/Rational.h>
#include <Foundation/Strings/StringBuilder.h>

W_CREATE_SIMPLE_TEST(Math, Rational)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Rational")
  {
    WRational r1(100, 1);

    W_TEST_BOOL(r1.IsValid());
    W_TEST_BOOL(r1.IsIntegral());

    WRational r2(100, 0);
    W_TEST_BOOL(!r2.IsValid());

    W_TEST_BOOL(r1 != r2);

    WRational r3(100, 1);
    W_TEST_BOOL(r3 == r1);

    WRational r4(0, 0);
    W_TEST_BOOL(r4.IsValid());


    WRational r5(30, 6);
    W_TEST_BOOL(r5.IsIntegral());
    W_TEST_INT(r5.GetIntegralResult(), 5);
    W_TEST_FLOAT(r5.GetFloatingPointResult(), 5, WMath::SmallEpsilon<float>());

    WRational reducedTest(5, 1);
    W_TEST_BOOL(r5.ReduceIntegralFraction() == reducedTest);

    WRational r6(31, 6);
    W_TEST_BOOL(!r6.IsIntegral());
    W_TEST_FLOAT(r6.GetFloatingPointResult(), 5.16666666666, WMath::SmallEpsilon<float>());


    W_TEST_INT(r6.GetDenominator(), 6);
    W_TEST_INT(r6.GetNumerator(), 31);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Rational String Formatting")
  {
    WRational r1(50, 25);

    WStringBuilder sb;
    sb.SetFormat("Rational: {}", r1);
    W_TEST_STRING(sb, "Rational: 2");


    WRational r2(233, 76);
    sb.SetFormat("Rational: {}", r2);
    W_TEST_STRING(sb, "Rational: 233/76");
  }
}
