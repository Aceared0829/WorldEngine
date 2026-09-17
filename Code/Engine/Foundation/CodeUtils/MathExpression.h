#pragma once

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/Strings/String.h>

class WLogInterface;

/// A wrapper around WExpression infrastructure to evaluate simple math expressions
class W_FOUNDATION_DLL WMathExpression
{
public:
  /// Creates a new invalid math expression.
  ///
  /// Need to call Reset before you can do anything with it.
  WMathExpression();

  /// Initializes using a given expression.
  ///
  /// If anything goes wrong it is logged and the math expression is in an invalid state.
  /// \param log
  ///   If null, default log interface will be used.
  explicit WMathExpression(WStringView sExpressionString); // [tested]

  /// Reinitializes using the given expression.
  ///
  /// An empty string or nullptr are considered to be 'invalid' expressions.
  void Reset(WStringView sExpressionString);

  /// Whether the expression is valid and can be evaluated.
  bool IsValid() const { return m_bIsValid; }

  /// Returns the original expression string that this MathExpression can evaluate.
  WStringView GetExpressionString() const { return m_sOriginalExpression; }

  struct Input
  {
    WHashedString m_sName;
    float m_fValue;
  };

  /// Evaluates parsed expression with the given inputs.
  ///
  /// Only way this function can fail is if the expression was not valid.
  /// \see IsValid
  float Evaluate(WArrayPtr<Input> inputs = WArrayPtr<Input>()); // [tested]

private:
  WHashedString m_sOriginalExpression;
  bool m_bIsValid = false;

  WExpressionByteCode m_ByteCode;
  WExpressionVM m_VM;
};
