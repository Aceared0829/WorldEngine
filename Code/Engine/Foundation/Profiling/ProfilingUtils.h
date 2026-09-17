#pragma once

#include <Foundation/Basics.h>

class W_FOUNDATION_DLL WProfilingUtils
{
public:
  /// Captures profiling data via WProfilingSystem::Capture and saves it to the giben file location.
  static WResult SaveProfilingCapture(WStringView sCapturePath);
  /// Reads two profiling captures and merges them into one.
  static WResult MergeProfilingCaptures(WStringView sCapturePath1, WStringView sCapturePath2, WStringView sMergedCapturePath);
};
