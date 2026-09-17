#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Dialogs/PreferencesDlg.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Preferences/Preferences.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

class WPreferencesObjectManager : public WDocumentObjectManager
{
public:
  virtual void GetCreateableTypes(WDynamicArray<const WRTTI*>& out_types) const override
  {
    for (auto pRtti : m_KnownTypes)
    {
      out_types.PushBack(pRtti);
    }
  }

  WHybridArray<const WRTTI*, 16> m_KnownTypes;
};


class WPreferencesDocument : public WDocument
{
  W_ADD_DYNAMIC_REFLECTION(WPreferencesDocument, WDocument);


public:
  WPreferencesDocument(WStringView sDocumentPath)
    : WDocument(sDocumentPath, W_DEFAULT_NEW(WPreferencesObjectManager))
  {
  }

public:
  virtual WDocumentInfo* CreateDocumentInfo() override { return W_DEFAULT_NEW(WDocumentInfo); }
};

W_BEGIN_DYNAMIC_REFLECTED_TYPE(WPreferencesDocument, 1, WRTTINoAllocator)
W_END_DYNAMIC_REFLECTED_TYPE;

WQtPreferencesDlg::WQtPreferencesDlg(QWidget* pParent)
  : WQtDialog(pParent)
{
  setupUi(this);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);

  m_pDocument = W_DEFAULT_NEW(WPreferencesDocument, "<none>");

  // if this is set, all properties are applied immediately
  // m_pDocument->GetObjectManager()->m_PropertyEvents.AddEventHandler(WMakeDelegate(&WQtPreferencesDlg::PropertyChangedEventHandler,
  // this));
  std::unique_ptr<WQtDocumentTreeModel> pModel(new WQtDocumentTreeModel(m_pDocument->GetObjectManager()));
  pModel->AddAdapter(new WQtDummyAdapter(m_pDocument->GetObjectManager(), WGetStaticRTTI<WDocumentRoot>(), "Children"));
  pModel->AddAdapter(new WQtNamedAdapter(m_pDocument->GetObjectManager(), WPreferences::GetStaticRTTI(), "", "Name"));

  Tree->Initialize(m_pDocument, std::move(pModel));
  Tree->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  Tree->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);

  RegisterAllPreferenceTypes();
  AllPreferencesToObject();

  Properties->SetDocument(m_pDocument);

  m_pDocument->GetSelectionManager()->SetSelection(m_pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
}

WQtPreferencesDlg::~WQtPreferencesDlg()
{
  delete Tree;
  Tree = nullptr;

  delete Properties;
  Properties = nullptr;

  W_DEFAULT_DELETE(m_pDocument);
}

WUuid WQtPreferencesDlg::NativeToObject(WPreferences* pPreferences)
{
  const WRTTI* pType = pPreferences->GetDynamicRTTI();
  // Write properties to graph.
  WAbstractObjectGraph graph;
  WRttiConverterContext context;
  WRttiConverterWriter conv(&graph, &context, true, true);

  const WUuid guid = WUuid::MakeUuid();
  context.RegisterObject(guid, pType, pPreferences);
  WAbstractObjectNode* pNode = conv.AddObjectToGraph(pType, pPreferences, "root");

  // Read from graph and write into matching document object.
  auto pRoot = m_pDocument->GetObjectManager()->GetRootObject();
  WDocumentObject* pObject = m_pDocument->GetObjectManager()->CreateObject(pType);
  m_pDocument->GetObjectManager()->AddObject(pObject, pRoot, "Children", -1);

  WDocumentObjectConverterReader objectConverter(
    &graph, m_pDocument->GetObjectManager(), WDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
  objectConverter.ApplyPropertiesToObject(pNode, pObject);

  return pObject->GetGuid();
}

void WQtPreferencesDlg::ObjectToNative(WUuid objectGuid, const WDocument* pPrefDocument)
{
  WDocumentObject* pObject = m_pDocument->GetObjectManager()->GetObject(objectGuid);
  const WRTTI* pType = pObject->GetTypeAccessor().GetType();

  // Write object to graph.
  WAbstractObjectGraph graph;
  auto filter = [](const WDocumentObject*, const WAbstractProperty* pProp) -> bool
  {
    if (pProp->GetFlags().IsSet(WPropertyFlags::ReadOnly))
      return false;
    return true;
  };
  WDocumentObjectConverterWriter objectConverter(&graph, m_pDocument->GetObjectManager(), filter);
  WAbstractObjectNode* pNode = objectConverter.AddObjectToGraph(pObject, "root");

  // Read from graph and write to native object.
  WRttiConverterContext context;
  WRttiConverterReader conv(&graph, &context);

  WPreferences* pPreferences = WPreferences::QueryPreferences(pType, pPrefDocument);
  conv.ApplyPropertiesToObject(pNode, pType, pPreferences);

  pPreferences->TriggerPreferencesChangedEvent();
}


void WQtPreferencesDlg::on_ButtonOk_clicked()
{
  ApplyAllChanges();
  accept();
}


void WQtPreferencesDlg::RegisterAllPreferenceTypes()
{
  WPreferencesObjectManager* pManager = static_cast<WPreferencesObjectManager*>(m_pDocument->GetObjectManager());

  WTempHybridArray<WPreferences*, 16> AllPrefs;
  WPreferences::GatherAllPreferences(AllPrefs);

  for (auto pref : AllPrefs)
  {
    pManager->m_KnownTypes.PushBack(pref->GetDynamicRTTI());
  }
}


void WQtPreferencesDlg::AllPreferencesToObject()
{
  WTempHybridArray<WPreferences*, 16> AllPrefs;
  WPreferences::GatherAllPreferences(AllPrefs);

  WTempHybridArray<const WAbstractProperty*, 32> properties;

  WMap<WString, WPreferences*> appPref;
  WMap<WString, WPreferences*> projPref;
  WMap<WString, WPreferences*> docPref;

  for (auto pref : AllPrefs)
  {
    bool noVisibleProperties = true;

    // ignore all objects that have no visible properties
    pref->GetDynamicRTTI()->GetAllProperties(properties);
    for (const WAbstractProperty* prop : properties)
    {
      if (prop->GetAttributeByType<WHiddenAttribute>() != nullptr)
        continue;

      noVisibleProperties = false;
      break;
    }

    if (noVisibleProperties)
      continue;

    switch (pref->GetDomain())
    {
      case WPreferences::Domain::Application:
        appPref[pref->GetName()] = pref;
        break;
      case WPreferences::Domain::Project:
        projPref[pref->GetName()] = pref;
        break;
      case WPreferences::Domain::Document:
        docPref[pref->GetName()] = pref;
        break;
    }
  }

  // create the objects in a certain order

  for (auto it = appPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = projPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }

  for (auto it = docPref.GetIterator(); it.IsValid(); ++it)
  {
    m_DocumentBinding[NativeToObject(it.Value())] = it.Value()->GetDocumentAssociation();
  }
}

void WQtPreferencesDlg::PropertyChangedEventHandler(const WDocumentObjectPropertyEvent& e)
{
  const WUuid guid = e.m_pObject->GetGuid();
  W_ASSERT_DEV(m_DocumentBinding.Contains(guid), "Object GUID is not in the known list!");

  ObjectToNative(guid, m_DocumentBinding[guid]);
}

void WQtPreferencesDlg::ApplyAllChanges()
{
  for (auto it = m_DocumentBinding.GetIterator(); it.IsValid(); ++it)
  {
    ObjectToNative(it.Key(), it.Value());
  }
}
