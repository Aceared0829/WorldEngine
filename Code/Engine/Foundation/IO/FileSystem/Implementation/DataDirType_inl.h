#pragma once

#include <Foundation/Strings/StringBuilder.h>

inline WDataDirectoryReaderWriterBase::WDataDirectoryReaderWriterBase(WInt32 iDataDirUserData, bool bIsReader)
{
  m_iDataDirUserData = iDataDirUserData;
  m_pDataDirType = nullptr;
  m_bIsReader = bIsReader;
}

inline WResult WDataDirectoryReaderWriterBase::Open(WStringView sFile, WDataDirectoryType* pDataDirectory, WFileShareMode::Enum fileShareMode)
{
  m_pDataDirType = pDataDirectory;
  m_sFilePath = sFile;

  return InternalOpen(fileShareMode);
}

inline const WString128& WDataDirectoryReaderWriterBase::GetFilePath() const
{
  return m_sFilePath;
}

inline WDataDirectoryType* WDataDirectoryReaderWriterBase::GetDataDirectory() const
{
  return m_pDataDirType;
}
