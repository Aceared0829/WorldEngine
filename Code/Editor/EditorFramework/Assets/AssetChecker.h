#pragma once

#include <EditorFramework/Assets/AssetCheckRule.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Deque.h>

/// Input for WAssetChecker::Run.
struct WAssetCheckOptions
{
  WDynamicArray<WAssetCheckRule*> m_Rules; ///< Rules to run. Owned by the caller (the dialog).
  WString m_sDocumentTypeName;              ///< If non-empty, only assets of this document type are checked.
  WString m_sNameFilter;                    ///< WSearchPatternFilter, matched against the data-dir-relative asset path.
  bool m_bAutoFix = false;                   ///< If true, rules that CanFix() apply fixes and modified documents are saved.
};

/// Result for a single asset that produced at least one note.
struct WAssetCheckResult
{
  WUuid m_AssetGuid;
  WString m_sAssetPath;    ///< Data-dir-relative path, for display.
  WString m_sAbsAssetPath; ///< Absolute path, for opening on double-click.
  WDynamicArray<WAssetCheckNote> m_Notes;
  bool m_bSaved = false;
};

/// Aggregated outcome of a check run.
struct WAssetCheckSummary
{
  WDeque<WAssetCheckResult> m_Results; ///< Only assets that produced notes.
  WUInt32 m_uiAssetsChecked = 0;
  WUInt32 m_uiWarnings = 0;
  WUInt32 m_uiErrors = 0;
  WUInt32 m_uiFixed = 0;
  WUInt32 m_uiSaved = 0;
  bool m_bCanceled = false;
};

/// Runs a set of WAssetCheckRules over the assets selected by WAssetCheckOptions.
///
/// Run is UI-free (it only drives the global WProgress, which surfaces a modal progress dialog in
/// the editor) so that a headless tool could call it as well. It must run on the main thread
/// because it opens, modifies and saves documents.
class W_EDITORFRAMEWORK_DLL WAssetChecker
{
public:
  static void Run(const WAssetCheckOptions& options, WAssetCheckSummary& out_summary);
};
