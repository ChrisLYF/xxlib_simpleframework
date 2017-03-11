struct Scene : MP
{
	
	xx::MemHeader_MPObject monstersMH;	// ������������ṩ�ڴ�ͷ
	xx::List<MonsterBase*> monsters;	// �����е� monster

	xx::MemHeader_MPObject errMH;	// ������������ṩ�ڴ�ͷ
	xx::String err;

	static const int64_t msPerFrame = 1000 / 20;
	int64_t ticks = 0;

	Scene();
	~Scene();

	int Update();				// call by Run
	int Run();


	/****************************************************************/
	// lua
	/****************************************************************/

	void LoadLuaFile(char const* fn);
	lua_State* L = nullptr;
	lua_State* co = nullptr;		// SceneManager �Լ��Ľű�( ִ�������� objs ֮ Updates )

	// ������ lua �д��� Monster1 ����
	xx::MPtr<Monster1> CreateMonster1(char const* luacode);
	xx::MPtr<Monster2> CreateMonster2();
};
