#pragma once

#include <Foundation/Basics.h>

// Deactivate Doxygen document generation for the following block.
/// \cond

struct WOSFileData
{
  WOSFileData() { m_pFileHandle = nullptr; }

  FILE* m_pFileHandle;
};

#if W_ENABLED(W_SUPPORTS_FILE_ITERATORS)

struct WFileIterationData
{
  // This is storing DIR*, which we can't forward declare
  WHybridArray<void*, 16> m_Handles;
  WString m_wildcardSearch;
};

#endif

/// \endcond
