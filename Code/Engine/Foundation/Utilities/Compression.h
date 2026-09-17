#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>

///The compression method to be used
enum class WCompressionMethod : WUInt16
{
  ZStd = 0 ///< Only available when ZStd support is enabled in the build (default)
};

/// This namespace contains utilities which can be used to compress and decompress data.
namespace WCompressionUtils
{
  ///Compresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  W_FOUNDATION_DLL WResult Compress(WArrayPtr<const WUInt8> uncompressedData, WCompressionMethod method, WDynamicArray<WUInt8>& out_data);

  ///Decompresses the given data using the compression method eMethod into the dynamic array given in out_Data.
  W_FOUNDATION_DLL WResult Decompress(WArrayPtr<const WUInt8> compressedData, WCompressionMethod method, WDynamicArray<WUInt8>& out_data);
} // namespace WCompressionUtils
