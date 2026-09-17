#pragma once

#include <Foundation/Types/UniquePtr.h>
#include <ModelImporter2/Importer/Importer.h>

namespace WModelImporter2
{
  W_MODELIMPORTER2_DLL WUniquePtr<Importer> RequestImporterForFileType(WStringView sFile);

} // namespace WModelImporter2
