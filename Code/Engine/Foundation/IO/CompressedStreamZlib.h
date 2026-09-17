
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZLIB_SUPPORT

struct z_stream_s;

/// Stream reader for ZIP-compressed data with known size
///
/// Specialized reader for ZIP/APK archive support, particularly for Android APK file access.
/// Unlike the general-purpose WCompressedStreamReaderZlib, this reader requires the exact
/// compressed input size to be known in advance. Used internally by WArchiveReader for
/// reading individual compressed entries from archive files.
class W_FOUNDATION_DLL WCompressedStreamReaderZip : public WStreamReader
{
public:
  WCompressedStreamReaderZip();
  ~WCompressedStreamReaderZip();

  /// Configures the reader with input stream and exact compressed size
  ///
  /// The exact compressed input size must be known in advance for proper decompression.
  /// This method can be called multiple times to reuse the decoder instance, which is
  /// more efficient than creating new instances for each decompression operation.
  void SetInputStream(WStreamReader* pInputStream, WUInt64 uiInputSize);

  /// Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  /// However, since this is a compressed stream, the decompression still needs to be done, so this won't save any time.
  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) override; // [tested]

private:
  WUInt64 m_uiRemainingInputSize = 0;
  bool m_bReachedEnd = false;
  WDynamicArray<WUInt8> m_CompressedCache;
  WStreamReader* m_pInputStream = nullptr;
  z_stream_s* m_pZLibStream = nullptr;
};


/// General-purpose zlib decompression stream reader
///
/// Decompresses data that was compressed using WCompressedStreamWriterZlib or any zlib-compatible format.
/// The reader wraps another stream (file, memory, etc.) as its data source and handles decompression transparently.
/// Uses an internal 256-byte cache to minimize source stream reads and optimize decompression performance.
/// Unlike WCompressedStreamReaderZip, this reader does not require knowing the compressed size in advance.
class W_FOUNDATION_DLL WCompressedStreamReaderZlib : public WStreamReader
{
public:
  /// Takes an input stream as the source from which to read the compressed data.
  WCompressedStreamReaderZlib(WStreamReader* pInputStream); // [tested]

  ~WCompressedStreamReaderZlib();                            // [tested]

  /// Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  /// However, since this is a compressed stream, the decompression still needs to be done, so this won't save any time.
  virtual WUInt64 ReadBytes(void* pReadBuffer, WUInt64 uiBytesToRead) override; // [tested]

private:
  bool m_bReachedEnd = false;
  WDynamicArray<WUInt8> m_CompressedCache;
  WStreamReader* m_pInputStream = nullptr;
  z_stream_s* m_pZLibStream = nullptr;
};

/// Zlib compression stream writer for efficient data compression
///
/// Compresses incoming data using zlib and forwards it to another stream (file, memory, etc.).
/// Uses an internal 255-byte cache for efficient compression without requiring the entire dataset
/// in memory or dynamic allocations. Data is compressed incrementally as it arrives.
///
/// Note about Flush(): Calling Flush() writes available compressed data but does not guarantee
/// all input data becomes readable from the output stream, as significant data may remain in
/// the compressor's internal buffers. Use CloseStream() to ensure complete data output.
class W_FOUNDATION_DLL WCompressedStreamWriterZlib : public WStreamWriter
{
public:
  /// Compression level settings balancing speed vs. compression ratio
  enum Compression
  {
    Uncompressed = 0,
    Fastest = 1,
    Fast = 3,
    Average = 5,
    High = 7,
    Highest = 9,
    Default = Fastest ///< Recommended setting: good compression with good speed. Higher levels provide minimal space savings but significantly longer compression times.
  };

  /// The constructor takes another stream writer to pass the output into, and a compression level.
  WCompressedStreamWriterZlib(WStreamWriter* pOutputStream, Compression ratio = Compression::Default); // [tested]

  /// Calls CloseStream() internally.
  ~WCompressedStreamWriterZlib(); // [tested]

  /// Compresses \a uiBytesToWrite from \a pWriteBuffer.
  ///
  /// Will output bursts of 256 bytes to the output stream every once in a while.
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override; // [tested]

  /// Finishes the stream and writes all remaining data to the output stream.
  ///
  /// After calling this function, no more data can be written to the stream. GetCompressedSize() will return the final compressed size
  /// of the data.
  /// Note that this function is not the same as Flush(), since Flush() assumes that more data can be written to the stream afterwards,
  /// which is not the case for CloseStream().
  WResult CloseStream(); // [tested]

  /// Returns the size of the data in its uncompressed state.
  WUInt64 GetUncompressedSize() const { return m_uiUncompressedSize; } // [tested]

  /// Returns the compressed data size
  ///
  /// Only accurate after CloseStream() has been called. Before that, the value is approximate
  /// because data may still be cached internally. Note that this returns the compressed data size,
  /// not the total bytes written to the output stream, which includes additional framing overhead
  /// (approximately 1 byte per 255 compressed bytes, plus one terminator byte).
  WUInt64 GetCompressedSize() const { return m_uiCompressedSize; } // [tested]

  /// Writes the currently available compressed data to the stream.
  ///
  /// This does NOT guarantee that you can read all the uncompressed data from the output stream afterwards, because a lot of data
  /// will still be inside the compressor and thus not yet written to the stream.
  virtual WResult Flush() override;

private:
  WUInt64 m_uiUncompressedSize = 0;
  WUInt64 m_uiCompressedSize = 0;

  WStreamWriter* m_pOutputStream = nullptr;
  z_stream_s* m_pZLibStream = nullptr;

  WDynamicArray<WUInt8> m_CompressedCache;
};

#endif // BUILDSYSTEM_ENABLE_ZLIB_SUPPORT
