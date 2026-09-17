#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WEditAction, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

////////////////////////////////////////////////////////////////////////
// WEditActions
////////////////////////////////////////////////////////////////////////

WActionDescriptorHandle WEditActions::s_hEditCategory;
WActionDescriptorHandle WEditActions::s_hCopy;
WActionDescriptorHandle WEditActions::s_hPaste;
WActionDescriptorHandle WEditActions::s_hPasteAsChild;
WActionDescriptorHandle WEditActions::s_hPasteAtOriginalLocation;
WActionDescriptorHandle WEditActions::s_hDelete;

void WEditActions::RegisterActions()
{
  s_hEditCategory = W_REGISTER_CATEGORY("EditCategory");
  s_hCopy = W_REGISTER_ACTION_1("Selection.Copy", WActionScope::Document, "Document", "Ctrl+C", WEditAction, WEditAction::ButtonType::Copy);
  s_hPaste = W_REGISTER_ACTION_1("Selection.Paste", WActionScope::Document, "Document", "Ctrl+V", WEditAction, WEditAction::ButtonType::Paste);
  s_hPasteAsChild = W_REGISTER_ACTION_1("Selection.PasteAsChild", WActionScope::Document, "Document", "", WEditAction, WEditAction::ButtonType::PasteAsChild);
  s_hPasteAtOriginalLocation = W_REGISTER_ACTION_1("Selection.PasteAtOriginalLocation", WActionScope::Document, "Document", "", WEditAction, WEditAction::ButtonType::PasteAtOriginalLocation);
  s_hDelete = W_REGISTER_ACTION_1("Selection.Delete", WActionScope::Document, "Document", "", WEditAction, WEditAction::ButtonType::Delete);
}

void WEditActions::UnregisterActions()
{
  WActionManager::UnregisterAction(s_hEditCategory);
  WActionManager::UnregisterAction(s_hCopy);
  WActionManager::UnregisterAction(s_hPaste);
  WActionManager::UnregisterAction(s_hPasteAsChild);
  WActionManager::UnregisterAction(s_hPasteAtOriginalLocation);
  WActionManager::UnregisterAction(s_hDelete);
}

void WEditActions::MapActions(WStringView sMapping, bool bDeleteAction, bool bAdvancedPasteActions)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", sMapping);

  pMap->MapAction(s_hEditCategory, "G.Edit", 3.5f);

  pMap->MapAction(s_hCopy, "G.Edit", "EditCategory", 1.0f);
  pMap->MapAction(s_hPaste, "G.Edit", "EditCategory", 2.0f);

  if (bAdvancedPasteActions)
  {
    pMap->MapAction(s_hPasteAsChild, "G.Edit", "EditCategory", 2.5f);
    pMap->MapAction(s_hPasteAtOriginalLocation, "G.Edit", "EditCategory", 2.7f);
  }

  if (bDeleteAction)
    pMap->MapAction(s_hDelete, "G.Edit", "EditCategory", 3.0f);
}


void WEditActions::MapContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", sMapping);

  pMap->MapAction(s_hEditCategory, "", 10.0f);

  pMap->MapAction(s_hCopy, "EditCategory", 1.0f);
  pMap->MapAction(s_hPasteAsChild, "EditCategory", 2.0f);
  pMap->MapAction(s_hDelete, "EditCategory", 3.0f);
}


void WEditActions::MapViewContextMenuActions(WStringView sMapping)
{
  WActionMap* pMap = WActionMapManager::GetActionMap(sMapping);
  W_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the edit actions failed!", sMapping);

  pMap->MapAction(s_hEditCategory, "", 10.0f);

  pMap->MapAction(s_hCopy, "EditCategory", 1.0f);
  pMap->MapAction(s_hPasteAsChild, "EditCategory", 2.0f);
  pMap->MapAction(s_hPasteAtOriginalLocation, "EditCategory", 2.5f);
  pMap->MapAction(s_hDelete, "EditCategory", 3.0f);
}

////////////////////////////////////////////////////////////////////////
// WEditAction
////////////////////////////////////////////////////////////////////////

