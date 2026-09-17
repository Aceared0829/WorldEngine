#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

class WOpenDdlReader;
class WOpenDdlWriter;
class WOpenDdlReaderElement;

namespace WOpenDdlUtils
{
  /// Converts the data that \a pElement points to to an WColor.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as WColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns W_FAILURE.
  W_FOUNDATION_DLL WResult ConvertToColor(const WOpenDdlReaderElement* pElement, WColor& out_result); // [tested]

  /// Converts the data that \a pElement points to to an WColorGammaUB.
  ///
  /// \a pElement may be a primitives list of 3 or 4 floats or of 3 or 4 unsigned int8 values.
  /// It may also be a group that contains such a primitives list as the only child.
  /// floats will be interpreted as linear colors, unsigned int 8 will be interpreted as WColorGammaUB.
  /// If only 3 values are given, alpha will be filled with 1.0f.
  /// If less than 3 or more than 4 values are given, the function returns W_FAILURE.
  W_FOUNDATION_DLL WResult ConvertToColorGamma(const WOpenDdlReaderElement* pElement, WColorGammaUB& out_result); // [tested]

  /// Converts the data that \a pElement points to to an WTime.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float or double.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToTime(const WOpenDdlReaderElement* pElement, WTime& out_result); // [tested]

  /// Converts the data that \a pElement points to to an WVec2.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec2(const WOpenDdlReaderElement* pElement, WVec2& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec3.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec3(const WOpenDdlReaderElement* pElement, WVec3& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec4.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec4(const WOpenDdlReaderElement* pElement, WVec4& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec2I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec2I(const WOpenDdlReaderElement* pElement, WVec2I32& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec3I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec3I(const WOpenDdlReaderElement* pElement, WVec3I32& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec4I32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 int32.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec4I(const WOpenDdlReaderElement* pElement, WVec4I32& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec2U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec2U(const WOpenDdlReaderElement* pElement, WVec2U32& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec3U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 3 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec3U(const WOpenDdlReaderElement* pElement, WVec3U32& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WVec4U32.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 uint32.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToVec4U(const WOpenDdlReaderElement* pElement, WVec4U32& out_vResult); // [tested]

  /// Converts the data that \a pElement points to to an WMat3.
  ///
  /// \a pElement maybe be a primitives list of exactly 9 floats.
  /// The elements are expected to be in column-major format. See WMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToMat3(const WOpenDdlReaderElement* pElement, WMat3& out_mResult); // [tested]

  /// Converts the data that \a pElement points to to an WMat4.
  ///
  /// \a pElement maybe be a primitives list of exactly 16 floats.
  /// The elements are expected to be in column-major format. See WMatrixLayout::ColumnMajor.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToMat4(const WOpenDdlReaderElement* pElement, WMat4& out_mResult); // [tested]

  /// Converts the data that \a pElement points to to an WTransform.
  ///
  /// \a pElement maybe be a primitives list of exactly 12 floats.
  /// The first 9 elements are expected to be a mat3 in column-major format. See WMatrixLayout::ColumnMajor.
  /// The last 3 elements are the position vector.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToTransform(const WOpenDdlReaderElement* pElement, WTransform& out_result); // [tested]

  /// Converts the data that \a pElement points to to an WQuat.
  ///
  /// \a pElement maybe be a primitives list of exactly 4 floats.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToQuat(const WOpenDdlReaderElement* pElement, WQuat& out_qResult); // [tested]

  /// Converts the data that \a pElement points to to an WUuid.
  ///
  /// \a pElement maybe be a primitives list of exactly 2 unsigned_int64.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToUuid(const WOpenDdlReaderElement* pElement, WUuid& out_result); // [tested]

  /// Converts the data that \a pElement points to to an WAngle.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 float.
  /// The value is assumed to be in radians.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToAngle(const WOpenDdlReaderElement* pElement, WAngle& out_result); // [tested]

  /// Converts the data that \a pElement points to to an WHashedString.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 string.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToHashedString(const WOpenDdlReaderElement* pElement, WHashedString& out_sResult); // [tested]

