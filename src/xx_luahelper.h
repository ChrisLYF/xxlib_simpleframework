#pragma once
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
	// todo: Lua_BindFunc ֮�����Ա������. ����Ϊ Lua_SetGlobal ���Ӻ���ָ������

	// todo: ����һ�½ṹ���е��ӽṹ��� lua userdata ��װ( ����˵ ud ��˵�����ֿ�����Ҫ�Ƶ� uservalue ��ȥ, ������ uservalue ���ӳ� container ����������? )

	// todo: �Ľ���ʼ metatable ����, Ĭ�Ͽɰ���ĸ��ӹ�ϵ, �Զ���Ϊ�̳й�ϵ


	// �ɷ��� LUA ������������: float, double, int64, ��ʽ string, �Լ� T, T*
	// ���� T �ַ�Ϊ һ��ṹ�� �Լ� MPtr<T> ( T Ϊ MPObject ������ )
	// T* ��Ϊһ��ָ�� �� MPObject* ������ָ��
	// String* ��ָ���� lua �е�ǰ�� nil �����
	// ��֧�ָ��ӽṹ�崴��Ϊ ud( �����й��캯������Ҫ����������. ��þ��Ǹ� pod )
	// ֻ֧�ֵ��̳�




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

	// ��ȡ�ڴ��. �������� Lua_NewState ������ L ����
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

	// ������� mt, �������Ƹ���Ԫ�ص�����, ���������ϲ���( ÿ��һ����ѯ�ƺ��ͻ��� 1/5, һֱ���� )
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

	inline int Lua_Resume(lua_State* co, xx::String* err, int narg = 0)
	{
		assert(co);
		int status = lua_resume(co, nullptr, narg);
		if (status == LUA_YIELD)
		{
			return 0;	// todo: �ݴ溯���ķ���ֵ? 
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

	// �򻯳���������. �÷�: return LuaError("...");
	inline int Lua_Error(lua_State* L, char const* errmsg)
	{
		lua_pushstring(L, errmsg);
		lua_error(L);
		return 0;
	}



	/************************************************************************************/
	// Lua_PushMetatable
	/************************************************************************************/

	// �� _G[ TypeId ] ȡ��Ԫ��ѹ��ջ��
	template<typename MP, typename T>
	void Lua_PushMetatable(lua_State* L)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);			// _G
		lua_rawgeti(L, -1, TupleIndexOf<T, typename MP::Tuple>::value);	// _G, mt
		lua_replace(L, -2);												// mt
	}



	/************************************************************************************/
	// Lua_UD
	/************************************************************************************/
	// auto ud = (Lua_UD*)lua_touserdata(L, -1);
	// auto t = (T*)(ud + 1)

	// ѹ�� lua �� userdata ��������̬
	enum class Lua_UDTypes : uint8_t
	{
		Pointer,					// ָ��( ������ MPObject ���� )
		MPtr,						// MPtr<T>( ��Ȼ�� MPObject ���� )
		Struct						// �ṹ��( һ������ MPObject ���� )
	};

	// ѹ�� lua �� userdata ������ͷ. Ӧ����ռ 8 �ֽ�. �������������
	template<typename T>
	struct Lua_UD
	{
		int typeIndex;				// MP �е���������( �������жϼ̳й�ϵ, �� dynamic_cast )
		Lua_UDTypes udType;			// ������̬
		bool isMPObject;			// �Ƿ�� MPObject ����( ��Ϊָ��������̬��һ������˵�� )
		T data;
	};


	/************************************************************************************/
	// Lua<T, cond>
	/************************************************************************************/

	template<typename MP, typename T, typename ENABLE = void>
	struct Lua
	{
		// �� v ѹ��ջ��( L ���Ǹ� )
		static void Push(lua_State* L, T const& v);

		// �� idx ��������� v
		static void To(lua_State* L, T& v, int idx);

		// �Դ� idx ��������� v. �ɹ����� true
		static bool TryTo(lua_State* L, T& v, int idx);
	};

	// string
	template<typename MP>
	struct Lua<MP, std::string, void>
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
	struct Lua<MP, String*, void>
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
	struct Lua<MP, char const*, void>
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
	struct Lua<MP, T, std::enable_if_t<std::is_floating_point<T>::value>>
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
	struct Lua<MP, T, std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value>>
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

	// T*
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<std::is_pointer<T>::value>>
	{
		typedef std::remove_pointer_t<T> TT;

		static inline void Push(lua_State* L, T const& v)
		{
			auto ud = (Lua_UD<T>*)lua_newuserdata(L, sizeof(Lua_UD<T>));	// ud
			Lua_PushMetatable<MP, TT>(L);									// ud, mt
			lua_setmetatable(L, -2);										// ud
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
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			v = ((Lua_UD<T>*)lua_touserdata(L, idx))->data;
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (!lua_isuserdata(L, idx)) return false;
			auto ud = (Lua_UD<T>*)lua_touserdata(L, idx);
			auto tid = TupleIndexOf<TT, typename MP::Tuple>::value;
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
				v = (TT*)&ud->data;			// �����Ͻ����ֵ��Σ�յ�. ����� lua ���վ�û��. ��Ҫ����ʹ��
				return true;
			}
			return false;
		}
	};

	// T
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<std::is_class<T>::value && !IsMPtr<T>::value>>
	{
		// todo: ��ֵ��, �ṹ����������
		static inline void Push(lua_State* L, T const& v)
		{
			auto ud = (Lua_UD<T>*)lua_newuserdata(L, sizeof(Lua_UD<T>));	// ud
			Lua_PushMetatable<MP, T>(L);									// ud, mt
			lua_setmetatable(L, -2);										// ud
			ud->typeIndex = TupleIndexOf<T, typename MP::Tuple>::value;
			ud->udType = Lua_UDTypes::Struct;
			ud->isMPObject = false;
			new (&ud->data) T(v);
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			v = ((Lua_UD<T>*)lua_touserdata(L, idx))->data;
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (!lua_isuserdata(L, idx)) return false;
			auto ud = (Lua_UD<T>*)lua_touserdata(L, idx);
			if (!Lua_GetMemPool<MP>(L).IsBaseOf(TupleIndexOf<T, typename MP::Tuple>::value, ud->typeIndex)) return false;
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
				return false;				// MPObject ��֧����ֵ��ʽʹ��
			case Lua_UDTypes::Struct:
				v = ud->data;
				return true;
			}
		}
	};

	// MPtr<T>
	template<typename MP, typename T>
	struct Lua<MP, T, std::enable_if_t<IsMPtr<T>::value>>
	{
		typedef typename T::ChildType TT;

		static inline void Push(lua_State* L, T const& v)
		{
			auto ud = (Lua_UD<T>*)lua_newuserdata(L, sizeof(Lua_UD<T>));	// ud
			Lua_PushMetatable<MP, TT>(L);									// ud, mt
			lua_setmetatable(L, -2);										// ud
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
		}
		static inline void To(lua_State* L, T& v, int idx)
		{
			v = ((Lua_UD<T>*)lua_touserdata(L, idx))->data;
		}
		static inline bool TryTo(lua_State* L, T& v, int idx)
		{
			if (!lua_isuserdata(L, idx)) return false;
			auto ud = (Lua_UD<T>*)lua_touserdata(L, idx);
			if (!Lua_GetMemPool<MP>(L).IsBaseOf(TupleIndexOf<TT, typename MP::Tuple>::value, ud->typeIndex)) return false;
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
		}
	};

	/************************************************************************************/
	// Lua_SetGlobal
	/************************************************************************************/

	// ��ȫ�����±귽ʽѹ��ֵ
	template<typename MP, typename T>
	void Lua_SetGlobal(lua_State* L, lua_Integer const& key, T const& v)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);	// _G
		Lua<MP, T>::Push(L, v);									// _G, v
		lua_rawseti(L, -2, key);								// _G
		lua_pop(L, 1);											//
	}

	// ��ȫ�����ִ� key ��ʽѹ��ֵ
	template<typename MP, typename T>
	void Lua_SetGlobal(lua_State* L, char const* const& key, T const& v)
	{
		Lua<MP, T>::Push(L, v);									// v
		lua_setglobal(L, key);									//
	}



	/************************************************************************************/
	// Lua_SetGlobalNil
	/************************************************************************************/

	// ��ȫ����� key( integer ) ��
	inline void Lua_SetGlobalNil(lua_State* L, lua_Integer const& key)
	{
		if (!L) return;
		lua_rawgeti(L, LUA_REGISTRYINDEX, LUA_RIDX_GLOBALS);	// _G
		lua_pushnil(L);											// _G, nil
		lua_rawseti(L, -2, key);								// _G
		lua_pop(L, 1);											//
	}

	// ��ȫ����� key( string ) ��
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
			auto rtv = Lua<MP, std::tuple_element_t<N - 1, Tuple>>::TryTo(L, std::get<N - 1>(t), -(int)(std::tuple_size<Tuple>::value - N + 1));
			if (!rtv) return false;
			return Lua_TupleFiller<MP, Tuple, N - 1>::Fill(L, t);
		}
	};
	template<typename MP, typename Tuple>
	struct Lua_TupleFiller <MP, Tuple, 1 >
	{
		static bool Fill(lua_State* L, Tuple& t)
		{
			return Lua<MP, std::tuple_element_t<0, Tuple>>::TryTo(L, std::get<0>(t), -(int)(std::tuple_size<Tuple>::value));
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

	// ��֪����: ���Ϊ YIELD ��ʽִ�еĺ���, ������ֱ�ӷ���ֵ.
	// �в��� �з���ֵ
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(std::enable_if_t<sizeof...(Args) && !std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		std::tuple<Args...> t;
		if (Lua_FillTuple<MP>(L, t))
		{
			auto rtv = FuncTupleCaller(o, f, t, std::make_index_sequence<sizeof...(Args)>());
			if (YIELD) return lua_yield(L, 0);
			Lua<MP, R>::Push(L, rtv);
			return 1;
		}
		return Lua_Error(L, "error!!! bad arg data type? type cast fail?");
	}
	// �޲��� �з���ֵ
	template<typename MP, bool YIELD, typename T, typename R, typename ...Args>
	int Lua_CallFunc(std::enable_if_t<!sizeof...(Args) && !std::is_void<R>::value, lua_State*> L, T* o, R(T::* f)(Args...))
	{
		auto rtv = (o->*f)();
		if (YIELD) return lua_yield(L, 0);
		Lua<MP, R>::Push(L, rtv);
		return 1;
	}
	// �в��� �޷���ֵ
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
	// �޲��� �޷���ֵ
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
			// todo: ��������ȡ�������, ��¼��ĳ��. ��ǰ�����ڴ�ϵ�
			return 0;
		}, 0);
		lua_setglobal(L, "Log");
	}



	/************************************************************************************/
	// Lua_BindFunc_Ensure
	/************************************************************************************/

	// ���� ud.Ensure ����
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
			auto b = xx::Lua<MP, MPObject*>::TryTo(L, self, 1) && self;
			lua_pushboolean(L, b);
			return 1;
		}, 0);
		lua_rawset(L, -3);
	}

	/************************************************************************************/
	// Lua_BindFunc_Release
	/************************************************************************************/

	// ���� ud.Release ����
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
			auto b = xx::Lua<MP, MPObject*>::TryTo(L, self, 1) && self;
			if (b) self->Release();
			return 0;
		}, 0);
		lua_rawset(L, -3);
	}




	/************************************************************************************/
	// Lua_NewState
	/************************************************************************************/

	// ����������һ�� lua_State*, ���ڴ��Ϊ�ڴ���䷽ʽ, Ĭ�� openLibs �Լ����� mt
	// ������ lua_getallocf �������õ� mp ָ��
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

			// ������������͵� ��mt ( __index ָ���Լ� )
			for (int i = 0; i < MP::typesSize; ++i)
			{
				lua_newtable(L);									// _G, mt
				lua_pushstring(L, "__index");						// _G, mt, __index
				lua_pushvalue(L, -2);								// _G, mt, __index, mt
				lua_rawset(L, -3);									// _G, mt
				lua_rawseti(L, -2, i);								// _G
			}
			assert(lua_gettop(L) == 1);

			// �������� mt ָ�� mt
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

			lua_pop(L, 2);											//
			assert(lua_gettop(L) == 0);
		}
		return L;
	}

}

