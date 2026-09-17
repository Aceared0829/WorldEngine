#include <ModelImporter2/ModelImporterPCH.h>

#include <ModelImporter2/ImporterAssimp/ImporterAssimp.h>
#include <ModelImporter2/ImporterMagicaVoxel/ImporterMagicaVoxel.h>
#include <ModelImporter2/ImporterSourceBSP/ImporterSourceBSP.h>
#include <ModelImporter2/ModelImporter.h>

namespace WModelImporter2
{

  WUniquePtr<Importer> RequestImporterForFileType(WStringView sFile)
  {
    if (sFile.HasExtension(".fbx") || sFile.HasExtension(".obj") || sFile.HasExtension(".gltf") || sFile.HasExtension(".glb") || sFile.HasExtension(".blend"))
    {
      return W_DEFAULT_NEW(ImporterAssimp);
    }

    if (sFile.HasExtension(".bsp"))
    {
      return W_DEFAULT_NEW(ImporterSourceBSP);
    }

    if (sFile.HasExtension(".vox"))
    {
      return W_DEFAULT_NEW(ImporterMagicaVoxel);
    }

    return nullptr;
  }


} // namespace WModelImporter2
