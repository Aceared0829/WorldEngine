#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/TranslationLookup.h>
#include <Foundation/Utilities/AssetInfoFile.h>

// Version of the file layout, not of the values inside it.
static constexpr WUInt16 s_uiAssetInfoFileVersion = 1;

static constexpr WStringView s_sRootObject = "AssetInfo"_wsv;
static constexpr WStringView s_sValuesObject = "Values"_wsv;
static constexpr WStringView s_sVersion = "Version"_wsv;
static constexpr WStringView s_sAssetHash = "AssetHash"_wsv;
static constexpr WStringView s_sTypeVersion = "TypeVersion"_wsv;

void WAssetInfoFile::SetValue(WStringView sKey, const WVariant& value)
{
  if (!value.IsValid())
  {
    m_Values.Remove(sKey);
    return;
  }

  m_Values[sKey] = value;
}

WVariant WAssetInfoFile::GetValue(WStringView sKey) const
{
  auto it = m_Values.Find(sKey);

  if (!it.IsValid())
    return WVariant();

  return it.Value();
}

WResult WAssetInfoFile::Write(WStreamWriter& inout_stream, const WAssetFileHeader& header) const
{
  WOpenDdlWriter ddl;
  ddl.SetOutputStream(&inout_stream);

  ddl.SetFloatPrecisionMode(WOpenDdlWriter::FloatPrecisionMode::Readable);

  ddl.BeginObject(s_sRootObject);
  {
    WOpenDdlUtils::StoreUInt16(ddl, s_uiAssetInfoFileVersion, s_sVersion);

    // Stored here rather than in a binary WAssetFileHeader, so that the entire file stays readable text.
    WOpenDdlUtils::StoreUInt64(ddl, header.GetFileHash(), s_sAssetHash);
    WOpenDdlUtils::StoreUInt16(ddl, header.GetFileVersion(), s_sTypeVersion);

    ddl.BeginObject(s_sValuesObject);
    {
      for (auto it = m_Values.GetIterator(); it.IsValid(); ++it)
      {
        WOpenDdlUtils::StoreVariant(ddl, it.Value(), it.Key());
      }
    }
    ddl.EndObject();
  }
  ddl.EndObject();

  return W_SUCCESS;
}

