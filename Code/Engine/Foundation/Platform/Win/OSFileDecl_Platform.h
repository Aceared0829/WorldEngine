#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

// Avoid conflicts with windows.h
#ifdef DeleteFile
#  undef DeleteFile
#endif

#ifdef CopyFile
#  undef CopyFile
#endif

#if W_DISABLED(W_USE_POSIX_FILE_API)

#  include <Foundation/Platform/Win/Utils/MinWindows.h>

struct WOSFileData
{
  WOSFileData() { m_pFileHandle = W_WINDOWS_INVALID_HANDLE_VALUE; }

  WMinWindows::HANDLE m_pFileHandle;
};

struct WFileIterationData
{
  WHybridArray<WMinWindows::HANDLE, 16> m_Handles;
};

#endif

/// \endcond
