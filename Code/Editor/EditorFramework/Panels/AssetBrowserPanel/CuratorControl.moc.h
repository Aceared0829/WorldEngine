#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Basics.h>
#include <QWidget>

struct WAssetCuratorEvent;
struct WToolsProjectEvent;
struct WAssetProcessorEvent;
class QToolButton;

/// \brief
class W_EDITORFRAMEWORK_DLL WQtCuratorControl : public QWidget
{
  Q_OBJECT
public:
  explicit WQtCuratorControl(QWidget* pParent);
  ~WQtCuratorControl();

protected:
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mouseReleaseEvent(QMouseEvent* e) override;

private Q_SLOTS:
  void SlotUpdateTransformStats();
  void UpdateBackgroundProcessState();
  void BackgroundProcessClicked(bool checked);

private:
  void ScheduleUpdateTransformStats();
  void AssetCuratorEvents(const WAssetCuratorEvent& e);
  void AssetProcessorEvents(const WAssetProcessorEvent& e);
  void ProjectEvents(const WToolsProjectEvent& e);

  bool m_bScheduled = false;
  QToolButton* m_pBackgroundProcess = nullptr;
};
