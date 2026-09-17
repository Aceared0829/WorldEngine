#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <GuiFoundation/UIServices/DynamicStringEnum.h>

WMap<WString, WDynamicStringEnum> WDynamicStringEnum::s_DynamicEnums;
WDelegate<void(WStringView sEnumName, WDynamicStringEnum& e)> WDynamicStringEnum::s_RequestUnknownCallback;
WEvent<WDynamicStringEnum::RefreshValuesEvent&> WDynamicStringEnum::s_RefreshValuesEvent;

// static
WDynamicStringEnum& WDynamicStringEnum::GetDynamicEnum(WStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  if (!bExisted && s_RequestUnknownCallback.IsValid())
  {
    s_RequestUnknownCallback(sEnumName, it.Value());
  }

  return it.Value();
}

// static
WDynamicStringEnum& WDynamicStringEnum::CreateDynamicEnum(WStringView sEnumName)
{
  bool bExisted = false;
  auto it = s_DynamicEnums.FindOrAdd(sEnumName, &bExisted);

  WDynamicStringEnum& e = it.Value();
  e.Clear();
  e.SetStorageFile(nullptr);

  return e;
}

// static
void WDynamicStringEnum::RemoveEnum(WStringView sEnumName)
{
  s_DynamicEnums.Remove(sEnumName);
}

void WDynamicStringEnum::Clear()
{
  m_ValidValues.Clear();
}

void WDynamicStringEnum::AddValidValue(WStringView sValue, bool bSortValues /*= false*/)
{
  WString sNewValue = sValue;

  if (!m_ValidValues.Contains(sNewValue))
    m_ValidValues.PushBack(sNewValue);

  if (bSortValues)
    SortValues();
}

void WDynamicStringEnum::RemoveValue(WStringView sValue)
{
  m_ValidValues.RemoveAndCopy(sValue);
}

bool WDynamicStringEnum::IsValueValid(WStringView sValue) const
{
  return m_ValidValues.Contains(sValue);
}

void WDynamicStringEnum::SortValues()
{
  WCompareString_NoCase cmp;
  m_ValidValues.Sort(cmp);
}

void WDynamicStringEnum::SetEditCommand(WStringView sCmd, const WVariant& value)
{
  m_sEditCommand = sCmd;
  m_EditCommandValue = value;
}

void WDynamicStringEnum::ReadFromStorage()
{
  Clear();

  WStringBuilder sFile, tmp;

  WFileReader file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  sFile.ReadAll(file);

  WTempHybridArray<WStringView, 32> values;

  sFile.Split(false, values, "\n", "\r");

  for (auto val : values)
  {
    AddValidValue(val.GetData(tmp));
  }
}

void WDynamicStringEnum::SaveToStorage()
{
  if (m_sStorageFile.IsEmpty())
    return;

  WFileWriter file;
  if (file.Open(m_sStorageFile).Failed())
    return;

  WStringBuilder tmp;

  for (const auto& val : m_ValidValues)
  {
    tmp.Set(val, "\n");
    file.WriteBytes(tmp.GetData(), tmp.GetElementCount()).IgnoreResult();
  }
}
