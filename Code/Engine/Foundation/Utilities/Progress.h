#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/String.h>

class WProgress;
class WProgressRange;

/// Through these events the state of an WProgress instance is communicated.
///
/// Other code can use this to visualize the progress in different ways.
/// For instance a GUI application can show a progress bar dialog and a game
/// could show a loading screen.
struct W_FOUNDATION_DLL WProgressEvent
{
  enum class Type
  {
    ProgressStarted, ///< Sent when the the first progress starts
    ProgressEnded,   ///< Sent when progress finishes or is canceled
    ProgressChanged, ///< Sent whenever the progress value changes. Not necessarily in every update step.
    CancelClicked,   ///< The user just clicked cancel (for the first time).
  };

  Type m_Type;
  WProgress* m_pProgressbar;
};

/// Manages the way a progress bar is subdivided and advanced.
///
/// WProgress represents a single progress bar. It can be sub-divided into groups and sub-groups using WProgressbarRange.
/// From the ranges and the current advancement, a final progress percentage is computed. Every time a significant change
/// takes place, an event is broadcast. This allows other code to display the progress, either in a GUI application
/// or in a fullscreen loading screen or in any other way appropriate.
class W_FOUNDATION_DLL WProgress
{
public:
  WProgress();
  ~WProgress();

  /// Returns the current overall progress in [0; 1] range.
  float GetCompletion() const;

  /// Sets the current overall progress in [0; 1] range. Should not be called directly, typically called by WProgressRange.
  void SetCompletion(float fCompletion);

  /// Resets the progress bar to 0% completion.
  void Reset();

  /// Returns the current 'headline' text for the progress bar
  WStringView GetMainDisplayText() const;

  /// Returns the current detail text for the progress bar
  WStringView GetStepDisplayText() const;

  /// Used to inform WProgress of outside user input. May have an effect or not.
  void UserClickedCancel();

  /// Whether the user requested to cancel the operation.
  bool WasCanceled() const;

  /// Returns whether the current operations may be canceled or not.
  bool AllowUserCancel() const;

  /// Returns the currently set default WProgress instance. This will always be valid.
  static WProgress* GetGlobalProgressbar();

  /// Allows to set a custom WProgress instance as the global default instance.
  static void SetGlobalProgressbar(WProgress* pProgress);

  /// Events are sent when the progress changes
  WEvent<const WProgressEvent&> m_Events;

  /// Custom user data.
  void* m_pUserData = nullptr;

private:
  friend class WProgressRange;
  void SetActiveRange(WProgressRange* pRange);

  WProgressRange* m_pActiveRange = nullptr;
  WProgressRange* m_pRootRange = nullptr;

  WString m_sCurrentDisplayText;
  bool m_bCancelClicked = false;
  bool m_bEnableCancel = true;

  float m_fLastReportedCompletion = 0.0f;
  float m_fCurrentCompletion = 0.0f;
};

/// WProgressRange is the preferred method to inform the system of progress.
///
/// WProgressRange is a scoped class, ie. upon creation it adds a range to the current progress
/// and upon destruction the entire range is considered to be completed.
/// Ranges can be nested. For instance when a top level range consists of three 'steps',
/// then opening a nested range will sub-divide that first step. When the nested range is closed,
/// the first top-level step is finished and BeginNextStep() should be called on the top-level range.
/// Subsequently the second step is active and can again be further subdivided with another nested WProgressRange.
class W_FOUNDATION_DLL WProgressRange
{
  W_DISALLOW_COPY_AND_ASSIGN(WProgressRange);

public:
  /// Creates a progress range scope.
  ///
  /// If any other progress range is currently active, it will become the parent range and the currently active step will be subdivided.
  /// \param szDisplayText is the main display text for this range.
  /// \param uiSteps is the number of steps that this range will be subdivided into
  /// \param bAllowCancel specifies whether the user can cancel this operation
  /// \param pProgressbar can be specified, if available, otherwise the currently active WProgress instance is used.
  WProgressRange(WStringView sDisplayText, WUInt32 uiSteps, bool bAllowCancel, WProgress* pProgressbar = nullptr);

  /// Creates a progress range scope without steps. Use SetCompletion to manually set the completion value.
  WProgressRange(WStringView sDisplayText, bool bAllowCancel, WProgress* pProgressbar = nullptr);

  /// The destructor closes the current range. All progress in this range is assumed to have completed,
  /// even if BeginNextStep() has not been called once for every subdivision step.
  ~WProgressRange();

  /// Returns the WProgress instance that this range uses.
  WProgress* GetProgressbar() const;

  /// Allows to weigh each step differently.
  ///
  /// This makes it possible to divide an operation into two steps, but have one part take up 90% and the other 10%.
  /// \param uiStep The index for the step to set the weight
  /// \param fWeight The weighting in [0; 1] range
  void SetStepWeighting(WUInt32 uiStep, float fWeight);

  /// Should be called whenever a new sub-step is started to advance the progress.
  ///
  /// \param szStepDisplayText The sub-text for the next step to be displayed.
  /// \param uiNumSteps How many steps have been completed.
  /// \return Returns false if the user clicked cancel.
  bool BeginNextStep(WStringView sStepDisplayText, WUInt32 uiNumSteps = 1);

  /// Manually set the completion value between 0..1.
  bool SetCompletion(double fCompletionFactor);

  /// Whether the user requested to cancel the operation.
  bool WasCanceled() const;

private:
  friend class WProgress;

  void Init(WStringView sDisplayText, bool bAllowCancel, WProgress* pProgressbar);
  float GetStepWeight(WUInt32 uiStep) const;
  void ComputeCurStepBaseAndRange(double& out_base, double& out_range);

  WProgressRange* m_pParentRange = nullptr;
  WProgress* m_pProgressbar = nullptr;

  WInt32 m_iCurrentStep = 0;
  WString m_sDisplayText;
  WString m_sStepDisplayText;
  WHashTable<WUInt32, float> m_StepWeights;

  bool m_bAllowCancel = false;
  double m_fPercentageBase = 0.0;
  double m_fPercentageRange = 0.0;
  double m_fWeightedCompletion = 0.0;
  double m_fSummedWeight = 0.0;
};
