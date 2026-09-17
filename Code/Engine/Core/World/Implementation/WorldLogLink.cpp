#include <Core/CorePCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldLogLink.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/ConversionUtils.h>

namespace
{
  /// The link markup can easily be longer than the temp buffer that BuildString() is given,
  /// so the result is written into a thread local buffer instead (same approach as BuildString()
  /// for WArgErrorCode). All arguments of one format string are built before any of them is used,
  /// so a small ring of buffers is needed instead of a single one.
  WStringView StoreInThreadLocalBuffer(const WStringBuilder& sText)
  {
    constexpr WUInt32 uiNumBuffers = WFormatString::MaxNumParameters;

    static thread_local WStringBuilder s_Buffers[uiNumBuffers];
    static thread_local WUInt32 s_uiNextBuffer = 0;

    WStringBuilder& sBuffer = s_Buffers[s_uiNextBuffer];
    s_uiNextBuffer = (s_uiNextBuffer + 1) % uiNumBuffers;

    sBuffer = sText;
    return sBuffer.GetView();
  }

  void AppendLink(WStringBuilder& out_sResult, WStringView sScheme, WUInt64 uiHandleData, WStringView sDisplayText, WStringView sFallbackName)
  {
    out_sResult.Append("[[");

    if (!sDisplayText.IsEmpty())
    {
      out_sResult.Append(sDisplayText);
    }
    else if (!sFallbackName.IsEmpty())
    {
      out_sResult.Append(sFallbackName);
    }
    else
    {
      out_sResult.AppendFormat("{}", WArgU(uiHandleData, 1, false, 16));
    }

    out_sResult.Append("|", sScheme);
    out_sResult.AppendFormat("{}", WArgU(uiHandleData, 1, false, 16));
    out_sResult.Append("]]");
  }

  bool ParseLink(WStringView sLinkTarget, WStringView sScheme, WUInt64& out_uiHandleData)
  {
    sLinkTarget.Trim();

    if (!sLinkTarget.TrimWordStart(sScheme))
      return false;

    return WConversionUtils::ConvertHexStringToUInt64(sLinkTarget, out_uiHandleData).Succeeded();
  }
} // namespace

WArgGameObject::WArgGameObject(const WGameObject* pObject, WStringView sDisplayText)
  : m_hObject(pObject != nullptr ? pObject->GetHandle() : WGameObjectHandle())
  , m_sDisplayText(sDisplayText)
{
}

WArgComponent::WArgComponent(const WComponent* pComponent, WStringView sDisplayText)
  : m_hComponent(pComponent != nullptr ? pComponent->GetHandle() : WComponentHandle())
  , m_sDisplayText(sDisplayText)
{
}

void WWorldLogLinkUtils::AppendGameObjectLink(WStringBuilder& out_sResult, WGameObjectHandle hObject, WStringView sDisplayText)
{
  WStringView sName;

  if (sDisplayText.IsEmpty() && !hObject.IsInvalidated())
  {
    if (const WWorld* pWorld = WWorld::GetWorld(hObject))
    {
      W_LOCK(pWorld->GetReadMarker());

      const WGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject))
      {
        sName = pObject->GetName();
      }
    }
  }

  AppendLink(out_sResult, s_sGameObjectScheme, hObject.GetInternalID().m_Data, sDisplayText, sName);
}

void WWorldLogLinkUtils::AppendComponentLink(WStringBuilder& out_sResult, WComponentHandle hComponent, WStringView sDisplayText)
{
  WStringBuilder sName;

  if (sDisplayText.IsEmpty() && !hComponent.IsInvalidated())
  {
    if (const WWorld* pWorld = WWorld::GetWorld(hComponent))
    {
      W_LOCK(pWorld->GetReadMarker());

      const WComponent* pComponent = nullptr;
      if (pWorld->TryGetComponent(hComponent, pComponent))
      {
        sName = pComponent->GetDynamicRTTI()->GetTypeName();

        if (const WGameObject* pOwner = pComponent->GetOwner(); pOwner != nullptr && !pOwner->GetName().IsEmpty())
        {
          sName.AppendFormat(" ({})", pOwner->GetName());
        }
      }
    }
  }

  AppendLink(out_sResult, s_sComponentScheme, hComponent.GetInternalID().m_Data, sDisplayText, sName);
}

bool WWorldLogLinkUtils::ParseGameObjectLink(WStringView sLinkTarget, WGameObjectHandle& out_hObject)
{
  WUInt64 uiData = 0;
  if (!ParseLink(sLinkTarget, s_sGameObjectScheme, uiData))
    return false;

  out_hObject = WGameObjectHandle(WGameObjectId(uiData));
  return true;
}

bool WWorldLogLinkUtils::ParseComponentLink(WStringView sLinkTarget, WComponentHandle& out_hComponent)
{
  WUInt64 uiData = 0;
  if (!ParseLink(sLinkTarget, s_sComponentScheme, uiData))
    return false;

  out_hComponent = WComponentHandle(WComponentId(uiData));
  return true;
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgGameObject& arg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);

  WStringBuilder sLink;
  WWorldLogLinkUtils::AppendGameObjectLink(sLink, arg.m_hObject, arg.m_sDisplayText);
  return StoreInThreadLocalBuffer(sLink);
}

WStringView BuildString(char* szTmp, WUInt32 uiLength, const WArgComponent& arg)
{
  W_IGNORE_UNUSED(szTmp);
  W_IGNORE_UNUSED(uiLength);

  WStringBuilder sLink;
  WWorldLogLinkUtils::AppendComponentLink(sLink, arg.m_hComponent, arg.m_sDisplayText);
  return StoreInThreadLocalBuffer(sLink);
}
