#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/CodeUtils/Expression/ExpressionCompiler.h>
#include <Foundation/CodeUtils/Expression/ExpressionParser.h>
#include <Foundation/CodeUtils/Expression/ExpressionVM.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/UniquePtr.h>
#include <TestFramework/Utilities/TestLogInterface.h>

namespace
{
  static WUInt32 s_uiNumASTDumps = 0;

  void MakeASTOutputPath(WStringView sOutputName, WStringBuilder& out_sOutputPath)
  {
    WUInt32 uiCounter = s_uiNumASTDumps;
    ++s_uiNumASTDumps;

    out_sOutputPath.SetFormat(":output/Expression/{}_{}_AST.dgml", WArgU(uiCounter, 2, true), sOutputName);
  }

  void DumpDisassembly(const WExpressionByteCode& byteCode, WStringView sOutputName, WUInt32 uiCounter)
  {
    WStringBuilder sDisassembly;
    byteCode.Disassemble(sDisassembly);

    WStringBuilder sFileName;
    sFileName.SetFormat(":output/Expression/{}_{}_ByteCode.txt", WArgU(uiCounter, 2, true), sOutputName);

    WFileWriter fileWriter;
    if (fileWriter.Open(sFileName).Succeeded())
    {
      fileWriter.WriteBytes(sDisassembly.GetData(), sDisassembly.GetElementCount()).IgnoreResult();

      WLog::Error("Disassembly was dumped to: {}", sFileName);
    }
    else
    {
      WLog::Error("Failed to dump Disassembly to: {}", sFileName);
    }
  }

  static WUInt32 s_uiNumByteCodeComparisons = 0;

  bool CompareByteCode(const WExpressionByteCode& testCode, const WExpressionByteCode& referenceCode)
  {
    WUInt32 uiCounter = s_uiNumByteCodeComparisons;
    ++s_uiNumByteCodeComparisons;

    if (testCode != referenceCode)
    {
      DumpDisassembly(referenceCode, "Reference", uiCounter);
      DumpDisassembly(testCode, "Test", uiCounter);
      return false;
    }

    return true;
  }

  static WHashedString s_sA = WMakeHashedString("a");
  static WHashedString s_sB = WMakeHashedString("b");
  static WHashedString s_sC = WMakeHashedString("c");
  static WHashedString s_sD = WMakeHashedString("d");
  static WHashedString s_sOutput = WMakeHashedString("output");

  static WUniquePtr<WExpressionParser> s_pParser;
  static WUniquePtr<WExpressionCompiler> s_pCompiler;
  static WUniquePtr<WExpressionVM> s_pVM;

  template <typename T>
  struct StreamDataTypeDeduction
  {
  };

  template <>
  struct StreamDataTypeDeduction<WFloat16>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Half;
    static WFloat16 Default() { return WMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<float>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Float;
    static float Default() { return WMath::MinValue<float>(); }
  };

