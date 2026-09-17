#pragma once

#include <ModelImporter2/Importer/Importer.h>

namespace WModelImporter2
{
  /// Importer implementation to import Source engine BSP files.
  class ImporterSourceBSP : public Importer
  {
  public:
    ImporterSourceBSP();
    ~ImporterSourceBSP();

  protected:
    virtual WResult DoImport() override;
  };
} // namespace WModelImporter2
