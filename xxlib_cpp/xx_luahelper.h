﻿#pragma once
#include <tuple>
#include <type_traits>
#include <utility>
#include <array>
#include <initializer_list>
#include "xx_mempool.h"
#include "xx_dict.h"
#include "xx_bbuffer.h"
#include "lua.hpp"

#include <iostream>

namespace xx
{
	// todo: 当前这个还用不了. 要去掉之前的 MP 模板参数设计




	// todo: 排查所有 lua 函数的异常处理( lua 内部可能会使用 long jump 直接跳出函数, 导致 RAII 失效之类 )

	// todo: Lua_BindFunc 之非类成员函数版. 考虑为 Lua_SetGlobal 增加函数指针重载
	// todo: 考虑一下结构体中的子结构体的 lua userdata 包装( 就是说 ud 的说明部分可能需要移到 uservalue 中去, 或是用 uservalue 来延长 container 的生命周期? )


	// 可放入 LUA 的数据类型有: float, double, int64, 各式 string, 以及 T, T*
	// 其中 T 又分为 一般结构体 以及 MPtr<T> ( T 为 MPObject 派生类 )
	// T* 分为一般指针 或 MPObject* 派生类指针
	// String* 空指针于 lua 中当前用 nil 来表达
	// 不支持复杂结构体创建为 ud( 可以有构造函数但不要有析构函数. 最好就是个 pod )
	// 只支持单继承





	/************************************************************************************/
	// lua 值 序列化 相关
	/************************************************************************************/

	// 序列化 lua 值支持的数据类型
	enum class LuaTypes : uint8_t
	{
		Nil, True, False, Integer, Double, String, Bytes, Table, TableEnd
	};

	// 序列化 idx 处的 lua 值( 针对 String / Userdata / Table, 用 pd 来去重 )( idx 须为绝对值 ). 返回非 0 就是出错. 遇到了不能处理的数据类型.
	inline int Lua_ToBBufferCore(xx::Dict<void*, uint32_t>& pd, xx::BBuffer& bb, lua_State* const& L, int idx)
	{
		assert(idx > 0);
		switch (auto t = lua_type(L, idx))
		{
		case LUA_TNIL:
			bb.Write(LuaTypes::Nil);
			return 0;

		case LUA_TBOOLEAN:
			bb.Write(lua_toboolean(L, idx) ? LuaTypes::True : LuaTypes::False);
			return 0;

		case LUA_TNUMBER:
			if (lua_isinteger(L, idx)) bb.Write(LuaTypes::Integer, lua_tointeger(L, idx));
			else bb.Write(LuaTypes::Double, lua_tonumber(L, idx));
			return 0;

		case LUA_TSTRING:
		{
			bb.Write(LuaTypes::String);

			size_t len;
			auto ptr = lua_tolstring(L, idx, &len);
			auto rtv = pd.Add((void*)ptr, bb.dataLen);
			bb.Write(pd.ValueAt(rtv.index));
			if (!rtv.success) return 0;

			bb.Write((uint32_t)len);
			bb.WriteBuf(ptr, (uint32_t)len);
			return 0;
		}
		case LUA_TUSERDATA:
		{
			bb.Write(LuaTypes::Bytes);

			auto ptr = lua_touserdata(L, idx);
			auto rtv = pd.Add((void*)ptr, bb.dataLen);
			bb.Write(pd.ValueAt(rtv.index));
			if (!rtv.success) return 0;

			auto len = (uint32_t)lua_rawlen(L, idx);
			bb.Write(len);
			bb.WriteBuf((char*)ptr, len);
			return 0;
		}
		case LUA_TTABLE:
		{
			bb.Write(LuaTypes::Table);

			auto ptr = lua_topointer(L, idx);
			auto rtv = pd.Add((void*)ptr, bb.dataLen);
			bb.Write(pd.ValueAt(rtv.index));
			if (!rtv.success) return 0;

			lua_pushnil(L);								// ... t, nil
			while (lua_next(L, idx) != 0)				// ... t, k, v
			{
				if (auto r = Lua_ToBBufferCore(pd, bb, L, idx + 1)) return r;
				if (auto r = Lua_ToBBufferCore(pd, bb, L, idx + 2)) return r;

				lua_pop(L, 1);							// ... t, k
			}											// ... t
			bb.Write(LuaTypes::TableEnd);
			return 0;
		}
		default:
			return t;
		}
	}

