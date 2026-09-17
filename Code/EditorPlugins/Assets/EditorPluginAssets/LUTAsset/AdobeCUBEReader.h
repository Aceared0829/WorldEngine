
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Types/Status.h>

class WLogInterface;
class WStreamReader;

/// Simple implementation to read Adobe CUBE LUT files
///
/// Currently only reads 3D LUTs as this is the data we need for our lookup textures in the tone mapping step.
class WAdobeCUBEReader
{
public:
  WAdobeCUBEReader();
  ~WAdobeCUBEReader();

  WStatus ParseFile(WStreamReader& inout_stream, WLogInterface* pLog = nullptr);

  WVec3 GetDomainMin() const;
  WVec3 GetDomainMax() const;

  WUInt32 GetLUTSize() const;
  const WString& GetTitle() const;

  WVec3 GetLUTEntry(WUInt32 r, WUInt32 g, WUInt32 b) const;

protected:
  WUInt32 m_uiLUTSize = 0;
  WString m_sTitle = "<UNTITLED>";

  WVec3 m_vDomainMin = WVec3::MakeZero();
  WVec3 m_vDomainMax = WVec3(1.0f);

  WDynamicArray<WVec3> m_LUTValues;

  WUInt32 GetLUTIndex(WUInt32 r, WUInt32 g, WUInt32 b) const;
};
