#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/Compression.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
#  define ZSTD_STATIC_LINKING_ONLY // ZSTD_findDecompressedSize
#  include <zstd/zstd.h>
#endif

namespace WCompressionUtils
{
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
  static WResult CompressZStd(WArrayPtr<const WUInt8> uncompressedData, WDynamicArray<WUInt8>& out_data)
  {
    size_t uiSizeBound = ZSTD_compressBound(uncompressedData.GetCount());
    if (uiSizeBound > WMath::MaxValue<WUInt32>())
    {
      WLog::Error("Can't compress since the output container can't hold enough elements ({0})", static_cast<WUInt64>(uiSizeBound));
      return W_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<WUInt32>(uiSizeBound));

    size_t const cSize = ZSTD_compress(out_data.GetData(), uiSizeBound, uncompressedData.GetPtr(), uncompressedData.GetCount(), 1);
    if (ZSTD_isError(cSize))
    {
      WLog::Error("Compression failed with error: '{0}'.", ZSTD_getErrorName(cSize));
      return W_FAILURE;
    }

    out_data.SetCount(static_cast<WUInt32>(cSize));

    return W_SUCCESS;
  }

  static WResult DecompressZStd(WArrayPtr<const WUInt8> compressedData, WDynamicArray<WUInt8>& out_data)
  {
    WUInt64 uiSize = ZSTD_findDecompressedSize(compressedData.GetPtr(), compressedData.GetCount());

    if (uiSize == ZSTD_CONTENTSIZE_ERROR)
    {
      WLog::Error("Can't decompress since it wasn't compressed with ZStd");
      return W_FAILURE;
    }
    else if (uiSize == ZSTD_CONTENTSIZE_UNKNOWN)
    {
      WLog::Error("Can't decompress since the original size can't be determined, was the data compressed using the streaming variant?");
      return W_FAILURE;
    }

    if (uiSize > WMath::MaxValue<WUInt32>())
    {
      WLog::Error("Can't compress since the output container can't hold enough elements ({0})", uiSize);
      return W_FAILURE;
    }

    out_data.SetCountUninitialized(static_cast<WUInt32>(uiSize));

    size_t const uiActualSize = ZSTD_decompress(out_data.GetData(), WMath::SafeConvertToSizeT(uiSize), compressedData.GetPtr(), compressedData.GetCount());

    if (uiActualSize != uiSize)
    {
      WLog::Error("Error during ZStd decompression: '{0}'.", ZSTD_getErrorName(uiActualSize));
      return W_FAILURE;
    }

    return W_SUCCESS;
  }
#endif

  WResult Compress(WArrayPtr<const WUInt8> uncompressedData, WCompressionMethod method, WDynamicArray<WUInt8>& out_data)
  {
    out_data.Clear();

    if (uncompressedData.IsEmpty())
    {
      return W_SUCCESS;
    }

    switch (method)
    {
      case WCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return CompressZStd(uncompressedData, out_data);
#else
        WLog::Error("ZStd compression disabled in build settings!");
        return W_FAILURE;
#endif
      default:
        WLog::Error("Unsupported compression method {0}!", static_cast<WUInt32>(method));
        return W_FAILURE;
    }
  }

  WResult Decompress(WArrayPtr<const WUInt8> compressedData, WCompressionMethod method, WDynamicArray<WUInt8>& out_data)
  {
    out_data.Clear();

    if (compressedData.IsEmpty())
    {
      return W_SUCCESS;
    }

    switch (method)
    {
      case WCompressionMethod::ZStd:
#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT
        return DecompressZStd(compressedData, out_data);
#else
        WLog::Error("ZStd compression disabled in build settings!");
        return W_FAILURE;
#endif
      default:
        WLog::Error("Unsupported compression method {0}!", static_cast<WUInt32>(method));
        return W_FAILURE;
    }
  }
} // namespace WCompressionUtils
