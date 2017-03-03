#pragma once
#include <cassert>
#include <tuple>
#include <type_traits>
#include <utility>
#include <array>
#include <initializer_list>
#include "xx_mempool.h"
#include "lua.hpp"

#include <iostream>

namespace xx
{
	// todo: �����ú��������ּ����зּ�. ���������.

	// �ɷ��� LUA ������������: float, double, int64, ��ʽ string, �Լ� T, T*
	// ���� T �ַ�Ϊ һ��ṹ�� �Լ� MPtr<T> ( T Ϊ MPObject ������ )
	// T* ��Ϊһ��ָ�� �� MPObject* ������ָ��
	// String* ��ָ���� lua �е�ǰ�� nil �����

	/************************************************************************************/
	// Lua_Container / Lua_SetGlobalContainer
	/************************************************************************************/

	// �ڴ�����ȫ��������
	constexpr auto Lua_Container = "objs";

	// ����ȫ������
	inline void Lua_SetGlobalContainer(lua_State* L)
	{
		lua_newtable(L);										// t
		lua_setglobal(L, Lua_Container);						//
	}



	/************************************************************************************/
	// Lua_PushMetatables
	/************************************************************************************/

	// ��������������Ԫ����õ� L( һֱռ�� stack �ռ�, �Ա��ڴӸ� L  )
	template<typename MP, typename Tuple, size_t... Indexs>
	void Lua_PushMetatables(lua_State* L, std::index_sequence<Indexs...> const& idxs)
	{
		// isMPObject = true: idx Ϊ��. false: idx Ϊ��, idx �����жϸ��ӹ�ϵ
		auto func = [&](bool isMPObject, int idx)
		{
			if (idx == 0) return;								// MPObject ���ͱ���ע�� mt

			lua_newtable(L);									// mt
			lua_pushinteger(L, isMPObject ? idx : -idx);		// mt, idx/-idx
			lua_rawseti(L, -2, 1);								// mt
			lua_pushstring(L, "__index");						// mt, __index
			lua_pushvalue(L, -2);								// mt, __index, mt
			lua_rawset(L, -3);									// mt
		};
		std::initializer_list<int>{ (
			func(IsMPObject<std::tuple_element_t<Indexs, Tuple>>::value, Indexs)
			, 0)... };
	}

	// ��������������Ԫ����õ� L( һֱռ�� stack �ռ�, �Ա��ڴӸ� L  )
	template<typename MP>
	inline void Lua_PushMetatables(lua_State* L)
	{
		assert(0 == lua_gettop(L));
		Lua_PushMetatables<MP, typename MP::Tuple>(L, std::make_index_sequence<MP::typesSize>());
	}



	/************************************************************************************/
	// Lua_NewState
	/************************************************************************************/

	// ����������һ�� lua_State*, ���ڴ��Ϊ�ڴ���䷽ʽ, Ĭ�� openLibs
	template<typename MP>
	inline lua_State* Lua_NewState(MP& mp, bool openLibs = true, bool setGlobalContainer = true, bool pushMetatables = true)
	{
		auto L = lua_newstate([](void *ud, void *ptr, size_t osize, size_t nsize)
		{
			return ((MemPoolBase*)ud)->Realloc(ptr, nsize, osize);
		}, &mp);
		// ֮������� lua_getallocf �������õ� mp

		if (openLibs) luaL_openlibs(L);

		if (setGlobalContainer)
		{
			Lua_SetGlobalContainer(L);
		}

		if (pushMetatables)
		{
			Lua_PushMetatables<MP>(L);
		}

		return L;
	}


	/************************************************************************************/
	// Lua_CopyItemsFromParentMetatable
	/************************************************************************************/

	// ��ָ�� index �� clone Ԫ�ص� -1 ��
	template<typename MP, typename PT>
	inline void Lua_CopyItemsFromParentMetatable(lua_State* L)
	{
		lua_pushnil(L);
		while (lua_next(L, TupleIndexOf<PT, typename MP::Tuple>::value) != 0)
		{
			lua_pushvalue(L, -2);
			lua_insert(L, -2);
			lua_rawset(L, -4);
		}
	}


	/************************************************************************************/
	// Lua_RegisterCoroutine
	/************************************************************************************/

