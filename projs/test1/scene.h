#include "xx_scene.h"

struct Monster1;
// more predefine here...

struct Scene : SceneBase
{
	static const int64_t msPerFrame = 1000 / 20;
	int64_t ticks = 0;
	bool running = true;		// ������֪ͨ Run �˳�

	Scene();
	~Scene();

	void Update();				// call by Run
	int Run();


	/****************************************************************/
	// lua
	/****************************************************************/

	lua_State* L = nullptr;
	lua_State* co = nullptr;		// SceneManager �Լ��Ľű�( ִ�������� objs ֮ Updates )
	xx::String* err = nullptr;

	// ������ lua �д��� Monster1 ����
	xx::MPtr<Monster1> CreateMonster1(char const* luacode);
};
