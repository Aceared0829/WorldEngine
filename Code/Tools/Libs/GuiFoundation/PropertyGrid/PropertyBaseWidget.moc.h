#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <GuiFoundation/PropertyGrid/Implementation/PropertyEventHandler.h>
#include <QWidget>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Object/VariantSubAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>

class WDocumentObject;
class WQtTypeWidget;
class QHBoxLayout;
class QVBoxLayout;
class QLabel;
class QMenu;
class QComboBox;
class WQtGroupBoxBase;
class WQtAddSubElementButton;
class WQtPropertyGridWidget;
class WQtElementGroupButton;
class QMimeData;
struct WCommandHistoryEvent;
class WObjectAccessorBase;

/// Base class for all property widgets
class W_GUIFOUNDATION_DLL WQtPropertyWidget : public QWidget
{
  Q_OBJECT;

public:
  explicit WQtPropertyWidget();
  virtual ~WQtPropertyWidget();

  void Init(WQtPropertyGridWidget* pGrid, WObjectAccessorBase* pObjectAccessor, const WRTTI* pType, const WAbstractProperty* pProp);

  WQtPropertyGridWidget* GetPropertyGrid() { return m_pGrid; }
  WObjectAccessorBase* GetObjectAccessor() { return m_pObjectAccessor; }
  const WRTTI* GetType() const { return m_pType; }
  const WAbstractProperty* GetProperty() const { return m_pProp; }

  /// This is called whenever the selection in the editor changes and thus the widget may need to display a different value.
  ///
  /// If the array holds more than one element, the user selected multiple objects. In this case, the code should check whether
  /// the values differ across the selected objects and if so, the widget should display "multiple values".
  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items);
  const WHybridArray<WPropertySelection, 8>& GetSelection() const { return m_Items; }

  /// If this returns true (default), a QLabel is created and the text that GetLabel() returns is displayed.
  virtual bool HasLabel() const { return true; }

  /// The return value is used to display a label, if HasLabel() returns true.
  virtual const char* GetLabel(WStringBuilder& ref_sTmp) const;

  virtual void ExtendContextMenu(QMenu& ref_menu);

  /// Whether the variable that the widget represents is currently set to the default value or has been modified.
  virtual void SetIsDefault(bool bIsDefault) { m_bIsDefault = bIsDefault; }

  /// If the property is of type WVariant this function returns whether all items have the same type.
  /// If true is returned, out_Type contains the common type. Note that 'invalid' can be a common type.
  bool GetCommonVariantSubType(const WArrayPtr<WPropertySelection>& items, const WAbstractProperty* pProperty, WVariantType::Enum& out_type);

  WVariant GetCommonValue(const WArrayPtr<WPropertySelection>& items, const WAbstractProperty* pProperty);
  void PrepareToDie();

  /// By default disables the widget, but can be overridden to make a widget more interactable (for example to be able to copy text from it).
  virtual void SetReadOnly(bool bReadOnly = true);

public:
  static const WRTTI* GetCommonBaseType(const WArrayPtr<WPropertySelection>& items);
  static QColor SetPaletteBackgroundColor(WColorGammaUB inputColor, QPalette& ref_palette);

public Q_SLOTS:
  void OnCustomContextMenu(const QPoint& pt);

protected:
  void Broadcast(WPropertyEvent::Type type);
  void PropertyChangedHandler(const WPropertyEvent& ed);

  virtual void OnInit() = 0;
  bool IsUndead() const { return m_bUndead; }

protected:
  virtual void DoPrepareToDie() = 0;

  virtual bool eventFilter(QObject* pWatched, QEvent* pEvent) override;

  WQtPropertyGridWidget* m_pGrid = nullptr;
  WObjectAccessorBase* m_pObjectAccessor = nullptr;
  const WRTTI* m_pType = nullptr;
  const WAbstractProperty* m_pProp = nullptr;
  WHybridArray<WPropertySelection, 8> m_Items;
  bool m_bIsDefault; ///< Whether the variable that the widget represents is currently set to the default value or has been modified.

private:
  bool m_bUndead;    ///< Widget is being destroyed
};


/// Fallback widget for all property types for which no other widget type is registered
class W_GUIFOUNDATION_DLL WQtUnsupportedPropertyWidget : public WQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit WQtUnsupportedPropertyWidget(const char* szMessage = nullptr);

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override {}

  QHBoxLayout* m_pLayout;
  QLabel* m_pWidget;
  WString m_sMessage;
};


