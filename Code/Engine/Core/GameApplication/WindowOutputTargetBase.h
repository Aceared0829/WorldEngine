#pragma once

#include <Core/CoreDLL.h>

class WImage;

/// Result of polling an asynchronous capture operation.
struct WCaptureImageResult
{
  using StorageType = WUInt8;

  enum Enum
  {
    Ready,      ///< The capture has finished and the image is available.
    Pending,    ///< The capture is still in progress, poll again next frame.
    NotStarted, ///< No capture operation is in progress.
    Default = NotStarted
  };
};

/// Base class for window output targets
///
/// A window output target is usually tied tightly to a window (\sa WWindowBase) and represents the
/// graphics APIs side of the render output.
/// E.g. in a DirectX implementation this would be a swapchain.
///
/// This interface provides the high level functionality that is needed by WGameApplication to work with
/// the render output.
class W_CORE_DLL WWindowOutputTargetBase
{
public:
  virtual ~WWindowOutputTargetBase() = default;
  virtual void AcquireImage() = 0;
  virtual void PresentImage(bool bEnableVSync) = 0;

  /// Starts an asynchronous capture of the current back buffer.
  /// Returns W_FAILURE if a capture is already in flight or if the operation cannot be started.
  virtual WResult StartCaptureImage() = 0;

  /// Polls the state of a pending capture. Returns Ready once the image data
  /// has been copied into out_image.
  virtual WEnum<WCaptureImageResult> WaitCaptureImage(WImage& out_image) = 0;
};
