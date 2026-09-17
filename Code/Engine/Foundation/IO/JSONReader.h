#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONParser.h>
#include <Foundation/Types/Variant.h>

/// This JSON reader will read an entire JSON document into a hierarchical structure of WVariants.
///
/// The reader will parse the entire document and create a data structure of WVariants, which can then be traversed easily.
/// Note that this class is much less efficient at reading large JSON documents, as it will dynamically allocate and copy objects around
/// quite a bit. For small to medium sized documents that might be good enough, for large files one should prefer to write a dedicated
/// class derived from WJSONParser.
class W_FOUNDATION_DLL WJSONReader : public WJSONParser
{
public:
  enum class ElementType : WInt8
  {
    None,       ///< The JSON document is entirely empty (not even containing an empty object or array)
    Dictionary, ///< The top level element in the JSON document is an object
    Array,      ///< The top level element in the JSON document is an array
  };

  WJSONReader();

  /// Reads the entire stream and creates the internal data structure that represents the JSON document.
  ///
  /// Parses the complete JSON document from the input stream, building the variant tree structure.
  /// The entire document must be valid JSON - parsing stops on the first error encountered.
  ///
  /// \param ref_input Stream containing the JSON document to parse
  /// \param uiFirstLineOffset Line number offset for error reporting (useful when JSON is embedded)
  /// \return W_SUCCESS if parsing completed without errors, W_FAILURE if any parsing error occurred
  ///
  /// \note After successful parsing, use GetTopLevelObject() or GetTopLevelArray() to access the data.
  WResult Parse(WStreamReader& ref_input, WUInt32 uiFirstLineOffset = 0);

  /// Returns the top-level object of the JSON document.
  const WVariantDictionary& GetTopLevelObject() const { return m_Stack.PeekBack().m_Dictionary; }

  /// Returns the top-level array of the JSON document.
  const WVariantArray& GetTopLevelArray() const { return m_Stack.PeekBack().m_Array; }

  /// Returns whether the top level element is an array or an object.
  ElementType GetTopLevelElementType() const { return m_Stack.PeekBack().m_Mode; }

private:
  /// This function can be overridden to skip certain variables, however the overriding function must still call this.
  virtual bool OnVariable(WStringView sVarName) override;

  /// [internal] Do not override further.
  virtual void OnReadValue(WStringView sValue) override;

  /// [internal] Do not override further.
  virtual void OnReadValue(double fValue) override;

  /// [internal] Do not override further.
  virtual void OnReadValue(bool bValue) override;

  /// [internal] Do not override further.
  virtual void OnReadValueNULL() override;

  /// [internal] Do not override further.
  virtual void OnBeginObject() override;

  /// [internal] Do not override further.
  virtual void OnEndObject() override;

  /// [internal] Do not override further.
  virtual void OnBeginArray() override;

  /// [internal] Do not override further.
  virtual void OnEndArray() override;

  virtual void OnParsingError(WStringView sMessage, bool bFatal, WUInt32 uiLine, WUInt32 uiColumn) override;

protected:
  struct Element
  {
    WString m_sName;
    ElementType m_Mode = ElementType::None;
    WVariantArray m_Array;
    WVariantDictionary m_Dictionary;
  };

  WHybridArray<Element, 32> m_Stack;

  bool m_bParsingError = false;
  WString m_sLastName;
};
