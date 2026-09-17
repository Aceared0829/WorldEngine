#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

#  pragma once

inline lua_State* WLuaWrapper::GetLuaState()
{
  return m_pState;
}

inline WInt32 WLuaWrapper::ReturnToScript() const
{
  return (m_States.m_iParametersPushed);
}

inline WUInt32 WLuaWrapper::GetNumberOfFunctionParameters() const
{
  return ((int)lua_gettop(m_pState));
}

inline bool WLuaWrapper::IsParameterBool(WUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TBOOLEAN);
}

inline bool WLuaWrapper::IsParameterFloat(WUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool WLuaWrapper::IsParameterInt(WUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNUMBER);
}

inline bool WLuaWrapper::IsParameterString(WUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TSTRING);
}

inline bool WLuaWrapper::IsParameterNil(WUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TNIL);
}

inline bool WLuaWrapper::IsParameterTable(WUInt32 uiParameter) const
{
  return (lua_type(m_pState, uiParameter + s_iParamOffset) == LUA_TTABLE);
}

inline void WLuaWrapper::PushParameter(WInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushParameter(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushParameter(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushParameter(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushParameter(const char* szParameter, WUInt32 uiLength)
{
  lua_pushlstring(m_pState, szParameter, uiLength);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushParameterNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushReturnValue(WInt32 iParameter)
{
  lua_pushinteger(m_pState, iParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushReturnValue(bool bParameter)
{
  lua_pushboolean(m_pState, bParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushReturnValue(float fParameter)
{
  lua_pushnumber(m_pState, fParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushReturnValue(const char* szParameter)
{
  lua_pushstring(m_pState, szParameter);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushReturnValue(const char* szParameter, WUInt32 uiLength)
{
  lua_pushlstring(m_pState, szParameter, uiLength);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::PushReturnValueNil()
{
  lua_pushnil(m_pState);
  m_States.m_iParametersPushed++;
}

inline void WLuaWrapper::SetVariableNil(const char* szName) const
{
  lua_pushnil(m_pState);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void WLuaWrapper::SetVariable(const char* szName, WInt32 iValue) const
{
  lua_pushinteger(m_pState, iValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void WLuaWrapper::SetVariable(const char* szName, float fValue) const
{
  lua_pushnumber(m_pState, fValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void WLuaWrapper::SetVariable(const char* szName, bool bValue) const
{
  lua_pushboolean(m_pState, bValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void WLuaWrapper::SetVariable(const char* szName, const char* szValue) const
{
  lua_pushstring(m_pState, szValue);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void WLuaWrapper::SetVariable(const char* szName, const char* szValue, WUInt32 uiLen) const
{
  lua_pushlstring(m_pState, szValue, uiLen);

  if (m_States.m_iOpenTables == 0)
    lua_setglobal(m_pState, szName);
  else
    lua_setfield(m_pState, -2, szName);
}

inline void WLuaWrapper::PushTable(const char* szTableName, bool bGlobalTable)
{
  if (bGlobalTable || m_States.m_iOpenTables == 0)
    lua_getglobal(m_pState, szTableName);
  else
  {
    lua_pushstring(m_pState, szTableName);
    lua_gettable(m_pState, -2);
  }

  m_States.m_iParametersPushed++;
}

inline int WLuaWrapper::GetIntParameter(WUInt32 uiParameter) const
{
  return ((int)(lua_tointeger(m_pState, uiParameter + s_iParamOffset)));
}

inline bool WLuaWrapper::GetBoolParameter(WUInt32 uiParameter) const
{
  return (lua_toboolean(m_pState, uiParameter + s_iParamOffset) != 0);
}

inline float WLuaWrapper::GetFloatParameter(WUInt32 uiParameter) const
{
  return ((float)(lua_tonumber(m_pState, uiParameter + s_iParamOffset)));
}

inline const char* WLuaWrapper::GetStringParameter(WUInt32 uiParameter) const
{
  return (lua_tostring(m_pState, uiParameter + s_iParamOffset));
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
