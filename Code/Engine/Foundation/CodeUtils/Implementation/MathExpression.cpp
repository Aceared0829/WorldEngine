#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/MathExpression.h>

static WHashedString s_sOutput = WMakeHashedString("output");

WMathExpression::WMathExpression() = default;

WMathExpression::WMathExpression(WStringView sExpressionString)
{
  Reset(sExpressionString);
}

void WMathExpression::Reset(WStringView sExpressionString)
{
  m_sOriginalExpression.Assign(sExpressionString);
  m_ByteCode.Clear();
  m_bIsValid = false;

  if (sExpressionString.IsEmpty())
    return;

  WStringBuilder tmp = s_sOutput.GetView();
  tmp.Append(" = ", sExpressionString);

  WExpression::StreamDesc outputs[] = {
    {s_sOutput, WProcessingStream::DataType::Float},
  };

  WExpressionParser parser;
  WExpressionParser::Options parserOptions;
  parserOptions.m_bTreatUnknownVariablesAsInputs = true;

  WExpressionAST ast;
  if (parser.Parse(tmp, WArrayPtr<WExpression::StreamDesc>(), outputs, parserOptions, ast).Failed())
    return;

  WExpressionCompiler compiler;
  if (compiler.Compile(ast, m_ByteCode).Failed())
    return;

  m_bIsValid = true;
}

float WMathExpression::Evaluate(WArrayPtr<Input> inputs)
{
  float fOutput = WMath::NaN<float>();

  if (!IsValid() || m_ByteCode.IsEmpty())
  {
    WLog::Error("Can't evaluate invalid math expression '{0}'", m_sOriginalExpression);
    return fOutput;
  }

  WTempHybridArray<WProcessingStream, 8> inputStreams;
  for (auto& input : inputs)
  {
    if (input.m_sName.IsEmpty())
      continue;

    inputStreams.PushBack(WProcessingStream(input.m_sName, WMakeArrayPtr(&input.m_fValue, 1).ToByteArray(), WProcessingStream::DataType::Float));
  }

  WProcessingStream outputStream(s_sOutput, WMakeArrayPtr(&fOutput, 1).ToByteArray(), WProcessingStream::DataType::Float);
  WArrayPtr<WProcessingStream> outputStreams = WMakeArrayPtr(&outputStream, 1);

  if (m_VM.Execute(m_ByteCode, inputStreams, outputStreams, 1).Failed())
  {
    WLog::Error("Failed to execute expression VM");
  }

  return fOutput;
}
