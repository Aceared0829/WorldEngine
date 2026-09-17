#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>

WDoubleBufferedLinearAllocator::WDoubleBufferedLinearAllocator(WStringView sName0, WAllocator* pParent)
{
  constexpr WUInt32 uiInitialSize = 1024 * 1024; // 1 MB

  WStringBuilder sName = sName0;
  sName.Append("0");

  m_pCurrentAllocator = W_DEFAULT_NEW(LinearAllocatorType, sName, pParent, uiInitialSize);

  sName = sName0;
  sName.Append("1");

  m_pOtherAllocator = W_DEFAULT_NEW(LinearAllocatorType, sName, pParent, uiInitialSize);
}

WDoubleBufferedLinearAllocator::~WDoubleBufferedLinearAllocator()
{
  W_DEFAULT_DELETE(m_pCurrentAllocator);
  W_DEFAULT_DELETE(m_pOtherAllocator);
}

void WDoubleBufferedLinearAllocator::Swap()
{
  WMath::Swap(m_pCurrentAllocator, m_pOtherAllocator);

  m_pCurrentAllocator->Reset();
}

void WDoubleBufferedLinearAllocator::Reset()
{
  m_pCurrentAllocator->Reset();
  m_pOtherAllocator->Reset();
}


// clang-format off
W_BEGIN_SUBSYSTEM_DECLARATION(Foundation, FrameAllocator)

  ON_CORESYSTEMS_STARTUP
  {
    WFrameAllocator::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    WFrameAllocator::Shutdown();
  }

W_END_SUBSYSTEM_DECLARATION;
// clang-format on

WDoubleBufferedLinearAllocator* WFrameAllocator::s_pAllocator;

// static
void WFrameAllocator::Swap()
{
  W_PROFILE_SCOPE("FrameAllocator.Swap");

  s_pAllocator->Swap();
}

// static
void WFrameAllocator::Reset()
{
  if (s_pAllocator)
  {
    s_pAllocator->Reset();
  }
}

// static
void WFrameAllocator::Startup()
{
  s_pAllocator = W_DEFAULT_NEW(WDoubleBufferedLinearAllocator, "FrameAllocator", WFoundation::GetAlignedAllocator());
}

// static
void WFrameAllocator::Shutdown()
{
  W_DEFAULT_DELETE(s_pAllocator);
}

W_STATICLINK_FILE(Foundation, Foundation_Memory_Implementation_FrameAllocator);
