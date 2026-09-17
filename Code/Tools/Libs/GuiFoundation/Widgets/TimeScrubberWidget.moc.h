#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Time/Time.h>

#include <QToolBar>
#include <QWidget>

class QMouseEvent;
class QPushButton;
class QLineEdit;

class W_GUIFOUNDATION_DLL WQtTimeScrubberWidget : public QWidget
{
  Q_OBJECT

public:
  explicit WQtTimeScrubberWidget(QWidget* pParent);
  ~WQtTimeScrubberWidget();

  /// Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(WUInt64 uiNumTicks);

  /// Sets the duration.
  void SetDuration(WTime time);

  /// Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(WUInt64 uiTick);

  /// Sets the current position.
  void SetScrubberPosition(WTime time);

Q_SIGNALS:
  void ScrubberPosChangedEvent(WUInt64 uiNewScrubberTickPos);

private:
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void mousePressEvent(QMouseEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
  virtual void mouseMoveEvent(QMouseEvent* event) override;
  void SetScrubberPosFromPixelCoord(WInt32 x);

  WUInt64 m_uiDurationTicks = 0;
  WTime m_Duration;
  WUInt64 m_uiScrubberTickPos = 0;
  double m_fNormScrubberPosition = 0.0;
  bool m_bDragging = false;
};

class W_GUIFOUNDATION_DLL WQtTimeScrubberToolbar : public QToolBar
{
  Q_OBJECT

public:
  explicit WQtTimeScrubberToolbar(QWidget* pParent);

  /// Sets the duration in 'ticks'. There are 4800 ticks per second.
  void SetDuration(WUInt64 uiNumTicks);

  /// Sets the current position in 'ticks'. There are 4800 ticks per second.
  void SetScrubberPosition(WUInt64 uiTick);

  void SetButtonState(bool bPlaying, bool bRepeatEnabled);

Q_SIGNALS:
  void ScrubberPosChangedEvent(WUInt64 uiNewScrubberTickPos);
  void PlayPauseEvent();
  void RepeatEvent();
  void DurationChangedEvent(double fDuration);
  void AdjustDurationEvent();

private:
  WQtTimeScrubberWidget* m_pScrubber = nullptr;
  QPushButton* m_pPlayButton = nullptr;
  QPushButton* m_pRepeatButton = nullptr;
  QLineEdit* m_pDuration = nullptr;
  QPushButton* m_pAdjustDurationButton = nullptr;
};
