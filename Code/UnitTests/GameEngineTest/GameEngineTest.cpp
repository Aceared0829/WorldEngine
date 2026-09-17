#include <GameEngineTest/GameEngineTestPCH.h>

#include <RendererCore/Textures/TextureUtils.h>
#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/TestSetup.h>

W_TESTFRAMEWORK_ENTRY_POINT_BEGIN("GameEngineTest", "GameEngine Tests")
{
  WTextureUtils::s_bForceFullQualityAlways = true; // never allow to use low-res textures
  WTestFramework::GetInstance()->SetTestTimeout(1000 * 60 * 20);
  WTestFramework::s_bCallstackOnAssert = true;
}
W_TESTFRAMEWORK_ENTRY_POINT_END()
