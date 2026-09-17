#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>

/// A file writer that caches all written data and only opens and writes to the output file when everything is finished.
/// Useful to ensure that only complete files are written, or nothing at all, in case of a crash.
class W_FOUNDATION_DLL WDeferredFileWriter : public WStreamWriter
{
  W_DISALLOW_COPY_AND_ASSIGN(WDeferredFileWriter);

public:
  WDeferredFileWriter();

  /// Upon destruction the file is closed and thus written, unless Discard was called before.
  ~WDeferredFileWriter() { Close().IgnoreResult(); }

  /// This must be configured before anything is written to the file.
  void SetOutput(WStringView sFileToWriteTo, bool bOnlyWriteIfDifferent = false);         // [tested]

  virtual WResult WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite) override; // [tested]

  /// Upon calling this the content is written to the file specified with SetOutput().
  /// The return value is W_FAILURE if the file could not be opened or not completely written.
  WResult Close(bool* out_pWasWrittenTo = nullptr); // [tested]

  /// Calling this abandons the content and a later Close or destruction of the instance
  /// will no longer write anything to file.
  void Discard(); // [tested]

private:
  bool m_bOnlyWriteIfDifferent = false;
  bool m_bAlreadyClosed = false;
  WString m_sOutputFile;
  WDefaultMemoryStreamStorage m_Storage;
  WMemoryStreamWriter m_Writer;
};
