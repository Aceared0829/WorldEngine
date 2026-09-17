#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

#include <QPushButton>

class QMenu;
class WDocument;
class WDynamicStringEnum;
class WQtSearchableMenu;

/// A push button with a searchable drop-down menu that lets the user pick a value from an WDynamicStringEnum.
///
/// Emits ValueSelected once the user picks a value. If the enum supports editing (storage file or edit command), an
/// entry to edit the available values is added as well. This is the shared building block used wherever a dynamic
/// string enum has to be edited, e.g. the property grid widget for WDynamicStringEnumAttribute.
class W_EDITORFRAMEWORK_DLL WQtDynamicStringEnumMenuButton : public QPushButton
{
  Q_OBJECT;

public:
  explicit WQtDynamicStringEnumMenuButton(QWidget* pParent = nullptr);

  /// Selects which dynamic string enum the menu presents.
  void SetEnum(WStringView sEnumName);
  WDynamicStringEnum* GetEnum() const { return m_pEnum; }

  /// The document is passed along when refreshing values and when invoking the enum's edit command. May be null.
  void SetDocument(const WDocument* pDocument) { m_pDocument = pDocument; }

  /// Updates the text shown on the button to the currently selected value.
  void SetCurrentValue(WStringView sValue);

Q_SIGNALS:
  void ValueSelected(const QString& sValue);

private Q_SLOTS:
  void onMenuAboutToShow();

private:
  const WDocument* m_pDocument = nullptr;
  WDynamicStringEnum* m_pEnum = nullptr;
  QMenu* m_pMenu = nullptr;
  WQtSearchableMenu* m_pSearchableMenu = nullptr;
  WString m_sEnumName;

  static WMap<WString, QString> s_LastSearch;
};
