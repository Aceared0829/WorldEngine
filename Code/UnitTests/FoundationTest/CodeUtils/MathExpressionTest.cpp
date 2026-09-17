#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/MathExpression.h>

W_CREATE_SIMPLE_TEST(CodeUtils, MathExpression)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "Basics")
  {
    {
      WMathExpression expr("");
      W_TEST_BOOL(!expr.IsValid());

      expr.Reset("");
      W_TEST_BOOL(!expr.IsValid());
    }
    {
      WMathExpression expr(nullptr);
      W_TEST_BOOL(!expr.IsValid());

      expr.Reset(nullptr);
      W_TEST_BOOL(!expr.IsValid());
    }
    {
      WMathExpression expr("1.5 + 2.5");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      WMathExpression expr("1- 2");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), -1.0, 0.0);
    }
    {
      WMathExpression expr("1 *2");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      WMathExpression expr(" 1.0/2 ");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      WMathExpression expr("1 - -1");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      WMathExpression expr("abs(-3)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      WMathExpression expr("sqrt(4)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
    {
      WMathExpression expr("saturate(4)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 1.0, 0.0);
    }
    {
      WMathExpression expr("min(3, 4)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      WMathExpression expr("max(3, 4)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
    {
      WMathExpression expr("clamp(2, 3, 4)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
    {
      WMathExpression expr("clamp(5, 3, 4)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 4.0, 0.0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Operator Priority")
  {
    {
      WMathExpression expr("1 - 2 * 4");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), -7.0, 0.0);
    }
    {
      WMathExpression expr("-1 - 2 * 4");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), -9.0, 0.0);
    }
    {
      WMathExpression expr("1 - 2.0 / 4");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 0.5, 0.0);
    }
    {
      WMathExpression expr("abs (-4 + 2)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 2.0, 0.0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Braces")
  {
    {
      WMathExpression expr("(1 - 2) * 4");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), -4.0, 0.0);
    }
    {
      WMathExpression expr("(((((0)))))");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 0.0, 0.0);
    }
    {
      WMathExpression expr("(1 + 2) * (3 - 2)");
      W_TEST_BOOL(expr.IsValid());
      W_TEST_DOUBLE(expr.Evaluate(), 3.0, 0.0);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Variables")
  {
    WTempHybridArray<WMathExpression::Input, 4> inputs;
    inputs.SetCount(4);

    {
      WMathExpression expr("_var1 + v2Ar");
      W_TEST_BOOL(expr.IsValid());

      inputs[0] = {WMakeHashedString("_var1"), 1.0};
      inputs[1] = {WMakeHashedString("v2Ar"), 2.0};

      double result = expr.Evaluate(inputs);
      W_TEST_DOUBLE(result, 3.0, 0.0);

      inputs[0].m_fValue = 2.0;
      inputs[1].m_fValue = 0.5;

      result = expr.Evaluate(inputs);
      W_TEST_DOUBLE(result, 2.5, 0.0);
    }

    // Make sure we got the spaces right and don't count it as part of the variable.
    {
      WMathExpression expr("  a +  b /c*d");
      W_TEST_BOOL(expr.IsValid());

      inputs[0] = {WMakeHashedString("a"), 1.0};
      inputs[1] = {WMakeHashedString("b"), 4.0};
      inputs[2] = {WMakeHashedString("c"), 2.0};
      inputs[3] = {WMakeHashedString("d"), 3.0};

      double result = expr.Evaluate(inputs);
      W_TEST_DOUBLE(result, 7.0, 0.0);
    }
  }


  W_TEST_BLOCK(WTestBlock::Enabled, "Invalid Expressions")
  {
    WMuteLog logErrorSink;
    WLogSystemScope ls(&logErrorSink);

    {
      WMathExpression expr("1+");
      W_TEST_BOOL(!expr.IsValid());
    }
    {
      WMathExpression expr("1+/1");
      W_TEST_BOOL(!expr.IsValid());
    }
    {
      WMathExpression expr("(((((0))))");
      W_TEST_BOOL(!expr.IsValid());
    }
    {
      WMathExpression expr("_va£r + asdf");
      W_TEST_BOOL(!expr.IsValid());
    }
    {
      WMathExpression expr("sqrt(2, 4)");
      W_TEST_BOOL(!expr.IsValid());
    }
  }
}