  template <>
  struct StreamDataTypeDeduction<WInt8>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Byte;
    static WInt8 Default() { return WMath::MinValue<WInt8>(); }
  };

  template <>
  struct StreamDataTypeDeduction<WInt16>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Short;
    static WInt16 Default() { return WMath::MinValue<WInt16>(); }
  };

  template <>
  struct StreamDataTypeDeduction<int>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Int;
    static int Default() { return WMath::MinValue<int>(); }
  };

  template <>
  struct StreamDataTypeDeduction<WVec3>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Float3;
    static WVec3 Default() { return WVec3(WMath::MinValue<float>()); }
  };

  template <>
  struct StreamDataTypeDeduction<WVec3I32>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Int3;
    static WVec3I32 Default() { return WVec3I32(WMath::MinValue<int>()); }
  };

  template <>
  struct StreamDataTypeDeduction<WVec4>
  {
    static constexpr WProcessingStream::DataType Type = WProcessingStream::DataType::Float4;
    static WVec4 Default() { return WVec4(WMath::MinValue<float>()); }
  };

  template <typename T>
  void Compile(WStringView sCode, WExpressionByteCode& out_byteCode, WStringView sDumpAstOutputName = WStringView())
  {
    WExpression::StreamDesc inputs[] = {
      {s_sA, StreamDataTypeDeduction<T>::Type},
      {s_sB, StreamDataTypeDeduction<T>::Type},
      {s_sC, StreamDataTypeDeduction<T>::Type},
      {s_sD, StreamDataTypeDeduction<T>::Type},
    };

    WExpression::StreamDesc outputs[] = {
      {s_sOutput, StreamDataTypeDeduction<T>::Type},
    };

    WExpressionAST ast;
    W_TEST_BOOL(s_pParser->Parse(sCode, inputs, outputs, {}, ast).Succeeded());

    WStringBuilder sOutputPath;
    if (sDumpAstOutputName.IsEmpty() == false)
    {
      MakeASTOutputPath(sDumpAstOutputName, sOutputPath);
    }
    W_TEST_BOOL(s_pCompiler->Compile(ast, out_byteCode, sOutputPath).Succeeded());
  }

  template <typename T>
  T Execute(const WExpressionByteCode& byteCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0))
  {
    WProcessingStream inputs[] = {
      WProcessingStream(s_sA, WMakeArrayPtr(&a, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      WProcessingStream(s_sB, WMakeArrayPtr(&b, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      WProcessingStream(s_sC, WMakeArrayPtr(&c, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
      WProcessingStream(s_sD, WMakeArrayPtr(&d, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    T output = StreamDataTypeDeduction<T>::Default();
    WProcessingStream outputs[] = {
      WProcessingStream(s_sOutput, WMakeArrayPtr(&output, 1).ToByteArray(), StreamDataTypeDeduction<T>::Type),
    };

    W_TEST_BOOL(s_pVM->Execute(byteCode, inputs, outputs, 1).Succeeded());

    return output;
  };

  template <typename T>
  T TestInstruction(WStringView sCode, T a = T(0), T b = T(0), T c = T(0), T d = T(0), bool bDumpASTs = false)
  {
    WExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestInstruction" : "");
    return Execute<T>(byteCode, a, b, c, d);
  }

  template <typename T>
  T TestConstant(WStringView sCode, bool bDumpASTs = false)
  {
    WExpressionByteCode byteCode;
    Compile<T>(sCode, byteCode, bDumpASTs ? "TestConstant" : "");
    W_TEST_INT(byteCode.GetNumInstructions(), 2); // MovX_C, StoreX
    W_TEST_INT(byteCode.GetNumTempRegisters(), 1);
    return Execute<T>(byteCode);
  }

  enum TestBinaryFlags
  {
    LeftConstantOptimization = W_BIT(0),
    NoInstructionsCountCheck = W_BIT(2),
  };

  template <typename R, typename T, WUInt32 flags>
  void TestBinaryInstruction(WStringView sOp, T a, T b, T expectedResult, bool bDumpASTs = false)
  {
    constexpr bool boolInputs = std::is_same<T, bool>::value;
    using U = typename std::conditional<boolInputs, int, T>::type;

    U aAsU;
    U bAsU;
    U expectedResultAsU;
    if constexpr (boolInputs)
    {
      aAsU = a ? 1 : 0;
      bAsU = b ? 1 : 0;
      expectedResultAsU = expectedResult ? 1 : 0;
    }
    else
    {
      aAsU = a;
      bAsU = b;
      expectedResultAsU = expectedResult;
    }

    auto TestRes = [](U res, U expectedRes, const char* szCode, const char* szAValue, const char* szBValue)
    {
      if constexpr (std::is_same<T, float>::value)
      {
        W_TEST_FLOAT_MSG(res, expectedRes, WMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, int>::value)
      {
        W_TEST_INT_MSG(res, expectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, bool>::value)
      {
        const char* szRes = (res != 0) ? "true" : "false";
        const char* szExpectedRes = (expectedRes != 0) ? "true" : "false";
        W_TEST_STRING_MSG(szRes, szExpectedRes, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, WVec3>::value)
      {
        W_TEST_VEC3_MSG(res, expectedRes, WMath::DefaultEpsilon<float>(), "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else if constexpr (std::is_same<T, WVec3I32>::value)
      {
        W_TEST_INT_MSG(res.x, expectedRes.x, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        W_TEST_INT_MSG(res.y, expectedRes.y, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
        W_TEST_INT_MSG(res.z, expectedRes.z, "%s (a=%s, b=%s)", szCode, szAValue, szBValue);
      }
      else
      {
        W_ASSERT_NOT_IMPLEMENTED;
      }
    };

    const bool functionStyleSyntax = sOp.FindSubString("(");
    const char* formatString = functionStyleSyntax ? "output = {0}{1}, {2})" : "output = {1} {0} {2}";
    const char* aInput = boolInputs ? "(a != 0)" : "a";
    const char* bInput = boolInputs ? "(b != 0)" : "b";

    WStringBuilder aValue;
    WStringBuilder bValue;
    if constexpr (std::is_same<T, WVec3>::value || std::is_same<T, WVec3I32>::value)
    {
      aValue.SetFormat("vec3({}, {}, {})", a.x, a.y, a.z);
      bValue.SetFormat("vec3({}, {}, {})", b.x, b.y, b.z);
    }
    else
    {
      aValue.SetFormat("{}", a);
      bValue.SetFormat("{}", b);
    }

    int oneConstantInstructions = 3; // LoadX, OpX_RC, StoreX
    int oneConstantRegisters = 1;
    if constexpr (std::is_same<R, bool>::value)
    {
      oneConstantInstructions += 3; // + MovX_C, MovX_C, SelI_RRR
      oneConstantRegisters += 2;    // Two more registers needed for constants above
    }
    if constexpr (boolInputs)
    {
      oneConstantInstructions += 1; // + NotEqI_RC
    }

    int numOutputElements = 1;
    bool hasDifferentOutputElements = false;
    if constexpr (std::is_same<T, WVec3>::value || std::is_same<T, WVec3I32>::value)
    {
      numOutputElements = 3;

      for (int i = 1; i < 3; ++i)
      {
        if (expectedResult.GetData()[i] != expectedResult.GetData()[i - 1])
        {
          hasDifferentOutputElements = true;
          break;
        }
      }
    }

    WStringBuilder code;
    WExpressionByteCode byteCode;

    code.SetFormat(formatString, sOp, aInput, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryNoConstants" : "");
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aValue, bInput);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryLeftConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      int leftConstantInstructions = oneConstantInstructions;
      int leftConstantRegisters = oneConstantRegisters;
      if constexpr ((flags & LeftConstantOptimization) == 0)
      {
        leftConstantInstructions += 1;
        leftConstantRegisters += 1;
      }

      if (byteCode.GetNumInstructions() != leftConstantInstructions || byteCode.GetNumTempRegisters() != leftConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryLeftConstant", 0);
        W_TEST_INT(byteCode.GetNumInstructions(), leftConstantInstructions);
        W_TEST_INT(byteCode.GetNumTempRegisters(), leftConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aInput, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryRightConstant" : "");
    if constexpr ((flags & NoInstructionsCountCheck) == 0)
    {
      if (byteCode.GetNumInstructions() != oneConstantInstructions || byteCode.GetNumTempRegisters() != oneConstantRegisters)
      {
        DumpDisassembly(byteCode, "BinaryRightConstant", 0);
        W_TEST_INT(byteCode.GetNumInstructions(), oneConstantInstructions);
        W_TEST_INT(byteCode.GetNumTempRegisters(), oneConstantRegisters);
      }
    }
    TestRes(Execute<U>(byteCode, aAsU, bAsU), expectedResultAsU, code, aValue, bValue);

    code.SetFormat(formatString, sOp, aValue, bValue);
    Compile<U>(code, byteCode, bDumpASTs ? "BinaryConstant" : "");
    if (hasDifferentOutputElements == false)
    {
      int bothConstantsInstructions = 1 + numOutputElements; // MovX_C + StoreX * numOutputElements
      int bothConstantsRegisters = 1;
      if (byteCode.GetNumInstructions() != bothConstantsInstructions || byteCode.GetNumTempRegisters() != bothConstantsRegisters)
      {
        DumpDisassembly(byteCode, "BinaryConstant", 0);
        W_TEST_INT(byteCode.GetNumInstructions(), bothConstantsInstructions);
        W_TEST_INT(byteCode.GetNumTempRegisters(), bothConstantsRegisters);
      }
    }
    TestRes(Execute<U>(byteCode), expectedResultAsU, code, aValue, bValue);
  }

  template <typename T>
  bool CompareCode(WStringView sTestCode, WStringView sReferenceCode, WExpressionByteCode& out_testByteCode, bool bDumpASTs = false)
  {
    Compile<T>(sTestCode, out_testByteCode, bDumpASTs ? "Test" : "");

    WExpressionByteCode referenceByteCode;
    Compile<T>(sReferenceCode, referenceByteCode, bDumpASTs ? "Reference" : "");

    return CompareByteCode(out_testByteCode, referenceByteCode);
  }

  template <typename T>
  void TestInputOutput()
  {
    WStringView testCode = "output = a + b * 2";
    WExpressionByteCode testByteCode;
    Compile<T>(testCode, testByteCode);

    constexpr WUInt32 uiCount = 17;
    WTempHybridArray<T, uiCount> a;
    WTempHybridArray<T, uiCount> b;
    WTempHybridArray<T, uiCount> o;
    WTempHybridArray<float, uiCount> expectedOutput;
    a.SetCountUninitialized(uiCount);
    b.SetCountUninitialized(uiCount);
    o.SetCount(uiCount);
    expectedOutput.SetCountUninitialized(uiCount);

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      a[i] = static_cast<T>(3.0f * i);
      b[i] = static_cast<T>(1.5f * i);
      expectedOutput[i] = a[i] + b[i] * 2.0f;
    }

    WProcessingStream inputs[] = {
      WProcessingStream(s_sA, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      WProcessingStream(s_sB, b.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
      WProcessingStream(s_sC, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type), // Dummy stream, not actually used
      WProcessingStream(s_sD, a.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type), // Dummy stream, not actually used
    };

    WProcessingStream outputs[] = {
      WProcessingStream(s_sOutput, o.GetByteArrayPtr(), StreamDataTypeDeduction<T>::Type),
    };

    W_TEST_BOOL(s_pVM->Execute(testByteCode, inputs, outputs, uiCount, WExpression::GlobalData(), WExpressionVM::Flags::BestPerformance).Succeeded());

    for (WUInt32 i = 0; i < uiCount; ++i)
    {
      W_TEST_FLOAT(static_cast<float>(o[i]), expectedOutput[i], WMath::DefaultEpsilon<float>());
    }
  }

  static const WEnum<WExpression::RegisterType> s_TestFunc1InputTypes[] = {WExpression::RegisterType::Float, WExpression::RegisterType::Int};
  static const WEnum<WExpression::RegisterType> s_TestFunc2InputTypes[] = {WExpression::RegisterType::Float, WExpression::RegisterType::Float, WExpression::RegisterType::Int};

  static void TestFunc1(WExpression::Inputs inputs, WExpression::Output output, const WExpression::GlobalData& globalData)
  {
    const WExpression::Register* pX = inputs[0].GetPtr();
    const WExpression::Register* pY = inputs[1].GetPtr();
    const WExpression::Register* pXEnd = inputs[0].GetEndPtr();
    WExpression::Register* pOutput = output.GetPtr();

    while (pX < pXEnd)
    {
      pOutput->f = pX->f.CompMul(pY->i.ToFloat());

      ++pX;
      ++pY;
      ++pOutput;
    }
  }

  static void TestFunc2(WExpression::Inputs inputs, WExpression::Output output, const WExpression::GlobalData& globalData)
  {
    const WExpression::Register* pX = inputs[0].GetPtr();
    const WExpression::Register* pY = inputs[1].GetPtr();
    const WExpression::Register* pXEnd = inputs[0].GetEndPtr();
    WExpression::Register* pOutput = output.GetPtr();

    if (inputs.GetCount() >= 3)
    {
      const WExpression::Register* pZ = inputs[2].GetPtr();

      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f + pZ->i.ToFloat();

        ++pX;
        ++pY;
        ++pZ;
        ++pOutput;
      }
    }
    else
    {
      while (pX < pXEnd)
      {
        pOutput->f = pX->f.CompMul(pY->f) * 2.0f;

        ++pX;
        ++pY;
        ++pOutput;
      }
    }
  }

  WExpressionFunction s_TestFunc1 = {
    {WMakeHashedString("TestFunc"), WExpression::FunctionDesc::TypeList(s_TestFunc1InputTypes), 2, WExpression::RegisterType::Float},
    &TestFunc1,
  };

  WExpressionFunction s_TestFunc2 = {
    {WMakeHashedString("TestFunc"), WExpression::FunctionDesc::TypeList(s_TestFunc2InputTypes), 3, WExpression::RegisterType::Float},
    &TestFunc2,
  };

} // namespace

W_CREATE_SIMPLE_TEST(CodeUtils, Expression)
{
  s_uiNumByteCodeComparisons = 0;

  WStringBuilder outputPath = WTestFramework::GetInstance()->GetAbsOutputPath();
  W_TEST_BOOL(WFileSystem::AddDataDirectory(outputPath.GetData(), "test", "output", WDataDirUsage::AllowWrites) == W_SUCCESS);

  s_pParser = W_DEFAULT_NEW(WExpressionParser);
  s_pCompiler = W_DEFAULT_NEW(WExpressionCompiler);
  s_pVM = W_DEFAULT_NEW(WExpressionVM);
  W_SCOPE_EXIT(s_pParser = nullptr; s_pCompiler = nullptr; s_pVM = nullptr;);

  W_TEST_BLOCK(WTestBlock::Enabled, "Unary instructions")
  {
    // Negate
    W_TEST_INT(TestInstruction("output = -a", 2), -2);
    W_TEST_FLOAT(TestInstruction("output = -a", 2.5f), -2.5f, WMath::DefaultEpsilon<float>());
    W_TEST_INT(TestConstant<int>("output = -2"), -2);
    W_TEST_FLOAT(TestConstant<float>("output = -2.5"), -2.5f, WMath::DefaultEpsilon<float>());

    // Absolute
    W_TEST_INT(TestInstruction("output = abs(a)", -2), 2);
    W_TEST_FLOAT(TestInstruction("output = abs(a)", -2.5f), 2.5f, WMath::DefaultEpsilon<float>());
    W_TEST_INT(TestConstant<int>("output = abs(-2)"), 2);
    W_TEST_FLOAT(TestConstant<float>("output = abs(-2.5)"), 2.5f, WMath::DefaultEpsilon<float>());

    // Saturate
    W_TEST_INT(TestInstruction("output = saturate(a)", -1), 0);
    W_TEST_INT(TestInstruction("output = saturate(a)", 2), 1);
    W_TEST_FLOAT(TestInstruction("output = saturate(a)", -1.5f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = saturate(a)", 2.5f), 1.0f, WMath::DefaultEpsilon<float>());

    W_TEST_INT(TestConstant<int>("output = saturate(-1)"), 0);
    W_TEST_INT(TestConstant<int>("output = saturate(2)"), 1);
    W_TEST_FLOAT(TestConstant<float>("output = saturate(-1.5)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = saturate(2.5)"), 1.0f, WMath::DefaultEpsilon<float>());

    // Sqrt
    W_TEST_FLOAT(TestInstruction("output = sqrt(a)", 25.0f), 5.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = sqrt(a)", 2.0f), WMath::Sqrt(2.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = sqrt(25)"), 5.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = sqrt(2)"), WMath::Sqrt(2.0f), WMath::DefaultEpsilon<float>());

    // Exp
    W_TEST_FLOAT(TestInstruction("output = exp(a)", 0.0f), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = exp(a)", 2.0f), WMath::Exp(2.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = exp(0.0)"), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = exp(2.0)"), WMath::Exp(2.0f), WMath::DefaultEpsilon<float>());

    // Ln
    W_TEST_FLOAT(TestInstruction("output = ln(a)", 1.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = ln(a)", 2.0f), WMath::Ln(2.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = ln(1.0)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = ln(2.0)"), WMath::Ln(2.0f), WMath::DefaultEpsilon<float>());

    // Log2
    W_TEST_INT(TestInstruction("output = log2(a)", 1), 0);
    W_TEST_INT(TestInstruction("output = log2(a)", 8), 3);
    W_TEST_FLOAT(TestInstruction("output = log2(a)", 1.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = log2(a)", 4.0f), 2.0f, WMath::DefaultEpsilon<float>());

    W_TEST_INT(TestConstant<int>("output = log2(1)"), 0);
    W_TEST_INT(TestConstant<int>("output = log2(16)"), 4);
    W_TEST_FLOAT(TestConstant<float>("output = log2(1.0)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = log2(32.0)"), 5.0f, WMath::DefaultEpsilon<float>());

    // Log10
    W_TEST_FLOAT(TestInstruction("output = log10(a)", 10.0f), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = log10(a)", 1000.0f), 3.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = log10(10.0)"), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = log10(100.0)"), 2.0f, WMath::DefaultEpsilon<float>());

    // Pow2
    W_TEST_INT(TestInstruction("output = pow2(a)", 0), 1);
    W_TEST_INT(TestInstruction("output = pow2(a)", 3), 8);
    W_TEST_FLOAT(TestInstruction("output = pow2(a)", 4.0f), 16.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = pow2(a)", 6.0f), 64.0f, WMath::DefaultEpsilon<float>());

    W_TEST_INT(TestConstant<int>("output = pow2(0)"), 1);
    W_TEST_INT(TestConstant<int>("output = pow2(3)"), 8);
    W_TEST_FLOAT(TestConstant<float>("output = pow2(3.0)"), 8.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = pow2(5.0)"), 32.0f, WMath::DefaultEpsilon<float>());

    // Sin
    W_TEST_FLOAT(TestInstruction("output = sin(a)", WAngle::MakeFromDegree(90.0f).GetRadian()), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = sin(a)", WAngle::MakeFromDegree(45.0f).GetRadian()), WMath::Sin(WAngle::MakeFromDegree(45.0f)), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = sin(PI / 2)"), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = sin(PI / 4)"), WMath::Sin(WAngle::MakeFromDegree(45.0f)), WMath::DefaultEpsilon<float>());

    // Cos
    W_TEST_FLOAT(TestInstruction("output = cos(a)", 0.0f), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = cos(a)", WAngle::MakeFromDegree(45.0f).GetRadian()), WMath::Cos(WAngle::MakeFromDegree(45.0f)), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = cos(0)"), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = cos(PI / 4)"), WMath::Cos(WAngle::MakeFromDegree(45.0f)), WMath::DefaultEpsilon<float>());

    // Tan
    W_TEST_FLOAT(TestInstruction("output = tan(a)", 0.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = tan(a)", WAngle::MakeFromDegree(45.0f).GetRadian()), 1.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = tan(0)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = tan(PI / 4)"), 1.0f, WMath::DefaultEpsilon<float>());

    // ASin
    W_TEST_FLOAT(TestInstruction("output = asin(a)", 1.0f), WAngle::MakeFromDegree(90.0f).GetRadian(), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = asin(a)", WMath::Sin(WAngle::MakeFromDegree(45.0f))), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = asin(1)"), WAngle::MakeFromDegree(90.0f).GetRadian(), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = asin(sin(PI / 4))"), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::LargeEpsilon<float>());

    // ACos
    W_TEST_FLOAT(TestInstruction("output = acos(a)", 1.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = acos(a)", WMath::Cos(WAngle::MakeFromDegree(45.0f))), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = acos(1)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = acos(cos(PI / 4))"), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::LargeEpsilon<float>());

    // ATan
    W_TEST_FLOAT(TestInstruction("output = atan(a)", 0.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = atan(a)", 1.0f), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = atan(0)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = atan(1)"), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::DefaultEpsilon<float>());

    // RadToDeg
    W_TEST_FLOAT(TestInstruction("output = radToDeg(a)", WAngle::MakeFromDegree(135.0f).GetRadian()), 135.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = rad_to_deg(a)", WAngle::MakeFromDegree(180.0f).GetRadian()), 180.0f, WMath::LargeEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = radToDeg(PI / 2)"), 90.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = rad_to_deg(PI/4)"), 45.0f, WMath::DefaultEpsilon<float>());

    // DegToRad
    W_TEST_FLOAT(TestInstruction("output = degToRad(a)", 135.0f), WAngle::MakeFromDegree(135.0f).GetRadian(), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = deg_to_rad(a)", 180.0f), WAngle::MakeFromDegree(180.0f).GetRadian(), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = degToRad(90.0)"), WAngle::MakeFromDegree(90.0f).GetRadian(), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = deg_to_rad(45)"), WAngle::MakeFromDegree(45.0f).GetRadian(), WMath::DefaultEpsilon<float>());

    // Round
    W_TEST_FLOAT(TestInstruction("output = round(a)", 12.34f), 12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = round(a)", -12.34f), -12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = round(a)", 12.54f), 13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = round(a)", -12.54f), -13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = round(4.3)"), 4, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = round(4.51)"), 5, WMath::DefaultEpsilon<float>());

    // Floor
    W_TEST_FLOAT(TestInstruction("output = floor(a)", 12.34f), 12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = floor(a)", -12.34f), -13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = floor(a)", 12.54f), 12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = floor(a)", -12.54f), -13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = floor(4.3)"), 4, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = floor(4.51)"), 4, WMath::DefaultEpsilon<float>());

    // Ceil
    W_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.34f), 13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.34f), -12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = ceil(a)", 12.54f), 13, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = ceil(a)", -12.54f), -12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = ceil(4.3)"), 5, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = ceil(4.51)"), 5, WMath::DefaultEpsilon<float>());

    // Trunc
    W_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.34f), 12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.34f), -12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = trunc(a)", 12.54f), 12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = trunc(a)", -12.54f), -12, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = trunc(4.3)"), 4, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = trunc(4.51)"), 4, WMath::DefaultEpsilon<float>());

    // Frac
    W_TEST_FLOAT(TestInstruction("output = frac(a)", 12.34f), 0.34f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = frac(a)", -12.34f), -0.34f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = frac(a)", 12.54f), 0.54f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = frac(a)", -12.54f), -0.54f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = frac(4.3)"), 0.3f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = frac(4.51)"), 0.51f, WMath::DefaultEpsilon<float>());

    // Length
    W_TEST_VEC3(TestInstruction<WVec3>("output = length(a)", WVec3(0, 4, 3)), WVec3(5), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(TestInstruction<WVec3>("output = length(a)", WVec3(-3, 4, 0)), WVec3(5), WMath::DefaultEpsilon<float>());

    // Normalize
    W_TEST_VEC3(TestInstruction<WVec3>("output = normalize(a)", WVec3(1, 4, 3)), WVec3(1, 4, 3).GetNormalized(), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(TestInstruction<WVec3>("output = normalize(a)", WVec3(-3, 7, 22)), WVec3(-3, 7, 22).GetNormalized(), WMath::DefaultEpsilon<float>());

    // Length and normalize optimization
    {
      WStringView testCode = "var x = length(a); var na = normalize(a); output = b * x + na";
      WStringView referenceCode = "var x = length(a); var na = a / x; output = b * x + na";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<WVec3>(testCode, referenceCode, testByteCode));

      WVec3 a = WVec3(0, 4, 3);
      WVec3 b = WVec3(1, 0, 0);
      WVec3 res = b * a.GetLength() + a.GetNormalized();
      W_TEST_VEC3(Execute(testByteCode, a, b), res, WMath::DefaultEpsilon<float>());
    }

    // BitwiseNot
    W_TEST_INT(TestInstruction("output = ~a", 1), ~1);
    W_TEST_INT(TestInstruction("output = ~a", 8), ~8);
    W_TEST_INT(TestConstant<int>("output = ~1"), ~1);
    W_TEST_INT(TestConstant<int>("output = ~17"), ~17);

    // LogicalNot
    W_TEST_INT(TestInstruction("output = !(a == 1)", 1), 0);
    W_TEST_INT(TestInstruction("output = !(a == 1)", 8), 1);
    W_TEST_INT(TestConstant<int>("output = !(1 == 1)"), 0);
    W_TEST_INT(TestConstant<int>("output = !(8 == 1)"), 1);

    // All
    W_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", WVec3(1, 2, 3), WVec3(1, 2, 3)), WVec3(1), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(TestInstruction("var t = (a == b); output = all(t)", WVec3(1, 2, 3), WVec3(1, 2, 4)), WVec3(0), WMath::DefaultEpsilon<float>());

    // Any
    W_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", WVec3(1, 2, 3), WVec3(4, 5, 3)), WVec3(1), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(TestInstruction("var t = (a == b); output = any(t)", WVec3(1, 2, 3), WVec3(4, 5, 6)), WVec3(0), WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Binary instructions")
  {
    // Add
    TestBinaryInstruction<int, int, LeftConstantOptimization>("+", 3, 5, 8);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("+", 3.5f, 5.3f, 8.8f);

    // Subtract
    TestBinaryInstruction<int, int, 0>("-", 9, 5, 4);
    TestBinaryInstruction<float, float, 0>("-", 9.5f, 5.3f, 4.2f);

    // Multiply
    TestBinaryInstruction<int, int, LeftConstantOptimization>("*", 3, 5, 15);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("*", 3.5f, 5.3f, 18.55f);

    // Divide
    TestBinaryInstruction<int, int, 0>("/", 11, 5, 2);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("/", -11, 4, -2); // divide by power of 2 optimization
    TestBinaryInstruction<int, int, 0>("/", 11, -4, -2);                        // divide by power of 2 optimization only works for positive divisors
    TestBinaryInstruction<float, float, 0>("/", 12.6f, 3.0f, 4.2f);

    // Modulo
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", 13, 5, 3);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", -13, 5, -3);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", 13, 4, 1);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("%", -13, 4, -1);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("%", 13.5, 5.0, 3.5);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("mod(", -13.5, 5.0, -3.5);

    // Log
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 2.0f, 1024.0f, 10.0f);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("log(", 7.1f, 81.62f, WMath::Log(7.1f, 81.62f));

    // Pow
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("pow(", 2, 5, 32);
    TestBinaryInstruction<int, int, NoInstructionsCountCheck>("pow(", 3, 3, 27);

    // Pow is replaced by multiplication for constant exponents up until 16.
    // Test all of them to ensure the multiplication tables are correct.
    for (int i = 0; i <= 16; ++i)
    {
      WStringBuilder testCode;
      testCode.SetFormat("output = pow(a, {})", i);

      WExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      W_TEST_INT(Execute(testByteCode, 3), WMath::Pow(3, i));
    }

    {
      WStringView testCode = "output = pow(a, 7)";
      WStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; output = a6 * a";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      W_TEST_INT(Execute(testByteCode, 3), 2187);
    }

    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 2.0, 5.0, 32.0);
    TestBinaryInstruction<float, float, NoInstructionsCountCheck>("pow(", 3.0f, 7.9f, WMath::Pow(3.0f, 7.9f));

    {
      WStringView testCode = "output = pow(a, 15.0)";
      WStringView referenceCode = "var a2 = a * a; var a3 = a2 * a; var a6 = a3 * a3; var a12 = a6 * a6; output = a12 * a3";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      W_TEST_FLOAT(Execute(testByteCode, 2.1f), WMath::Pow(2.1f, 15.0f), WMath::DefaultEpsilon<float>());
    }

    // Min
    TestBinaryInstruction<int, int, LeftConstantOptimization>("min(", 11, 5, 5);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("min(", 12.6f, 3.0f, 3.0f);

    // Max
    TestBinaryInstruction<int, int, LeftConstantOptimization>("max(", 11, 5, 11);
    TestBinaryInstruction<float, float, LeftConstantOptimization>("max(", 12.6f, 3.0f, 12.6f);

    // Dot
    TestBinaryInstruction<WVec3, WVec3, NoInstructionsCountCheck>("dot(", WVec3(1, -2, 3), WVec3(-5, -6, 7), WVec3(28));
    TestBinaryInstruction<WVec3I32, WVec3I32, NoInstructionsCountCheck>("dot(", WVec3I32(1, -2, 3), WVec3I32(-5, -6, 7), WVec3I32(28));

    // Cross
    TestBinaryInstruction<WVec3, WVec3, NoInstructionsCountCheck>("cross(", WVec3(1, 0, 0), WVec3(0, 1, 0), WVec3(0, 0, 1));
    TestBinaryInstruction<WVec3, WVec3, NoInstructionsCountCheck>("cross(", WVec3(0, 1, 0), WVec3(0, 0, 1), WVec3(1, 0, 0));
    TestBinaryInstruction<WVec3, WVec3, NoInstructionsCountCheck>("cross(", WVec3(0, 0, 1), WVec3(1, 0, 0), WVec3(0, 1, 0));

    // Reflect
    TestBinaryInstruction<WVec3, WVec3, NoInstructionsCountCheck>("reflect(", WVec3(1, 2, -1), WVec3(0, 0, 1), WVec3(1, 2, 1));

    // BitshiftLeft
    TestBinaryInstruction<int, int, 0>("<<", 11, 5, 11 << 5);

    // BitshiftRight
    TestBinaryInstruction<int, int, 0>(">>", 0xABCD, 8, 0xAB);

    // BitwiseAnd
    TestBinaryInstruction<int, int, LeftConstantOptimization>("&", 0xFFCD, 0xABFF, 0xABCD);

    // BitwiseXor
    TestBinaryInstruction<int, int, LeftConstantOptimization>("^", 0xFFCD, 0xABFF, 0xFFCD ^ 0xABFF);

    // BitwiseOr
    TestBinaryInstruction<int, int, LeftConstantOptimization>("|", 0x00CD, 0xAB00, 0xABCD);

    // Equal
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("==", 11, 5, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("==", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("==", true, false, false);

    // NotEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("!=", 11, 5, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("!=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("!=", true, false, true);

    // Less
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<", 11, 5, 0);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<", 12.6f, 12.6f, 0.0f);

    // LessEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<=", 11, 5, 0);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>("<=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 3.0f, 0.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>("<=", 12.6f, 12.6f, 1.0f);

    // Greater
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">", 11, 5, 1);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">", 11, 11, 0);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">", 12.6f, 12.6f, 0.0f);

    // GreaterEqual
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">=", 11, 5, 1);
    TestBinaryInstruction<bool, int, LeftConstantOptimization>(">=", 11, 11, 1);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 3.0f, 1.0f);
    TestBinaryInstruction<bool, float, LeftConstantOptimization>(">=", 12.6f, 12.6f, 1.0f);

    // LogicalAnd
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("&&", true, false, false);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("&&", true, true, true);

    // LogicalOr
    TestBinaryInstruction<bool, bool, LeftConstantOptimization | NoInstructionsCountCheck>("||", true, false, true);
    TestBinaryInstruction<bool, bool, LeftConstantOptimization>("||", false, false, false);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Ternary instructions")
  {
    // Clamp
    W_TEST_INT(TestInstruction("output = clamp(a, b, c)", -1, 0, 10), 0);
    W_TEST_INT(TestInstruction("output = clamp(a, b, c)", 2, 0, 10), 2);
    W_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", -1.5f, 0.0f, 1.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = clamp(a, b, c)", 2.5f, 0.0f, 1.0f), 1.0f, WMath::DefaultEpsilon<float>());

    W_TEST_INT(TestConstant<int>("output = clamp(-1, 0, 10)"), 0);
    W_TEST_INT(TestConstant<int>("output = clamp(2, 0, 10)"), 2);
    W_TEST_FLOAT(TestConstant<float>("output = clamp(-1.5, 0, 2)"), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = clamp(2.5, 0, 2)"), 2.0f, WMath::DefaultEpsilon<float>());

    // Select
    W_TEST_INT(TestInstruction("output = (a == 1) ? b : c", 1, 2, 3), 2);
    W_TEST_INT(TestInstruction("output = a != 1 ? b : c", 1, 2, 3), 3);
    W_TEST_FLOAT(TestInstruction("output = (a == 1) ? b : c", 1.0f, 2.4f, 3.5f), 2.4f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = a != 1 ? b : c", 1.0f, 2.4f, 3.5f), 3.5f, WMath::DefaultEpsilon<float>());
    W_TEST_INT(TestInstruction("output = (a == 1) ? (b > 2) : (c > 2)", 1, 2, 3), 0);
    W_TEST_INT(TestInstruction("output = a != 1 ? b > 2 : c > 2", 1, 2, 3), 1);

    W_TEST_INT(TestConstant<int>("output = (1 == 1) ? 2 : 3"), 2);
    W_TEST_INT(TestConstant<int>("output = 1 != 1 ? 2 : 3"), 3);
    W_TEST_FLOAT(TestConstant<float>("output = (1.0 == 1.0) ? 2.4 : 3.5"), 2.4f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = 1.0 != 1.0 ? 2.4 : 3.5"), 3.5f, WMath::DefaultEpsilon<float>());
    W_TEST_INT(TestConstant<int>("output = (1 == 1) ? false : true"), 0);
    W_TEST_INT(TestConstant<int>("output = 1 != 1 ? false : true"), 1);

    // Lerp
    W_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", 1.0f, 5.0f, 0.75f), 4.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = lerp(a, b, c)", -1.0f, -11.0f, 0.1f), -2.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = lerp(1, 5, 0.75)"), 4.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = lerp(-1, -11, 0.1)"), -2.0f, WMath::DefaultEpsilon<float>());

    // SmoothStep
    W_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.0f, 0.0f, 1.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.2f, 0.0f, 1.0f), WMath::SmoothStep(0.2f, 0.0f, 1.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.5f, 0.0f, 1.0f), 0.5f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.2f, 0.2f, 0.8f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smoothstep(a, b, c)", 0.4f, 0.2f, 0.8f), WMath::SmoothStep(0.4f, 0.2f, 0.8f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = smoothstep(0.2, 0, 1)"), WMath::SmoothStep(0.2f, 0.0f, 1.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = smoothstep(0.4, 0.2, 0.8)"), WMath::SmoothStep(0.4f, 0.2f, 0.8f), WMath::DefaultEpsilon<float>());

    // SmootherStep
    W_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.0f, 0.0f, 1.0f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.2f, 0.0f, 1.0f), WMath::SmootherStep(0.2f, 0.0f, 1.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.5f, 0.0f, 1.0f), 0.5f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.2f, 0.2f, 0.8f), 0.0f, WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestInstruction("output = smootherstep(a, b, c)", 0.4f, 0.2f, 0.8f), WMath::SmootherStep(0.4f, 0.2f, 0.8f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = smootherstep(0.2, 0, 1)"), WMath::SmootherStep(0.2f, 0.0f, 1.0f), WMath::DefaultEpsilon<float>());
    W_TEST_FLOAT(TestConstant<float>("output = smootherstep(0.4, 0.2, 0.8)"), WMath::SmootherStep(0.4f, 0.2f, 0.8f), WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Local variables")
  {
    WExpressionByteCode referenceByteCode;
    {
      WStringView code = "output = (a + b) * 2";
      Compile<float>(code, referenceByteCode);
    }

    WExpressionByteCode testByteCode;

    WStringView code = "var e = a + b; output = e * 2";
    Compile<float>(code, testByteCode);
    W_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e = e * 2; output = e";
    Compile<float>(code, testByteCode);
    W_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; e *= 2; output = e";
    Compile<float>(code, testByteCode);
    W_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    code = "var e = a + b; var f = e; e = 2; output = f * e";
    Compile<float>(code, testByteCode);
    W_TEST_BOOL(CompareByteCode(testByteCode, referenceByteCode));

    W_TEST_FLOAT(Execute(testByteCode, 2.0f, 3.0f), 10.0f, WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Assignment")
  {
    {
      WStringView testCode = "output = 40; output += 2";
      WStringView referenceCode = "output = 42";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      W_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, WMath::DefaultEpsilon<float>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Integer arithmetic")
  {
    WExpressionByteCode testByteCode;

    WStringView code = "output = ((a & 0xFF) << 8) | (b & 0xFFFF >> 8)";
    Compile<int>(code, testByteCode);

    const int a = 0xABABABAB;
    const int b = 0xCDCDCDCD;
    W_TEST_INT(Execute(testByteCode, a, b), 0xABCD);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constant folding")
  {
    WStringView testCode = "var x = abs(-7) + saturate(2) + 2\n"
                            "var v = (sqrt(25) - 4) * 5\n"
                            "var m = min(300, 1000) / max(1, 3);"
                            "var r = m - x * 5 - v - clamp(13, 1, 3);\n"
                            "output = r";

    WStringView referenceCode = "output = 42";

    {
      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));

      W_TEST_FLOAT(Execute<float>(testByteCode), 42.0f, WMath::DefaultEpsilon<float>());
    }

    {
      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));

      W_TEST_INT(Execute<int>(testByteCode), 42);
    }

    testCode = "";
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Constant instructions")
  {
    // There are special instructions in the vm which take the constant as the first operand in place and
    // don't require an extra mov for the constant.
    // This test checks whether the compiler transforms operations with constants as second operands to the preferred form.

    WStringView testCode = "output = (2 + a) + (-1 + b) + (2 * c) + (d / 5) + min(1, c) + max(2, d)";

    {
      WStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d * 0.2) + min(c, 1) + max(d, 2)";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      W_TEST_INT(testByteCode.GetNumInstructions(), 16);
      W_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      W_TEST_FLOAT(Execute(testByteCode, 1.0f, 2.0f, 3.0f, 40.f), 59.0f, WMath::DefaultEpsilon<float>());
    }

    {
      WStringView referenceCode = "output = (a + 2) + (b + -1) + (c * 2) + (d / 5) + min(c, 1) + max(d, 2)";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      W_TEST_INT(testByteCode.GetNumInstructions(), 16);
      W_TEST_INT(testByteCode.GetNumTempRegisters(), 4);
      W_TEST_INT(Execute(testByteCode, 1, 2, 3, 40), 59);
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Integer and float conversions")
  {
    WStringView testCode = "var x = 7; var y = 0.6\n"
                            "var e = a * x * b * y\n"
                            "int i = c * 2; i *= i; e += i\n"
                            "output = e";

    WStringView referenceCode = "int i = (int(c) * 2); output = int((float(a * 7 * b) * 0.6) + float(i * i))";

    WExpressionByteCode testByteCode;
    W_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
    W_TEST_INT(Execute(testByteCode, 1, 2, 3), 44);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Bool conversions")
  {
    WStringView testCode = "var x = true\n"
                            "bool y = a\n"
                            "output = x == y";

    {
      WStringView referenceCode = "bool r = true == (a != 0); output = r ? 1 : 0";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
      W_TEST_INT(Execute(testByteCode, 14), 1);
    }

    {
      WStringView referenceCode = "bool r = true == (a != 0); output = r ? 1.0 : 0.0";

      WExpressionByteCode testByteCode;
      W_TEST_BOOL(CompareCode<float>(testCode, referenceCode, testByteCode));
      W_TEST_FLOAT(Execute(testByteCode, 15.0f), 1.0f, WMath::DefaultEpsilon<float>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Load Inputs/Store Outputs")
  {
    TestInputOutput<float>();
    TestInputOutput<WFloat16>();

    TestInputOutput<int>();
    TestInputOutput<WInt16>();
    TestInputOutput<WInt8>();
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Function overloads")
  {
    s_pParser->RegisterFunction(s_TestFunc1.m_Desc);
    s_pParser->RegisterFunction(s_TestFunc2.m_Desc);

    s_pVM->RegisterFunction(s_TestFunc1);
    s_pVM->RegisterFunction(s_TestFunc2);

    {
      // take TestFunc1 overload for all ints
      WStringView testCode = "output = TestFunc(1, 2, 3)";
      WExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      W_TEST_INT(Execute<int>(testByteCode), 2);
    }

    {
      // take TestFunc1 overload for float, int
      WStringView testCode = "output = TestFunc(1.0, 2, 3)";
      WExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      W_TEST_INT(Execute<int>(testByteCode), 2);
    }

    {
      // take TestFunc2 overload for int, float
      WStringView testCode = "output = TestFunc(1, 2.0, 3)";
      WExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      W_TEST_INT(Execute<int>(testByteCode), 7);
    }

    {
      // take TestFunc2 overload for all float
      WStringView testCode = "output = TestFunc(1.0, 2.0, 3)";
      WExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      W_TEST_INT(Execute<int>(testByteCode), 7);
    }

    {
      // take TestFunc1 overload when only two params are given
      WStringView testCode = "output = TestFunc(1.0, 2.0)";
      WExpressionByteCode testByteCode;
      Compile<int>(testCode, testByteCode);
      W_TEST_INT(Execute<int>(testByteCode), 2);
    }

    s_pParser->UnregisterFunction(s_TestFunc1.m_Desc);
    s_pParser->UnregisterFunction(s_TestFunc2.m_Desc);

    s_pVM->UnregisterFunction(s_TestFunc1);
    s_pVM->UnregisterFunction(s_TestFunc2);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Common subexpression elimination")
  {
    WStringView testCode = "var x1 = a * max(b, c)\n"
                            "var x2 = max(c, b) * a\n"
                            "var y1 = a * pow(2, 3)\n"
                            "var y2 = 8 * a\n"
                            "output = x1 + x2 + y1 + y2";

    WStringView referenceCode = "var x = a * max(b, c); var y = a * 8; output = x + x + y + y";

    WExpressionByteCode testByteCode;
    W_TEST_BOOL(CompareCode<int>(testCode, referenceCode, testByteCode));
    W_TEST_INT(Execute(testByteCode, 2, 4, 8), 64);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Vector constructors")
  {
    {
      WStringView testCode = "var x = vec3(1, 2, 3)\n"
                              "var y = vec4(x, 4)\n"
                              "vec3 z = vec2(1, 2)\n"
                              "var w = vec4()\n"
                              "output = vec4(x) + y + vec4(z) + w";

      WExpressionByteCode testByteCode;
      Compile<WVec3>(testCode, testByteCode);
      W_TEST_VEC3(Execute<WVec3>(testByteCode), WVec3(3, 6, 6), WMath::DefaultEpsilon<float>());
    }

    {
      WStringView testCode = "var x = vec4(a.xy, (vec2(6, 8) - vec2(3, 4)).xy)\n"
                              "var y = vec4(1, vec2(2, 3), 4)\n"
                              "var z = vec4(1, vec3(2, 3, 4))\n"
                              "var w = vec4(1, 2, a.zw)\n"
                              "var one = vec4(1)\n"
                              "output = vec4(x) + y + vec4(z) + w + one";

      WExpressionByteCode testByteCode;
      Compile<WVec3>(testCode, testByteCode);
      W_TEST_VEC3(Execute(testByteCode, WVec3(1, 2, 3)), WVec3(5, 9, 13), WMath::DefaultEpsilon<float>());
    }

    {
      WStringView testCode = "var x = vec4(1, 2, 3, 4)\n"
                              "var y = x.z\n"
                              "x.yz = 7\n"
                              "x.xz = vec2(2, 7)\n"
                              "output = x * y";

      WExpressionByteCode testByteCode;
      Compile<WVec3>(testCode, testByteCode);
      W_TEST_VEC3(Execute<WVec3>(testByteCode), WVec3(6, 21, 21), WMath::DefaultEpsilon<float>());
    }

    {
      WStringView testCode = "var x = 1\n"
                              "x.z = 7.5\n"
                              "output = x";

      WExpressionByteCode testByteCode;
      Compile<WVec3>(testCode, testByteCode);
      W_TEST_VEC3(Execute<WVec3>(testByteCode), WVec3(1, 0, 7), WMath::DefaultEpsilon<float>());
    }
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Vector instructions")
  {
    // The VM does only support scalar data types.
    // This test checks whether the compiler transforms everything correctly to scalar operation.

    WStringView testCode = "output = a * vec3(1, 2, 3) + sqrt(b)";

    WStringView referenceCode = "output.x = a.x + sqrt(b.x)\n"
                                 "output.y = a.y * 2 + sqrt(b.y)\n"
                                 "output.z = a.z * 3 + sqrt(b.z)";

    WExpressionByteCode testByteCode;
    W_TEST_BOOL(CompareCode<WVec3>(testCode, referenceCode, testByteCode));
    W_TEST_VEC3(Execute(testByteCode, WVec3(1, 3, 5), WVec3(4, 9, 16)), WVec3(3, 9, 19), WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Vector swizzle")
  {
    WStringView testCode = "var n = vec4(1, 2, 3, 4)\n"
                            "var m = vec4(5, 6, 7, 8)\n"
                            "var p = n.xxyy + m.zzww * m.abgr + n.w\n"
                            "output = p";

    // vec3(1, 1, 2) + vec3(7, 7, 8) * vec3(8, 7, 6) + 4
    // output.x = 1 + 7 * 8 + 4 = 61
    // output.y = 1 + 7 * 7 + 4 = 54
    // output.z = 2 + 8 * 6 + 4 = 54

    WExpressionByteCode testByteCode;
    Compile<WVec3>(testCode, testByteCode);
    W_TEST_VEC3(Execute<WVec3>(testByteCode), WVec3(61, 54, 54), WMath::DefaultEpsilon<float>());
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Parser errors")
  {
    WExpression::StreamDesc inputs[] = {
      {s_sA, StreamDataTypeDeduction<WVec4>::Type},
      {s_sB, StreamDataTypeDeduction<WVec4>::Type},
      {s_sC, StreamDataTypeDeduction<WVec4>::Type},
      {s_sD, StreamDataTypeDeduction<WVec4>::Type},
    };

    WExpression::StreamDesc outputs[] = {
      {s_sOutput, StreamDataTypeDeduction<WVec4>::Type},
    };

    {
      WStringView testCode = "output = vec4(.x, abs(a.z) * 0.1, 0, 1)";

      WTestLogInterface log;
      WTestLogSystemScope logSystemScope(&log);

      log.ExpectMessage("(1,15): Invalid argument 0 for 'vec4'", WLogMsgType::ErrorMsg);

      WExpressionAST ast;
      W_TEST_BOOL(s_pParser->Parse(testCode, inputs, outputs, {}, ast).Failed());
    }

    {
      WStringView testCode = "output = vec4(a.x, abs(a.z) * 0.1, a., 1)";

      WTestLogInterface log;
      WTestLogSystemScope logSystemScope(&log);

      log.ExpectMessage("(1,38): Syntax error, expected token type Identifier but got NonIdentifier", WLogMsgType::ErrorMsg);
      log.ExpectMessage("(1,38): Invalid argument 2 for 'vec4'", WLogMsgType::ErrorMsg);

      WExpressionAST ast;
      W_TEST_BOOL(s_pParser->Parse(testCode, inputs, outputs, {}, ast).Failed());
    }
  }
}
