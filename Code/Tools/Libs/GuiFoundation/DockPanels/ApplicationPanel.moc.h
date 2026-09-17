#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ads/DockWidget.h>

class WQtContainerWindow;
namespace ads
{
  class CDockManager;
}

/// Base class for all panels that are supposed to be application wide (not tied to some document).
class W_GUIFOUNDATION_DLL WQtApplicationPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  WQtApplicationPanel(ads::CDockManager* pDockManager, const char* szPanelName);
  ~WQtApplicationPanel();

  void EnsureVisible();

  static const WDynamicArray<WQtApplicationPanel*>& GetAllApplicationPanels() { return s_AllApplicationPanels; }

protected:
  virtual void ToolsProjectEventHandler(const WToolsProjectEvent& e);
  virtual bool event(QEvent* event) override;

private:
  friend class WQtContainerWindow;

  static WDynamicArray<WQtApplicationPanel*> s_AllApplicationPanels;

  WQtContainerWindow* m_pContainerWindow = nullptr;
};

W_DECLARE_REFLECTABLE_TYPE(W_GUIFOUNDATION_DLL, WQtApplicationPanel);
