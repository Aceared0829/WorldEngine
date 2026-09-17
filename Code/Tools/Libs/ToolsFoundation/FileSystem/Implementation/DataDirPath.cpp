#include <ToolsFoundation/ToolsFoundationDLL.h>

#include <ToolsFoundation/FileSystem/DataDirPath.h>

bool WDataDirPath::UpdateDataDirInfos(WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex /*= 0*/) const
{
  const WUInt32 uiCount = dataDirRoots.GetCount();
  for (WUInt32 i = 0; i < uiCount; ++i)
  {
    WUInt32 uiCurrentIndex = (uiLastKnownDataDirIndex + i) % uiCount;
    W_ASSERT_DEBUG(!dataDirRoots[uiCurrentIndex].EndsWith_NoCase("/"), "");
    if (m_sAbsolutePath.StartsWith_NoCase(dataDirRoots[uiCurrentIndex]) && !dataDirRoots[uiCurrentIndex].IsEmpty())
    {
      m_uiDataDirIndex = static_cast<WUInt8>(uiCurrentIndex);
      const char* szParentFolder = WPathUtils::FindPreviousSeparator(m_sAbsolutePath.GetData(), m_sAbsolutePath.GetData() + dataDirRoots[uiCurrentIndex].GetElementCount());
      m_uiDataDirParent = static_cast<WUInt16>(szParentFolder - m_sAbsolutePath.GetData());
      m_uiDataDirLength = static_cast<WUInt8>(dataDirRoots[uiCurrentIndex].GetElementCount() - m_uiDataDirParent);
      return true;
    }
  }

  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex = 0;
  return false;
}