WEditAction::WEditAction(const WActionContext& context, const char* szName, ButtonType button)
  : WButtonAction(context, szName, false, "")
{
  m_ButtonType = button;

  switch (m_ButtonType)
  {
    case WEditAction::ButtonType::Copy:
      SetIconPath(":/GuiFoundation/Icons/Copy.svg");
      break;
    case WEditAction::ButtonType::Paste:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg");
      break;
    case WEditAction::ButtonType::PasteAsChild:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg"); /// TODO Icon
      break;
    case WEditAction::ButtonType::PasteAtOriginalLocation:
      SetIconPath(":/GuiFoundation/Icons/Paste.svg");
      break;
    case WEditAction::ButtonType::Delete:
      SetIconPath(":/GuiFoundation/Icons/Delete.svg");
      break;
  }

  m_Context.m_pDocument->GetSelectionManager()->m_Events.AddEventHandler(WMakeDelegate(&WEditAction::SelectionEventHandler, this));

  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}

WEditAction::~WEditAction()
{
  if (m_Context.m_pDocument)
  {
    m_Context.m_pDocument->GetSelectionManager()->m_Events.RemoveEventHandler(WMakeDelegate(&WEditAction::SelectionEventHandler, this));
  }
}

void WEditAction::Execute(const WVariant& value)
{
  switch (m_ButtonType)
  {
    case WEditAction::ButtonType::Copy:
    {
      WStringBuilder sMimeType;

      WAbstractObjectGraph graph;
      if (!m_Context.m_pDocument->CopySelectedObjects(graph, sMimeType))
        break;

      // Serialize to string
      WContiguousMemoryStreamStorage streamStorage;
      WMemoryStreamWriter memoryWriter(&streamStorage);
      WAbstractGraphDdlSerializer::Write(memoryWriter, &graph, nullptr, false);
      memoryWriter.WriteBytes("\0", 1).IgnoreResult(); // null terminate

      // Write to clipboard
      QClipboard* clipboard = QApplication::clipboard();
      QMimeData* mimeData = new QMimeData();
      QByteArray encodedData((const char*)streamStorage.GetData(), streamStorage.GetStorageSize32());

      mimeData->setData(sMimeType.GetData(), encodedData);
      mimeData->setText(QString::fromUtf8((const char*)streamStorage.GetData()));
      clipboard->setMimeData(mimeData);
    }
    break;

    case WEditAction::ButtonType::Paste:
    case WEditAction::ButtonType::PasteAsChild:
    case WEditAction::ButtonType::PasteAtOriginalLocation:
    {
      // Check for clipboard data of the correct type.
      QClipboard* clipboard = QApplication::clipboard();
      auto mimedata = clipboard->mimeData();

      WTempHybridArray<WString, 4> MimeTypes;
      m_Context.m_pDocument->GetSupportedMimeTypesForPasting(MimeTypes);

      WInt32 iFormat = -1;
      {
        for (WUInt32 i = 0; i < MimeTypes.GetCount(); ++i)
        {
          if (mimedata->hasFormat(MimeTypes[i].GetData()))
          {
            iFormat = i;
            break;
          }
        }

        if (iFormat < 0)
          break;
      }

      // Paste at current selected object.
      WPasteObjectsCommand cmd;
      cmd.m_sMimeType = MimeTypes[iFormat];

      QByteArray ba = mimedata->data(MimeTypes[iFormat].GetData());
      cmd.m_sGraphTextFormat = ba.data();

      const WDocumentObject* pNewParent = m_Context.m_pDocument->GetSelectionManager()->GetCurrentObject();
      if (pNewParent && m_ButtonType != ButtonType::PasteAsChild)
      {
        // default behavior copied from Unity: paste as a sibling of the currently selected item
        // this way if you just select and object and copy/paste it, the new object has the same parent (the clone becomes a sibling of the original)
        // but you can also select any other object as the reference, and clone as a sibling to that one
        pNewParent = pNewParent->GetParent();
      }

      if (pNewParent)
      {
        cmd.m_Parent = pNewParent->GetGuid();
      }

      if (m_ButtonType == ButtonType::PasteAtOriginalLocation)
      {
        cmd.m_bAllowPickedPosition = false;
      }

      auto history = m_Context.m_pDocument->GetCommandHistory();

      history->StartTransaction("Paste");

      if (history->AddCommand(cmd).Failed())
        history->CancelTransaction();
      else
        history->FinishTransaction();
    }
    break;

    case WEditAction::ButtonType::Delete:
    {
      m_Context.m_pDocument->DeleteSelectedObjects();
    }
    break;
  }
}

void WEditAction::SelectionEventHandler(const WSelectionManagerEvent& e)
{
  if (m_ButtonType == ButtonType::Copy || m_ButtonType == ButtonType::Delete)
  {
    SetEnabled(!m_Context.m_pDocument->GetSelectionManager()->IsSelectionEmpty());
  }
}
