#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ads/DockWidget.h>

class WDocument;

class W_GUIFOUNDATION_DLL WQtDocumentPanel : public ads::CDockWidget
{
public:
  Q_OBJECT

public:
  WQtDocumentPanel(ads::CDockManager* pDockManager, QWidget* pParent, WDocument* pDocument);
  ~WQtDocumentPanel();

  virtual bool event(QEvent* pEvent) override;

private:
  WDocument* m_pDocument = nullptr;
};
