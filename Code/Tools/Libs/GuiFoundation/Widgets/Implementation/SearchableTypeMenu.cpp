#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>

bool WQtTypeMenu::s_bShowInDevelopmentFeatures = false;
WMap<WString, WDynamicArray<WString>>* WQtSearchableMenuRecentList::s_pStorage = nullptr;

void WQtSearchableMenuRecentList::SetStorage(WMap<WString, WDynamicArray<WString>>* pStorage)
{
  s_pStorage = pStorage;
}

void WQtSearchableMenuRecentList::UseEntry(WStringView sListName, WStringView sEntry)
{
  if (s_pStorage == nullptr || sListName.IsEmpty())
    return;

  // the list is ordered most-recently-used first, which is also the order in which the menu displays it
  auto& list = (*s_pStorage)[sListName];

  const WUInt32 uiIndex = list.IndexOf(sEntry);

  if (uiIndex != WInvalidIndex && uiIndex != 0)
  {
    // already in the list, but not at the front, remove it
    list.RemoveAtAndCopy(uiIndex);
  }

  if (uiIndex != 0)
  {
    list.InsertAt(0, sEntry);
  }

  while (list.GetCount() > s_uiMaxEntries)
  {
    list.PopBack();
  }
}

WArrayPtr<const WString> WQtSearchableMenuRecentList::GetList(WStringView sListName)
{
  if (s_pStorage == nullptr)
    return {};

  auto it = s_pStorage->Find(sListName);

  if (!it.IsValid())
    return {};

  return it.Value();
}

struct TypeComparer
{
  W_FORCE_INLINE bool Less(const WRTTI* a, const WRTTI* b) const
  {
    const WCategoryAttribute* pCatA = a->GetAttributeByType<WCategoryAttribute>();
    const WCategoryAttribute* pCatB = b->GetAttributeByType<WCategoryAttribute>();
    if (pCatA != nullptr && pCatB == nullptr)
    {
      return true;
    }
    else if (pCatA == nullptr && pCatB != nullptr)
    {
      return false;
    }
    else if (pCatA != nullptr && pCatB != nullptr)
    {
      WInt32 iRes = WStringUtils::Compare(pCatA->GetCategory(), pCatB->GetCategory());
      if (iRes != 0)
      {
        return iRes < 0;
      }
    }

    return a->GetTypeName().Compare(b->GetTypeName()) < 0;
  }
};

WString WQtTypeMenu::s_sLastMenuSearch;

QMenu* WQtTypeMenu::CreateCategoryMenu(const char* szCategory, WMap<WString, QMenu*>& existingMenus)
{
  if (WStringUtils::IsNullOrEmpty(szCategory))
    return m_pMenu;


  auto it = existingMenus.Find(szCategory);
  if (it.IsValid())
    return it.Value();

  WStringBuilder sPath = szCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QMenu* pParentMenu = m_pMenu;

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath, existingMenus);
  }

  sPath = szCategory;
  sPath = sPath.GetFileName();

  QMenu* pNewMenu = pParentMenu->addMenu(WMakeQString(WTranslate(sPath)));
  existingMenus[szCategory] = pNewMenu;

  return pNewMenu;
}

void WQtTypeMenu::OnMenuAction()
{
  const WRTTI* pRtti = static_cast<const WRTTI*>(sender()->property("type").value<void*>());

  OnMenuAction(pRtti);
}

void WQtTypeMenu::OnMenuAction(const WRTTI* pRtti)
{
  m_pLastSelectedType = pRtti;

  WQtSearchableMenuRecentList::UseEntry(m_sActiveRecentList, pRtti->GetTypeName());

  Q_EMIT TypeSelected(WMakeQString(pRtti->GetTypeName()));
}

