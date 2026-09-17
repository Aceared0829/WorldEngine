#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

namespace
{
  static constexpr const char* s_szOpCodeNames[] = {
    "Nop",

    "",

    "AbsF_R",
    "AbsI_R",
    "SqrtF_R",

    "ExpF_R",
    "LnF_R",
    "Log2F_R",
    "Log2I_R",
    "Log10F_R",
    "Pow2F_R",

    "SinF_R",
    "CosF_R",
    "TanF_R",

    "ASinF_R",
    "ACosF_R",
    "ATanF_R",

    "RoundF_R",
    "FloorF_R",
    "CeilF_R",
    "TruncF_R",

    "NotB_R",
    "NotI_R",

    "IToF_R",
    "FToI_R",

    "",
    "",

    "AddF_RR",
    "AddI_RR",

    "SubF_RR",
    "SubI_RR",

    "MulF_RR",
    "MulI_RR",

    "DivF_RR",
    "DivI_RR",

    "MinF_RR",
    "MinI_RR",

    "MaxF_RR",
    "MaxI_RR",

    "ShlI_RR",
    "ShrI_RR",
    "AndI_RR",
    "XorI_RR",
    "OrI_RR",

    "EqF_RR",
    "EqI_RR",
    "EqB_RR",

    "NEqF_RR",
    "NEqI_RR",
    "NEqB_RR",

    "LtF_RR",
    "LtI_RR",

    "LEqF_RR",
    "LEqI_RR",

    "GtF_RR",
    "GtI_RR",

    "GEqF_RR",
    "GEqI_RR",

    "AndB_RR",
    "OrB_RR",

    "",
    "",

    "AddF_RC",
    "AddI_RC",

    "SubF_RC",
    "SubI_RC",

    "MulF_RC",
    "MulI_RC",

    "DivF_RC",
    "DivI_RC",

    "MinF_RC",
    "MinI_RC",

    "MaxF_RC",
    "MaxI_RC",

    "ShlI_RC",
    "ShrI_RC",
    "AndI_RC",
    "XorI_RC",
    "OrI_RC",

    "EqF_RC",
    "EqI_RC",
    "EqB_RC",

    "NEqF_RC",
    "NEqI_RC",
    "NEqB_RC",

    "LtF_RC",
    "LtI_RC",

    "LEqF_RC",
    "LEqI_RC",

    "GtF_RC",
    "GtI_RC",

    "GEqF_RC",
    "GEqI_RC",

    "AndB_RC",
    "OrB_RC",

    "",
    "",

    "SelF_RRR",
    "SelI_RRR",
    "SelB_RRR",

    "",
    "",

    "MovX_R",
    "MovX_C",
    "LoadF",
    "LoadI",
    "StoreF",
    "StoreI",

    "Call",

    "",
  };

  static_assert(W_ARRAY_SIZE(s_szOpCodeNames) == WExpressionByteCode::OpCode::Count);
  static_assert(WExpressionByteCode::OpCode::LastBinary - WExpressionByteCode::OpCode::FirstBinary == WExpressionByteCode::OpCode::LastBinaryWithConstant - WExpressionByteCode::OpCode::FirstBinaryWithConstant);


  static constexpr WUInt32 GetMaxOpCodeLength()
  {
    WUInt32 uiMaxLength = 0;
    for (WUInt32 i = 0; i < W_ARRAY_SIZE(s_szOpCodeNames); ++i)
    {
      uiMaxLength = WMath::Max(uiMaxLength, WStringUtils::GetStringElementCount(s_szOpCodeNames[i]));
    }
    return uiMaxLength;
  }

  static constexpr WUInt32 s_uiMaxOpCodeLength = GetMaxOpCodeLength();

} // namespace

const char* WExpressionByteCode::OpCode::GetName(Enum code)
{
  W_ASSERT_DEBUG(code >= 0 && static_cast<WUInt32>(code) < W_ARRAY_SIZE(s_szOpCodeNames), "Out of bounds access");
  return s_szOpCodeNames[code];
}

//////////////////////////////////////////////////////////////////////////

//clang-format off
W_BEGIN_STATIC_REFLECTED_TYPE(WExpressionByteCode, WNoBase, 1, WRTTINoAllocator)
W_END_STATIC_REFLECTED_TYPE;
//clang-format on

WExpressionByteCode::WExpressionByteCode() = default;

WExpressionByteCode::WExpressionByteCode(const WExpressionByteCode& other)
{
  *this = other;
}

WExpressionByteCode::~WExpressionByteCode()
{
  Clear();
}

void WExpressionByteCode::operator=(const WExpressionByteCode& other)
{
  Clear();
  Init(other.GetByteCode(), other.GetInputs(), other.GetOutputs(), other.GetFunctions(), other.GetNumTempRegisters(), other.GetNumInstructions());
}

