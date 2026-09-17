#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class QProgressDialog;
class QWinTaskbarProgress;
class QWinTaskbarButton;
class WProgress;
struct WProgressEvent;

/// A Qt implementation to display the state of an WProgress instance.
///
/// Create a single instance of this at application startup and link it to an WProgress instance.
/// Whenever the instance's progress state changes, this class will display a simple progress bar.
class W_GUIFOUNDATION_DLL WQtProgressbar
{
public:
  WQtProgressbar();
  ~WQtProgressbar();

  /// Sets the WProgress instance that should be visualized.
  void SetProgressbar(WProgress* pProgress);

  bool IsProcessingEvents() const { return m_iNestedProcessEvents > 0; }

private:
  void ProgressbarEventHandler(const WProgressEvent& e);

  void EnsureCreated();
  void EnsureDestroyed();

  QProgressDialog* m_pDialog = nullptr;
  WProgress* m_pProgress = nullptr;
  WInt32 m_iNestedProcessEvents = 0;

  QMetaObject::Connection m_OnDialogDestroyed;
};