	// Ϊ״̬��( ������ MPObject )��Э�̷��� LUA_REGISTRYINDEX ������ָ��
	// ��Ҫ��״̬�����캯��������. o ���� this
	inline lua_State* Lua_RegisterCoroutine(lua_State* L, void* key)
	{
		auto co = lua_newthread(L);							// key, co
		lua_rawsetp(L, LUA_REGISTRYINDEX, key);				//
		return co;
	}



	/************************************************************************************/
	// Lua_ReleaseCoroutine
	/************************************************************************************/

	// �Ƴ�һ��Э�� ( λ�� LUA_REGISTRYINDEX �� )
	inline void Lua_UnregisterCoroutine(lua_State* L, void* key)
	{
		if (!L) return;										// ����� scene ����( L ���� )�ͻᵼ��������
		lua_pushnil(L);
		lua_rawsetp(L, LUA_REGISTRYINDEX, key);
	}


	/************************************************************************************/
	// Lua_Resume
	/************************************************************************************/

	inline int Lua_Resume(lua_State* co, std::string& err)
	{
		assert(co);
		int status = lua_resume(co, nullptr, 0);
		if (status == LUA_YIELD)
		{
			return 0;	// todo: �ݴ溯���ķ���ֵ? 
		}
		else if (status == LUA_ERRRUN && lua_isstring(co, -1))
		{
			err = lua_tostring(co, -1);
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

	// �򻯳���������. �÷�: return LuaError("...");
	inline int Lua_Error(lua_State* L, char const* errmsg)
	{
		lua_pushstring(L, errmsg);
		lua_error(L);
		return 0;
	}


	/************************************************************************************/
	// Set XXXXXXX Nil
	/************************************************************************************/

	// ��ȫ����������� key ��
	inline static void SetContainerNil(lua_State* L, lua_Integer const& key)
	{
		if (!L) return;
		auto rtv = lua_getglobal(L, Lua_Container);			// objs
		assert(rtv == LUA_TTABLE);
		lua_pushnil(L);										// objs, nil
		lua_rawseti(L, -2, key);							// objs
		lua_pop(L, 1);										//
	}

	// ��ȫ����� key ��
	inline static void SetGlobalNil(lua_State* L, char const* key)
	{
		if (!L) return;
		lua_pushnil(L);
		lua_setglobal(L, key);
	}




	/************************************************************************************/
	// Lua<T, cond>
	/************************************************************************************/

	template<typename MP, typename T, typename ENABLE = void>
	struct Lua
	{
		// �� T ���Ͷ�Ӧ�� mt ѹջ( ������� mt ʱ����ջ�� �� Bind ʱѹ�� upvalue )( L ���Ǹ� )
		static void PushMetatable(lua_State* L);

		// �� v ѹ��ջ��( L ���Ǹ� )
		static void Push(lua_State* L, T const& v);

		// �� idx ��������� v
		static void To(MP& mp, lua_State* L, T& v, int idx);

		// �Դ� idx ��������� v. �ɹ����� true
		static bool TryTo(MP& mp, lua_State* L, T& v, int idx);

		// ��ȫ��������һ��ֵ( L ���Ǹ� )
		static void SetContainer(lua_State* L, lua_Integer const& key, T const& v);

		// ��ȫ����һ��ֵ( L ���Ǹ� )
		static void SetGlobal(lua_State* L, char const* const& key, T const& v);

		// �� v ѹ��ջ��( L �Ǹ�. ��Է���ֵ �ṹ�� T, �� upvalue �� mt )
		static void PushRtv(lua_State* L, T const& v);
	};

	// void
	template<typename MP>
	struct Lua<MP, void, void>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushnil(L);
		}
	};