bool WExpressionByteCode::operator==(const WExpressionByteCode& other) const
{
  return GetByteCode() == other.GetByteCode() &&
         GetInputs() == other.GetInputs() &&
         GetOutputs() == other.GetOutputs() &&
         GetFunctions() == other.GetFunctions();
}

void WExpressionByteCode::Clear()
{
  WMemoryUtils::Destruct(m_pInputs, m_uiNumInputs);
  WMemoryUtils::Destruct(m_pOutputs, m_uiNumOutputs);
  WMemoryUtils::Destruct(m_pFunctions, m_uiNumFunctions);

  m_pInputs = nullptr;
  m_pOutputs = nullptr;
  m_pFunctions = nullptr;
  m_pByteCode = nullptr;

  m_uiByteCodeCount = 0;
  m_uiNumInputs = 0;
  m_uiNumOutputs = 0;
  m_uiNumFunctions = 0;

  m_uiNumTempRegisters = 0;
  m_uiNumInstructions = 0;

  m_Data.Clear();
}

void WExpressionByteCode::Disassemble(WStringBuilder& out_sDisassembly) const
{
  out_sDisassembly.Append("// Inputs:\n");
  for (WUInt32 i = 0; i < m_uiNumInputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pInputs[i].m_sName, WProcessingStream::GetDataTypeName(m_pInputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Outputs:\n");
  for (WUInt32 i = 0; i < m_uiNumOutputs; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {}({})\n", i, m_pOutputs[i].m_sName, WProcessingStream::GetDataTypeName(m_pOutputs[i].m_DataType));
  }

  out_sDisassembly.Append("\n// Functions:\n");
  for (WUInt32 i = 0; i < m_uiNumFunctions; ++i)
  {
    out_sDisassembly.AppendFormat("//  {}: {} {}(", i, WExpression::RegisterType::GetName(m_pFunctions[i].m_OutputType), m_pFunctions[i].m_sName);
    const WUInt32 uiNumArguments = m_pFunctions[i].m_InputTypes.GetCount();
    for (WUInt32 j = 0; j < uiNumArguments; ++j)
    {
      out_sDisassembly.Append(WExpression::RegisterType::GetName(m_pFunctions[i].m_InputTypes[j]));
      if (j < uiNumArguments - 1)
      {
        out_sDisassembly.Append(", ");
      }
    }
    out_sDisassembly.Append(")\n");
  }

  out_sDisassembly.AppendFormat("\n// Temp Registers: {}\n", GetNumTempRegisters());
  out_sDisassembly.AppendFormat("// Instructions: {}\n\n", GetNumInstructions());

  auto AppendConstant = [](WUInt32 x, WStringBuilder& out_sString)
  {
    out_sString.AppendFormat("0x{}({})", WArgU(x, 8, true, 16), WArgF(*reinterpret_cast<float*>(&x), 6));
  };

  const StorageType* pByteCode = GetByteCodeStart();
  const StorageType* pByteCodeEnd = GetByteCodeEnd();

  while (pByteCode < pByteCodeEnd)
  {
    OpCode::Enum opCode = GetOpCode(pByteCode);
    {
      const char* szOpCode = OpCode::GetName(opCode);
      WUInt32 uiOpCodeLength = WStringUtils::GetStringElementCount(szOpCode);

      out_sDisassembly.Append(szOpCode);
      for (WUInt32 i = uiOpCodeLength; i < s_uiMaxOpCodeLength + 1; ++i)
      {
        out_sDisassembly.Append(" ");
      }
    }

    if (opCode > OpCode::FirstUnary && opCode < OpCode::LastUnary)
    {
      WUInt32 r = GetRegisterIndex(pByteCode);
      WUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{}\n", r, x);
    }
    else if (opCode > OpCode::FirstBinary && opCode < OpCode::LastBinary)
    {
      WUInt32 r = GetRegisterIndex(pByteCode);
      WUInt32 a = GetRegisterIndex(pByteCode);
      WUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{}\n", r, a, b);
    }
    else if (opCode > OpCode::FirstBinaryWithConstant && opCode < OpCode::LastBinaryWithConstant)
    {
      WUInt32 r = GetRegisterIndex(pByteCode);
      WUInt32 a = GetRegisterIndex(pByteCode);
      WUInt32 b = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} ", r, a);
      AppendConstant(b, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode > OpCode::FirstTernary && opCode < OpCode::LastTernary)
    {
      WUInt32 r = GetRegisterIndex(pByteCode);
      WUInt32 a = GetRegisterIndex(pByteCode);
      WUInt32 b = GetRegisterIndex(pByteCode);
      WUInt32 c = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} r{} r{} r{}\n", r, a, b, c);
    }
    else if (opCode == OpCode::MovX_C)
    {
      WUInt32 r = GetRegisterIndex(pByteCode);
      WUInt32 x = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} ", r);
      AppendConstant(x, out_sDisassembly);
      out_sDisassembly.Append("\n");
    }
    else if (opCode == OpCode::LoadF || opCode == OpCode::LoadI)
    {
      WUInt32 r = GetRegisterIndex(pByteCode);
      WUInt32 i = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("r{} i{}({})\n", r, i, m_pInputs[i].m_sName);
    }
    else if (opCode == OpCode::StoreF || opCode == OpCode::StoreI)
    {
      WUInt32 o = GetRegisterIndex(pByteCode);
      WUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("o{}({}) r{}\n", o, m_pOutputs[o].m_sName, r);
    }
    else if (opCode == OpCode::Call)
    {
      WUInt32 uiIndex = GetFunctionIndex(pByteCode);
      const char* szName = m_pFunctions[uiIndex].m_sName;

      WStringBuilder sName;
      if (WStringUtils::IsNullOrEmpty(szName))
      {
        sName.SetFormat("Unknown_{0}", uiIndex);
      }
      else
      {
        sName = szName;
      }

      WUInt32 r = GetRegisterIndex(pByteCode);

      out_sDisassembly.AppendFormat("{1} r{2}", sName, r);

      WUInt32 uiNumArgs = GetFunctionArgCount(pByteCode);
      for (WUInt32 uiArgIndex = 0; uiArgIndex < uiNumArgs; ++uiArgIndex)
      {
        WUInt32 x = GetRegisterIndex(pByteCode);
        out_sDisassembly.AppendFormat(" r{0}", x);
      }

      out_sDisassembly.Append("\n");
    }
    else
    {
      W_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

static constexpr WTypeVersion s_uiByteCodeVersion = 6;

WResult WExpressionByteCode::Save(WStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiByteCodeVersion);

  WUInt32 uiDataSize = static_cast<WUInt32>(m_Data.GetByteBlobPtr().GetCount());

  inout_stream << uiDataSize;

  inout_stream << m_uiNumInputs;
  for (auto& input : GetInputs())
  {
    W_SUCCEED_OR_RETURN(input.Serialize(inout_stream));
  }

  inout_stream << m_uiNumOutputs;
  for (auto& output : GetOutputs())
  {
    W_SUCCEED_OR_RETURN(output.Serialize(inout_stream));
  }

  inout_stream << m_uiNumFunctions;
  for (auto& function : GetFunctions())
  {
    W_SUCCEED_OR_RETURN(function.Serialize(inout_stream));
  }

  inout_stream << m_uiByteCodeCount;
  W_SUCCEED_OR_RETURN(inout_stream.WriteBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType)));

  inout_stream << m_uiNumTempRegisters;
  inout_stream << m_uiNumInstructions;

  return W_SUCCESS;
}

