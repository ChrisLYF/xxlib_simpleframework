#pragma once
#include "xx_bytesutils.h"
namespace xx::Lua_BBuffer
{
	// todo: �޸� xx_luahelper.h
	// todo: lua_State ͨ���Զ��� alloc �ķ�ʽ�� mempool ָ�봫��
	// todo: ��������Щ�������� metatable, ӳ�䵽ȫ��, �� BBuffer. �ṩ new �� gc �ӿں���
	// todo: BBuffer.new() ��ͨ�� lua ��λ�� mp, �� lua ���� userdata �ռ�, ���� xx::BBuffer_v ����, �� BBuffer ���Ԫ��

	// todo: new, gc

	inline static int write_i8(lua_State* L)
	{
		return 0;
	}
	inline static int write_i16(lua_State* L)
	{
		return 0;
	}
	inline static int write_i32(lua_State* L)
	{
		return 0;
	}
	inline static int write_i64(lua_State* L)
	{
		return 0;
	}

	inline static int write_u8(lua_State* L)
	{
		return 0;
	}
	inline static int write_u16(lua_State* L)
	{
		return 0;
	}
	inline static int write_u32(lua_State* L)
	{
		return 0;
	}
	inline static int write_u64(lua_State* L)
	{
		return 0;
	}

	inline static int write_float(lua_State* L)
	{
		return 0;
	}
	inline static int write_double(lua_State* L)
	{
		return 0;
	}

	inline static int write_string(lua_State* L)
	{
		return 0;
	}

};
