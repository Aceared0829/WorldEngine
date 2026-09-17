#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Types/UniquePtr.h>

class WOpenDdlReaderElement;

/// Represents a named block of serialized data within a DDL document.
///
/// DDL documents can contain multiple named blocks (Header, Objects, Types) each containing
/// an object graph. This structure holds one such block with its name and deserialized content.
struct W_FOUNDATION_DLL WSerializedBlock
{
  WString m_Name;                            ///< Name of the block (e.g., "Header", "Objects", "Types")
  WUniquePtr<WAbstractObjectGraph> m_Graph; ///< Deserialized object graph for this block
};

/// Low-level DDL serializer for WAbstractObjectGraph instances.
///
/// This class provides efficient DDL (Data Definition Language) serialization of abstract object graphs.
/// DDL is a human-readable text format that supports comments, structured data, and type information.
class W_FOUNDATION_DLL WAbstractGraphDdlSerializer
{
public:
  /// Writes an object graph to a DDL stream with optional type information.
  ///
  /// \param pGraph The main object graph to serialize
  /// \param pTypesGraph Optional type information for versioning support
  /// \param bCompactMmode If true, minimizes whitespace for smaller files
  /// \param typeMode Controls verbosity of type names in output
  static void Write(WStreamWriter& inout_stream, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypesGraph = nullptr, bool bCompactMmode = true, WOpenDdlWriter::TypeStringMode typeMode = WOpenDdlWriter::TypeStringMode::Shortest);

  /// Reads an object graph from a DDL stream with optional patching.
  ///
  /// \param pGraph Output graph to populate with deserialized data
  /// \param pTypesGraph Optional type information output for version tracking
  /// \param bApplyPatches If true, applies version patches during deserialization
  static WResult Read(WStreamReader& inout_stream, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  static void Write(WOpenDdlWriter& inout_stream, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypesGraph = nullptr);
  static WResult Read(const WOpenDdlReaderElement* pRootElement, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = true);

  /// Writes a complete document with separate header, objects, and types sections.
  ///
  /// This creates a structured DDL document with named blocks for different types of data.
  /// Commonly used for asset files that need metadata (header), main content (objects),
  /// and type information (types) in separate, clearly organized sections.
  static void WriteDocument(WStreamWriter& inout_stream, const WAbstractObjectGraph* pHeader, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypes, bool bCompactMode = true, WOpenDdlWriter::TypeStringMode typeMode = WOpenDdlWriter::TypeStringMode::Shortest);

  /// Reads a complete document and separates header, objects, and types into distinct graphs.
  static WResult ReadDocument(WStreamReader& inout_stream, WUniquePtr<WAbstractObjectGraph>& ref_pHeader, WUniquePtr<WAbstractObjectGraph>& ref_pGraph, WUniquePtr<WAbstractObjectGraph>& ref_pTypes, bool bApplyPatches = true);

  /// Reads only the header section from a document without processing the full content.
  ///
  /// This is useful for quickly extracting metadata or file information without the overhead
  /// of deserializing the entire document. Commonly used for asset browsers and file inspection.
  static WResult ReadHeader(WStreamReader& inout_stream, WAbstractObjectGraph* pGraph);

private:
  static WResult ReadBlocks(WStreamReader& stream, WDynamicArray<WSerializedBlock>& blocks);
};