WResult WExpressionByteCode::Load(WStreamReader& inout_stream, WByteArrayPtr externalMemory /*= WByteArrayPtr()*/)
{
  WTypeVersion version = inout_stream.ReadVersion(s_uiByteCodeVersion);
  if (version != s_uiByteCodeVersion)
  {
    WLog::Error("Invalid expression byte code version {}. Expected {}", version, s_uiByteCodeVersion);
    return W_FAILURE;
  }

  WUInt32 uiDataSize = 0;
  inout_stream >> uiDataSize;

  void* pData = nullptr;
  if (externalMemory.IsEmpty())
  {
    m_Data.SetCountUninitialized(uiDataSize);
    m_Data.ZeroFill();
    pData = m_Data.GetByteBlobPtr().GetPtr();
  }
  else
  {
    if (externalMemory.GetCount() < uiDataSize)
    {
      WLog::Error("External memory is too small. Expected at least {} bytes but got {} bytes.", uiDataSize, externalMemory.GetCount());
      return W_FAILURE;
    }

    if (WMemoryUtils::IsAligned(externalMemory.GetPtr(), alignof(WExpression::StreamDesc)) == false)
    {
      WLog::Error("External memory is not properly aligned. Expected an alignment of at least {} bytes.", alignof(WExpression::StreamDesc));
      return W_FAILURE;
    }

    pData = externalMemory.GetPtr();
  }

  // Inputs
  {
    inout_stream >> m_uiNumInputs;
    m_pInputs = static_cast<WExpression::StreamDesc*>(pData);
    for (WUInt32 i = 0; i < m_uiNumInputs; ++i)
    {
      W_SUCCEED_OR_RETURN(m_pInputs[i].Deserialize(inout_stream));
    }

    pData = WMemoryUtils::AddByteOffset(pData, GetInputs().ToByteArray().GetCount());
  }

  // Outputs
  {
    inout_stream >> m_uiNumOutputs;
    m_pOutputs = static_cast<WExpression::StreamDesc*>(pData);
    for (WUInt32 i = 0; i < m_uiNumOutputs; ++i)
    {
      W_SUCCEED_OR_RETURN(m_pOutputs[i].Deserialize(inout_stream));
    }

    pData = WMemoryUtils::AddByteOffset(pData, GetOutputs().ToByteArray().GetCount());
  }

  // Functions
  {
    pData = WMemoryUtils::AlignForwards(pData, alignof(WExpression::FunctionDesc));

    inout_stream >> m_uiNumFunctions;
    m_pFunctions = static_cast<WExpression::FunctionDesc*>(pData);
    for (WUInt32 i = 0; i < m_uiNumFunctions; ++i)
    {
      W_SUCCEED_OR_RETURN(m_pFunctions[i].Deserialize(inout_stream));
    }

    pData = WMemoryUtils::AddByteOffset(pData, GetFunctions().ToByteArray().GetCount());
  }

  // ByteCode
  {
    pData = WMemoryUtils::AlignForwards(pData, alignof(StorageType));

    inout_stream >> m_uiByteCodeCount;
    m_pByteCode = static_cast<StorageType*>(pData);
    inout_stream.ReadBytes(m_pByteCode, m_uiByteCodeCount * sizeof(StorageType));
  }

  inout_stream >> m_uiNumTempRegisters;
  inout_stream >> m_uiNumInstructions;

  return W_SUCCESS;
}