	// string
	template<typename MP>
	struct Lua<MP, std::string, void>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushnil(L);
		}
		static inline void Push(lua_State* L, std::string const& v)
		{
			lua_pushlstring(L, v.data(), v.size());
		}
		static inline void To(MP& mp, lua_State* L, std::string& v, int idx)
		{
			size_t len;
			auto s = lua_tolstring(L, idx, &len);
			v.assign(s, len);
		}
		static inline bool TryTo(MP& mp, lua_State* L, std::string& v, int idx)
		{
			if (!lua_isstring(L, idx)) return false;
			To(mp, L, v, idx);
			return true;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, std::string const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);						// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, std::string const& v)
		{
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void PushRtv(lua_State* L, std::string const& v)
		{
			lua_pushlstring(L, v.data(), v.size());
		}
	};

	// xx::String*
	template<typename MP>
	struct Lua<MP, String*, void>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushnil(L);
		}
		static inline void Push(lua_State* L, String* const& v)
		{
			if (v) lua_pushlstring(L, v->buf, v->dataLen);
			else lua_pushnil(L);
			
		}
		static inline void To(MP& mp, lua_State* L, String*& v, int idx)
		{
			if (v) v->Release();
			if (lua_isnil(L, idx))
			{
				v = nullptr;
				return;
			}
			size_t len;
			auto s = lua_tolstring(L, idx, &len);
			v = mp.Create<String>(s, len);
		}
		static inline bool TryTo(MP& mp, lua_State* L, String*& v, int idx)
		{
			if (!lua_isstring(L, idx) && !lua_isnil(L, idx)) return false;
			To(mp, L, v, idx);
			return true;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, String* const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);						// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, String* const& v)
		{
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void PushRtv(lua_State* L, String* const& v)
		{
			if(v) lua_pushlstring(L, v->buf, v->dataLen);
			else lua_pushnil(L);
		}
	};

	// char*
	template<typename MP>
	struct Lua<MP, char const*, void>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushnil(L);
		}
		static inline void Push(lua_State* L, char const* const& v)
		{
			if(v) lua_pushstring(L, v);
			else lua_pushnil(L);
		}
		static inline void To(MP& mp, lua_State* L, char const*& v, int idx)
		{
			if (lua_isnil(L, idx)) v = nullptr;
			else v = lua_tostring(L, idx);
		}
		static inline bool TryTo(MP& mp, lua_State* L, char const*& v, int idx)
		{
			if (!lua_isstring(L, idx) && !lua_isnil(L, idx)) return false;
			To(mp, L, v, idx);
			return true;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, char const* const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);						// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, char const* const& v)
		{
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void PushRtv(lua_State* L, char const* const& v)
		{
			lua_pushstring(L, v);
		}
	};

	// fp
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<std::is_floating_point<T>::value>>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushnil(L);
		}
		static inline void Push(lua_State* L, T const& v)
		{
			lua_pushnumber(L, (lua_Number)v);
		}
		static inline void To(MP& mp, lua_State* L, T& v, int idx)
		{
			v = (T)lua_tonumber(L, idx);
		}
		static inline bool TryTo(MP& mp, lua_State* L, T& v, int idx)
		{
			if (!lua_isnumber(L, idx)) return false;
			To(mp, L, v, idx);
			return true;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, T const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);						// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, T const& v)
		{
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void PushRtv(lua_State* L, T const& v)
		{
			lua_pushnumber(L, (lua_Number)v);
		}
	};

	// int
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushnil(L);
		}
		static inline void Push(lua_State* L, T const& v)
		{
			lua_pushinteger(L, (lua_Integer)v);
		}
		static inline void To(MP& mp, lua_State* L, T& v, int idx)
		{
			v = (T)lua_tointeger(L, idx);
		}
		static inline bool TryTo(MP& mp, lua_State* L, T& v, int idx)
		{
			if (!lua_isinteger(L, idx)) return false;
			To(mp, L, v, idx);
			return true;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, T const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);						// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, T const& v)
		{
			Push(L, v);
			lua_pop(L, 1);													//
		}
		static inline void PushRtv(lua_State* L, T const& v)
		{
			lua_pushinteger(L, (lua_Integer)v);
		}
	};

	// T*
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<std::is_pointer<T>::value>>
	{
		typedef std::remove_pointer_t<T> TT;

		static inline void PushMetatable(lua_State* L)
		{
			lua_pushvalue(L, TupleIndexOf<TT, typename MP::Tuple>::value);
		}
		static inline void Push(lua_State* L, T const& v)
		{
			lua_pushlightuserdata(L, v);								// lud
			PushMetatable(L);											// lud, mt
			lua_setmetatable(L, -2);									// lud
		}
		static inline void To(MP& mp, lua_State* L, T& v, int idx)
		{
			v = (T)lua_touserdata(L, idx);
		}
		static inline bool TryTo(MP& mp, lua_State* L, T& v, int idx)
		{
			if (!lua_islightuserdata(L, idx)) return false;
			auto rtv = lua_getmetatable(L, idx);						// ... mt?
			if (!rtv) return false;
			rtv = lua_rawgeti(L, -1, 1);								// ... mt, idx
			if (rtv != LUA_TNUMBER) goto LabFail;
			auto idx = lua_tointeger(L, -1);
			if (idx == 0) goto LabFail;
			else if (idx > 0)											// ����� MPObject ����, ֱ��ȡָ�뱾��� typeId �����ж�
			{
				if (idx >= MP::typesSize) goto LabFail;
				v = (T)lua_touserdata(L, idx);							// �����ȡ���ɵ��жϲ��� . ��� v Ϊ��, ����Ϊ��Ȼ��ת
				if (v && !mp.IsBaseOf(TupleIndexOf<TT, typename MP::Tuple>::value, ((MPObject*)v)->typeId())) goto LabFail;
				goto LabSuccess;
			}
			else
			{
				idx = -idx;
				if (idx >= MP::typesSize) goto LabFail;
				v = (T)lua_touserdata(L, idx);
				if (v && !mp.IsBaseOf(TupleIndexOf<TT, typename MP::Tuple>::value, idx)) goto LabFail;
				goto LabSuccess;
			}
		LabSuccess:
			lua_pop(L, 2);
			return true;
		LabFail:
			lua_pop(L, 2);
			v = nullptr;
			return false;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, T const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);					// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);													// objs, lud
			lua_rawseti(L, -2, key);									// objs
			lua_pop(L, 1);												//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, T const& v)
		{
			Push(L, v);													// lud
			lua_setglobal(L, key);										//
		}
		static inline void PushRtv(lua_State* L, T const& v)
		{
			lua_pushlightuserdata(L, v);								// lud
			lua_getupvalue(L, -lua_gettop(L) - 1, 1);					// lud, mt
			lua_setmetatable(L, -2);									// lud
		}
	};

	// T
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<std::is_class<T>::value && !IsMPtr<T>::value>>
	{
		static inline void PushMetatable(lua_State* L)
		{
			lua_pushvalue(L, TupleIndexOf<T, typename MP::Tuple>::value);
		}
		// todo: ��ֵ��
		static inline void Push(lua_State* L, T const& v)
		{
			auto p = (T*)lua_newuserdata(L, sizeof(v));					// ud
			new (p) T(v);
			PushMetatable(L);											// ud, mt
			lua_setmetatable(L, -2);									// ud
		}
		static inline void To(MP& mp, lua_State* L, T& v, int idx)
		{
			v = *(T*)lua_touserdata(L, idx);
		}
		static inline bool TryTo(MP& mp, lua_State* L, T& v, int idx)
		{
			if (!lua_isuserdata(L, idx)) return false;
			auto rtv = lua_getmetatable(L, idx);						// ... mt?
			if (!rtv) return false;
			rtv = lua_rawgeti(L, -1, 1);								// ... mt, idx
			if (rtv != LUA_TNUMBER) goto LabFail;
			auto idx = lua_tointeger(L, -1);
			if (idx >= 0) goto LabFail;
			idx = -idx;
			if (idx >= MP::typesSize) goto LabFail;
			if (!mp.IsBaseOf(TupleIndexOf<T, typename MP::Tuple>::value, idx)) goto LabFail;
			v = *(T*)lua_touserdata(L, idx);
			lua_pop(L, 2);
			return true;
		LabFail:
			lua_pop(L, 2);
			return false;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, T const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);					// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);													// objs, ud
			lua_rawseti(L, -2, key);									// objs
			lua_pop(L, 1);												//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, T const& v)
		{
			Push(L, v);													// ud
			lua_setglobal(L, key);										//
		}
		static inline void PushRtv(lua_State* L, T&& v)
		{
			auto p = (T*)lua_newuserdata(L, sizeof(v));					// ud
			new (p) T((T&&)v);
			lua_getupvalue(L, -lua_gettop(L) - 1, 1);					// ud, mt
			lua_setmetatable(L, -2);									// ud
		}
	};

	// MPtr<T>
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<IsMPtr<T>::value>>
	{
		typedef typename T::ChildType TT;

		static inline void PushMetatable(lua_State* L)
		{
			lua_pushvalue(L, TupleIndexOf<TT, typename MP::Tuple>::value);
		}
		static inline void Push(lua_State* L, T const& v)
		{
			auto p = lua_newuserdata(L, sizeof(v));						// ud
			new (p) T(v);
			PushMetatable(L);											// lud, mt
			lua_setmetatable(L, -2);									// ud
		}
		static inline void To(MP& mp, lua_State* L, T& v, int idx)
		{
			v = *(T*)lua_touserdata(L, idx);
		}
		static inline bool TryTo(MP& mp, lua_State* L, T& v, int idx)
		{
			if (!lua_isuserdata(L, idx)) return false;
			auto rtv = lua_getmetatable(L, idx);						// ... mt?
			if (!rtv) return false;
			rtv = lua_rawgeti(L, -1, 1);								// ... mt, idx
			if (rtv != LUA_TNUMBER) goto LabFail;
			auto idx = lua_tointeger(L, -1);
			if (idx <= 0 || idx >= MP::typesSize) goto LabFail;
			auto& p = *(T*)lua_touserdata(L, idx);
			if (p && !mp.IsBaseOf(TupleIndexOf<TT, typename MP::Tuple>::value, p->typeId())) goto LabFail;
			v = p;
			lua_pop(L, 2);
			return true;
		LabFail:
			lua_pop(L, 2);
			return false;
		}
		static inline void SetContainer(lua_State* L, lua_Integer const& key, T const& v)
		{
			auto rtv = lua_getglobal(L, Lua_Container);					// objs
			assert(rtv == LUA_TTABLE);
			Push(L, v);													// objs, lud
			lua_rawseti(L, -2, key);									// objs
			lua_pop(L, 1);												//
		}
		static inline void SetGlobal(lua_State* L, char const* const& key, T const& v)
		{
			Push(L, v);													// lud
			lua_setglobal(L, key);										//
		}
		static inline void PushRtv(lua_State* L, T const& v)
		{
			auto p = lua_newuserdata(L, sizeof(v));						// ud
			new (p) T(v);
			lua_getupvalue(L, -lua_gettop(L) - 1, 1);					// ud, mt
			lua_setmetatable(L, -2);									// ud
		}
	};



	/************************************************************************************/
	// Lua_TupleFiller
	/************************************************************************************/

	template<typename MP, typename Tuple, std::size_t N>
	struct Lua_TupleFiller
	{
		static bool Fill(MP& mp, lua_State* L, Tuple& t)
		{
			auto rtv = Lua<MP, std::tuple_element_t<N - 1, Tuple>>::TryTo(mp, L, std::get<N - 1>(t), -(int)(std::tuple_size<Tuple>::value - N + 1));
			if (!rtv) return false;
			return Lua_TupleFiller<MP, Tuple, N - 1>::Fill(mp, L, t);
		}
	};
	template<typename MP, typename Tuple>
	struct Lua_TupleFiller <MP, Tuple, 1 >
	{
		static bool Fill(MP& mp, lua_State* L, Tuple& t)
		{
			return Lua<MP, std::tuple_element_t<0, Tuple>>::TryTo(mp, L, std::get<0>(t), -(int)(std::tuple_size<Tuple>::value));
		}
	};

	template<typename MP, typename Tuple>
	bool Lua_FillTuple(MP& mp, lua_State* L, Tuple& t)
	{
		return Lua_TupleFiller<MP, Tuple, std::tuple_size<Tuple>::value>::Fill(mp, L, t);
	}

	/************************************************************************************/
	// Lua_CallFunc
	/************************************************************************************/

	// ��֪����: ���Ϊ YIELD ��ʽִ�еĺ���, ������ֱ�ӷ���ֵ.
	// �в��� �з���ֵ
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(MP& mp, std::enable_if_t<sizeof...(Args) && !std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		std::tuple<Args...> t;
		if (Lua_FillTuple(mp, L, t))
		{
			auto rtv = FuncTupleCaller(o, f, t, std::make_index_sequence<sizeof...(Args)>());
			if (YIELD) return lua_yield(L, 0);
			Lua<MP, R>::PushRtv(L, rtv);
			return 1;
		}
		return 0;
	}
	// �޲��� �з���ֵ
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(MP& mp, std::enable_if_t<!sizeof...(Args) && !std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		auto rtv = (o->*f)();
		if (YIELD) return lua_yield(L, 0);
		Lua<MP, R>::PushRtv(L, rtv);
		return 1;
	}
	// �в��� �޷���ֵ
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(MP& mp, std::enable_if_t<sizeof...(Args) && std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		std::tuple<Args...> t;
		if (Lua_FillTuple(mp, L, t))
		{
			FuncTupleCaller(o, f, t, std::make_index_sequence<sizeof...(Args)>());
			if (YIELD) return lua_yield(L, 0);
			else return 0;
		}
		return 0;
	}
	// �޲��� �޷���ֵ
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(MP& mp, std::enable_if_t<!sizeof...(Args) && std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		(o->*f)();
		if (YIELD) return lua_yield(L, 0);
		else return 0;
	}


	/************************************************************************************/
	// Lua_BindFunc_Ensure
	/************************************************************************************/

	template<typename T>
	int Lua_BindFunc_Ensure_Impl(lua_State* L)
	{
		auto top = lua_gettop(L);
		if (top != 1)
		{
			return Lua_Error(L, "error!!! func args num wrong.");
		}
		bool hasValue = false;
		if (!lua_isuserdata(L, 1))
		{
			return Lua_Error(L, "error!!! self is not MPtr<T>.");
		}
		auto rtv = lua_getmetatable(L, 1);							// ... mt?
		if (!rtv)
		{
			lua_pop(L, 1);
			return Lua_Error(L, "error!!! self is no mt.");
		}
		rtv = lua_rawgeti(L, -1, 1);								// ... mt, idx
		if (rtv != LUA_TNUMBER)
		{
			lua_pop(L, 2);
			return Lua_Error(L, "error!!! self's mt is no [1].");
		}
		auto idx = (int)lua_tointeger(L, -1);
		if (idx <= 0 || idx >= MP::typesSize)
		{
			lua_pop(L, 2);
			return Lua_Error(L, "error!!! self's mt[1] out of range.");
		}
		auto& p = *(MPtr<T>*)lua_touserdata(L, 1);
		hasValue = (bool)p;
		lua_pop(L, 2);
		lua_pushboolean(L, hasValue);
		return 1;
	}

	// ����ȷ�� ud �Ƿ�Ϸ��� Ensure ָ��
	template<typename T>
	inline void Lua_BindFunc_Ensure(lua_State* L)
	{
		static_assert(std::is_base_of<MPObject, T>::value, "only MPObject* struct have Ensure func.");
		lua_pushstring(L, "Ensure");
		lua_pushcfunction(L, Lua_BindFunc_Ensure_Impl<T>);
		lua_rawset(L, -3);
	}



	// �� self תΪ T* ����̬����. Ϊ�����ʾת��ʧ�ܻ�ָ���Ұ
	// ���� lua_islightuserdata ��� is_base_of<MPObject, T> ���жϵ������������
	template<typename MP, typename T, typename ENABLE = void>
	struct LuaSelf;

	template<typename MP, typename T>
	struct LuaSelf<MP, T, std::enable_if_t< std::is_base_of<MPObject, T>::value>>
	{
		static T* Get(MP& mp, lua_State* L)
		{
			// todo: type check
			if (lua_islightuserdata(L, 1))
			{
				return (T*)lua_touserdata(L, 1);
			}
			auto& ptr = *(MPtr<T>*)lua_touserdata(L, 1);
			if (ptr) return ptr.pointer;
			return nullptr;
		}
	};
	template<typename MP, typename T>
	struct LuaSelf<MP, T, std::enable_if_t<!std::is_base_of<MPObject, T>::value>>
	{
		static T* Get(MP& mp, lua_State* L)
		{
			// todo: type check
			return (T*)lua_touserdata(L, 1);
		}
	};

}