  /// Converts the data that \a pElement points to to an WTempHashedString.
  ///
  /// \a pElement maybe be a primitives list of exactly 1 uint64.
  /// It may also be a group that contains such a primitives list as the only child.
  W_FOUNDATION_DLL WResult ConvertToTempHashedString(const WOpenDdlReaderElement* pElement, WTempHashedString& out_sResult); // [tested]

  /// Uses the elements custom type name to infer which type the object holds and reads it into the WVariant.
  ///
  /// Depending on the custom type name, one of the other ConvertToXY functions is called and the respective conditions to the data format apply.
  /// Supported type names are: "Color", "ColorGamma", "Time", "Vec2", "Vec3", "Vec4", "Mat3", "Mat4", "Transform", "Quat", "Uuid", "Angle", "HashedString", "TempHashedString"
  /// Type names are case sensitive.
  W_FOUNDATION_DLL WResult ConvertToVariant(const WOpenDdlReaderElement* pElement, WVariant& out_result); // [tested]

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

  /// Writes an WColor to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreColor(WOpenDdlWriter& ref_writer, const WColor& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WColorGammaUB to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreColorGamma(WOpenDdlWriter& ref_writer, const WColorGammaUB& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WTime to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreTime(WOpenDdlWriter& ref_writer, const WTime& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec2 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec2(WOpenDdlWriter& ref_writer, const WVec2& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec3 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec3(WOpenDdlWriter& ref_writer, const WVec3& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec4 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec4(WOpenDdlWriter& ref_writer, const WVec4& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec2 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec2I(WOpenDdlWriter& ref_writer, const WVec2I32& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec3 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec3I(WOpenDdlWriter& ref_writer, const WVec3I32& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec4 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec4I(WOpenDdlWriter& ref_writer, const WVec4I32& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec2 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec2U(WOpenDdlWriter& ref_writer, const WVec2U32& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec3 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec3U(WOpenDdlWriter& ref_writer, const WVec3U32& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVec4 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVec4U(WOpenDdlWriter& ref_writer, const WVec4U32& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WMat3 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreMat3(WOpenDdlWriter& ref_writer, const WMat3& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WMat4 to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreMat4(WOpenDdlWriter& ref_writer, const WMat4& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WTransform to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreTransform(WOpenDdlWriter& ref_writer, const WTransform& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WQuat to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreQuat(WOpenDdlWriter& ref_writer, const WQuat& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WUuid to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreUuid(WOpenDdlWriter& ref_writer, const WUuid& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WAngle to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreAngle(WOpenDdlWriter& ref_writer, const WAngle& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WHashedString to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreHashedString(WOpenDdlWriter& ref_writer, const WHashedString& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WTempHashedString to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreTempHashedString(WOpenDdlWriter& ref_writer, const WTempHashedString& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an WVariant to DDL such that the type can be reconstructed.
  W_FOUNDATION_DLL void StoreVariant(WOpenDdlWriter& ref_writer, const WVariant& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single string and an optional name.
  W_FOUNDATION_DLL void StoreString(WOpenDdlWriter& ref_writer, const WStringView& value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreBool(WOpenDdlWriter& ref_writer, bool value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreFloat(WOpenDdlWriter& ref_writer, float value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreDouble(WOpenDdlWriter& ref_writer, double value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreInt8(WOpenDdlWriter& ref_writer, WInt8 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreInt16(WOpenDdlWriter& ref_writer, WInt16 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreInt32(WOpenDdlWriter& ref_writer, WInt32 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreInt64(WOpenDdlWriter& ref_writer, WInt64 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreUInt8(WOpenDdlWriter& ref_writer, WUInt8 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreUInt16(WOpenDdlWriter& ref_writer, WUInt16 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreUInt32(WOpenDdlWriter& ref_writer, WUInt32 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes a primitives list with a single value and an optional name.
  W_FOUNDATION_DLL void StoreUInt64(WOpenDdlWriter& ref_writer, WUInt64 value, WStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Writes an invalid variant and an optional name.
  W_FOUNDATION_DLL void StoreInvalid(WOpenDdlWriter& ref_writer, WStringView sName = {}, bool bGlobalName = false);
} // namespace WOpenDdlUtils
