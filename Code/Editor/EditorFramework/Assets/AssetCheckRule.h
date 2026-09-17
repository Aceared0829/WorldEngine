#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Uuid.h>

class WDocument;
class WDocumentObject;
class WObjectAccessorBase;
class WAssetChecker;
class WAbstractProperty;

/// Severity of an WAssetCheckNote.
struct WAssetCheckSeverity
{
  enum Enum : WUInt8
  {
    Warning,
    Error,
  };
};

/// A single issue reported by an WAssetCheckRule for one asset.
struct W_EDITORFRAMEWORK_DLL WAssetCheckNote
{
  WAssetCheckSeverity::Enum m_Severity = WAssetCheckSeverity::Warning;
  WString m_sMessage;
  WUuid m_ObjectGuid;   ///< Invalid if the note is not tied to a specific object.
  bool m_bFixed = false; ///< True if an auto-fix resolved this issue.
};

/// Passed to WAssetCheckRule::CheckDocument to inspect the document and report issues.
///
/// The rule reports issues via ReportIssue. If IsAutoFixAllowed returns true, the rule may also
/// modify the document through GetObjectAccessor and report the fixed issues with bFixed = true.
/// The runner (WAssetChecker) owns the undo transaction; rules must not start or finish one.
class W_EDITORFRAMEWORK_DLL WAssetCheckContext
{
public:
  WDocument* GetDocument() const { return m_pDocument; }
  WObjectAccessorBase* GetObjectAccessor() const;

  /// Whether the rule is allowed to modify the document to fix issues.
  bool IsAutoFixAllowed() const { return m_bAutoFix; }

  /// Records an issue. When bFixed is true, m_uiFixCount is incremented so the runner knows the
  /// document was modified and needs to be saved.
  void ReportIssue(WAssetCheckSeverity::Enum severity, WStringView sMessage, const WDocumentObject* pObject = nullptr, bool bFixed = false);

  /// Returns the object's "Name" property (if present and non-empty), otherwise its type name.
  static WString GetObjectDisplayName(const WDocumentObject* pObject);

private:
  friend class WAssetChecker;

  WDocument* m_pDocument = nullptr;
  bool m_bAutoFix = false;
  WUInt32 m_uiFixCount = 0; ///< Reset per rule by the runner; counts fixes applied to the document.
  WDynamicArray<WAssetCheckNote>* m_pNotes = nullptr;
};

/// Base class for asset check rules.
///
/// Derive in any editor plugin (with W_ADD_DYNAMIC_REFLECTION and a default allocator) and the
/// rule is auto-discovered via reflection. A rule inspects an open document and reports issues,
/// optionally fixing them when auto-fix is enabled.
class W_EDITORFRAMEWORK_DLL WAssetCheckRule : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WAssetCheckRule, WReflectedClass);

public:
  virtual WStringView GetDisplayName() const = 0;
  virtual WStringView GetDescription() const = 0;

  /// Whether the rule can fix the issues it reports. If false, auto-fix has no effect for it.
  virtual bool CanFix() const { return false; }

  /// Whether the rule should run on documents of the given type. Defaults to all types.
  virtual bool AppliesToDocumentType(WStringView sDocumentTypeName) const { return true; }

  /// Inspects the document and reports all issues via ref_ctx.ReportIssue.
  ///
  /// The default implementation visits the document's root object and every object below it and
  /// calls CheckObject for each (child objects reached through properties marked with
  /// WTemporaryAttribute are skipped). Rules that follow this per-object / per-property structure
  /// only need to override CheckObject or CheckProperty. Override CheckDocument itself to replace
  /// the traversal entirely.
  ///
  /// If ref_ctx.IsAutoFixAllowed() is true, the rule also fixes the issues through
  /// ref_ctx.GetObjectAccessor() and reports them with bFixed = true. The runner manages the undo
  /// transaction, so implementations must not call Start/Finish/CancelTransaction.
  virtual void CheckDocument(WAssetCheckContext& ref_ctx);

  /// Allocates one instance of every reflected, allocatable rule type, sorted by display name.
  static void CreateRules(WDynamicArray<WAssetCheckRule*>& out_rules);
  static void DestroyRules(WDynamicArray<WAssetCheckRule*>& ref_rules);

protected:
  /// Inspects a single object as part of the default CheckDocument traversal.
  ///
  /// The default iterates all of the object's properties, skipping those marked with
  /// WTemporaryAttribute, and calls CheckProperty for each. Child objects are visited separately
  /// by the traversal and must not be recursed into here.
  virtual void CheckObject(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject);

  /// Inspects a single property of an object. The default does nothing.
  ///
  /// Called for Member, Array, Set and Map properties alike. Use GetPropertyValues to read the
  /// contained element values without having to special-case the property category.
  virtual void CheckProperty(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject, const WAbstractProperty* pProp);

  /// Reads all element values of a property, regardless of its category.
  ///
  /// Member properties yield their single value; Array and Set properties yield all elements; Map
  /// properties yield all values. out_indices receives the index/key that addresses each value,
  /// usable with WObjectAccessorBase::RemoveValue and related functions; for Member properties it
  /// is an invalid WVariant. Returns W_FAILURE for categories that hold no readable values (such
  /// as pointer properties, whose targets are visited as separate child objects).
  static WResult GetPropertyValues(WObjectAccessorBase* pAcc, const WDocumentObject* pObject, const WAbstractProperty* pProp, WDynamicArray<WVariant>& out_indices, WDynamicArray<WVariant>& out_values);

private:
  void VisitObjectRecursive(WAssetCheckContext& ref_ctx, const WDocumentObject* pObject);
};
