#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/Variant.h>

/// The base class for JSON writers.
///
/// Declares a common interface for writing JSON files. Also implements some utility functions built on top of the interface (AddVariable()).
class W_FOUNDATION_DLL WJSONWriter
{
public:
  /// Modes to configure how much whitespace the JSON writer will output
  enum class WhitespaceMode
  {
    All,             ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
    LessIndentation, ///< Saves some space by using less space for indentation
    NoIndentation,   ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
    NewlinesOnly,    ///< All unnecessary whitespace, except for newlines, is not output.
    None,            ///< No whitespace, not even newlines, is output. This should be used when JSON is used for data exchange, but probably not read by humans.
  };

  /// Modes to configure how arrays are written.
  enum class ArrayMode
  {
    InOneLine,      ///< All array items are written in a single line in the file.
    OneLinePerItem, ///< Each array item is put on a separate line.
  };

  /// Constructor
  WJSONWriter();

  /// Destructor
  virtual ~WJSONWriter();

  /// Configures how much whitespace is output.
  void SetWhitespaceMode(WhitespaceMode whitespaceMode) { m_WhitespaceMode = whitespaceMode; }

  /// Configures how arrays are written.
  void SetArrayMode(ArrayMode arrayMode) { m_ArrayMode = arrayMode; }

  /// Shorthand for "BeginVariable(szName); WriteBool(value); EndVariable(); "
  void AddVariableBool(WStringView sName, bool value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteInt32(value); EndVariable(); "
  void AddVariableInt32(WStringView sName, WInt32 value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteUInt32(value); EndVariable(); "
  void AddVariableUInt32(WStringView sName, WUInt32 value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteInt64(value); EndVariable(); "
  void AddVariableInt64(WStringView sName, WInt64 value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteUInt64(value); EndVariable(); "
  void AddVariableUInt64(WStringView sName, WUInt64 value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteFloat(value); EndVariable(); "
  void AddVariableFloat(WStringView sName, float value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteDouble(value); EndVariable(); "
  void AddVariableDouble(WStringView sName, double value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteString(value); EndVariable(); "
  void AddVariableString(WStringView sName, WStringView value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteNULL(value); EndVariable(); "
  void AddVariableNULL(WStringView sName); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteTime(value); EndVariable(); "
  void AddVariableTime(WStringView sName, WTime value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteUuid(value); EndVariable(); "
  void AddVariableUuid(WStringView sName, WUuid value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteAngle(value); EndVariable(); "
  void AddVariableAngle(WStringView sName, WAngle value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteColor(value); EndVariable(); "
  void AddVariableColor(WStringView sName, const WColor& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteColorGamma(value); EndVariable(); "
  void AddVariableColorGamma(WStringView sName, const WColorGammaUB& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVec2(value); EndVariable(); "
  void AddVariableVec2(WStringView sName, const WVec2& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVec3(value); EndVariable(); "
  void AddVariableVec3(WStringView sName, const WVec3& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVec4(value); EndVariable(); "
  void AddVariableVec4(WStringView sName, const WVec4& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVec2I32(value); EndVariable(); "
  void AddVariableVec2I32(WStringView sName, const WVec2I32& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVec3I32(value); EndVariable(); "
  void AddVariableVec3I32(WStringView sName, const WVec3I32& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVec4I32(value); EndVariable(); "
  void AddVariableVec4I32(WStringView sName, const WVec4I32& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteQuat(value); EndVariable(); "
  void AddVariableQuat(WStringView sName, const WQuat& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteMat3(value); EndVariable(); "
  void AddVariableMat3(WStringView sName, const WMat3& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteMat4(value); EndVariable(); "
  void AddVariableMat4(WStringView sName, const WMat4& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteDataBuffer(value); EndVariable(); "
  void AddVariableDataBuffer(WStringView sName, const WDataBuffer& value); // [tested]

  /// Shorthand for "BeginVariable(szName); WriteVariant(value); EndVariable(); "
  void AddVariableVariant(WStringView sName, const WVariant& value); // [tested]


  /// Writes a bool to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteBool(bool value) = 0;

  /// Writes an int32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteInt32(WInt32 value) = 0;

  /// Writes a uint32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteUInt32(WUInt32 value) = 0;

  /// Writes an int64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteInt64(WInt64 value) = 0;

  /// Writes a uint64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteUInt64(WUInt64 value) = 0;

  /// Writes a float to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteFloat(float value) = 0;

  /// Writes a double to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteDouble(double value) = 0;

  /// Writes a string to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteString(WStringView value) = 0;

  /// Writes the value 'null' to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteNULL() = 0;

  /// Writes a time value to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteTime(WTime value) = 0;

  /// Writes an WColor to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteColor(const WColor& value) = 0;

  /// Writes an WColorGammaUB to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteColorGamma(const WColorGammaUB& value) = 0;

  /// Writes an WVec2 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2(const WVec2& value) = 0;

  /// Writes an WVec3 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3(const WVec3& value) = 0;

  /// Writes an WVec4 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4(const WVec4& value) = 0;

  /// Writes an WVec2I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2I32(const WVec2I32& value) = 0;

  /// Writes an WVec3I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3I32(const WVec3I32& value) = 0;

  /// Writes an WVec4I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4I32(const WVec4I32& value) = 0;

  /// Writes an WQuat to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteQuat(const WQuat& value) = 0;

  /// Writes an WMat3 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat3(const WMat3& value) = 0;

  /// Writes an WMat4 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat4(const WMat4& value) = 0;

  /// Writes an WUuid to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteUuid(const WUuid& value) = 0;

  /// Writes an WAngle to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteAngle(WAngle value) = 0; // [tested]

  /// Writes an WDataBuffer to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteDataBuffer(const WDataBuffer& value) = 0; // [tested]

  /// The default implementation dispatches all supported types to WriteBool, WriteInt32, etc. and asserts on the more complex types.
  ///
  /// A derived class may override this function to implement support for the remaining variant types, if required.
  virtual void WriteVariant(const WVariant& value); // [tested]

  /// Outputs a chunk of memory in some JSON form that can be interpreted as binary data when reading it again.
  ///
  /// How exactly the raw data is represented in JSON is up to the derived class. \a szDataType allows to additionally output a string
  /// that identifies the type of data.
  virtual void WriteBinaryData(WStringView sDataType, const void* pData, WUInt32 uiBytes, WStringView sValueString = nullptr) = 0;

  /// Begins outputting a variable. \a szName is the variable name.
  ///
  /// Between BeginVariable() and EndVariable() you can call the WriteXYZ functions once to write out the variable's data.
  /// You can also call BeginArray() and BeginObject() without a variable name to output an array or object variable.
  virtual void BeginVariable(WStringView sName) = 0;

  /// Ends outputting a variable.
  virtual void EndVariable() = 0;

  /// Begins outputting an array variable.
  ///
  /// If szName is nullptr this will create an anonymous array, which is necessary when you want to put an array as a value into another array.
  /// BeginArray() with a non-nullptr value for \a szName is identical to calling BeginVariable() first. In this case EndArray() will also
  /// end the variable definition, so no additional call to EndVariable() is required.
  virtual void BeginArray(WStringView sName = nullptr) = 0;

  /// Ends outputting an array variable.
  virtual void EndArray() = 0;

  /// Begins outputting an object variable.
  ///
  /// If szName is nullptr this will create an anonymous object, which is necessary when you want to put an object as a value into an array.
  /// BeginObject() with a non-nullptr value for \a szName is identical to calling BeginVariable() first. In this case EndObject() will also
  /// end the variable definition, so no additional call to EndVariable() is required.
  virtual void BeginObject(WStringView sName = nullptr) = 0;

  /// Ends outputting an object variable.
  virtual void EndObject() = 0;

  /// Indicates if an error was encountered while writing
  ///
  /// If any error was encountered at any time during writing, this will return true
  bool HadWriteError() const;

  /// Gives up on the output, so that the writer may be destroyed with containers still open.
  ///
  /// Derived writers are allowed to assert on destruction that every BeginObject() / BeginArray() was
  /// matched, because an unbalanced stream is usually a bug. That check makes an early return between
  /// Begin and End fatal, which is a problem for code that discovers half way through writing that it
  /// cannot continue - a failed lookup, an object that turned out to be invalid.
  ///
  /// Calling this marks the output as unusable and suppresses that check. The written text is not
  /// valid JSON afterwards and must not be used. To bail out and still produce usable output, use
  /// WStandardJSONWriter::EndAll() instead.
  void Abandon() { SetWriteErrorState(); }

protected:
  WhitespaceMode m_WhitespaceMode = WhitespaceMode::All;
  ArrayMode m_ArrayMode = ArrayMode::InOneLine;

  /// called internally when there was an error during writing
  void SetWriteErrorState();

private:
  bool m_bHadWriteError = false;
};


/// Standard-compliant JSON writer implementation.
///
/// Produces fully compliant JSON that any standard parser reads. The W types that JSON has no
/// representation for are written as ordinary objects, arrays and strings - a vector as
/// {"x":1,"y":2,"z":3}, a matrix as an array of rows, a uuid as its hyphenated string form, an angle
/// in degrees. Which W type a value came from is therefore not recorded, so a reader has to know
/// what it is reading; in particular WColor (linear) and WColorGammaUB (gamma, 0-255) are
/// indistinguishable from the JSON alone.
///
/// WriteBinaryData() writes MongoDB-style {"$type":..,"$binary":"hexdata"} for callers that want raw bytes.
class W_FOUNDATION_DLL WStandardJSONWriter : public WJSONWriter
{
public:
  /// Constructor.
  WStandardJSONWriter(); // [tested]

  /// Destructor.
  ~WStandardJSONWriter(); // [tested]

  /// All output is written to this binary stream.
  void SetOutputStream(WStreamWriter* pOutput); // [tested]

  /// \copydoc WJSONWriter::WriteBool()
  virtual void WriteBool(bool value) override; // [tested]

  /// \copydoc WJSONWriter::WriteInt32()
  virtual void WriteInt32(WInt32 value) override; // [tested]

  /// \copydoc WJSONWriter::WriteUInt32()
  virtual void WriteUInt32(WUInt32 value) override; // [tested]

  /// \copydoc WJSONWriter::WriteInt64()
  virtual void WriteInt64(WInt64 value) override; // [tested]

  /// \copydoc WJSONWriter::WriteUInt64()
  virtual void WriteUInt64(WUInt64 value) override; // [tested]

  /// \copydoc WJSONWriter::WriteFloat()
  virtual void WriteFloat(float value) override; // [tested]

  /// \copydoc WJSONWriter::WriteDouble()
  virtual void WriteDouble(double value) override; // [tested]

  /// \copydoc WJSONWriter::WriteString()
  virtual void WriteString(WStringView value) override; // [tested]

  /// \copydoc WJSONWriter::WriteNULL()
  virtual void WriteNULL() override; // [tested]

  /// Writes the time value as a double (i.e. redirects to WriteDouble()).
  virtual void WriteTime(WTime value) override; // [tested]

  /// Writes {"r":..,"g":..,"b":..,"a":..} with the linear float components.
  virtual void WriteColor(const WColor& value) override; // [tested]

  /// Writes {"r":..,"g":..,"b":..,"a":..} with the gamma space components, i.e. 0 to 255.
  virtual void WriteColorGamma(const WColorGammaUB& value) override; // [tested]

  /// Writes {"x":..,"y":..}.
  virtual void WriteVec2(const WVec2& value) override; // [tested]

  /// Writes {"x":..,"y":..,"z":..}.
  virtual void WriteVec3(const WVec3& value) override; // [tested]

  /// Writes {"x":..,"y":..,"z":..,"w":..}.
  virtual void WriteVec4(const WVec4& value) override; // [tested]

  /// Writes {"x":..,"y":..}.
  virtual void WriteVec2I32(const WVec2I32& value) override; // [tested]

  /// Writes {"x":..,"y":..,"z":..}.
  virtual void WriteVec3I32(const WVec3I32& value) override; // [tested]

  /// Writes {"x":..,"y":..,"z":..,"w":..}.
  virtual void WriteVec4I32(const WVec4I32& value) override; // [tested]

  /// Writes {"x":..,"y":..,"z":..,"w":..}, the same layout the property system uses.
  virtual void WriteQuat(const WQuat& value) override; // [tested]

  /// Writes the matrix row by row, as an array of arrays.
  virtual void WriteMat3(const WMat3& value) override; // [tested]

  /// Writes the matrix row by row, as an array of arrays.
  virtual void WriteMat4(const WMat4& value) override; // [tested]

  /// Writes the hyphenated string form, i.e. what WConversionUtils::ToString() produces.
  virtual void WriteUuid(const WUuid& value) override; // [tested]

  /// Writes the angle in degrees (i.e. redirects to WriteFloat()).
  virtual void WriteAngle(WAngle value) override; // [tested]

  /// Writes the bytes as a hex string, which is twice the size of the data.
  virtual void WriteDataBuffer(const WDataBuffer& value) override; // [tested]

  /// Implements the MongoDB way of writing binary data. First writes a "$type" variable, then a "$binary" variable that represents the raw
  /// data (Hex encoded, little endian).
  virtual void WriteBinaryData(WStringView sDataType, const void* pData, WUInt32 uiBytes, WStringView sValueString = nullptr) override; // [tested]

  /// \copydoc WJSONWriter::BeginVariable()
  virtual void BeginVariable(WStringView sName) override; // [tested]

  /// \copydoc WJSONWriter::EndVariable()
  virtual void EndVariable() override; // [tested]

  /// \copydoc WJSONWriter::BeginArray()
  virtual void BeginArray(WStringView sName = {}) override; // [tested]

  /// \copydoc WJSONWriter::EndArray()
  virtual void EndArray() override; // [tested]

  /// \copydoc WJSONWriter::BeginObject()
  virtual void BeginObject(WStringView sName = {}) override; // [tested]

  /// \copydoc WJSONWriter::EndObject()
  virtual void EndObject() override; // [tested]

  /// Writes text that is already valid JSON in place, without any escaping of special characters.
  ///
  /// For values that were authored as JSON elsewhere and are embedded in this document.
  /// Anything else, in particular a plain string or data from outside, produces a malformed document,
  /// use WriteString() for that instead.
  ///
  /// Counts as one value, so it can be used wherever WriteBool() and friends can. Indentation and the
  /// whitespace mode do not apply to the embedded text.
  void WriteRawJson(WStringView sJson); // [tested]

  /// Shorthand for "BeginVariable(sName); WriteRawJson(sJson); EndVariable();"
  void AddVariableRawJson(WStringView sName, WStringView sJson); // [tested]

  /// Closes every object and array that is still open, so that the output becomes valid JSON.
  ///
  /// For code that has to stop writing part way through - a lookup failed, the data turned out to be
  /// unusable - but still wants to return what it has. Without this, the only options are matching
  /// every Begin with an End on the error path or letting the destructor assert.
  ///
  /// A variable that was begun but has no value yet gets a null written for it, because a JSON object
  /// member without a value cannot be represented. Does nothing if nothing is open.
  void EndAll(); // [tested]

protected:
  void End();

  /// Writes an object with one float member per component, e.g. {"x":1,"y":2}.
  ///
  /// \param sComponentNames One character per component, in order, e.g. "xyzw" or "rgba".
  void WriteFloatComponents(const float* pValues, WUInt32 uiCount, WStringView sComponentNames);

  /// Writes a square matrix row by row, as an array of arrays.
  void WriteMatrix(const float* pValues, WUInt32 uiRowsAndColumns);

  enum State
  {
    Invalid,
    Empty,
    Variable,
    Object,
    NamedObject,
    Array,
    NamedArray,
  };

  struct JSONState
  {
    JSONState();

    State m_State;
    bool m_bRequireComma;
    bool m_bValueWasWritten;
  };

  struct W_FOUNDATION_DLL CommaWriter
  {
    CommaWriter(WStandardJSONWriter* pWriter);
    ~CommaWriter();

    WStandardJSONWriter* m_pWriter;
  };

  void OutputString(WStringView s);
  void OutputEscapedString(WStringView s);
  void OutputIndentation();

  WInt32 m_iIndentation;
  WStreamWriter* m_pOutput;

  WHybridArray<JSONState, 16> m_StateStack;
};
