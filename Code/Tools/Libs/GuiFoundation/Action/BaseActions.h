#pragma once

#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <QIcon>

///
class W_GUIFOUNDATION_DLL WNamedAction : public WAction
{
  W_ADD_DYNAMIC_REFLECTION(WNamedAction, WAction);

public:
  WNamedAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WAction(context)
    , m_sName(szName)
    , m_sIconPath(szIconPath)
  {
  }

  const char* GetName() const { return m_sName; }

  WStringView GetAdditionalDisplayString() { return m_sAdditionalDisplayString; }
  void SetAdditionalDisplayString(WStringView sString, bool bTriggerUpdate = true)
  {
    m_sAdditionalDisplayString = sString;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  const char* GetIconPath() const { return m_sIconPath; }
  void SetIconPath(const char* szIconPath) { m_sIconPath = szIconPath; }

protected:
  WString m_sName;
  WString m_sAdditionalDisplayString; // to add some context to the current action
  WString m_sIconPath;
};

///
class W_GUIFOUNDATION_DLL WCategoryAction : public WAction
{
  W_ADD_DYNAMIC_REFLECTION(WCategoryAction, WAction);

public:
  WCategoryAction(const WActionContext& context)
    : WAction(context)
  {
  }

  virtual void Execute(const WVariant& value) override {};
};

/// An action that represents a sub-menu. Can be within a menu bar, or the menu of a tool button).
///
/// This class can be used directly, but then every menu entry has to be mapped individually into the menu.
/// It is often more convenient to use derived types which already set up the content of the menu.
class W_GUIFOUNDATION_DLL WMenuAction : public WNamedAction
{
  W_ADD_DYNAMIC_REFLECTION(WMenuAction, WNamedAction);

public:
  WMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WNamedAction(context, szName, szIconPath)
  {
  }

  virtual void Execute(const WVariant& value) override {};
};

/// A menu action whose content is determined when opening the menu.
///
/// Every time this menu gets opened, GetEntries() is executed,
/// with the state of the previous menu items.
/// It can then return the same result, or adjust the entries (update check marks or show entirely different entries).
///
/// Derive from this, to create your own dynamic menu.
/// Or use something like WEnumerationMenuAction to get a menu for an enum type.
class W_GUIFOUNDATION_DLL WDynamicMenuAction : public WMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicMenuAction, WMenuAction);

public:
  struct Item
  {
    enum class CheckMark
    {
      NotCheckable,
      Unchecked,
      Checked
    };

    struct ItemFlags
    {
      using StorageType = WUInt8;

      enum Enum
      {
        Default = 0,
        Separator = W_BIT(0),
      };
      struct Bits
      {
        StorageType Separator : 1;
      };
    };

    Item() { m_CheckState = CheckMark::NotCheckable; }

    WString m_sDisplay;
    QIcon m_Icon;
    CheckMark m_CheckState;
    WBitflags<ItemFlags> m_ItemFlags;
    WVariant m_UserValue;
  };

  WDynamicMenuAction(const WActionContext& context, const char* szName, const char* szIconPath)
    : WMenuAction(context, szName, szIconPath)
  {
  }
  virtual void GetEntries(WDynamicArray<Item>& out_entries) = 0;
};

/// An action that is displayed as a tool button that is clickable but also has a sub-menu that can be opened for selecting a different action.
class W_GUIFOUNDATION_DLL WDynamicActionAndMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WDynamicActionAndMenuAction, WDynamicMenuAction);

public:
  WDynamicActionAndMenuAction(const WActionContext& context, const char* szName, const char* szIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

protected:
  bool m_bEnabled;
  bool m_bVisible;
};

/// A menu that lists all values of an enum type.
class W_GUIFOUNDATION_DLL WEnumerationMenuAction : public WDynamicMenuAction
{
  W_ADD_DYNAMIC_REFLECTION(WEnumerationMenuAction, WDynamicMenuAction);

public:
  WEnumerationMenuAction(const WActionContext& context, const char* szName, const char* szIconPath);
  void InitEnumerationType(const WRTTI* pEnumerationType);
  virtual void GetEntries(WDynamicArray<Item>& out_entries) override;
  virtual WInt64 GetValue() const = 0;

protected:
  const WRTTI* m_pEnumerationType;
};

/// The standard button action.
class W_GUIFOUNDATION_DLL WButtonAction : public WNamedAction
{
  W_ADD_DYNAMIC_REFLECTION(WButtonAction, WNamedAction);

public:
  WButtonAction(const WActionContext& context, const char* szName, bool bCheckable, const char* szIconPath);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsCheckable() const { return m_bCheckable; }
  void SetCheckable(bool bCheckable, bool bTriggerUpdate = true)
  {
    m_bCheckable = bCheckable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsChecked() const { return m_bChecked; }
  void SetChecked(bool bChecked, bool bTriggerUpdate = true)
  {
    m_bChecked = bChecked;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

protected:
  bool m_bCheckable;
  bool m_bChecked;
  bool m_bEnabled;
  bool m_bVisible;
};

/// An action that represents an integer value within a fixed range, and gets displayed as a slider.
class W_GUIFOUNDATION_DLL WSliderAction : public WNamedAction
{
  W_ADD_DYNAMIC_REFLECTION(WSliderAction, WNamedAction);

public:
  WSliderAction(const WActionContext& context, const char* szName);

  bool IsEnabled() const { return m_bEnabled; }
  void SetEnabled(bool bEnable, bool bTriggerUpdate = true)
  {
    m_bEnabled = bEnable;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  bool IsVisible() const { return m_bVisible; }
  void SetVisible(bool bVisible, bool bTriggerUpdate = true)
  {
    m_bVisible = bVisible;
    if (bTriggerUpdate)
      TriggerUpdate();
  }

  void GetRange(WInt32& out_iMin, WInt32& out_iMax) const
  {
    out_iMin = m_iMinValue;
    out_iMax = m_iMaxValue;
  }

  void SetRange(WInt32 iMin, WInt32 iMax, bool bTriggerUpdate = true);

  WInt32 GetValue() const { return m_iCurValue; }
  void SetValue(WInt32 iVal, bool bTriggerUpdate = true);

protected:
  bool m_bEnabled;
  bool m_bVisible;
  WInt32 m_iMinValue;
  WInt32 m_iMaxValue;
  WInt32 m_iCurValue;
};
