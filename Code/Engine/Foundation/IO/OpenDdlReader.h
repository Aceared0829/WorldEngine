#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/OpenDdlParser.h>
#include <Foundation/Logging/Log.h>

/// Represents a single element in an OpenDDL document
///
/// OpenDDL elements can be either custom objects that contain child elements,
/// or primitive data lists containing arrays of basic types (bool, integers, floats, strings).
/// This class provides access to the element's type, name, child hierarchy, and primitive data.
/// Elements are organized in a tree structure with parent-child relationships.
class W_FOUNDATION_DLL WOpenDdlReaderElement
{
public:
  W_DECLARE_POD_TYPE();

  /// Whether this is a custom object type that typically contains sub-elements.
  W_ALWAYS_INLINE bool IsCustomType() const { return m_PrimitiveType == WOpenDdlPrimitiveType::Custom; } // [tested]

  /// Whether this is a custom object type of the requested type.
  W_ALWAYS_INLINE bool IsCustomType(WStringView sTypeName) const
  {
    return m_PrimitiveType == WOpenDdlPrimitiveType::Custom && m_sCustomType == sTypeName;
  }

  /// Returns the string for the custom type name.
  W_ALWAYS_INLINE WStringView GetCustomType() const { return m_sCustomType; } // [tested]

  /// Whether the name of the object is non-empty.
  W_ALWAYS_INLINE bool HasName() const { return !m_sName.IsEmpty(); } // [tested]

  /// Returns the name of the object.
  W_ALWAYS_INLINE WStringView GetName() const { return m_sName; } // [tested]

  /// Returns whether the element name is a global or a local name.
  W_ALWAYS_INLINE bool IsNameGlobal() const { return (m_uiNumChildElements & W_BIT(31)) != 0; } // [tested]

  /// How many sub-elements the object has.
  WUInt32 GetNumChildObjects() const; // [tested]

  /// If this is a custom type element, the returned pointer is to the first child element.
  W_ALWAYS_INLINE const WOpenDdlReaderElement* GetFirstChild() const
  {
    return reinterpret_cast<const WOpenDdlReaderElement*>(m_pFirstChild);
  } // [tested]

  /// If the parent is a custom type element, the next child after this is returned.
  W_ALWAYS_INLINE const WOpenDdlReaderElement* GetSibling() const { return m_pSiblingElement; } // [tested]

  /// For non-custom types this returns how many primitives are stored at this element.
  WUInt32 GetNumPrimitives() const; // [tested]

  /// For non-custom types this returns the type of primitive that is stored at this element.
  W_ALWAYS_INLINE WOpenDdlPrimitiveType GetPrimitivesType() const { return m_PrimitiveType; } // [tested]

