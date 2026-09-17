#include <CoreTest/CoreTestPCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Utilities/GraphicsUtils.h>

W_CREATE_SIMPLE_TEST(World, Camera)
{
  W_TEST_BLOCK(WTestBlock::Enabled, "LookAt")
  {
    WCamera camera;

    camera.LookAt(WVec3(0, 0, 0), WVec3(1, 0, 0), WVec3(0, 0, 1));
    W_TEST_VEC3(camera.GetPosition(), WVec3(0, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(1, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(0, 1, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(0, 0, 1), WMath::DefaultEpsilon<float>());

    camera.LookAt(WVec3(0, 0, 0), WVec3(-1, 0, 0), WVec3(0, 0, 1));
    W_TEST_VEC3(camera.GetPosition(), WVec3(0, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(-1, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(0, -1, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(0, 0, 1), WMath::DefaultEpsilon<float>());

    camera.LookAt(WVec3(0, 0, 0), WVec3(0, 0, 1), WVec3(0, 1, 0));
    W_TEST_VEC3(camera.GetPosition(), WVec3(0, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(0, 0, 1), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(1, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(0, 1, 0), WMath::DefaultEpsilon<float>());

    camera.LookAt(WVec3(0, 0, 0), WVec3(0, 0, -1), WVec3(0, 1, 0));
    W_TEST_VEC3(camera.GetPosition(), WVec3(0, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(0, 0, -1), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(-1, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(0, 1, 0), WMath::DefaultEpsilon<float>());

    const WMat4 mLookAt = WGraphicsUtils::CreateLookAtViewMatrix(WVec3(2, 3, 4), WVec3(3, 3, 4), WVec3(0, 0, 1), WHandedness::LeftHanded);
    camera.SetViewMatrix(mLookAt);

    W_TEST_VEC3(camera.GetPosition(), WVec3(2, 3, 4), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(1, 0, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(0, 1, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(0, 0, 1), WMath::DefaultEpsilon<float>());

    // look at with dir == up vector
    camera.LookAt(WVec3(2, 3, 4), WVec3(2, 3, 5), WVec3(0, 0, 1));
    W_TEST_VEC3(camera.GetPosition(), WVec3(2, 3, 4), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(0, 0, 1), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(0, 1, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(-1, 0, 0), WMath::DefaultEpsilon<float>());

    camera.LookAt(WVec3(2, 3, 4), WVec3(2, 3, 3), WVec3(0, 0, 1));
    W_TEST_VEC3(camera.GetPosition(), WVec3(2, 3, 4), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirForwards(), WVec3(0, 0, -1), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirRight(), WVec3(0, 1, 0), WMath::DefaultEpsilon<float>());
    W_TEST_VEC3(camera.GetDirUp(), WVec3(1, 0, 0), WMath::DefaultEpsilon<float>());
  }
}
