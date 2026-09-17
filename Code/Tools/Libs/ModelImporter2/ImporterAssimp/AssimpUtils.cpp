#include <ModelImporter2/ModelImporterPCH.h>

#include <assimp/matrix4x4.h>
#include <assimp/quaternion.h>
#include <assimp/types.h>
#include <assimp/vector3.h>

namespace WModelImporter2
{
  WColor ConvertAssimpType(const aiColor4D& value, bool bInvert /*= false*/)
  {
    if (bInvert)
      return WColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b, 1.0f - value.a);
    else
      return WColor(value.r, value.g, value.b, value.a);
  }

  WColor ConvertAssimpType(const aiColor3D& value, bool bInvert /*= false*/)
  {
    if (bInvert)
      return WColor(1.0f - value.r, 1.0f - value.g, 1.0f - value.b);
    else
      return WColor(value.r, value.g, value.b);
  }

  WMat4 ConvertAssimpType(const aiMatrix4x4& value, bool bDummy /*= false*/)
  {
    W_ASSERT_DEBUG(!bDummy, "not implemented");

    return WMat4::MakeFromRowMajorArray(&value.a1);
  }

  WVec3 ConvertAssimpType(const aiVector3D& value, bool bDummy /*= false*/)
  {
    W_ASSERT_DEBUG(!bDummy, "not implemented");

    return WVec3(value.x, value.y, value.z);
  }

  WQuat ConvertAssimpType(const aiQuaternion& value, bool bDummy /*= false*/)
  {
    W_ASSERT_DEBUG(!bDummy, "not implemented");

    return WQuat(value.x, value.y, value.z, value.w);
  }

  float ConvertAssimpType(float value, bool bDummy /*= false*/)
  {
    W_ASSERT_DEBUG(!bDummy, "not implemented");

    return value;
  }

  int ConvertAssimpType(int value, bool bDummy /*= false*/)
  {
    W_ASSERT_DEBUG(!bDummy, "not implemented");

    return value;
  }

} // namespace WModelImporter2
