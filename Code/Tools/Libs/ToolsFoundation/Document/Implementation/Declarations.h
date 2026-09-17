#pragma once

class WDocument;
class WDocumentManager;
class WDocumentObjectManager;
class WAbstractObjectGraph;

struct WDocumentFlags
{
  using StorageType = WUInt8;

  enum Enum
  {
    None = 0,
    RequestWindow = W_BIT(0),        ///< Open the document visibly (not just internally)
    AddToRecentFilesList = W_BIT(1), ///< Add the document path to the recently used list for users
    AsyncSave = W_BIT(2),            ///<
    EmptyDocument = W_BIT(3),        ///< Don't populate a new document with default state (templates etc)
    Default = None,
  };

  struct Bits
  {
    StorageType RequestWindow : 1;
    StorageType AddToRecentFilesList : 1;
    StorageType AsyncSave : 1;
    StorageType EmptyDocument : 1;
  };
};

W_DECLARE_FLAGS_OPERATORS(WDocumentFlags);


struct W_TOOLSFOUNDATION_DLL WDocumentTypeDescriptor
{
  WString m_sFileExtension;
  WString m_sDocumentTypeName;
  bool m_bCanCreate = true;
  WString m_sIcon;
  const WRTTI* m_pDocumentType = nullptr;
  WDocumentManager* m_pManager = nullptr;
  WStringView m_sAssetCategory; // passed to WColorScheme::GetCategoryColor() with CategoryColorUsage::AssetMenuIcon

  /// This list is used to decide which asset types can be picked from the asset browser for a property.
  /// The strings are arbitrary and don't need to be registered anywhere else.
  /// An asset may be compatible for multiple scenarios, e.g. a skinned mesh may also be used as a static mesh, but not the other way round.
  /// In such a case the skinned mesh is set to be compatible to both "CompatibleAsset_Mesh_Static" and "CompatibleAsset_Mesh_Skinned", but the non-skinned mesh only to "CompatibleAsset_Mesh_Static".
  /// A component then only needs to specify that it takes an "CompatibleAsset_Mesh_Static" as input, and all asset types that are compatible to that will be browseable.
  WHybridArray<WString, 1> m_CompatibleTypes;
};


struct WDocumentEvent
{
  enum class Type
  {
    ModifiedChanged,
    ReadOnlyChanged,
    EnsureVisible,
    DocumentSaved,
    DocumentRenamed,
    DocumentStatusMsg,
  };

  Type m_Type;
  const WDocument* m_pDocument;

  WStringView m_sStatusMsg;
};

class W_TOOLSFOUNDATION_DLL WDocumentInfo : public WReflectedClass
{
  W_ADD_DYNAMIC_REFLECTION(WDocumentInfo, WReflectedClass);

public:
  WDocumentInfo();

  WUuid m_DocumentID;
};