	inline int Lua_ToBBuffer(xx::Dict<void*, uint32_t>& pd, xx::BBuffer& bb, lua_State* const& L, int idx)
	{
		pd.Clear();
		auto countPos = bb.WriteSpace(2);				// 留个写引用类型个数的空间
		auto rtv = xx::Lua_ToBBufferCore(pd, bb, L, idx);
		if (pd.Count() > 65536) return -65536;
		bb.WriteAt(countPos, (uint16_t)pd.Count());		// 写入引用类型个数
		return rtv;
	}



	// 反序列化并 push 1 个 lua 值( 通过 bb 填充, 遇到 String / Userdata / Table 就通过 od 去查 idx ). 返回非 0 就是出错. 
	// top 负空间 用于放置所有出现过的引用数据的副本. 故调该函数前, top 应传入 settop( gettop(L) + 引用数据个数 ) 后的 top 值
	// 压入的值将出现在栈顶. 故还需要自己将其 replace 到最初 top 值, 再 settop( 最初 top 值 )
	inline int Lua_PushFromBBuffer(xx::Dict<uint32_t, std::pair<int, LuaTypes>>& od, xx::BBuffer& bb, lua_State* const& L, int const& top, int const& numRefVals, int& index /* = 0*/)
	{
		LuaTypes lt;
		if (auto rtv = bb.Read(lt)) return rtv;

		switch (lt)
		{
		case LuaTypes::Nil:
			lua_pushnil(L);
			return 0;
		case LuaTypes::True:
			lua_pushboolean(L, 1);
			return 0;
		case LuaTypes::False:
			lua_pushboolean(L, 0);
			return 0;
		case LuaTypes::Integer:
		{
			lua_Integer i;
			if (auto rtv = bb.Read(i)) return rtv;
			lua_pushinteger(L, i);
			return 0;
		}
		case LuaTypes::Double:
		{
			lua_Number d;
			if (auto rtv = bb.Read(d)) return rtv;
			lua_pushnumber(L, d);
			return 0;
		}
		case LuaTypes::String:
		{
			uint32_t ptr_offset = 0, bb_offset_bak = bb.offset;
			if (auto rtv = bb.Read(ptr_offset)) return rtv;
			if (ptr_offset == bb_offset_bak)
			{
				if (index >= numRefVals) return -999;
				if (!od.Add(ptr_offset, std::make_pair(top - index, lt)).success) return -1;
				uint32_t len;
				if (auto rtv = bb.Read(len)) return rtv;
				if (len > bb.dataLen - bb.offset) return -2;
				lua_pushlstring(L, bb.buf + bb.offset, len);
				lua_copy(L, -1, top - index);					// 复制到引用存储区
				++index;
				bb.offset += len;
				return 0;
			}
			else
			{
				std::pair<int, LuaTypes> v;
				if (!od.TryGetValue(ptr_offset, v)) return -3;
				if (v.second != lt) return -4;
				lua_pushvalue(L, v.first);
				return 0;
			}
		}
		case LuaTypes::Bytes:
		{
			uint32_t ptr_offset = 0, bb_offset_bak = bb.offset;
			if (auto rtv = bb.Read(ptr_offset)) return rtv;
			if (ptr_offset == bb_offset_bak)
			{
				if (index >= numRefVals) return -999;
				if (!od.Add(ptr_offset, std::make_pair(top - index, lt)).success) return -5;
				uint32_t len;
				if (auto rtv = bb.Read(len)) return rtv;
				if (len > bb.dataLen - bb.offset) return -6;
				auto ptr = lua_newuserdata(L, len);
				memcpy(ptr, bb.buf + bb.offset, len);
				lua_copy(L, -1, top - index);					// 复制到引用存储区
				++index;
				bb.offset += len;
				return 0;
			}
			else
			{
				std::pair<int, LuaTypes> v;
				if (!od.TryGetValue(ptr_offset, v)) return -7;
				if (v.second != lt) return -8;
				lua_pushvalue(L, v.first);
				return 0;
			}
		}
		case LuaTypes::Table:
		{
			uint32_t ptr_offset = 0, bb_offset_bak = bb.offset;
			if (auto rtv = bb.Read(ptr_offset)) return rtv;
			if (ptr_offset == bb_offset_bak)
			{
				if (index >= numRefVals) return -999;
				if (!od.Add(ptr_offset, std::make_pair(top - index, lt)).success) return -9;
				if (bb.offset == bb.dataLen) return -10;
				lua_newtable(L);
				lua_copy(L, -1, top - index);					// 复制到引用存储区
				++index;
				while (true)
				{
					if (bb.offset == bb.dataLen) return -10;
					if (bb[bb.offset] == std::underlying_type_t<LuaTypes>(LuaTypes::TableEnd))
					{
						++bb.offset;
						break;
					}

					if (auto r = Lua_PushFromBBuffer(od, bb, L, top, numRefVals, index)) return r;	// key
					if (auto r = Lua_PushFromBBuffer(od, bb, L, top, numRefVals, index)) return r;	// value
					lua_rawset(L, -3);
				}
				return 0;
			}
			else
			{
				std::pair<int, LuaTypes> v;
				if (!od.TryGetValue(ptr_offset, v)) return -11;
				if (v.second != lt) return -12;
				lua_pushvalue(L, v.first);
				return 0;
			}
		}
		default:
			return -13;
		}
	}

