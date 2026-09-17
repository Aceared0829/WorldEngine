#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldLogLink.h>

W_CREATE_SIMPLE_TEST(World, WorldLogLink)
{
  WWorldDesc worldDesc("WorldLogLinkTest");
  WWorld world(worldDesc);
  W_LOCK(world.GetWriteMarker());

  WGameObjectDesc objDesc;
  objDesc.m_sName.Assign("LinkedObject");

  WGameObject* pObject = nullptr;
  const WGameObjectHandle hObject = world.CreateObject(objDesc, pObject);

  W_TEST_BLOCK(WTestBlock::Enabled, "Game object link")
  {
    WStringBuilder sMsg;
    sMsg.SetFormat("Object {} is broken.", WArgGameObject(hObject));

    // The display text falls back to the object's name.
    W_TEST_BOOL(sMsg.FindSubString("[[LinkedObject|WObject:") != nullptr);
    W_TEST_BOOL(sMsg.EndsWith("]] is broken."));

    WStringBuilder sExplicit;
    sExplicit.SetFormat("{}", WArgGameObject(pObject, "Custom Text"));
    W_TEST_BOOL(sExplicit.StartsWith("[[Custom Text|WObject:"));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Game object round trip")
  {
    WStringBuilder sMsg;
    sMsg.SetFormat("{}", WArgGameObject(hObject));

    const char* szSeparator = sMsg.FindSubString("|");
    W_TEST_BOOL(szSeparator != nullptr);

    const WStringView sTarget(szSeparator + 1, sMsg.GetView().GetEndPointer() - 2);

    WGameObjectHandle hParsed;
    W_TEST_BOOL(WWorldLogLinkUtils::ParseGameObjectLink(sTarget, hParsed));
    W_TEST_BOOL(hParsed == hObject);

    // A component link must not be accepted for a game object target and vice versa.
    WComponentHandle hWrong;
    W_TEST_BOOL(!WWorldLogLinkUtils::ParseComponentLink(sTarget, hWrong));
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Multiple links in one message")
  {
    WStringBuilder sMsg;
    sMsg.SetFormat("{} and {}", WArgGameObject(hObject, "First"), WArgGameObject(hObject, "Second"));

    // Both arguments are built before the message is assembled, so they must not overwrite each other.
    W_TEST_BOOL(sMsg.StartsWith("[[First|WObject:"));
    W_TEST_BOOL(sMsg.FindSubString("[[Second|WObject:") != nullptr);
  }

  W_TEST_BLOCK(WTestBlock::Enabled, "Invalid handles")
  {
    WStringBuilder sMsg;
    sMsg.SetFormat("{}", WArgGameObject(WGameObjectHandle()));

    // Nothing to derive a name from, so the handle value is used as the display text.
    W_TEST_BOOL(sMsg.StartsWith("[["));
    W_TEST_BOOL(sMsg.EndsWith("]]"));

    WGameObjectHandle hParsed;
    W_TEST_BOOL(!WWorldLogLinkUtils::ParseGameObjectLink("WObject:notahexnumber", hParsed));
    W_TEST_BOOL(!WWorldLogLinkUtils::ParseGameObjectLink("asset:{ 00000000-0000-0000-0000-000000000000 }", hParsed));

    // An invalid component handle must not be looked up either, its world index doesn't refer to a live world.
    WStringBuilder sComponentMsg;
    sComponentMsg.SetFormat("{}", WArgComponent(WComponentHandle()));
    W_TEST_BOOL(sComponentMsg.StartsWith("[["));
    W_TEST_BOOL(sComponentMsg.EndsWith("]]"));
  }
}
