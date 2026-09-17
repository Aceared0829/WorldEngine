#include <AngelScriptPlugin/AngelScriptPluginPCH.h>

#include <AngelScript/include/angelscript.h>
#include <AngelScriptPlugin/Runtime/AsEngineSingleton.h>
#include <AngelScriptPlugin/Runtime/AsStringFactory.h>
#include <AngelScriptPlugin/Utils/AngelScriptUtils.h>
#include <Core/World/GameObject.h>
#include <Core/World/World.h>

//////////////////////////////////////////////////////////////////////////
// WStringBase
//////////////////////////////////////////////////////////////////////////

template <typename T>
void RegisterStringBase(asIScriptEngine* pEngine, const char* szType)
{
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool StartsWith(WStringView) const", asMETHOD(T, StartsWith), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool StartsWith_NoCase(WStringView) const", asMETHOD(T, StartsWith_NoCase), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool EndsWith(WStringView) const", asMETHOD(T, EndsWith), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool EndsWith_NoCase(WStringView) const", asMETHOD(T, EndsWith_NoCase), asCALL_THISCALL));

  // FindSubString
  // FindSubString_NoCase
  // FindLastSubString
  // FindLastSubString_NoCase
  // FindWholeWord
  // FindWholeWord_NoCase

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int Compare(WStringView) const", asMETHOD(T, Compare), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int Compare_NoCase(WStringView) const", asMETHOD(T, Compare_NoCase), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int CompareN(WStringView, uint32) const", asMETHOD(T, CompareN), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "int CompareN_NoCase(WStringView, uint32) const", asMETHOD(T, CompareN_NoCase), asCALL_THISCALL));

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "uint32 GetElementCount() const", asMETHOD(T, GetElementCount), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEmpty() const", asMETHOD(T, IsEmpty), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqual(WStringView) const", asMETHOD(T, IsEqual), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqual_NoCase(WStringView) const", asMETHOD(T, IsEqual_NoCase), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqualN(WStringView, uint32) const", asMETHOD(T, IsEqualN), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsEqualN_NoCase(WStringView, uint32) const", asMETHOD(T, IsEqualN_NoCase), asCALL_THISCALL));

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool HasAnyExtension() const", asMETHOD(T, HasAnyExtension), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool HasExtension(WStringView) const", asMETHOD(T, HasExtension), asCALL_THISCALL));

  AS_CHECK(pEngine->RegisterObjectMethod(szType, "WStringView GetFileExtension(bool full = false) const", asMETHOD(T, GetFileExtension), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "WStringView GetFileName() const", asMETHOD(T, GetFileName), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "WStringView GetFileNameAndExtension() const", asMETHOD(T, GetFileNameAndExtension), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "WStringView GetFileDirectory() const", asMETHOD(T, GetFileDirectory), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsAbsolutePath() const", asMETHOD(T, IsAbsolutePath), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsRelativePath() const", asMETHOD(T, IsRelativePath), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool IsRootedPath() const", asMETHOD(T, IsRootedPath), asCALL_THISCALL));
  AS_CHECK(pEngine->RegisterObjectMethod(szType, "bool GetRootedPathRootName() const", asMETHOD(T, GetRootedPathRootName), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// WStringView
//////////////////////////////////////////////////////////////////////////

static int WStringView_opCmp(WStringView* lhs, const WStringView& rhs)
{
  if (*lhs < rhs)
    return -1;
  if (rhs < *lhs)
    return +1;

  return 0;
}

static void WStringView_Construct(void* pMemory)
{
  new (pMemory) WStringView();
}

static void WStringView_ConstructView(void* pMemory, const WStringView rhs)
{
  new (pMemory) WStringView(rhs);
}

static void WStringView_ConstructString(void* pMemory, const WString& rhs)
{
  const WString& str = WAsStringFactory::GetFactory()->StoreString(rhs);

  new (pMemory) WStringView(str);
}

static void WStringView_opAssignString(WStringView* lhs, const WString& rhs)
{
  const WString& str = WAsStringFactory::GetFactory()->StoreString(rhs);

  *lhs = str;
}

static void WStringView_ConstructStringBuilder(void* pMemory, const WStringBuilder& rhs)
{
  const WString& str = WAsStringFactory::GetFactory()->StoreString(rhs);

  new (pMemory) WStringView(str);
}

static void WStringView_opAssignStringBuilder(WStringView* lhs, const WStringBuilder& rhs)
{
  const WString& str = WAsStringFactory::GetFactory()->StoreString(rhs);

  *lhs = str;
}

static void WStringView_ConstructHS(void* pMemory, const WHashedString& rhs)
{
  const WString& str = rhs.GetString();

  new (pMemory) WStringView(str);
}

static void WStringView_opAssignHS(WStringView* lhs, const WHashedString& rhs)
{
  const WString& str = rhs.GetString();

  *lhs = str;
}

static bool WStringView_opEqual(WStringView* lhs, const WStringView& rhs)
{
  return *lhs == rhs;
}

void WAngelScriptEngineSingleton::Register_StringView()
{
  RegisterStringBase<WStringView>(m_pEngine, "WStringView");

  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "void Shrink(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack)", asMETHOD(WStringView, Shrink), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "WStringView GetShrunk(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack = 0) const", asMETHOD(WStringView, GetShrunk), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "WStringView GetSubString(WUInt32 uiFirstCharacter, WUInt32 uiNumCharacters) const", asMETHOD(WStringView, GetSubString), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "void ChopAwayFirstCharacterUtf8()", asMETHOD(WStringView, ChopAwayFirstCharacterUtf8), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "void ChopAwayFirstCharacterAscii()", asMETHOD(WStringView, ChopAwayFirstCharacterAscii), asCALL_THISCALL));
  // Trim
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "bool TrimWordStart(WStringView sWord)", asMETHOD(WStringView, TrimWordStart), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "bool TrimWordEnd(WStringView sWord)", asMETHOD(WStringView, TrimWordEnd), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "bool opEquals(const WStringView& in) const", asFUNCTIONPR(WStringView_opEqual, (WStringView*, const WStringView&), bool), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "int opCmp(const WStringView& in) const", asFUNCTIONPR(WStringView_opCmp, (WStringView*, const WStringView&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringView", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WStringView_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringView", asBEHAVE_CONSTRUCT, "void f(const WStringView)", asFUNCTION(WStringView_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringView", asBEHAVE_CONSTRUCT, "void f(const WString& in)", asFUNCTION(WStringView_ConstructString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringView", asBEHAVE_CONSTRUCT, "void f(const WHashedString& in)", asFUNCTION(WStringView_ConstructHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "void opAssign(const WString& in)", asFUNCTION(WStringView_opAssignString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "void opAssign(const WHashedString& in)", asFUNCTION(WStringView_opAssignHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringView", asBEHAVE_CONSTRUCT, "void f(const WStringBuilder& in)", asFUNCTION(WStringView_ConstructStringBuilder), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WStringView", "void opAssign(const WStringBuilder& in)", asFUNCTION(WStringView_opAssignStringBuilder), asCALL_CDECL_OBJFIRST));
}


//////////////////////////////////////////////////////////////////////////
// WString
//////////////////////////////////////////////////////////////////////////

static void WString_Construct(void* pMemory)
{
  new (pMemory) WString();
}

static void WString_Destruct(void* pMemory)
{
  WString* p = (WString*)pMemory;
  p->~WString();
}

static void WString_ConstructView(void* pMemory, WStringView rhs)
{
  new (pMemory) WString(rhs);
}

static void WString_ConstructString(void* pMemory, const WString& rhs)
{
  new (pMemory) WString(rhs);
}

static void WString_ConstructStringBuilder(void* pMemory, const WStringBuilder& rhs)
{
  new (pMemory) WString(rhs);
}

static void WString_ConstructHS(void* pMemory, const WHashedString& rhs)
{
  new (pMemory) WString(rhs.GetView());
}

static int WString_opCmp(const WString& lhs, const WString& rhs)
{
  if (lhs < rhs)
    return -1;
  if (rhs < lhs)
    return +1;

  return 0;
}

static void WString_opAssignString(WString* lhs, const WString& rhs)
{
  *lhs = rhs;
}

static void WString_opAssignStringView(WString* lhs, WStringView rhs)
{
  *lhs = rhs;
}

static void WString_opAssignStringBuilder(WString* lhs, const WStringBuilder& rhs)
{
  *lhs = rhs;
}

static void WString_opAssignHS(WString* lhs, const WHashedString& rhs)
{
  *lhs = rhs.GetView();
}

void WAngelScriptEngineSingleton::Register_String()
{
  RegisterStringBase<WString>(m_pEngine, "WString");

  AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "WStringView GetView() const", asMETHOD(WString, GetView), asCALL_THISCALL));
  // AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "uint32 GetCharacterCount() const", asMETHOD(WString, GetCharacterCount), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "int opCmp(const WString& in) const", asFUNCTIONPR(WString_opCmp, (const WString&, const WString&), int), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WString", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(WString_Destruct), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WString", asBEHAVE_CONSTRUCT, "void f(const WStringView)", asFUNCTION(WString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "void opAssign(const WStringView)", asFUNCTION(WString_opAssignStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WString", asBEHAVE_CONSTRUCT, "void f(const WString& in)", asFUNCTION(WString_ConstructString), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "void opAssign(const WString& in)", asFUNCTION(WString_opAssignString), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WString", asBEHAVE_CONSTRUCT, "void f(const WStringBuilder& in)", asFUNCTION(WString_ConstructStringBuilder), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "void opAssign(const WStringBuilder& in)", asFUNCTION(WString_opAssignStringBuilder), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WString", asBEHAVE_CONSTRUCT, "void f(const WHashedString& in)", asFUNCTION(WString_ConstructHS), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "void opAssign(const WHashedString& in)", asFUNCTION(WString_opAssignHS), asCALL_CDECL_OBJFIRST));

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WString", "void Clear()", asMETHOD(WString, Clear), asCALL_THISCALL));
  }
}

//////////////////////////////////////////////////////////////////////////
// WStringBuilder
//////////////////////////////////////////////////////////////////////////

static void WStringBuilder_Construct(void* pMemory)
{
  new (pMemory) WStringBuilder();
}

static void WStringBuilder_ConstructSV1(void* pMemory, const WStringView sView)
{
  new (pMemory) WStringBuilder(sView);
}

static void WStringBuilder_ConstructSV2(void* pMemory, const WStringView& sV1, const WStringView& sV2)
{
  new (pMemory) WStringBuilder(sV1, sV2);
}

static void WStringBuilder_ConstructSV3(void* pMemory, const WStringView& sV1, const WStringView& sV2, const WStringView& sV3)
{
  new (pMemory) WStringBuilder(sV1, sV2, sV3);
}

static void WStringBuilder_ConstructSV4(void* pMemory, const WStringView& sV1, const WStringView& sV2, const WStringView& sV3, const WStringView& sV4)
{
  new (pMemory) WStringBuilder(sV1, sV2, sV3, sV4);
}

static void WStringBuilder_Destruct(void* pMemory)
{
  WStringBuilder* p = (WStringBuilder*)pMemory;
  p->~WStringBuilder();
}

static void WStringBuilder_Format(WStringBuilder& ref_sStr, asIScriptGeneric* pGen)
{
  const WUInt32 uiNumArgs = (WUInt32)pGen->GetArgCount();
  const WStringView sText = *((WStringView*)pGen->GetArgObject(0));

  WTempHybridArray<WString, 12> stringStorage;
  WTempHybridArray<WStringView, 12> stringViews;
  stringStorage.Reserve(pGen->GetArgCount() - 1);

  WVariant res;
  for (WUInt32 uiArg = 1; uiArg < uiNumArgs; ++uiArg)
  {
    auto argTypeId = pGen->GetArgTypeId(uiArg);

    if (WAngelScriptUtils::ReadFromAsTypeAtLocation(pGen->GetEngine(), argTypeId, pGen->GetArgAddress(uiArg), res).Succeeded())
    {
      stringStorage.PushBack(res.ConvertTo<WString>());
      continue;
    }

    const char* typeName = "null";
    if (const asITypeInfo* pInfo = pGen->GetEngine()->GetTypeInfoById(argTypeId))
    {
      typeName = pInfo->GetName();
    }

    WLog::Error("Call to 'WStringBuilder::SetFormat': Argument {} got an unsupported type '{}' ({})", uiArg, typeName, argTypeId);
    break;
  }

  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  WFormatString fs(sText);
  fs.BuildFormattedText(ref_sStr, stringViews.GetData(), stringViews.GetCount());
}

static void WStringBuilder_SetFormat(asIScriptGeneric* pGen)
{
  WStringBuilder& sb = *((WStringBuilder*)pGen->GetObject());
  sb.Clear();

  WStringBuilder_Format(sb, pGen);
}

static void WStringBuilder_AppendFormat(asIScriptGeneric* pGen)
{
  WStringBuilder& sb = *((WStringBuilder*)pGen->GetObject());

  WStringBuilder tmp;
  WStringBuilder_Format(tmp, pGen);
  sb.Append(tmp);
}

static void WStringBuilder_PrependFormat(asIScriptGeneric* pGen)
{
  WStringBuilder& sb = *((WStringBuilder*)pGen->GetObject());

  WStringBuilder tmp;
  WStringBuilder_Format(tmp, pGen);
  sb.Prepend(tmp);
}

void WAngelScriptEngineSingleton::Register_StringBuilder()
{
  RegisterStringBase<WStringBuilder>(m_pEngine, "WStringBuilder");

  // Constructors
  {
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringBuilder", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(WStringBuilder_Destruct), asCALL_CDECL_OBJFIRST));

    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringBuilder", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WStringBuilder_Construct), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringBuilder", asBEHAVE_CONSTRUCT, "void f(const WStringView s1)", asFUNCTION(WStringBuilder_ConstructSV1), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringBuilder", asBEHAVE_CONSTRUCT, "void f(const WStringView& in, const WStringView& in)", asFUNCTION(WStringBuilder_ConstructSV2), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringBuilder", asBEHAVE_CONSTRUCT, "void f(const WStringView& in, const WStringView& in, const WStringView& in)", asFUNCTION(WStringBuilder_ConstructSV3), asCALL_CDECL_OBJFIRST));
    AS_CHECK(m_pEngine->RegisterObjectBehaviour("WStringBuilder", asBEHAVE_CONSTRUCT, "void f(const WStringView& in, const WStringView& in, const WStringView& in, const WStringView& in)", asFUNCTION(WStringBuilder_ConstructSV4), asCALL_CDECL_OBJFIRST));
  }

  // Operators
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void opAssign(const WStringBuilder& in rhs)", asMETHODPR(WStringBuilder, operator=, (const WStringBuilder&), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void opAssign(WStringView rhs)", asMETHODPR(WStringBuilder, operator=, (WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void opAssign(const WString& in rhs)", asMETHODPR(WStringBuilder, operator=, (const WString&), void), asCALL_THISCALL));
  }

  // Methods
  {
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "WStringView GetView() const", asMETHOD(WStringBuilder, GetView), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Clear()", asMETHOD(WStringBuilder, Clear), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "WUInt32 GetCharacterCount() const", asMETHOD(WStringBuilder, GetCharacterCount), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void ToUpper()", asMETHOD(WStringBuilder, ToUpper), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void ToLower()", asMETHOD(WStringBuilder, ToLower), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Set(WStringView sData1)", asMETHODPR(WStringBuilder, Set, (WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Set(WStringView sData1, WStringView sData2)", asMETHODPR(WStringBuilder, Set, (WStringView, WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Set(WStringView sData1, WStringView sData2, WStringView sData3)", asMETHODPR(WStringBuilder, Set, (WStringView, WStringView, WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Set(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4)", asMETHODPR(WStringBuilder, Set, (WStringView, WStringView, WStringView, WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Set(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4, WStringView sData5, WStringView sData6 = \"\")", asMETHODPR(WStringBuilder, Set, (WStringView, WStringView, WStringView, WStringView, WStringView, WStringView), void), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetPath(WStringView sData1, WStringView sData2, WStringView sData3 = \"\", WStringView sData4 = \"\")", asMETHOD(WStringBuilder, SetPath), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Append(WStringView sData1)", asMETHODPR(WStringBuilder, Append, (WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Append(WStringView sData1, WStringView sData2)", asMETHODPR(WStringBuilder, Append, (WStringView, WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Append(WStringView sData1, WStringView sData2, WStringView sData3)", asMETHODPR(WStringBuilder, Append, (WStringView, WStringView, WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Append(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4)", asMETHODPR(WStringBuilder, Append, (WStringView, WStringView, WStringView, WStringView), void), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Append(WStringView sData1, WStringView sData2, WStringView sData3, WStringView sData4, WStringView sData5, WStringView sData6 = \"\")", asMETHODPR(WStringBuilder, Append, (WStringView, WStringView, WStringView, WStringView, WStringView, WStringView), void), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Prepend(WStringView sData1, WStringView sData2 = \"\", WStringView sData3 = \"\", WStringView sData4 = \"\", WStringView sData5 = \"\", WStringView sData6 = \"\")", asMETHODPR(WStringBuilder, Prepend, (WStringView, WStringView, WStringView, WStringView, WStringView, WStringView), void), asCALL_THISCALL));

    // SetFormat
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void SetFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7, ?&in VarArg8)", asFUNCTION(WStringBuilder_SetFormat), asCALL_GENERIC));

    // AppendFormat
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7, ?&in VarArg8)", asFUNCTION(WStringBuilder_AppendFormat), asCALL_GENERIC));

    // PrependFormat
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PrependFormat(WStringView sText, ?&in VarArg1, ?&in VarArg2, ?&in VarArg3, ?&in VarArg4, ?&in VarArg5, ?&in VarArg6, ?&in VarArg7, ?&in VarArg8)", asFUNCTION(WStringBuilder_PrependFormat), asCALL_GENERIC));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Shrink(WUInt32 uiShrinkCharsFront, WUInt32 uiShrinkCharsBack)", asMETHOD(WStringBuilder, Shrink), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void Reserve(WUInt32 uiNumElements)", asMETHOD(WStringBuilder, Reserve), asCALL_THISCALL));

    // TODO AngelScript: WStringBuilder::ReplaceFirst
    // TODO AngelScript: WStringBuilder::ReplaceLast

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "WUInt32 ReplaceAll(WStringView sSearchFor, WStringView sReplacement)", asMETHOD(WStringBuilder, ReplaceAll), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "WUInt32 ReplaceAll_NoCase(WStringView sSearchFor, WStringView sReplacement)", asMETHOD(WStringBuilder, ReplaceAll_NoCase), asCALL_THISCALL));

    // TODO AngelScript: WStringBuilder::ReplaceWholeWord
    // TODO AngelScript: WStringBuilder::ReplaceWholeWordAll

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void MakeCleanPath()", asMETHOD(WStringBuilder, MakeCleanPath), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void PathParentDirectory(WUInt32 uiLevelsUp = 1)", asMETHOD(WStringBuilder, PathParentDirectory), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendPath(WStringView sPath1, WStringView sPath2 = \"\", WStringView sPath3 = \"\", WStringView sPath4 = \"\")", asMETHOD(WStringBuilder, AppendPath), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void AppendWithSeparator(WStringView sSeparator, WStringView sData1, WStringView sData2 = \"\", WStringView sData3 = \"\", WStringView sData4 = \"\", WStringView sData5 = \"\", WStringView sData6 = \"\")", asMETHOD(WStringBuilder, AppendWithSeparator), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void ChangeFileName(WStringView sNewFileName)", asMETHOD(WStringBuilder, ChangeFileName), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void ChangeFileNameAndExtension(WStringView sNewFileNameWithExtension)", asMETHOD(WStringBuilder, ChangeFileNameAndExtension), asCALL_THISCALL));

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void ChangeFileExtension(WStringView sNewExtension, bool bFullExtension = false)", asMETHOD(WStringBuilder, ChangeFileExtension), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "void RemoveFileExtension(bool bFullExtension = false)", asMETHOD(WStringBuilder, RemoveFileExtension), asCALL_THISCALL));

    // TODO AngelScript: WStringBuilder::MakeRelativeTo

    // bool IsPathBelowFolder(const char* szPathToFolder)
    // void Trim(const char* szTrimChars = " \f\n\r\t\v")
    // TrimLeft, TrimRight

    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "bool TrimWordStart(WStringView sWord)", asMETHOD(WStringBuilder, TrimWordStart), asCALL_THISCALL));
    AS_CHECK(m_pEngine->RegisterObjectMethod("WStringBuilder", "bool TrimWordEnd(WStringView sWord)", asMETHOD(WStringBuilder, TrimWordEnd), asCALL_THISCALL));
  }
}

//////////////////////////////////////////////////////////////////////////
// WTempHashedString
//////////////////////////////////////////////////////////////////////////

static void WTempHashedString_Construct(void* pMemory)
{
  new (pMemory) WTempHashedString();
}

static void WTempHashedString_ConstructView(void* pMemory, WStringView sView)
{
  new (pMemory) WTempHashedString(sView);
}

static void WTempHashedString_ConstructTempHashed(void* pMemory, const WTempHashedString& sString)
{
  new (pMemory) WTempHashedString(sString);
}

static void WTempHashedString_ConstructHS(void* pMemory, const WHashedString& sString)
{
  new (pMemory) WTempHashedString(sString);
}

static void WTempHashedString_AssignStringView(WTempHashedString* pStr, WStringView sView)
{
  *pStr = sView;
}

static void WTempHashedString_AssignHS(WTempHashedString* pStr, const WHashedString& sString)
{
  *pStr = sString;
}

void WAngelScriptEngineSingleton::Register_TempHashedString()
{
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WTempHashedString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WTempHashedString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WTempHashedString", asBEHAVE_CONSTRUCT, "void f(const WTempHashedString& in)", asFUNCTION(WTempHashedString_ConstructTempHashed), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WTempHashedString", asBEHAVE_CONSTRUCT, "void f(const WStringView)", asFUNCTION(WTempHashedString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WTempHashedString", asBEHAVE_CONSTRUCT, "void f(const WHashedString& in)", asFUNCTION(WTempHashedString_ConstructHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTempHashedString", "void opAssign(WStringView)", asFUNCTION(WTempHashedString_AssignStringView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTempHashedString", "void opAssign(const WHashedString& in)", asFUNCTION(WTempHashedString_AssignHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTempHashedString", "bool opEquals(WTempHashedString) const", asMETHODPR(WTempHashedString, operator==, (const WTempHashedString&) const, bool), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WTempHashedString", "bool IsEmpty() const", asMETHOD(WTempHashedString, IsEmpty), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WTempHashedString", "void Clear()", asMETHOD(WTempHashedString, Clear), asCALL_THISCALL));
}

//////////////////////////////////////////////////////////////////////////
// WHashedString
//////////////////////////////////////////////////////////////////////////

static void WHashedString_Construct(void* pMemory)
{
  new (pMemory) WHashedString();
}

static void WHashedString_ConstructView(void* pMemory, WStringView sView)
{
  WHashedString* obj = new (pMemory) WHashedString();
  obj->Assign(sView);
}

static void WHashedString_ConstructHS(void* pMemory, const WHashedString& sString)
{
  new (pMemory) WHashedString(sString);
}

static void WHashedString_AssignStringView(WHashedString* pStr, const WStringView sView)
{
  pStr->Assign(sView);
}

static bool WHashedString_EqualsStringView(WHashedString* pStr, const WStringView sView)
{
  return *pStr == sView;
}

void WAngelScriptEngineSingleton::Register_HashedString()
{
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WHashedString", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(WHashedString_Construct), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WHashedString", asBEHAVE_CONSTRUCT, "void f(const WStringView)", asFUNCTION(WHashedString_ConstructView), asCALL_CDECL_OBJFIRST));
  AS_CHECK(m_pEngine->RegisterObjectBehaviour("WHashedString", asBEHAVE_CONSTRUCT, "void f(const WHashedString& in)", asFUNCTION(WHashedString_ConstructHS), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "void opAssign(const WStringView)", asFUNCTION(WHashedString_AssignStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "bool IsEmpty() const", asMETHOD(WHashedString, IsEmpty), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "void Clear()", asMETHOD(WHashedString, Clear), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "void Assign(const WStringView)", asMETHODPR(WHashedString, Assign, (WStringView), void), asCALL_THISCALL));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "bool opEquals(const WHashedString& in) const", asMETHODPR(WHashedString, operator==, (const WHashedString&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "bool opEquals(const WTempHashedString& in) const", asMETHODPR(WHashedString, operator==, (const WTempHashedString&) const, bool), asCALL_THISCALL));
  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "bool opEquals(const WStringView) const", asFUNCTION(WHashedString_EqualsStringView), asCALL_CDECL_OBJFIRST));

  AS_CHECK(m_pEngine->RegisterObjectMethod("WHashedString", "WStringView GetView() const", asMETHOD(WHashedString, GetView), asCALL_THISCALL));
}