	inline int Lua_PushFromBBuffer(xx::Dict<uint32_t, std::pair<int, LuaTypes>>& od, xx::BBuffer& bb, lua_State* const& L)
	{
		uint16_t refValsCount = 0;
		if (auto r = bb.Read(refValsCount)) return r;	// 读出 引用值个数
		auto topbak = lua_gettop(L);					// 记录原始 top
		lua_settop(L, topbak + refValsCount);			// 空出引用类型存放位
		int index = 0;
		od.Clear();
		auto rtv = xx::Lua_PushFromBBuffer(od, bb, L, topbak + refValsCount, refValsCount, index);
		if (rtv)
		{
			lua_settop(L, topbak);						// 还原栈顶
			return rtv;
		}
		else
		{
			lua_replace(L, topbak + 1);					// 将还原出来的数据放到理论栈顶
			lua_settop(L, topbak + 1);					// 修正栈顶
			return 0;
		}
	}





	/************************************************************************************/
	// Lua_GetMainThread
	/************************************************************************************/

	inline lua_State* Lua_GetMainThread(lua_State* L)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
		auto rtv = lua_tothread(L, -1);
		lua_pop(L, 1);
		return rtv;
	}


	/************************************************************************************/
	// Lua_GetMainThread
	/************************************************************************************/

	// 获取内存池. 仅限于用 Lua_NewState 创建的 L 可用
	template<typename MP>
	MP& Lua_GetMemPool(lua_State* L)
	{
		MP* mp;
		lua_getallocf(L, (void**)&mp);
		return *mp;
	}



	/************************************************************************************/
	// Lua_CloneParentMetatables
	/************************************************************************************/

	// 针对所有 mt, 级联复制父的元素到自身, 避免逐级向上查找( 每多一级查询似乎就会慢 1/5, 一直叠加 )
	template<typename MP>
	inline void Lua_CloneParentMetatables(MP& mp, lua_State* L)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);	// _G
		for (int tidx = MP::typesSize - 1; tidx >= 0; --tidx)
		{
			auto pidx = mp.pids[tidx];
			if (pidx == tidx) continue;
			lua_rawgeti(L, -1, tidx);							// _G, mt
			lua_rawgeti(L, -2, pidx);							// _G, mt, pmt
			lua_pushnil(L);										// _G, mt, pmt, nil
			while (lua_next(L, -2) != 0)						// _G, mt, pmt, k, v
			{
				lua_pushvalue(L, -2);							// _G, mt, pmt, k, v, k
				auto dt = lua_rawget(L, -5);					// _G, mt, pmt, k, v, ?
				if (dt == LUA_TNIL)
				{
					lua_pop(L, 1);								// _G, mt, pmt, k, v
					lua_pushvalue(L, -2);						// _G, mt, pmt, k, v, k
					lua_insert(L, -2);							// _G, mt, pmt, k, k, v
					lua_rawset(L, -5);							// _G, mt, pmt, k
				}
				else
				{
					lua_pop(L, 2);								// _G, mt, pmt, k
				}
			}													// _G, mt, pmt
			lua_pop(L, 2);										// _G
		}
		lua_pop(L, 1);											//
	}




	/************************************************************************************/
	// Lua_RegisterCoroutine
	/************************************************************************************/

	// 为状态机( 基类是 MPObject )建协程放至 LUA_REGISTRYINDEX 并返回指针
	// 主要供状态机构造函数处调用. o 传入 this
	inline lua_State* Lua_RegisterCoroutine(lua_State* L, void* key)
	{
		auto co = lua_newthread(L);							// key, co
		lua_rawsetp(L, LUA_REGISTRYINDEX, key);				//
		return co;
	}



	/************************************************************************************/
	// Lua_ReleaseCoroutine
	/************************************************************************************/

	// 移除一个协程 ( 位于 LUA_REGISTRYINDEX 表 )
	inline void Lua_UnregisterCoroutine(lua_State* L, void* key)
	{
		if (!L) return;										// 如果是 scene 析构( L 已死 )就会导致这个情况
		lua_pushnil(L);
		lua_rawsetp(L, LUA_REGISTRYINDEX, key);
	}


	/************************************************************************************/
	// Lua_Resume
	/************************************************************************************/

	inline int Lua_Resume(lua_State* co, xx::String* err, int narg = 0)
	{
		assert(co);
		int status = lua_resume(co, nullptr, narg);
		if (status == LUA_YIELD)
		{
			return 0;	// todo: 暂存函数的返回值? 
		}
		else if (status == LUA_ERRRUN && lua_isstring(co, -1))
		{
			if (err) err->Assign(lua_tostring(co, -1));
			lua_pop(co, -1);
			return -1;
		}
		else
		{
			return 1;	// LUA_OK
		}
	}

	/************************************************************************************/
	// LuaError
	/************************************************************************************/

	// 简化出错函数调用. 用法: return LuaError("...");
	inline int Lua_Error(lua_State* L, char const* errmsg)
	{
		lua_pushstring(L, errmsg);
		return lua_error(L);
	}



	/************************************************************************************/
	// Lua_PushMetatable
	/************************************************************************************/

	// 从 _G[ TypeId ] 取出元表压入栈顶
	template<typename MP, typename T>
	void Lua_PushMetatable(lua_State* L)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);			// _G
		lua_rawgeti(L, -1, TypeId<T>::value);							// _G, mt
		lua_replace(L, -2);												// mt
	}



	/************************************************************************************/
	// Lua_UD
	/************************************************************************************/
	// auto ud = (Lua_UD*)lua_touserdata(L, -1);
	// auto t = (T*)(ud + 1)

	// 压入 lua 的 userdata 的数据形态
	enum class Lua_UDTypes : uint8_t
	{
		Pointer,					// 指针( 可能是 MPObject 派生 )
		MPtr,						// MPtr<T>( 必然是 MPObject 派生 )
		Struct						// 结构体( 一定不是 MPObject 派生 )
	};

	// 压入 lua 的 userdata 的数据头. 应该是占 8 字节. 后面紧跟数据区
	template<typename T>
	struct Lua_UD
	{
		int typeIndex;				// MP 中的类型索引( 主用于判断继承关系, 做 dynamic_cast )
		Lua_UDTypes udType;			// 数据形态
		bool isMPObject;			// 是否从 MPObject 派生( 作为指针数据形态的一个补充说明 )
		T data;
	};


	/************************************************************************************/
	// LuaFunc<T, cond>
	/************************************************************************************/

	template<typename MP, typename T, typename ENABLE = void>
	struct LuaFunc
	{
		// 将 v 压入栈顶( L 得是根 )
		static void Push(lua_State* L, T const& v);

		// 从 idx 读出数据填到 v
		static void To(lua_State* L, T& v, int idx);

		// 试从 idx 读出数据填到 v. 成功返回 true
		static bool TryTo(lua_State* L, T& v, int idx);
	};

	// string
	template<typename MP>
	struct LuaFunc<MP, std::string, void>
	{
		static inline void Push(lua_State* L, std::string const& v)
		{
			lua_pushlstring(L, v.data(), v.size());
		}
		static inline void To(lua_State* L, std::string& v, int idx)
		{
			size_t len;
			auto s = lua_tolstring(L, idx, &len);
			v.assign(s, len);
		}
		static inline bool TryTo(lua_State* L, std::string& v, int idx)
		{
			if (!lua_isstring(L, idx)) return false;
			To(L, v, idx);
			return true;
		}
	};

	// xx::String*
	template<typename MP>
	struct LuaFunc<MP, String*, void>
	{
		static inline void Push(lua_State* L, String* const& v)
		{
			if (v) lua_pushlstring(L, v->buf, v->dataLen);
			else lua_pushnil(L);
		}
		static inline void To(lua_State* L, String*& v, int idx)
		{
			if (v) v->Release();
			if (lua_isnil(L, idx))
			{
				v = nullptr;
				return;
			}
			size_t len;
			auto s = lua_tolstring(L, idx, &len);
			Lua_GetMemPool<MP>(L).CreateTo(v, s, (uint32_t)len);
		}
		static inline bool TryTo(lua_State* L, String*& v, int idx)
		{
			if (!lua_isstring(L, idx) && !lua_isnil(L, idx)) return false;
			To(L, v, idx);
			return true;
		}
	};

	// char*
	template<typename MP>
	struct LuaFunc<MP, char const*, void>
	{
		static inline void Push(lua_State* L, char const* const& v)
		{
			if (v) lua_pushstring(L, v);
			else lua_pushnil(L);
		}
		static inline void To(lua_State* L, char const*& v, int idx)
		{
			if (lua_isnil(L, idx)) v = nullptr;
			else v = lua_tostring(L, idx);
		}
		static inline bool TryTo(lua_State* L, char const*& v, int idx)
		{
			if (!lua_isstring(L, idx) && !lua_isnil(L, idx)) return false;
			To(L, v, idx);
			return true;
		}
	};

	// fp
	template<typename MP, typename T>
	struct LuaFunc<MP, T, std::enable_if_t<std::is_floating_point<T>::value>>
	{
		static inline void Push(lua_State* L, T const& v)
		{
			lua_pushnumber(L, (lua_Number)v);
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			v = (T)lua_tonumber(L, idx);
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (!lua_isnumber(L, idx)) return false;
			To(L, v, idx);
			return true;
		}
	};

	// int
	template<typename MP, typename T>
	struct LuaFunc<MP, T, std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
	{
		static inline void Push(lua_State* L, T const& v)
		{
			lua_pushinteger(L, (lua_Integer)v);
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			v = (T)lua_tointeger(L, idx);
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (!lua_isinteger(L, idx)) return false;
			To(L, v, idx);
			return true;
		}
	};

	// todo: enum ?

	// T*
	template<typename MP, typename T>
	struct LuaFunc<MP, T, std::enable_if_t<std::is_pointer<T>::value>>
	{
		typedef std::remove_pointer_t<T> TT;

		static inline void Push(lua_State* L, T const& v)
		{
			auto ud = (Lua_UD<T>*)lua_newuserdata(L, sizeof(Lua_UD<T>));	// ud
			if (std::is_base_of<MPObject, TT>::value && v)
			{
				ud->typeIndex = (decltype(ud->typeIndex))((MPObject*)v)->typeId();
			}
			else
			{
				ud->typeIndex = TupleIndexOf_v<TT, typename MP::Tuple>;
			}
			ud->udType = Lua_UDTypes::Pointer;
			ud->isMPObject = IsMPObject<TT>::value;
			new (&ud->data) T(v);
			Lua_PushMetatable<MP, TT>(L);									// ud, mt
			lua_setmetatable(L, -2);										// ud
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			if (lua_isnil(L, idx)) v = nullptr;
			else v = ((Lua_UD<T>*)lua_touserdata(L, idx))->data;
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (lua_isnil(L, idx))
			{
				v = nullptr;
				return true;
			}
			if (!lua_isuserdata(L, idx)) return false;
			auto ud = (Lua_UD<T>*)lua_touserdata(L, idx);
			auto tid = TypeId<TT>::value;
			if (!Lua_GetMemPool<MP>(L).IsBaseOf(tid, ud->typeIndex)) return false;
			switch (ud->udType)
			{
			case Lua_UDTypes::Pointer:
				v = ud->data;
				return true;
			case Lua_UDTypes::MPtr:
				v = ((MPtr<TT>*)&ud->data)->Ensure();
				return true;
			case Lua_UDTypes::Struct:
				v = (TT*)&ud->data;			// 理论上讲这个值是危险的. 如果被 lua 回收就没了. 需要立即使用
				return true;
			}
			return false;
		}
	};

	// T
	template<typename MP, typename T>
	struct LuaFunc<MP, T, std::enable_if_t<std::is_class<T>::value && !IsMPtr<T>::value>>
	{
		// todo: 右值版, 结构体析构函数
		static inline void Push(lua_State* L, T const& v)
		{
			auto ud = (Lua_UD<T>*)lua_newuserdata(L, sizeof(Lua_UD<T>));	// ud
			ud->typeIndex = TypeId<T>::value;
			ud->udType = Lua_UDTypes::Struct;
			ud->isMPObject = false;
			new (&ud->data) T(v);
			Lua_PushMetatable<MP, T>(L);									// ud, mt
			lua_setmetatable(L, -2);										// ud
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			v = ((Lua_UD<T>*)lua_touserdata(L, idx))->data;
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (!lua_isuserdata(L, idx)) return false;
			auto ud = (Lua_UD<T>*)lua_touserdata(L, idx);
			if (!Lua_GetMemPool<MP>(L).IsBaseOf(TypeId<T>::value, ud->typeIndex)) return false;
			switch (ud->udType)
			{
			case Lua_UDTypes::Pointer:
				if (ud->isMPObject) return false;
				else
				{
					v = **(T**)&ud->data;
					return true;
				}
			case Lua_UDTypes::MPtr:
				return false;				// MPObject 不支持以值方式使用
			case Lua_UDTypes::Struct:
				v = ud->data;
				return true;
			}
			return true;
		}
	};

	// MPtr<T>
	template<typename MP, typename T>
	struct LuaFunc<MP, T, std::enable_if_t<IsMPtr<T>::value>>
	{
		typedef typename T::ChildType TT;

		static inline void Push(lua_State* L, T const& v)
		{
			auto ud = (Lua_UD<T>*)lua_newuserdata(L, sizeof(Lua_UD<T>));	// ud
			if (std::is_base_of<MPObject, TT>::value && v)
			{
				ud->typeIndex = (decltype(ud->typeIndex))((MPObject*)v.pointer)->typeId();
			}
			else
			{
				ud->typeIndex = TupleIndexOf_v<TT, typename MP::Tuple>;
			}
			ud->udType = Lua_UDTypes::MPtr;
			ud->isMPObject = true;
			new (&ud->data) T(v);
			Lua_PushMetatable<MP, TT>(L);									// ud, mt
			lua_setmetatable(L, -2);										// ud
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			if (lua_isnil(L, idx)) v = nullptr;
			else v = ((Lua_UD<T>*)lua_touserdata(L, idx))->data;
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (lua_isnil(L, idx))
			{
				v = nullptr;
				return true;
			}
			if (!lua_isuserdata(L, idx)) return false;
			auto ud = (Lua_UD<T>*)lua_touserdata(L, idx);
			if (!Lua_GetMemPool<MP>(L).IsBaseOf(TypeId<TT>::value, ud->typeIndex)) return false;
			switch (ud->udType)
			{
			case Lua_UDTypes::Pointer:
				v = *(TT**)&ud->data;
				return true;
			case Lua_UDTypes::MPtr:
				v = ud->data;
				return true;
			case Lua_UDTypes::Struct:
				return false;
			}
			return true;
		}
	};

	/************************************************************************************/
	// Lua_TryTo, Lua_Push
	/************************************************************************************/

	template<typename MP, typename T>
	bool Lua_TryTo(lua_State* L, T& v, int idx)
	{
		return LuaFunc<MP, T>::TryTo(L, v, idx);
	}

	template<typename MP, typename T>
	void Lua_Push(lua_State* L, T const& v)
	{
		return LuaFunc<MP, T>::Push(L, v);
	}


	/************************************************************************************/
	// Lua_SetGlobal
	/************************************************************************************/

	// 在全局以下标方式压入值
	template<typename MP, typename T>
	void Lua_SetGlobal(lua_State* L, lua_Integer const& key, T const& v)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);	// _G
		Lua_Push<MP>(L, v);										// _G, v
		lua_rawseti(L, -2, key);								// _G
		lua_pop(L, 1);											//
	}

	// 在全局以字串 key 方式压入值
	template<typename MP, typename T>
	void Lua_SetGlobal(lua_State* L, char const* const& key, T const& v)
	{
		Lua_Push<MP>(L, v);										// v
		lua_setglobal(L, key);									//
	}



	/************************************************************************************/
	// Lua_SetGlobalNil
	/************************************************************************************/

	// 从全局清除 key( integer ) 项
	inline void Lua_SetGlobalNil(lua_State* L, lua_Integer const& key)
	{
		if (!L) return;
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);	// _G
		lua_pushnil(L);											// _G, nil
		lua_rawseti(L, -2, key);								// _G
		lua_pop(L, 1);											//
	}

	// 从全局清除 key( string ) 项
	inline void Lua_SetGlobalNil(lua_State* L, char const* key)
	{
		if (!L) return;
		lua_pushnil(L);
		lua_setglobal(L, key);
	}


	/************************************************************************************/
	// Lua_TupleFiller
	/************************************************************************************/

	template<typename MP, typename Tuple, std::size_t N>
	struct Lua_TupleFiller
	{
		static bool Fill(lua_State* L, Tuple& t)
		{
			auto rtv = Lua_TryTo<MP>(L, std::get<N - 1>(t), -(int)(std::tuple_size<Tuple>::value - N + 1));
			if (!rtv) return false;
			return Lua_TupleFiller<MP, Tuple, N - 1>::Fill(L, t);
		}
	};
	template<typename MP, typename Tuple>
	struct Lua_TupleFiller <MP, Tuple, 1 >
	{
		static bool Fill(lua_State* L, Tuple& t)
		{
			return Lua_TryTo<MP>(L, std::get<0>(t), -(int)(std::tuple_size<Tuple>::value));
		}
	};

	template<typename MP, typename Tuple>
	bool Lua_FillTuple(lua_State* L, Tuple& t)
	{
		return Lua_TupleFiller<MP, Tuple, std::tuple_size<Tuple>::value>::Fill(L, t);
	}

	/************************************************************************************/
	// Lua_CallFunc
	/************************************************************************************/

	// 已知问题: 标记为 YIELD 方式执行的函数, 将忽略直接返回值.
	// 有参数 有返回值
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(std::enable_if_t<sizeof...(Args) && !std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		std::tuple<Args...> t;
		if (Lua_FillTuple<MP>(L, t))
		{
			auto rtv = FuncTupleCaller(o, f, t, std::make_index_sequence<sizeof...(Args)>());
			if (YIELD) return lua_yield(L, 0);
			Lua_Push<MP>(L, rtv);
			return 1;
		}
		return Lua_Error(L, "error!!! bad arg data type? type cast fail?");
	}
	// 无参数 有返回值
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(std::enable_if_t<!sizeof...(Args) && !std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		auto rtv = (o->*f)();
		if (YIELD) return lua_yield(L, 0);
		Lua_Push<MP>(L, rtv);
		return 1;
	}
	// 有参数 无返回值
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(std::enable_if_t<sizeof...(Args) && std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		std::tuple<Args...> t;
		if (Lua_FillTuple<MP>(L, t))
		{
			FuncTupleCaller(o, f, t, std::make_index_sequence<sizeof...(Args)>());
			if (YIELD) return lua_yield(L, 0);
			else return 0;
		}
		return Lua_Error(L, "error!!! bad arg data type? type cast fail?");
	}
	// 无参数 无返回值
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(std::enable_if_t<!sizeof...(Args) && std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		(o->*f)();
		if (YIELD) return lua_yield(L, 0);
		else return 0;
	}





	/************************************************************************************/
	// Lua_SetGlobalFunc_Log
	/************************************************************************************/

	inline void Lua_SetGlobalFunc_Log(lua_State* L)
	{
		lua_pushcclosure(L, [](lua_State* L)
		{
			// todo: 遍历并获取所有入参, 记录到某处. 当前主用于打断点
			return 0;
		}, 0);
		lua_setglobal(L, "Log");
	}



	/************************************************************************************/
	// Lua_BindFunc_Ensure
	/************************************************************************************/

	// 生成 ud.Ensure 函数
	template<typename MP>
	void Lua_BindFunc_Ensure(lua_State* L)
	{
		lua_pushstring(L, "Ensure");
		lua_pushcclosure(L, [](lua_State* L)
		{
			auto top = lua_gettop(L);
			if (top != 1)
			{
				return Lua_Error(L, "error!!! func args num wrong.");
			}
			MPObject* self = nullptr;
			auto b = xx::Lua_TryTo<MP>(L, self, 1) && self;
			lua_pushboolean(L, b);
			return 1;
		}, 0);
		lua_rawset(L, -3);
	}

	/************************************************************************************/
	// Lua_BindFunc_Release
	/************************************************************************************/

	// 生成 ud.Release 函数
	template<typename MP>
	void Lua_BindFunc_Release(lua_State* L)
	{
		lua_pushstring(L, "Release");
		lua_pushcclosure(L, [](lua_State* L)
		{
			auto top = lua_gettop(L);
			if (top != 1)
			{
				return Lua_Error(L, "error!!! func args num wrong.");
			}
			MPObject* self = nullptr;
			auto b = xx::Lua_TryTo<MP>(L, self, 1) && self;
			if (b) self->Release();
			return 0;
		}, 0);
		lua_rawset(L, -3);
	}

	/************************************************************************************/
	// Lua_BindFunc_ToString
	/************************************************************************************/

	// 生成 ud.ToString 函数( 和原始 ToString 接口不一样的地方在于, LUA 并不传入参数, 而是接收返回值 )
	template<typename MP>
	void Lua_BindFunc_ToString(lua_State* L, char const* fnName = "ToString")
	{
		lua_pushstring(L, fnName);
		lua_pushcclosure(L, [](lua_State* L)
		{
			auto top = lua_gettop(L);
			if (top != 1)
			{
				return Lua_Error(L, "error!!! func args num wrong.");
			}
			MPObject* self = nullptr;
			auto b = xx::Lua_TryTo<MP>(L, self, 1) && self;
			if (b)
			{
				auto& mp = Lua_GetMemPool<MP>(L);
				String_v str(mp);
				self->ToString(*str);
				lua_pushlstring(L, str->buf, str->dataLen);
				return 1;
			}
			return 0;
		}, 0);
		lua_rawset(L, -3);
	}



	/************************************************************************************/
	// Lua_NewState
	/************************************************************************************/

	// 创建并返回一个 lua_State*, 以内存池为内存分配方式, 默认 openLibs 以及创建 mt
	// 可以用 lua_getallocf 函数来得到 mp 指针
	template<typename MP>
	inline lua_State* Lua_NewState(MP& mp, bool openLibs = true, bool registerMetatables = true)
	{
		auto L = lua_newstate([](void *ud, void *ptr, size_t osize, size_t nsize)
		{
			return ((MemPoolBase*)ud)->Realloc(ptr, nsize, osize);
		}, &mp);

		if (openLibs) luaL_openlibs(L);
		if (registerMetatables)
		{
			lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);	// _G

			// 先造出所有类型的 空mt ( __index 指向自己 )
			for (int i = 0; i < MP::typesSize; ++i)
			{
				lua_newtable(L);									// _G, mt
				lua_pushstring(L, "__index");						// _G, mt, __index
				lua_pushvalue(L, -2);								// _G, mt, __index, mt
				lua_rawset(L, -3);									// _G, mt
				if (mp.template IsBaseOf<MPObject>(i))
				{
					Lua_BindFunc_ToString<MP>(L, "__tostring");
				}
				lua_rawseti(L, -2, i);								// _G
			}
			assert(lua_gettop(L) == 1);

			// 遍历设其 mt 指向父 mt
			for (int i = 0; i < MP::typesSize; ++i)
			{
				if (i == mp.pids[i]) continue;
				lua_rawgeti(L, -1, i);								// _G, mt
				lua_rawgeti(L, -2, mp.pids[i]);						// _G, mt, pmt
				lua_setmetatable(L, -2);							// _G, mt
				lua_pop(L, 1);										// _G
			}
			assert(lua_gettop(L) == 1);

			lua_rawgeti(L, -1, 0);									// _G, MPObject's mt
			Lua_BindFunc_Ensure<MP>(L);								// _G, MPObject's mt
			Lua_BindFunc_Release<MP>(L);							// _G, MPObject's mt
			Lua_BindFunc_ToString<MP>(L);							// _G, MPObject's mt

			lua_pop(L, 2);											//
			assert(lua_gettop(L) == 0);
		}
		return L;
	}

}