/************************************************************************************/
// xxLua_BindFunc
/************************************************************************************/

// ������
#define xxLua_BindFunc(MPTYPE, LUA, T, F, YIELD)										\
lua_pushstring(LUA, #F);																\
xx::Lua<MPTYPE, decltype(xx::GetFuncReturnType(&T::F))>::PushMetatable(LUA);			\
lua_pushcclosure(LUA, [](lua_State* L)													\
{																						\
	auto top = lua_gettop(L);															\
	auto numArgs = xx::GetFuncArgsCount(&T::F);											\
	if (top != numArgs + 1)																\
	{																					\
		return xx::Lua_Error(L, "error!!! wrong num args.");							\
	}																					\
	MPTYPE* mp;																			\
	lua_getallocf(L, (void**)&mp);														\
	auto self = xx::LuaSelf<MPTYPE, T>::Get(*mp, L);									\
	if (!self)																			\
	{																					\
		return xx::Lua_Error(L, "error!!! self is nil or bad data type!");				\
	}																					\
	return xx::Lua_CallFunc<MPTYPE, YIELD>(*mp, L, self, &T::F);						\
}, 1);																					\
lua_rawset(LUA, -3);


/************************************************************************************/
// xxLua_BindField
/************************************************************************************/

// ��Ա������
#define xxLua_BindField(MP, LUA, T, F, writeable)										\
lua_pushstring(LUA, #F);																\
xx::Lua<decltype(MP), decltype(xx::GetFieldType(&T::F))>::PushMetatable(LUA);			\
lua_pushcclosure(LUA, [](lua_State* L)													\
{																						\
	auto top = lua_gettop(L);															\
	if (!top)																			\
	{																					\
		return xx::Lua_Error(L, "error!!! forget : ?");									\
	}																					\
	auto self = xx::LuaSelf<decltype(MP), T>::Get(MP, L);								\
	if (!self)																			\
	{																					\
		return xx::Lua_Error(L, "error!!! self is nil or bad data type!");				\
	}																					\
	if (top == 1)																		\
	{																					\
		xx::Lua<decltype(MP), decltype(self->F)>::PushRtv(MP, L, self->F);				\
		return 1;																		\
	}																					\
	if (top == 2)																		\
	{																					\
		if (!writeable)																	\
		{																				\
			return xx::Lua_Error(L, "error!!! readonly!");								\
		}																				\
		else if (!xx::Lua<decltype(MP), decltype(self->F)>::TryTo(MP, L, self->F, 2))	\
		{																				\
			return xx::Lua_Error(L, "error!!! bad data type!");							\
		}																				\
		return 0;																		\
	}																					\
	return xx::Lua_Error(L, "error!!! too many args!");									\
}, 1);																					\
lua_rawset(LUA, -3);






// ��������Ƶ� Lua<T>::SetContainer

///************************************************************************************/
//// Lua_ContainerPush
///************************************************************************************/

//// �� MPtr<T> ����ȫ������( key Ϊ pureVersionNumber ). ��Ҫ���� coroutine ���� objs[ pvn ] ��� self
//template<typename MP, typename T>
//inline void Lua_ContainerPush(MP& mp, lua_State* L, MPtr<T> const& o)
//{
//	auto rtv = lua_getglobal(L, Lua_Container);						// objs
//	assert(rtv == LUA_TTABLE);
//	auto p = lua_newuserdata(L, sizeof(o));							// objs, ud
//	new (p) MPtr<T>(o);
//	lua_pushvalue(L, TupleIndexOf<T, typename MP::Tuple>::value);	// objs, ud, mt
//	lua_setmetatable(L, -2);
//	lua_rawseti(L, -2, o->pureVersionNumber());						// objs
//	lua_pop(L, 1);													//
//}

//// �� o תΪ MPtr<T> ����ȫ������( key Ϊ pureVersionNumber ). ��Ҫ���� coroutine ���� objs[ pvn ] ��� self
//template<typename MP, typename T>
//inline void Lua_ContainerPush(MP& mp, lua_State* L, T const* o)
//{
//	static_assert(std::is_base_of<MPObject, T>::value, "the T must be inherit of MPObject");
//	Lua_ContainerPush(mp, L, MPtr<T>(o));
//}


///************************************************************************************/
//// Lua_ContainerPop
///************************************************************************************/

//// ��ȫ������ɱ����( key Ϊ pureVersionNumber )
//// ��Ҫ��״̬����������������. o ���� this
//inline void Lua_ContainerPop(lua_State* L, MPObject const* o)
//{
//	assert(o);
//	if (!L) return;
//	auto rtv = lua_getglobal(L, Lua_Container);			// objs
//	assert(rtv == LUA_TTABLE);
//	lua_pushnil(L);										// objs, nil
//	lua_rawseti(L, -2, o->pureVersionNumber());			// objs
//	lua_pop(L, 1);										//
//}

//template<typename T>
//inline void Lua_ContainerPop(lua_State* L, MPtr<T> const& o)
//{
//	Lua_ContainerPop(L, o->pointer);
//}


// ��������Ƶ� Lua<T>::SetGlobal

//template<typename MP, typename T>
//inline void Lua_SetGlobal(MP& mp, lua_State* L, T* o, char const* name)
//{
//	lua_pushlightuserdata(L, o);						// lud
//	lua_pushvalue(L, Lua_Metatable<T>::index);			// lud, mt
//	lua_setmetatable(L, -2);							// lud
//	lua_setglobal(L, name);
//}
