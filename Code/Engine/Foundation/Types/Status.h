#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Strings/StringBuilder.h>

class WLogInterface;

/// An WResult with an additional message for the reason of failure
struct [[nodiscard]] W_FOUNDATION_DLL WStatus
{
  /// Sets the status to W_FAILURE and stores the error message.
  explicit WStatus(const char* szError)
    : m_Result(W_FAILURE)
    , m_sMessage(szError)
  {
  }

  /// Sets the status to W_FAILURE and stores the error message.
  explicit WStatus(WStringView sError)
    : m_Result(W_FAILURE)
    , m_sMessage(sError)
  {
  }

  /// Sets the status, but doesn't store a message string.
  W_ALWAYS_INLINE WStatus(WResult r)
    : m_Result(r)
  {
  }

  /// Sets the status, but doesn't store a message string.
  W_ALWAYS_INLINE WStatus(WResultEnum r)
    : m_Result(r)
  {
  }

  /// Sets the status to W_FAILURE and stores the error message. Can be used with WFmt().
  explicit WStatus(const WFormatString& fmt);

  [[nodiscard]] WResult GetResult() const { return m_Result; }

  [[nodiscard]] W_ALWAYS_INLINE bool Succeeded() const { return m_Result.Succeeded(); }
  [[nodiscard]] W_ALWAYS_INLINE bool Failed() const { return m_Result.Failed(); }

  /// Used to silence compiler warnings, when success or failure doesn't matter.
  W_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// If the state is W_FAILURE, the message is written to the given log (or the currently active thread-local log).
  ///
  /// The return value is the same as 'Failed()' but isn't marked as [[nodiscard]], ie returns true, if a failure happened,
  /// so can be used in a conditional.
  bool LogFailure(WLogInterface* pLog = nullptr) const;

  /// Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a szMsg is given, this will be the assert message.
  /// Additionally m_sMessage will be included as a detailed message.
  void AssertSuccess(const char* szMsg = nullptr) const;

  [[nodiscard]] const WString& GetMessageString() const { return m_sMessage; }

private:
  WResult m_Result;
  WString m_sMessage;
};

W_ALWAYS_INLINE WResult WToResult(const WStatus& result)
{
  return result.GetResult();
}