void WQtTypeMenu::FillMenu(QMenu* pMenu, const WRTTI* pBaseType, bool bDerivedTypes, bool bSimpleMenu)
{
  m_pMenu = pMenu;
  m_sActiveRecentList = m_sRecentListName.IsEmpty() ? WString(pBaseType->GetTypeName()) : m_sRecentListName;

  m_SupportedTypes.Clear();
  m_SupportedTypes.Insert(pBaseType);

  if (bDerivedTypes)
  {
    WReflectionUtils::GatherTypesDerivedFromClass(pBaseType, m_SupportedTypes);
  }

  // Make category-sorted array of types and skip all abstract, hidden or in development types
  WDynamicArray<const WRTTI*> supportedTypes;
  for (const WRTTI* pRtti : m_SupportedTypes)
  {
    if (pRtti->GetTypeFlags().IsAnySet(WTypeFlags::Abstract))
      continue;

    if (pRtti->GetAttributeByType<WHiddenAttribute>() != nullptr)
      continue;

    if (!s_bShowInDevelopmentFeatures && pRtti->GetAttributeByType<WInDevelopmentAttribute>() != nullptr)
      continue;

    supportedTypes.PushBack(pRtti);
  }
  supportedTypes.Sort(TypeComparer());

  if (!bSimpleMenu && supportedTypes.GetCount() > 10)
  {
    // only show a searchable menu when it makes some sense
    // also deactivating entries to prevent duplicates is currently not supported by the searchable menu
    m_pSearchableMenu = new WQtSearchableMenu(m_pMenu);
  }

  WStringBuilder sIconName;
  WStringBuilder sCategory = "";

  WMap<WString, QMenu*> existingMenus;

  if (m_pSearchableMenu == nullptr)
  {
    // first round: create all sub menus
    for (const WRTTI* pRtti : supportedTypes)
    {
      // Determine current menu
      const WCategoryAttribute* pCatA = pRtti->GetAttributeByType<WCategoryAttribute>();

      if (pCatA)
      {
        CreateCategoryMenu(pCatA->GetCategory(), existingMenus);
      }
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    // add recently used sub-menu
    {
      WStringBuilder sInternalPath, sDisplayName;

      WInt32 iToAdd = 8;

      auto lruList = WQtSearchableMenuRecentList::GetList(m_sActiveRecentList);
      for (const auto& sTypeName : lruList)
      {
        const WRTTI* pRtti = WRTTI::FindTypeByName(sTypeName);

        if (pRtti == nullptr)
          continue;

        if (!pRtti->IsDerivedFrom(pBaseType))
          continue;

        sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

        sInternalPath.Set(" *** RECENT ***/", pRtti->GetTypeName());

        sDisplayName = WTranslate(pRtti->GetTypeName());

        const WCategoryAttribute* pCatA = pRtti->GetAttributeByType<WCategoryAttribute>();
        const WColorAttribute* pColA = pRtti->GetAttributeByType<WColorAttribute>();

        WColor iconColor = WColor::MakeZero();

        if (pColA)
        {
          iconColor = pColA->GetColor();
        }
        else if (pCatA && iconColor == WColor::MakeZero())
        {
          iconColor = WColorScheme::GetCategoryColor(pCatA->GetCategory(), WColorScheme::CategoryColorUsage::MenuEntryIcon);
        }

        const QIcon actionIcon = WQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);

        m_pSearchableMenu->AddItem(sDisplayName, sInternalPath, QVariant::fromValue((void*)pRtti), actionIcon);

        if (--iToAdd <= 0)
          break;
      }
    }
  }

  WStringBuilder tmp;

  // second round: create the actions
  for (const WRTTI* pRtti : supportedTypes)
  {
    sIconName.Set(":/TypeIcons/", pRtti->GetTypeName(), ".svg");

    // Determine current menu
    const WCategoryAttribute* pCatA = pRtti->GetAttributeByType<WCategoryAttribute>();
    const WInDevelopmentAttribute* pInDev = pRtti->GetAttributeByType<WInDevelopmentAttribute>();
    const WColorAttribute* pColA = pRtti->GetAttributeByType<WColorAttribute>();

    WColor iconColor = WColor::MakeZero();

    if (pColA)
    {
      iconColor = pColA->GetColor();
    }
    else if (pCatA && iconColor == WColor::MakeZero())
    {
      iconColor = WColorScheme::GetCategoryColor(pCatA->GetCategory(), WColorScheme::CategoryColorUsage::MenuEntryIcon);
    }

    const QIcon actionIcon = WQtUiServices::GetCachedIconResource(sIconName.GetData(), iconColor);


    if (m_pSearchableMenu != nullptr)
    {
      WStringBuilder sFullPath;
      sFullPath = pCatA ? pCatA->GetCategory() : "";
      sFullPath.AppendPath(pRtti->GetTypeName());

      WStringBuilder sDisplayName = WTranslate(pRtti->GetTypeName());
      if (pInDev)
      {
        sDisplayName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      m_pSearchableMenu->AddItem(sDisplayName, sFullPath, QVariant::fromValue((void*)pRtti), actionIcon);
    }
    else
    {
      QMenu* pCat = CreateCategoryMenu(pCatA ? pCatA->GetCategory() : nullptr, existingMenus);

      WStringBuilder fullName = WTranslate(pRtti->GetTypeName());

      if (pInDev)
      {
        fullName.AppendFormat(" [ {} ]", pInDev->GetString());
      }

      // Add type action to current menu
      QAction* pAction = new QAction(fullName.GetData(), m_pMenu);
      pAction->setProperty("type", QVariant::fromValue((void*)pRtti));
      W_VERIFY(connect(pAction, SIGNAL(triggered()), this, SLOT(OnMenuAction())) != nullptr, "connection failed");

      pAction->setIcon(actionIcon);

      pCat->addAction(pAction);
    }
  }

  if (m_pSearchableMenu != nullptr)
  {
    connect(m_pSearchableMenu, &WQtSearchableMenu::MenuItemTriggered, m_pMenu, [this](const QString& sName, const QVariant& variant)
      {
        const WRTTI* pRtti = static_cast<const WRTTI*>(variant.value<void*>());

        OnMenuAction(pRtti);

        m_pMenu->close();
        //
      });

    connect(m_pSearchableMenu, &WQtSearchableMenu::SearchTextChanged, m_pMenu,
      [this](const QString& sText)
      { WQtTypeMenu::s_sLastMenuSearch = sText.toUtf8().data(); });

    m_pMenu->addAction(m_pSearchableMenu);

    // important to do this last to make sure the search bar gets focus
    m_pSearchableMenu->Finalize(WQtTypeMenu::s_sLastMenuSearch.GetData());
  }
}
