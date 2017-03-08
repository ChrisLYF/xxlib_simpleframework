#include "xx_scene.h"

struct Monster1;
struct Monster2;
// more predefine here...

struct Scene : SceneBase
{
	static const int64_t msPerFrame = 1000 / 20;
	int64_t ticks = 0;

	Scene();
	~Scene();

	int Update();				// call by Run
	int Run();


	/****************************************************************/
	// lua
	/****************************************************************/

	void SetLuaCode(char const* luacode);
	lua_State* L = nullptr;
	lua_State* co = nullptr;		// SceneManager �Լ��Ľű�( ִ�������� objs ֮ Updates )
	xx::String* err = nullptr;

	// ������ lua �д��� Monster1 ����
	xx::MPtr<Monster1> CreateMonster1(char const* luacode);
	xx::MPtr<Monster2> CreateMonster2();
	
};
