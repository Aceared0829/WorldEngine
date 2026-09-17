#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>

/// Helper class to manage the top level exception handler.
///
/// A default exception handler is provided but not set by default.
/// The default implementation will write the exception and callstack to the output
/// and create a memory dump using WriteDump that create a dump file in the folder
/// specified via SetExceptionHandler.

/// This class allows to hook into the OS top-level exception handler to handle application crashes
///
/// Derive from this class to implement custom behavior. Call WCrashHandler::SetCrashHandler() to
/// register which instance to use.
///
/// For typical use-cases use WCrashHandler_WriteMiniDump::g_Instance.
class W_FOUNDATION_DLL WCrashHandler
{
public:
  WCrashHandler();
  virtual ~WCrashHandler();

  static void SetCrashHandler(WCrashHandler* pHandler);
  static WCrashHandler* GetCrashHandler();

  virtual void HandleCrash(void* pOsSpecificData) = 0;

private:
  static WCrashHandler* s_pActiveHandler;
};

/// A default implementation of WCrashHandler that tries to write a mini-dump and prints the callstack.
///
/// To use it, call WCrashHandler::SetCrashHandler(&WCrashHandler_WriteMiniDump::g_Instance);
/// Do not forget to also specify the dump-file path, otherwise writing dump-files is skipped.
class W_FOUNDATION_DLL WCrashHandler_WriteMiniDump : public WCrashHandler
{
public:
  static WCrashHandler_WriteMiniDump g_Instance;

  struct PathFlags
  {
    using StorageType = WUInt8;

    enum Enum
    {
      AppendDate = W_BIT(0),      ///< Whether to append the current date to the crash-dump file (YYYY-MM-DD_HH-MM-SS)
      AppendSubFolder = W_BIT(1), ///< Whether to append "CrashDump" as a sub-folder
      AppendPID = W_BIT(2),       ///< Whether to append the process ID to the crash-dump file

      Default = AppendDate | AppendSubFolder | AppendPID
    };

    struct Bits
    {
      StorageType AppendDate : 1;
      StorageType AppendSubFolder : 1;
      StorageType AppendPID : 1;
    };
  };

public:
  WCrashHandler_WriteMiniDump();

  /// Sets the raw path for the dump-file to write
  void SetFullDumpFilePath(WStringView sFullAbsDumpFilePath);

  /// Sets the dump-file path to "{szAbsDirectoryPath}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(WStringView sAbsDirectoryPath, WStringView sAppName, WBitflags<PathFlags> flags = PathFlags::Default);

  /// Sets the dump-file path to "{WOSFile::GetApplicationDirectory()}/{szAppName}_{cur-date}.tmp"
  void SetDumpFilePath(WStringView sAppName, WBitflags<PathFlags> flags = PathFlags::Default);

  virtual void HandleCrash(void* pOsSpecificData) override;

protected:
  virtual bool WriteOwnProcessMiniDump(void* pOsSpecificData);
  virtual void PrintStackTrace(void* pOsSpecificData);

  WString m_sDumpFilePath;
};

W_DECLARE_FLAGS_OPERATORS(WCrashHandler_WriteMiniDump::PathFlags);