void WExpressionByteCode::Init(WArrayPtr<const StorageType> byteCode, WArrayPtr<const WExpression::StreamDesc> inputs, WArrayPtr<const WExpression::StreamDesc> outputs, WArrayPtr<const WExpression::FunctionDesc> functions, WUInt32 uiNumTempRegisters, WUInt32 uiNumInstructions)
{
  WUInt32 uiOutputsOffset = 0;
  WUInt32 uiFunctionsOffset = 0;
  WUInt32 uiByteCodeOffset = 0;

  WUInt32 uiDataSize = 0;
  uiDataSize += inputs.ToByteArray().GetCount();
  uiOutputsOffset = uiDataSize;
  uiDataSize += outputs.ToByteArray().GetCount();

  uiDataSize = WMemoryUtils::AlignSize<WUInt32>(uiDataSize, alignof(WExpression::FunctionDesc));
  uiFunctionsOffset = uiDataSize;
  uiDataSize += functions.ToByteArray().GetCount();

  uiDataSize = WMemoryUtils::AlignSize<WUInt32>(uiDataSize, alignof(StorageType));
  uiByteCodeOffset = uiDataSize;
  uiDataSize += byteCode.ToByteArray().GetCount();

  m_Data.SetCountUninitialized(uiDataSize);
  m_Data.ZeroFill();

  void* pData = m_Data.GetByteBlobPtr().GetPtr();

  W_ASSERT_DEV(inputs.GetCount() < WSmallInvalidIndex, "Too many inputs");
  m_pInputs = static_cast<WExpression::StreamDesc*>(pData);
  m_uiNumInputs = static_cast<WUInt16>(inputs.GetCount());
  WMemoryUtils::Copy(m_pInputs, inputs.GetPtr(), m_uiNumInputs);

  W_ASSERT_DEV(outputs.GetCount() < WSmallInvalidIndex, "Too many outputs");
  m_pOutputs = static_cast<WExpression::StreamDesc*>(WMemoryUtils::AddByteOffset(pData, uiOutputsOffset));
  m_uiNumOutputs = static_cast<WUInt16>(outputs.GetCount());
  WMemoryUtils::Copy(m_pOutputs, outputs.GetPtr(), m_uiNumOutputs);

  W_ASSERT_DEV(functions.GetCount() < WSmallInvalidIndex, "Too many functions");
  m_pFunctions = static_cast<WExpression::FunctionDesc*>(WMemoryUtils::AddByteOffset(pData, uiFunctionsOffset));
  m_uiNumFunctions = static_cast<WUInt16>(functions.GetCount());
  WMemoryUtils::Copy(m_pFunctions, functions.GetPtr(), m_uiNumFunctions);

  m_pByteCode = static_cast<StorageType*>(WMemoryUtils::AddByteOffset(pData, uiByteCodeOffset));
  m_uiByteCodeCount = byteCode.GetCount();
  WMemoryUtils::Copy(m_pByteCode, byteCode.GetPtr(), m_uiByteCodeCount);

  W_ASSERT_DEV(uiNumTempRegisters < WSmallInvalidIndex, "Too many temp registers");
  m_uiNumTempRegisters = static_cast<WUInt16>(uiNumTempRegisters);
  m_uiNumInstructions = uiNumInstructions;
}


W_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Expression_Implementation_ExpressionByteCode);