/************************************************************************************/
// xxLua_BindFunc
/************************************************************************************/

// ������
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
	if (!xx::Lua<MPTYPE, T*>::TryTo(L, self, 1))										\
	{																					\
		return xx::Lua_Error(L, "error!!! self is nil or bad data type!");				\
	}																					\
	return xx::Lua_CallFunc<MPTYPE, YIELD>(L, self, &T::F);								\
}, 0);																					\
lua_rawset(LUA, -3);


/************************************************************************************/
// xxLua_BindField
/************************************************************************************/

// ��Ա������
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
	if (!xx::Lua<MPTYPE, T*>::TryTo(L, self, 1))										\
	{																					\
		return xx::Lua_Error(L, "error!!! self is nil or bad data type!");				\
	}																					\
	if (top == 1)																		\
	{																					\
		xx::Lua<MPTYPE, decltype(self->F)>::Push(L, self->F);							\
		return 1;																		\
	}																					\
	if (top == 2)																		\
	{																					\
		if (!writeable)																	\
		{																				\
			return xx::Lua_Error(L, "error!!! readonly!");								\
		}																				\
		else if (!xx::Lua<MPTYPE, decltype(self->F)>::TryTo(L, self->F, 2))				\
		{																				\
			return xx::Lua_Error(L, "error!!! bad data type!");							\
		}																				\
		return 0;																		\
	}																					\
	return xx::Lua_Error(L, "error!!! too many args!");									\
}, 0);																					\
lua_rawset(LUA, -3);

