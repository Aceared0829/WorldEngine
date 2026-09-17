#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace WModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterMagicaVoxel : public Importer
  {
  public:
    ImporterMagicaVoxel();
    ~ImporterMagicaVoxel();

  protected:
    virtual WResult DoImport() override;
  };
} // namespace WModelImporter2
