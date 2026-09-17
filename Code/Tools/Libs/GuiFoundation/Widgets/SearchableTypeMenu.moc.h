#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QObject>

class WQtSearchableMenu;
class WRTTI;
class QMenu;

/// Keeps track of the most recently used entries of searchable menus.
///
/// Multiple independent lists are maintained, identified by a name, so that different menus (component types,
/// visual shader nodes, ...) don't push each other's entries out. Which list a menu uses is decided by the code
/// that fills the menu.
///
/// The lists are only stored in memory here. To persist them, some other code (the editor preferences) has to
/// set up the storage through SetStorage(), otherwise nothing is recorded at all.
class W_GUIFOUNDATION_DLL WQtSearchableMenuRecentList
{
public:
  /// Sets (or clears, when passing nullptr) the map in which the recent lists are stored.
  ///
  /// The caller keeps ownership of the map and is responsible for saving and loading it. As long as no storage
  /// is set, UseEntry() does nothing and GetList() returns an empty list.
  static void SetStorage(WMap<WString, WDynamicArray<WString>>* pStorage);

  /// Moves szEntry to the front of the list with the given name, creating the list if necessary.
  ///
  /// Entries beyond s_uiMaxEntries are discarded. Does nothing if no storage has been set.
  static void UseEntry(WStringView sListName, WStringView sEntry);

  /// Returns the entries of the given list, most-recently-used first. Returns an empty array for unknown lists.
  static WArrayPtr<const WString> GetList(WStringView sListName);

  /// How many entries a single list retains.
  static constexpr WUInt32 s_uiMaxEntries = 32;

private:
  static WMap<WString, WDynamicArray<WString>>* s_pStorage;
};

class W_GUIFOUNDATION_DLL WQtTypeMenu : public QObject
{
  Q_OBJECT

public:
  /// Fills pMenu with all (non-abstract, non-hidden) types derived from pBaseType.
  ///
  /// If bSimpleMenu is false and there are enough types, a searchable menu with a 'recently used' section is
  /// created, otherwise a plain hierarchical menu. Set m_sRecentListName before calling this to choose which
  /// recent list to use, otherwise the name of pBaseType is used.
  void FillMenu(QMenu* pMenu, const WRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu);

  static bool s_bShowInDevelopmentFeatures;

  /// Which recent list to record to and display. If empty, the base type name passed to FillMenu() is used.
  WString m_sRecentListName;

  const WRTTI* m_pLastSelectedType = nullptr;

Q_SIGNALS:
  void TypeSelected(QString sTypeName);

protected Q_SLOTS:
  void OnMenuAction();

private:
  QMenu* CreateCategoryMenu(const char* szCategory, WMap<WString, QMenu*>& existingMenus);
  void OnMenuAction(const WRTTI* pRtti);

  QMenu* m_pMenu = nullptr;
  WSet<const WRTTI*> m_SupportedTypes;
  WQtSearchableMenu* m_pSearchableMenu = nullptr;
  WString m_sActiveRecentList;

  static WString s_sLastMenuSearch;
};