/// Base class for most 'simple' property type widgets. Implements some of the standard functionality.
class W_GUIFOUNDATION_DLL WQtStandardPropertyWidget : public WQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit WQtStandardPropertyWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

protected:
  void BroadcastValueChanged(const WVariant& NewValue);
  virtual void DoPrepareToDie() override {}

  const WVariant& GetOldValue() const { return m_OldValue; }
  virtual void InternalSetValue(const WVariant& value) = 0;

protected:
  WVariant m_OldValue;
};


/// Base class for more 'advanced' property type widgets for Pointer or Class type properties.
/// Implements some of WQtTypeWidget functionality at property widget level.
class W_GUIFOUNDATION_DLL WQtEmbeddedClassPropertyWidget : public WQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit WQtEmbeddedClassPropertyWidget();
  ~WQtEmbeddedClassPropertyWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;

protected:
  void SetPropertyValue(const WAbstractProperty* pProperty, const WVariant& NewValue);

  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  virtual void OnPropertyChanged(const WString& sProperty) = 0;

private:
  void PropertyEventHandler(const WDocumentObjectPropertyEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);
  void FlushQueuedChanges();

protected:
  bool m_bTemporaryCommand = false;
  const WRTTI* m_pResolvedType = nullptr;
  WHybridArray<WPropertySelection, 8> m_ResolvedObjects;

  WHybridArray<WString, 1> m_QueuedChanges;
};


/// Used for pointers and embedded classes.
/// Does not inherit from WQtEmbeddedClassPropertyWidget as it just embeds
/// a WQtTypeWidget for the property's value which handles everything already.
class W_GUIFOUNDATION_DLL WQtPropertyTypeWidget : public WQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit WQtPropertyTypeWidget(bool bAddCollapsibleGroup = false);
  virtual ~WQtPropertyTypeWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual bool HasLabel() const override { return false; }
  virtual void SetIsDefault(bool bIsDefault) override;

protected:
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;

protected:
  QVBoxLayout* m_pLayout;
  WQtGroupBoxBase* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  WQtTypeWidget* m_pTypeWidget;
};

/// Used for property types that are pointers.
class W_GUIFOUNDATION_DLL WQtPropertyPointerWidget : public WQtPropertyWidget
{
  Q_OBJECT;

public:
  explicit WQtPropertyPointerWidget();
  virtual ~WQtPropertyPointerWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual bool HasLabel() const override { return false; }


public Q_SLOTS:
  void OnDeleteButtonClicked();

protected:
  virtual void OnInit() override;
  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  virtual void DoPrepareToDie() override;
  void UpdateTitle(const WRTTI* pType = nullptr);

protected:
  QHBoxLayout* m_pLayout = nullptr;
  WQtGroupBoxBase* m_pGroup = nullptr;
  WQtAddSubElementButton* m_pAddButton = nullptr;
  WQtElementGroupButton* m_pDeleteButton = nullptr;
  QHBoxLayout* m_pGroupLayout = nullptr;
  WQtTypeWidget* m_pTypeWidget = nullptr;
};


/// Base class for all container properties
class W_GUIFOUNDATION_DLL WQtPropertyContainerWidget : public WQtPropertyWidget
{
  Q_OBJECT;

public:
  WQtPropertyContainerWidget();
  virtual ~WQtPropertyContainerWidget();

  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual bool HasLabel() const override { return false; }
  virtual void SetIsDefault(bool bIsDefault) override;

public Q_SLOTS:
  void OnElementButtonClicked();
  void OnDragStarted(QMimeData& ref_mimeData);
  void OnContainerContextMenu(const QPoint& pt);
  void OnCustomElementContextMenu(const QPoint& pt);

protected:
  struct Element
  {
    Element() = default;

    Element(WQtGroupBoxBase* pSubGroup, WQtPropertyWidget* pWidget, WQtElementGroupButton* pHelpButton)
      : m_pSubGroup(pSubGroup)
      , m_pWidget(pWidget)
      , m_pHelpButton(pHelpButton)
    {
    }

    WQtGroupBoxBase* m_pSubGroup = nullptr;
    WQtPropertyWidget* m_pWidget = nullptr;
    WQtElementGroupButton* m_pHelpButton = nullptr;
  };

