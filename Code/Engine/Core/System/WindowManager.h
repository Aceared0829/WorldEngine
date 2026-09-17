#pragma once

#include <Core/CoreDLL.h>

#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Types/Delegate.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/UniquePtr.h>

class WWindowBase;
class WWindowOutputTargetBase;

using WRegisteredWndHandleData = WGenericId<16, 16>;

/// Handle type for windows registered with the WWindowManager.
///
/// Default-constructed handles are invalid and can be checked with IsInvalidated().
/// This handle type is separate from native platform window handles (WWindowHandle).
class WRegisteredWndHandle
{
  W_DECLARE_HANDLE_TYPE(WRegisteredWndHandle, WRegisteredWndHandleData);
};

/// Callback function type called when a registered window is destroyed.
using WWindowDestroyFunc = WDelegate<void(WRegisteredWndHandle)>;

/// Manages registered windows and their associated data.
///
/// The WindowManager provides a centralized system for managing windows throughout
/// their lifetime. Windows are registered with unique handles and can have associated
/// output targets and destruction callbacks.
class W_CORE_DLL WWindowManager final
{
  W_DECLARE_SINGLETON(WWindowManager);

public:
  WWindowManager();
  ~WWindowManager();

  /// Processes window messages for all registered windows.
  ///
  /// This should be called regularly (typically once per frame) to handle
  /// platform-specific window events.
  void Update();

  /// Registers a new window with the manager.
  ///
  /// \param sName Human-readable name for the window (for debugging)
  /// \param pCreatedBy Pointer identifying the creator (used for bulk operations)
  /// \param pWindow The window implementation to register
  /// \return Handle to the registered window
  ///
  /// The returned handle remains valid until the window is explicitly closed.
  /// The pCreatedBy parameter allows closing all windows created by a specific object.
  WRegisteredWndHandle Register(WStringView sName, const void* pCreatedBy, WUniquePtr<WWindowBase>&& pWindow);

  /// Retrieves handles for all registered windows.
  ///
  /// \param out_WindowIDs Array to fill with window handles
  /// \param pCreatedBy Optional filter to only return windows created by this object
  void GetRegistered(WDynamicArray<WRegisteredWndHandle>& out_windowHandles, const void* pCreatedBy = nullptr);

  /// Checks if a window handle is valid and refers to an existing window.
  ///
  /// Invalid handles can occur if the window was closed or if using a default-constructed handle.
  bool IsValid(WRegisteredWndHandle hWindow) const;

  /// Gets the name of a registered window.
  WStringView GetName(WRegisteredWndHandle hWindow) const;

  /// Gets the window implementation for a registered window.
  WWindowBase* GetWindow(WRegisteredWndHandle hWindow) const;

  /// Sets a callback to be invoked when the window is destroyed.
  ///
  /// The callback receives the window handle as parameter. Only one callback
  /// can be set per window; setting a new callback replaces the previous one.
  void SetDestroyCallback(WRegisteredWndHandle hWindow, WWindowDestroyFunc onDestroyCallback);

  /// Associates an output target with a registered window.
  ///
  /// Output targets are destroyed before the window to ensure proper cleanup order.
  /// Setting a new output target replaces any existing one.
  void SetOutputTarget(WRegisteredWndHandle hWindow, WUniquePtr<WWindowOutputTargetBase>&& pOutputTarget);

  /// Gets the output target associated with a window.
  WWindowOutputTargetBase* GetOutputTarget(WRegisteredWndHandle hWindow) const;

  /// Closes and unregisters a specific window.
  ///
  /// This first calls any registered destroy callback, then destroys the output target, then the window.
  /// The handle becomes invalid after this call.
  void Close(WRegisteredWndHandle hWindow);

  /// Closes all windows created by a specific object.
  ///
  /// \param pCreatedBy Identifier of the creator, or nullptr to close all windows
  ///
  /// This is useful for cleanup when an object that created multiple windows is destroyed.
  void CloseAll(const void* pCreatedBy);

private:
  struct Data
  {
    WString m_sName;
    const void* m_pCreatedBy = nullptr;
    WUniquePtr<WWindowBase> m_pWindow;
    WUniquePtr<WWindowOutputTargetBase> m_pOutputTarget;
    WWindowDestroyFunc m_OnDestroy;
  };

  WIdTable<WRegisteredWndHandleData, WUniquePtr<Data>> m_Data;
};
