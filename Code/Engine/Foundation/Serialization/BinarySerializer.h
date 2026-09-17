#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

/// Low-level binary serializer for WAbstractObjectGraph.
///
/// This class provides efficient binary serialization of abstract object graphs. It is used internally
/// by the higher-level serialization systems and offers the fastest serialization performance.
///
/// Features:
/// - Compact binary format optimized for size and speed
/// - Support for type information graphs (for versioning)
/// - Optional patch application during deserialization
/// - Platform-independent format (handles endianness)
class W_FOUNDATION_DLL WAbstractGraphBinarySerializer
{
public:
  /// Writes an abstract object graph to a binary stream.
  ///
  /// \param pTypesGraph Optional type information graph for versioning support
  static void Write(WStreamWriter& inout_stream, const WAbstractObjectGraph* pGraph, const WAbstractObjectGraph* pTypesGraph = nullptr); // [tested]

  /// Reads an abstract object graph from a binary stream.
  ///
  /// \param pTypesGraph Optional type information graph for versioning support
  /// \param bApplyPatches If true, applies version patches during deserialization
  static void Read(WStreamReader& inout_stream, WAbstractObjectGraph* pGraph, WAbstractObjectGraph* pTypesGraph = nullptr, bool bApplyPatches = false); // [tested]

private:
};