WResult WAssetInfoFile::Read(WStreamReader& inout_stream, WAssetFileHeader& out_header)
{
  m_Values.Clear();

  WOpenDdlReader ddl;
  if (ddl.ParseDocument(inout_stream).Failed())
    return W_FAILURE;

  const WOpenDdlReaderElement* pRoot = ddl.GetRootElement()->FindChildOfType(s_sRootObject);

  if (pRoot == nullptr)
    return W_FAILURE;

  const WOpenDdlReaderElement* pVersion = pRoot->FindChildOfType(WOpenDdlPrimitiveType::UInt16, s_sVersion);

  // A newer version may be structured differently, so it is not read at all.
  if (pVersion == nullptr || *pVersion->GetPrimitivesUInt16() > s_uiAssetInfoFileVersion)
    return W_FAILURE;

  const WOpenDdlReaderElement* pHash = pRoot->FindChildOfType(WOpenDdlPrimitiveType::UInt64, s_sAssetHash);
  const WOpenDdlReaderElement* pTypeVersion = pRoot->FindChildOfType(WOpenDdlPrimitiveType::UInt16, s_sTypeVersion);

  if (pHash == nullptr || pTypeVersion == nullptr)
    return W_FAILURE;

  out_header.SetFileHashAndVersion(*pHash->GetPrimitivesUInt64(), *pTypeVersion->GetPrimitivesUInt16());

  if (const WOpenDdlReaderElement* pValues = pRoot->FindChildOfType(s_sValuesObject))
  {
    for (const WOpenDdlReaderElement* pChild = pValues->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
    {
      if (pChild->GetName().IsEmpty())
        continue;

      WVariant value;

      // Skip a value of a type this build cannot represent, the remaining ones are still useful.
      if (WOpenDdlUtils::ConvertToVariant(pChild, value).Failed())
        continue;

      m_Values[pChild->GetName()] = value;
    }
  }

  return W_SUCCESS;
}

WResult WAssetInfoFile::WriteToFile(WStringView sAbsolutePath, const WAssetFileHeader& header) const
{
  // Remove a file from an earlier transform, it would be mistaken for current information.
  if (m_Values.IsEmpty())
  {
    WOSFile::DeleteFile(sAbsolutePath).IgnoreResult();
    return W_SUCCESS;
  }

  WDeferredFileWriter file;
  file.SetOutput(sAbsolutePath);

  if (Write(file, header).Failed())
  {
    file.Discard();
    return W_FAILURE;
  }

  return file.Close();
}

WResult WAssetInfoFile::ReadFromFile(WStringView sAbsolutePath, WUInt64 uiExpectedHash, WUInt16 uiExpectedTypeVersion)
{
  m_Values.Clear();

  WFileReader file;
  if (file.Open(sAbsolutePath).Failed())
    return W_FAILURE;

  WAssetFileHeader header;
  W_SUCCEED_OR_RETURN(Read(file, header));

  if (!header.IsFileUpToDate(uiExpectedHash, uiExpectedTypeVersion))
  {
    m_Values.Clear();
    return W_FAILURE;
  }

  return W_SUCCESS;
}

WStringBuilder WAssetInfoFile::GetInfoFilePathForOutput(WStringView sAbsoluteOutputPath)
{
  WStringBuilder sPath = sAbsoluteOutputPath;
  sPath.Append(".WAssetInfo");
  return sPath;
}

// Keys that are only ever shown as part of a combined line, and must not also be listed on their own.
static bool IsPartOfCombinedLine(WStringView sKey)
{
  return sKey == WAssetInfoFile::Keys::ImageHeight;
}

bool WAssetInfoFile::AppendValueToDisplayString(WStringBuilder& ref_sOut, WStringView sKey, WStringView sLinePrefix) const
{
  const WVariant value = GetValue(sKey);

  if (!value.IsValid() || IsPartOfCombinedLine(sKey))
    return false;

  if (sKey == Keys::ImageWidth)
  {
    const WVariant height = GetValue(Keys::ImageHeight);

    if (!height.IsValid())
      return false;

    ref_sOut.AppendFormat("{}Resolution: {} x {}", sLinePrefix, value, height);
    return true;
  }

  // The size is more useful than the half extents that it is stored as.
  if (sKey == Keys::BoundsHalfExtents)
  {
    if (!value.IsA<WVec3>())
      return false;

    const WVec3 vHalf = value.Get<WVec3>();
    ref_sOut.AppendFormat("{}Size: {} x {} x {}", sLinePrefix, WArgF(vHalf.x * 2, 3), WArgF(vHalf.y * 2, 3), WArgF(vHalf.z * 2, 3));
    return true;
  }

  // The center is shown as the range it produces, which reveals whether the origin sits inside the object or at its base.
  if (sKey == Keys::BoundsCenter)
  {
    const WVariant halfExtents = GetValue(Keys::BoundsHalfExtents);

    if (!value.IsA<WVec3>() || !halfExtents.IsValid() || !halfExtents.IsA<WVec3>())
      return false;

    const WVec3 vCenter = value.Get<WVec3>();
    const WVec3 vHalf = halfExtents.Get<WVec3>();
    ref_sOut.AppendFormat("{}Bounds: {} {} {} to {} {} {}", sLinePrefix,
      WArgF(vCenter.x - vHalf.x, 3), WArgF(vCenter.y - vHalf.y, 3), WArgF(vCenter.z - vHalf.z, 3),
      WArgF(vCenter.x + vHalf.x, 3), WArgF(vCenter.y + vHalf.y, 3), WArgF(vCenter.z + vHalf.z, 3));
    return true;
  }

  const WStringView sLabel = WTranslate(sKey);

  // Recorded lists are long enough to swamp everything else, so only the element count is shown.
  if (value.IsA<WVariantArray>())
  {
    ref_sOut.AppendFormat("{}{}: {}", sLinePrefix, sLabel, value.Get<WVariantArray>().GetCount());
    return true;
  }

  ref_sOut.AppendFormat("{}{}: {}", sLinePrefix, sLabel, value);
  return true;
}

void WAssetInfoFile::AppendToDisplayString(WStringBuilder& ref_sOut, WStringView sLinePrefix) const
{
  for (auto it = m_Values.GetIterator(); it.IsValid(); ++it)
  {
    AppendValueToDisplayString(ref_sOut, it.Key(), sLinePrefix);
  }
}

void WAssetInfoFile::AppendValuesToDisplayString(WStringBuilder& ref_sOut, WArrayPtr<const WStringView> keys, WStringView sLinePrefix) const
{
  for (WStringView sKey : keys)
  {
    AppendValueToDisplayString(ref_sOut, sKey, sLinePrefix);
  }
}
