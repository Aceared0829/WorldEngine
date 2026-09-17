
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class W_RENDERERFOUNDATION_DLL WGALResourceBase : public WRefCounted
{
public:
  void SetDebugName(const char* szName) const
  {
#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
    m_sDebugName.Assign(szName);
#endif

    SetDebugNamePlatform(szName);
  }

  virtual const WGALResourceBase* GetParentResource() const { return this; }

protected:
  friend class WGALDevice;

  inline ~WGALResourceBase() = default;

  virtual void SetDebugNamePlatform(const char* szName) const = 0;

#if W_ENABLED(W_COMPILE_FOR_DEVELOPMENT)
  mutable WHashedString m_sDebugName;
#endif
};

/// Base class for GAL resources, stores a creation description of the object and also allows for reference counting.
template <typename CreationDescription>
class WGALResource : public WGALResourceBase
{
public:
  W_ALWAYS_INLINE WGALResource(const CreationDescription& description)
    : m_Description(description)
  {
  }

  W_ALWAYS_INLINE const CreationDescription& GetDescription() const { return m_Description; }

protected:
  const CreationDescription m_Description;
};
