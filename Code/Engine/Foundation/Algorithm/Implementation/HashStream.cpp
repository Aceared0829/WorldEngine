#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashStream.h>

W_WARNING_PUSH()
W_WARNING_DISABLE_CLANG("-Wunused-function")

#define XXH_INLINE_ALL
#include <Foundation/ThirdParty/xxHash/xxhash.h>

W_WARNING_POP()

WHashStreamWriter32::WHashStreamWriter32(WUInt32 uiSeed)
{
  m_pState = XXH32_createState();
  W_VERIFY(XXH_OK == XXH32_reset((XXH32_state_t*)m_pState, uiSeed), "");
}

WHashStreamWriter32::~WHashStreamWriter32()
{
  XXH32_freeState((XXH32_state_t*)m_pState);
}

WResult WHashStreamWriter32::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return W_FAILURE;

  if (XXH_OK == XXH32_update((XXH32_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return W_SUCCESS;

  return W_FAILURE;
}

WUInt32 WHashStreamWriter32::GetHashValue() const
{
  return XXH32_digest((XXH32_state_t*)m_pState);
}


WHashStreamWriter64::WHashStreamWriter64(WUInt64 uiSeed)
{
  m_pState = XXH64_createState();
  W_VERIFY(XXH_OK == XXH64_reset((XXH64_state_t*)m_pState, uiSeed), "");
}

WHashStreamWriter64::~WHashStreamWriter64()
{
  XXH64_freeState((XXH64_state_t*)m_pState);
}

WResult WHashStreamWriter64::WriteBytes(const void* pWriteBuffer, WUInt64 uiBytesToWrite)
{
  if (uiBytesToWrite == 0)
    return W_SUCCESS;

  if (uiBytesToWrite > std::numeric_limits<size_t>::max())
    return W_FAILURE;

  if (XXH_OK == XXH64_update((XXH64_state_t*)m_pState, pWriteBuffer, static_cast<size_t>(uiBytesToWrite)))
    return W_SUCCESS;

  return W_FAILURE;
}

WUInt64 WHashStreamWriter64::GetHashValue() const
{
  return XXH64_digest((XXH64_state_t*)m_pState);
}
