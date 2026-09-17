

inline WDataDirPath::WDataDirPath() = default;

inline WDataDirPath::WDataDirPath(WStringView sAbsPath, WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  W_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  WStringBuilder sTmp = sAbsPath;
  WPathUtils::NormalizeWindowsDriveLetter(sTmp);
  m_sAbsolutePath = sTmp;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline WDataDirPath::WDataDirPath(const WStringBuilder& sAbsPath, WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  W_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  WStringBuilder sTmp = sAbsPath;
  WPathUtils::NormalizeWindowsDriveLetter(sTmp);
  m_sAbsolutePath = sTmp;
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline WDataDirPath::WDataDirPath(WString&& sAbsPath, WArrayPtr<WString> dataDirRoots, WUInt32 uiLastKnownDataDirIndex /*= 0*/)
{
  W_ASSERT_DEBUG(!sAbsPath.EndsWith_NoCase("/"), "");
  m_sAbsolutePath = std::move(sAbsPath);
  {
    WStringBuilder sTmp = m_sAbsolutePath;
    WPathUtils::NormalizeWindowsDriveLetter(sTmp);
    if (sTmp != m_sAbsolutePath)
      m_sAbsolutePath = sTmp;
  }
  UpdateDataDirInfos(dataDirRoots, uiLastKnownDataDirIndex);
}

inline WDataDirPath::operator WStringView() const
{
  return m_sAbsolutePath;
}

inline bool WDataDirPath::operator==(WStringView rhs) const
{
  return m_sAbsolutePath == rhs;
}

inline bool WDataDirPath::operator!=(WStringView rhs) const
{
  return m_sAbsolutePath != rhs;
}

inline bool WDataDirPath::IsValid() const
{
  return m_uiDataDirParent != 0;
}

inline void WDataDirPath::Clear()
{
  m_sAbsolutePath.Clear();
  m_uiDataDirParent = 0;
  m_uiDataDirLength = 0;
  m_uiDataDirIndex = 0;
}

inline const WString& WDataDirPath::GetAbsolutePath() const
{
  return m_sAbsolutePath;
}

inline WStringView WDataDirPath::GetDataDirParentRelativePath() const
{
  W_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const WUInt32 uiOffset = m_uiDataDirParent + 1;
  return WStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline WStringView WDataDirPath::GetDataDirRelativePath() const
{
  W_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  const WUInt32 uiOffset = WMath::Min(m_sAbsolutePath.GetElementCount(), m_uiDataDirParent + m_uiDataDirLength + 1u);
  return WStringView(m_sAbsolutePath.GetData() + uiOffset, m_sAbsolutePath.GetElementCount() - uiOffset);
}

inline WStringView WDataDirPath::GetDataDir() const
{
  W_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return WStringView(m_sAbsolutePath.GetData(), m_uiDataDirParent + m_uiDataDirLength);
}

inline WUInt8 WDataDirPath::GetDataDirIndex() const
{
  W_ASSERT_DEBUG(IsValid(), "Path is not in a data directory, only GetAbsolutePath is allowed to be called.");
  return m_uiDataDirIndex;
}

inline WStreamWriter& WDataDirPath::Write(WStreamWriter& inout_stream) const
{
  inout_stream << m_sAbsolutePath;
  inout_stream << m_uiDataDirParent;
  inout_stream << m_uiDataDirLength;
  inout_stream << m_uiDataDirIndex;
  return inout_stream;
}

inline WStreamReader& WDataDirPath::Read(WStreamReader& inout_stream)
{
  inout_stream >> m_sAbsolutePath;
  {
    // Caches written before the drive letter was normalized can still hold the other spelling.
    WStringBuilder sTmp = m_sAbsolutePath;
    WPathUtils::NormalizeWindowsDriveLetter(sTmp);
    if (sTmp != m_sAbsolutePath)
      m_sAbsolutePath = sTmp;
  }
  inout_stream >> m_uiDataDirParent;
  inout_stream >> m_uiDataDirLength;
  inout_stream >> m_uiDataDirIndex;
  return inout_stream;
}

bool WCompareDataDirPath::Less(WStringView lhs, WStringView rhs)
{
  int res = lhs.Compare_NoCase(rhs);
  if (res == 0)
  {
    return lhs.Compare(rhs) < 0;
  }

  return res < 0;
}

bool WCompareDataDirPath::Equal(WStringView lhs, WStringView rhs)
{
  return lhs.IsEqual(rhs);
}

inline WStreamWriter& operator<<(WStreamWriter& inout_stream, const WDataDirPath& value)
{
  return value.Write(inout_stream);
}

inline WStreamReader& operator>>(WStreamReader& inout_stream, WDataDirPath& out_value)
{
  return out_value.Read(inout_stream);
}
