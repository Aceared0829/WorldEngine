#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>
#include <QWidget>

class WAssetDocument;
class WQtEngineDocumentWindow;
class QGridLayout;
class WQtViewWidgetContainer;
class WQtEngineViewWidget;
struct WEngineViewConfig;
struct WEngineViewPreferences;

class W_EDITORFRAMEWORK_DLL WQtQuadViewWidget : public QWidget
{
  Q_OBJECT
public:
  using ViewFactory = WDelegate<WQtEngineViewWidget*(WQtEngineDocumentWindow*, WEngineViewConfig*)>;
  WQtQuadViewWidget(WAssetDocument* pDocument, WQtEngineDocumentWindow* pWindow, ViewFactory viewFactory, const char* szViewToolBarMapping);
  ~WQtQuadViewWidget();

  const WHybridArray<WQtViewWidgetContainer*, 4>& GetActiveMainViews() { return m_ActiveMainViews; }

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

protected:
  void SaveViewConfig(const WEngineViewConfig& cfg, WEngineViewPreferences& pref) const;
  void LoadViewConfig(WEngineViewConfig& cfg, WEngineViewPreferences& pref);
  void SaveViewConfigs() const;
  void LoadViewConfigs();
  void CreateViews(bool bQuad);

private:
  WAssetDocument* m_pDocument;
  WQtEngineDocumentWindow* m_pWindow;
  ViewFactory m_ViewFactory;
  WString m_sViewToolBarMapping;

  WEngineViewConfig m_ViewConfigSingle;
  WEngineViewConfig m_ViewConfigQuad[4];
  WHybridArray<WQtViewWidgetContainer*, 4> m_ActiveMainViews;
  QGridLayout* m_pViewLayout;
};