  virtual WQtGroupBoxBase* CreateElement(QWidget* pParent);
  virtual WQtPropertyWidget* CreateWidget(WUInt32 index);
  virtual Element& AddElement(WUInt32 index);
  virtual void RemoveElement(WUInt32 index);
  virtual void UpdateElement(WUInt32 index) = 0;
  void UpdateElements();
  virtual void GetRequiredElements(WDynamicArray<WVariant>& out_keys) const;
  virtual void UpdatePropertyMetaState();
  /// Some containers like WVariant can be both a map or an array so we can't reply on the property type alone. For these containers, this method can be overwritten to retrieve the category from something other than `m_pProp->GetCategory()`.
  virtual WPropertyCategory::Enum GetContainerCategory() const;

  void Clear();
  virtual void OnInit() override;

  void DeleteItems(WHybridArray<WPropertySelection, 8>& items);
  void MoveItems(WHybridArray<WPropertySelection, 8>& items, WInt32 iMove);
  virtual void DoPrepareToDie() override;
  virtual void dragEnterEvent(QDragEnterEvent* event) override;
  virtual void dragMoveEvent(QDragMoveEvent* event) override;
  virtual void dragLeaveEvent(QDragLeaveEvent* event) override;
  virtual void dropEvent(QDropEvent* event) override;
  virtual void paintEvent(QPaintEvent* event) override;
  virtual void showEvent(QShowEvent* event) override;

private:
  bool updateDropIndex(QDropEvent* pEvent);

protected:
  QHBoxLayout* m_pLayout;
  WQtGroupBoxBase* m_pGroup;
  QVBoxLayout* m_pGroupLayout;
  WQtAddSubElementButton* m_pAddButton = nullptr;
  QPalette m_Pal;

  WHybridArray<WVariant, 16> m_Keys;
  WDynamicArray<Element> m_Elements;
  WInt32 m_iDropSource = -1;
  WInt32 m_iDropTarget = -1;
};


class W_GUIFOUNDATION_DLL WQtPropertyStandardTypeContainerWidget : public WQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  WQtPropertyStandardTypeContainerWidget();
  virtual ~WQtPropertyStandardTypeContainerWidget();

protected:
  virtual WQtGroupBoxBase* CreateElement(QWidget* pParent) override;
  virtual WQtPropertyWidget* CreateWidget(WUInt32 index) override;
  virtual Element& AddElement(WUInt32 index) override;
  virtual void RemoveElement(WUInt32 index) override;
  virtual void UpdateElement(WUInt32 index) override;
};

class W_GUIFOUNDATION_DLL WQtPropertyTypeContainerWidget : public WQtPropertyContainerWidget
{
  Q_OBJECT;

public:
  WQtPropertyTypeContainerWidget();
  virtual ~WQtPropertyTypeContainerWidget();

protected:
  virtual void OnInit() override;
  virtual void UpdateElement(WUInt32 index) override;

  void StructureEventHandler(const WDocumentObjectStructureEvent& e);
  void CommandHistoryEventHandler(const WCommandHistoryEvent& e);

private:
  bool m_bNeedsUpdate = false;
};

class W_GUIFOUNDATION_DLL WQtVariantPropertyWidget : public WQtStandardPropertyWidget
{
  Q_OBJECT;

public:
  WQtVariantPropertyWidget();
  virtual ~WQtVariantPropertyWidget();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const WVariant& value) override;
  virtual void DoPrepareToDie() override;
  void UpdateTypeListSelection(WVariantType::Enum type);
  void ChangeVariantType(WVariantType::Enum type);
  void EnableTypeSelection(bool bEnable);

  virtual WResult GetVariantTypeDisplayName(WVariantType::Enum type, WStringBuilder& out_sName) const;

protected:
  QVBoxLayout* m_pLayout = nullptr;
  QComboBox* m_pTypeList = nullptr;
  WQtPropertyWidget* m_pWidget = nullptr;
  const WRTTI* m_pCurrentSubType = nullptr;
};

// Used for sub-containers of an WVariant, e.g. an WVariantArray or WVariantDictionary stored inside an WVariant. WVariantSubAccessor is used to create a view into a sub-tree container of the WVariant.
class W_GUIFOUNDATION_DLL WQtVariantContainerWidget : public WQtPropertyStandardTypeContainerWidget
{
  Q_OBJECT;

public:
  WQtVariantContainerWidget(WVariantType::Enum variantType);
  virtual ~WQtVariantContainerWidget() = default;

protected:
  virtual void OnInit() override;
  virtual void SetSelection(const WArrayPtr<WPropertySelection>& items) override;
  virtual WPropertyCategory::Enum GetContainerCategory() const override;

private:
  WUniquePtr<WVariantSubAccessor> m_pVariantSubAccessor;
  WEnum<WPropertyCategory> m_ContainerCategory;
};
