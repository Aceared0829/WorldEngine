#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/DataProcessing/Stream/ProcessingStream.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/ColorScheme.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <Foundation/Types/Variant.h>

class WStreamWriter;
class WStreamReader;

namespace WExpression
{
  struct Register
  {
    W_DECLARE_POD_TYPE();

    Register(){}; // NOLINT: using = default doesn't work here.

    union
    {
      WSimdVec4b b;
      WSimdVec4i i;
      WSimdVec4f f;
    };
  };

  struct RegisterType
  {
    using StorageType = WUInt8;

    enum Enum
    {
      Unknown,

      Bool,
      Int,
      Float,

      Count,

      Default = Float,
      MaxNumBits = 4,
    };

    static const char* GetName(Enum registerType);
  };

  using Output = WArrayPtr<Register>;
  using Inputs = WArrayPtr<WArrayPtr<const Register>>; // Inputs are in SOA form, means inner array contains all values for one input parameter, one for each instance.
  using GlobalData = WHashTable<WHashedString, WVariant>;

  /// Describes an input or output stream for a expression VM
  struct StreamDesc
  {
    WHashedString m_sName;
    WProcessingStream::DataType m_DataType;

    StreamDesc() = default;

    StreamDesc(const WHashedString sName, WProcessingStream::DataType dataType)
      : m_sName(sName)
      , m_DataType(dataType)
    {
    }

    bool operator==(const StreamDesc& other) const
    {
      return m_sName == other.m_sName && m_DataType == other.m_DataType;
    }

    WResult Serialize(WStreamWriter& inout_stream) const;
    WResult Deserialize(WStreamReader& inout_stream);
  };

  /// Describes an expression function and its signature, e.g. how many input parameter it has and their type
  struct FunctionDesc
  {
    using TypeList = WSmallArray<WEnum<WExpression::RegisterType>, 8, WStaticsAllocatorWrapper>;

    WHashedString m_sName;
    TypeList m_InputTypes;
    WUInt8 m_uiNumRequiredInputs = 0;
    WEnum<WExpression::RegisterType> m_OutputType;

    bool operator==(const FunctionDesc& other) const
    {
      return m_sName == other.m_sName &&
             m_InputTypes == other.m_InputTypes &&
             m_uiNumRequiredInputs == other.m_uiNumRequiredInputs &&
             m_OutputType == other.m_OutputType;
    }

    bool operator<(const FunctionDesc& other) const;

    WResult Serialize(WStreamWriter& inout_stream) const;
    WResult Deserialize(WStreamReader& inout_stream);

    WHashedString GetMangledName() const;
  };

  using Function = void (*)(WExpression::Inputs, WExpression::Output, const WExpression::GlobalData&);
  using ValidateGlobalDataFunction = WResult (*)(const WExpression::GlobalData&);

} // namespace WExpression

/// Describes an external function that can be called in expressions.
///  These functions need to be state-less and thread-safe.
struct WExpressionFunction
{
  WExpression::FunctionDesc m_Desc;

  WExpression::Function m_Func;

  // Optional validation function used to validate required global data for an expression function
  WExpression::ValidateGlobalDataFunction m_ValidateGlobalDataFunc;
};

/// Contains the default expression functions that are always available in the expression system.
struct W_FOUNDATION_DLL WDefaultExpressionFunctions
{
  static WExpressionFunction s_RandomFunc;
  static WExpressionFunction s_PerlinNoiseFunc;
};

/// Contains extended expression functions that need to be registered explicitly with the expression VM.
struct W_FOUNDATION_DLL WExtendedExpressionFunctions
{
  static WExpressionFunction s_SampleCurveFunc;
};

/// Add this attribute a string property that should be interpreted as expression source.
///
/// The Inputs/Outputs property reference another array property on the same object that contains objects
/// with a name and a type property that can be used for real time error checking of the expression source.
///
/// Optionally, a semicolon-separated list of custom keywords can be provided. These are highlighted in the
/// expression editor with a dedicated color, distinct from built-in types and functions.
class W_FOUNDATION_DLL WExpressionWidgetAttribute : public WTypeWidgetAttribute
{
  W_ADD_DYNAMIC_REFLECTION(WExpressionWidgetAttribute, WTypeWidgetAttribute);

public:
  WExpressionWidgetAttribute() = default;

  WExpressionWidgetAttribute(const char* szInputsProperty, const char* szOutputsProperty)
    : m_sInputsProperty(szInputsProperty)
    , m_sOutputsProperty(szOutputsProperty)
  {
  }

  /// \param szCustomKeywords Semicolon-separated list of identifiers to highlight as a custom keyword group.
  /// \param customKeywordColor The color used to highlight the custom keywords.
  WExpressionWidgetAttribute(const char* szCustomKeywords, WColorGammaUB customKeywordColor)
    : m_sCustomKeywords(szCustomKeywords)
    , m_CustomKeywordColor(customKeywordColor)
  {
  }

  const char* GetInputsProperty() const { return m_sInputsProperty; }
  const char* GetOutputsProperty() const { return m_sOutputsProperty; }
  const char* GetCustomKeywords() const { return m_sCustomKeywords; }
  WColorGammaUB GetCustomKeywordColor() const { return m_CustomKeywordColor; }

private:
  WUntrackedString m_sInputsProperty;
  WUntrackedString m_sOutputsProperty;
  WUntrackedString m_sCustomKeywords;
  WColorGammaUB m_CustomKeywordColor = WColorGammaUB(WColorScheme::DarkUI(WColorScheme::Yellow));
};
