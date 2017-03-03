#include "xx_luahelper.h"
#include <iostream>

// types defines
/***********************************************************************/

struct B;
struct A : xx::MPObject
{
	B* b;
	xx::MPtr<B> GetB();
	A();
	~A();
};

struct B : xx::MPObject
{
	xx::String* name = nullptr;
	void SetName(char const* name);
	xx::String* GetName();
	~B();
};

// MP defines
/***********************************************************************/

typedef xx::MemPool<A, xx::String, B> MP;

// impls
/***********************************************************************/

inline xx::MPtr<B> A::GetB()
{
	return b;
}
inline A::A()
{
	mempool<MP>().CreateTo(b);
}
inline A::~A()
{
	b->Release();
}

inline void B::SetName(char const* name)
{
	if (name)
	{
		if (this->name) this->name->Assign(name);
		else mempool<MP>().CreateTo(this->name, name);
	}
	else if (this->name)
	{
		this->name->Release();
		this->name = nullptr;
	}
}
inline xx::String* B::GetName()
{
	return name;
}
inline B::~B()
{
	if (name)
	{
		name->Release();
		name = nullptr;
	}
}

// main
/***********************************************************************/

int main()
{
	MP mp;
	auto L = xx::Lua_NewState(mp);
	std::cout << "top=" << lua_gettop(L) << std::endl;

	// ��ʼ bind A �ĺ���
	xx::Lua<MP, A>::PushMetatable(L);
	//xx::Lua_BindFunc_Ensure<A>(L);
	//xxLua_BindFunc(MP, L, A, GetB, false);
	//xxLua_BindField(MP, L, A, b, true);
	std::cout << "top=" << lua_gettop(L) << std::endl;

	lua_pushstring(L, "b");
	std::cout << "top=" << lua_gettop(L) << std::endl;
	xx::Lua<MP, decltype(xx::GetFieldType(&A::b))>::PushMetatable(L);
	std::cout << "top=" << lua_gettop(L) << std::endl;
	lua_pushcclosure(L, [](lua_State* L)
	{
		auto top = lua_gettop(L);
		if (!top)
		{
			return xx::Lua_Error(L, "error!!! forget : ?");
		}
		MP* mp;
		lua_getallocf(L, (void**)&mp);
		auto self = xx::LuaSelf<MP, A>::Get(*mp, L);
		if (!self)
		{
			return xx::Lua_Error(L, "error!!! self is nil or bad data type!");
		}
		if (top == 1)
		{
			//xx::Lua<MP, decltype(self->b)>::PushRtv(L, self->b);
			lua_pushlightuserdata(L, self->b);
			lua_getupvalue(L, -lua_gettop(L) - 1, 1);					// lud, mt
			assert(lua_istable(L, -1));
			lua_setmetatable(L, -2);									// lud

			assert(lua_gettop(L) == 2);
			return 1;
		}
		//if (top == 2)
		//{
		//	if (!true)
		//	{
		//		return xx::Lua_Error(L, "error!!! readonly!");
		//	}
		//	else if (!xx::Lua<MP, decltype(self->b)>::TryTo(*mp, L, self->b, 2))
		//	{
		//		return xx::Lua_Error(L, "error!!! bad data type!");
		//	}
		//	return 0;
		//}
		//return xx::Lua_Error(L, "error!!! too many args!");
		return 0;
	}, 1);
	lua_rawset(L, -3);
	std::cout << "top=" << lua_gettop(L) << std::endl;
	lua_pop(L, 1);
	std::cout << "top=" << lua_gettop(L) << std::endl;

	//// ��ʼ bind B �ĺ���
	//xx::Lua<MP, B>::PushMetatable(L);
	//xx::Lua_BindFunc_Ensure<B>(L);
	//xxLua_BindFunc(MP, L, B, SetName, false);
	//xxLua_BindFunc(MP, L, B, GetName, false);
	//xxLua_BindField(MP, L, B, name, true);
	//lua_pop(L, 1);

	// ��һ�� a ����ѹ�� L ֮ global
	auto a = mp.Create<A>();
	xx::Lua<MP, decltype(a)>::SetGlobal(L, "a", a);
	std::cout << "top=" << lua_gettop(L) << std::endl;
	auto rtv = luaL_dostring(L, R"%%(

	print( "a=", a )
	print( getmetatable(a) )
	for k,v in pairs(getmetatable(a)) do
		print( k, v )
	end

	print( "local b = a:b()" )
	local b = a:b()

	print( "a=", a )
	print( getmetatable(a) )
	for k,v in pairs(getmetatable(a)) do
		print( k, v )
	end


	--local b = a:GetB()
	--
	--print( b )
	--print( b:Ensure() )
	--
	--for k,v in pairs(getmetatable(b)) do
	--	print( k, v )
	--end
	--
	----b:SetName( "asdf" )
	----print( b:GetName() )
	----b:SetName( nil )
	----print( b:GetName() == nil )
	--
	--b:name( "asdf" )
	--print( b:name() )
	--b:name( 1 )
	--print( b:name() )

)%%");
	if (rtv)
	{
		std::cout << lua_tostring(L, -1) << std::endl;
	}
	a->Release();

	lua_close(L);
	std::cin.get();
	return 0;
}