/************************************************************************************/
// xxLua_BindFunc
/************************************************************************************/

// 函数绑定
#define xxLua_BindFunc(MPTYPE, LUA, T, F, YIELD)										\
lua_pushstring(LUA, #F);																\
lua_pushcclosure(LUA, [](lua_State* L)													\
{																						\
	auto top = lua_gettop(L);															\
	auto numArgs = xx::GetFuncArgsCount(&T::F);											\
	if (top != numArgs + 1)																\
	{																					\
		return xx::Lua_Error(L, "error!!! wrong num args.");							\
	}																					\
	T* self = nullptr;																	\
	if (!xx::Lua_TryTo<MPTYPE>(L, self, 1))												\
	{																					\
		return xx::Lua_Error(L, "error!!! self is nil or bad data type!");				\
	}																					\
	return xx::Lua_CallFunc<MPTYPE, YIELD>(L, self, &T::F);								\
}, 0);																					\
lua_rawset(LUA, -3);


/************************************************************************************/
// xxLua_BindField
/************************************************************************************/

// 成员变量绑定
#define xxLua_BindField(MPTYPE, LUA, T, F, writeable)									\
lua_pushstring(LUA, #F);																\
lua_pushcclosure(LUA, [](lua_State* L)													\
{																						\
	auto top = lua_gettop(L);															\
	if (!top)																			\
	{																					\
		return xx::Lua_Error(L, "error!!! forget : ?");									\
	}																					\
	T* self = nullptr;																	\
	if (!xx::Lua_TryTo<MPTYPE>(L, self, 1))												\
	{																					\
		return xx::Lua_Error(L, "error!!! self is nil or bad data type!");				\
	}																					\
	if (top == 1)																		\
	{																					\
		xx::Lua_Push<MPTYPE>(L, self->F);												\
		return 1;																		\
	}																					\
	if (top == 2)																		\
	{																					\
		if (!writeable)																	\
		{																				\
			return xx::Lua_Error(L, "error!!! readonly!");								\
		}																				\
		else if (!xx::Lua_TryTo<MPTYPE>(L, self->F, 2))									\
		{																				\
			return xx::Lua_Error(L, "error!!! bad data type!");							\
		}																				\
		return 0;																		\
	}																					\
	return xx::Lua_Error(L, "error!!! too many args!");									\
}, 0);																					\
lua_rawset(LUA, -3);

