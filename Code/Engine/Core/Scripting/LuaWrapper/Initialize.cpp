#include <Core/CorePCH.h>

#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

WLuaWrapper::WLuaWrapper()
{
  m_bReleaseOnExit = true;
  m_pState = nullptr;

  Clear();
}

WLuaWrapper::WLuaWrapper(lua_State* s)
{
  m_pState = s;
  m_bReleaseOnExit = false;
}

WLuaWrapper::~WLuaWrapper()
{
  if (m_bReleaseOnExit)
    lua_close(m_pState);
}

void WLuaWrapper::Clear()
{
  W_ASSERT_DEV(m_bReleaseOnExit, "Cannot clear a script that did not create the Lua state itself.");

  if (m_pState)
    lua_close(m_pState);

  m_pState = lua_newstate(lua_allocator, nullptr);

  luaL_openlibs(m_pState);
}

WResult WLuaWrapper::ExecuteString(const char* szString, const char* szDebugChunkName, WLogInterface* pLogInterface) const
{
  W_ASSERT_DEV(m_States.m_iLuaReturnValues == 0,
    "WLuaWrapper::ExecuteString: You didn't discard the return-values of the previous script call. {0} Return-values were expected.",
    m_States.m_iLuaReturnValues);

  if (!pLogInterface)
    pLogInterface = WLog::GetThreadLocalLogSystem();

  int error = luaL_loadbuffer(m_pState, szString, WStringUtils::GetStringElementCount(szString), szDebugChunkName);

  if (error != LUA_OK)
  {
    W_LOG_BLOCK("WLuaWrapper::ExecuteString");

    WLog::Error(pLogInterface, "[lua]Lua compile error: {0}", lua_tostring(m_pState, -1));
    WLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return W_FAILURE;
  }

  error = lua_pcall(m_pState, 0, 0, 0);

  if (error != LUA_OK)
  {
    W_LOG_BLOCK("WLuaWrapper::ExecuteString");

    WLog::Error(pLogInterface, "[lua]Lua error: {0}", lua_tostring(m_pState, -1));
    WLog::Info(pLogInterface, "[luascript]Script: {0}", szString);

    return W_FAILURE;
  }

  return W_SUCCESS;
}

void* WLuaWrapper::lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize)
{
  W_IGNORE_UNUSED(ud);

  /// \todo Create optimized allocator.

  if (nsize == 0)
  {
    delete[] (WUInt8*)ptr;
    return (nullptr);
  }

  WUInt8* ucPtr = new WUInt8[nsize];

  if (ptr != nullptr)
  {
    WMemoryUtils::Copy(ucPtr, (WUInt8*)ptr, WUInt32(osize < nsize ? osize : nsize));

    delete[] (WUInt8*)ptr;
  }

  return ((void*)ucPtr);
}


#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