  /// Validates primitive data type and count for safe array access
  ///
  /// Returns true if the element stores the requested type of primitives AND has at least the desired amount of them, so that accessing the
  /// data array at certain indices is safe.
  bool HasPrimitives(WOpenDdlPrimitiveType type, WUInt32 uiMinNumberOfPrimitives = 1) const;

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const bool* GetPrimitivesBool() const { return reinterpret_cast<const bool*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WInt8* GetPrimitivesInt8() const { return reinterpret_cast<const WInt8*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WInt16* GetPrimitivesInt16() const { return reinterpret_cast<const WInt16*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WInt32* GetPrimitivesInt32() const { return reinterpret_cast<const WInt32*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WInt64* GetPrimitivesInt64() const { return reinterpret_cast<const WInt64*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WUInt8* GetPrimitivesUInt8() const { return reinterpret_cast<const WUInt8*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WUInt16* GetPrimitivesUInt16() const { return reinterpret_cast<const WUInt16*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WUInt32* GetPrimitivesUInt32() const { return reinterpret_cast<const WUInt32*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WUInt64* GetPrimitivesUInt64() const { return reinterpret_cast<const WUInt64*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const float* GetPrimitivesFloat() const { return reinterpret_cast<const float*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const double* GetPrimitivesDouble() const { return reinterpret_cast<const double*>(m_pFirstChild); } // [tested]

  /// Returns a pointer to the primitive data cast to a specific type. Only valid if GetPrimitivesType() actually returns this type.
  W_ALWAYS_INLINE const WStringView* GetPrimitivesString() const { return reinterpret_cast<const WStringView*>(m_pFirstChild); } // [tested]

  /// Searches for a child with the given name. It does not matter whether the object's name is 'local' or 'global'.
  /// \a szName is case-sensitive.
  const WOpenDdlReaderElement* FindChild(WStringView sName) const; // [tested]

  /// Searches for a child element that has the given type, name and if it is a primitives list, at least the desired number of primitives.
  const WOpenDdlReaderElement* FindChildOfType(WOpenDdlPrimitiveType type, WStringView sName, WUInt32 uiMinNumberOfPrimitives = 1) const;

  /// Searches for a child element with the given type and optionally also a certain name.
  const WOpenDdlReaderElement* FindChildOfType(WStringView sType, WStringView sName = nullptr) const;

private:
  friend class WOpenDdlReader;

  WOpenDdlPrimitiveType m_PrimitiveType = WOpenDdlPrimitiveType::Custom;
  WUInt32 m_uiNumChildElements = 0;
  const void* m_pFirstChild = nullptr;
  const WOpenDdlReaderElement* m_pLastChild = nullptr;
  WStringView m_sCustomType;
  WStringView m_sName;
  const WOpenDdlReaderElement* m_pSiblingElement = nullptr;
};

/// Parses OpenDDL documents into an in-memory tree structure
///
/// OpenDDL (Open Data Description Language) is a text format for describing structured data.
/// This reader parses an entire DDL document and creates a tree of WOpenDdlReaderElement objects
/// that can be traversed to extract data. The parser handles both custom object types and
/// primitive data arrays. All parsed data remains valid until the reader is destroyed.
/// Use FindElement() to locate elements by global name, or traverse the tree starting from GetRootElement().
class W_FOUNDATION_DLL WOpenDdlReader : public WOpenDdlParser
{
public:
  WOpenDdlReader();
  ~WOpenDdlReader();

  /// Parses an OpenDDL document from a stream
  ///
  /// Returns W_FAILURE if an unrecoverable parsing error was encountered.
  /// The parsed element tree can be accessed via GetRootElement() after successful parsing.
  /// All previous parse results are cleared before parsing begins.
  ///
  /// \param stream Input data stream containing OpenDDL text
  /// \param uiFirstLineOffset Line number offset for error reporting (useful for sub-documents)
  /// \param pLog Interface for outputting parsing error details (nullptr disables logging)
  /// \param uiCacheSizeInKB Internal cache size - increase for documents with large primitive arrays
  WResult ParseDocument(WStreamReader& inout_stream, WUInt32 uiFirstLineOffset = 0, WLogInterface* pLog = WLog::GetThreadLocalLogSystem(),
    WUInt32 uiCacheSizeInKB = 4); // [tested]

  /// Every document has exactly one root element.
  const WOpenDdlReaderElement* GetRootElement() const; // [tested]

  /// Searches for an element with a global name. NULL if there is no such element.
  const WOpenDdlReaderElement* FindElement(WStringView sGlobalName) const; // [tested]

protected:
  virtual void OnBeginObject(WStringView sType, WStringView sName, bool bGlobalName) override;
  virtual void OnEndObject() override;

  virtual void OnBeginPrimitiveList(WOpenDdlPrimitiveType type, WStringView sName, bool bGlobalName) override;
  virtual void OnEndPrimitiveList() override;

  virtual void OnPrimitiveBool(WUInt32 count, const bool* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveInt8(WUInt32 count, const WInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt16(WUInt32 count, const WInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt32(WUInt32 count, const WInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveInt64(WUInt32 count, const WInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveUInt8(WUInt32 count, const WUInt8* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt16(WUInt32 count, const WUInt16* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt32(WUInt32 count, const WUInt32* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveUInt64(WUInt32 count, const WUInt64* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveFloat(WUInt32 count, const float* pData, bool bThisIsAll) override;
  virtual void OnPrimitiveDouble(WUInt32 count, const double* pData, bool bThisIsAll) override;

  virtual void OnPrimitiveString(WUInt32 count, const WStringView* pData, bool bThisIsAll) override;

  virtual void OnParsingError(WStringView sMessage, bool bFatal, WUInt32 uiLine, WUInt32 uiColumn) override;

protected:
  WOpenDdlReaderElement* CreateElement(WOpenDdlPrimitiveType type, WStringView sType, WStringView sName, bool bGlobalName);
  WStringView CopyString(const WStringView& string);
  void StorePrimitiveData(bool bThisIsAll, WUInt32 bytecount, const WUInt8* pData);

  void ClearDataChunks();
  WUInt8* AllocateBytes(WUInt32 uiNumBytes);

  static constexpr WUInt32 s_uiChunkSize = 1000 * 4; // 4 KiB

  WHybridArray<WUInt8*, 16> m_DataChunks;
  WUInt8* m_pCurrentChunk;
  WUInt32 m_uiBytesInChunkLeft;

  WDynamicArray<WUInt8> m_TempCache;

  WDeque<WOpenDdlReaderElement> m_Elements;
  WHybridArray<WOpenDdlReaderElement*, 16> m_ObjectStack;

  WDeque<WString> m_Strings;

  WMap<WString, WOpenDdlReaderElement*> m_GlobalNames;
};
