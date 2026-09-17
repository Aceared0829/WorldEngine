#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/Stream.h>

/// A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 32 bit hash value for any kind of data.
class W_FOUNDATION_DLL WHashStreamWriter32 : public WStreamWriter
{
public:
  /// Pass an initial seed for the hash calculation.
  WHashStreamWriter32(WUInt32 uiSeed = 0);
  ~WHashStreamWriter32();

  /// Writes bytes directly to the stream.
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override;

  /// Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash
  /// value.
  WUInt32 GetHashValue() const;

private:
  void* m_pState = nullptr;
};


/// A stream writer that hashes the data written to it.
///
/// This stream writer allows to conveniently generate a 64 bit hash value for any kind of data.
class W_FOUNDATION_DLL WHashStreamWriter64 : public WStreamWriter
{
public:
  /// Pass an initial seed for the hash calculation.
  WHashStreamWriter64(WUInt64 uiSeed = 0);
  ~WHashStreamWriter64();

  /// Writes bytes directly to the stream.
  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override;

  /// Returns the current hash value. You can read this at any time between write operations, or after writing is done to get the final hash
  /// value.
  WUInt64 GetHashValue() const;

private:
  void* m_pState = nullptr;
};
