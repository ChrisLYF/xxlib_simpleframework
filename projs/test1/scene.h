struct Scene : MP
{
	// ֡�������
	static const int64_t msPerFrame = 1000 / 20;
	int64_t ticks = 0;

	// ����������
	xx::List_v<MonsterBase*> monsters;

	// ���»�����
	int Update();				// call by Run
	int Run();

	// misc
	Scene();
	~Scene();

	/****************************************************************/
	// lua ������
	/****************************************************************/

	void LoadLuaFile(char const* fn);
	lua_State* L = nullptr;
	lua_State* co = nullptr;		// SceneManager �Լ��Ľű�( ִ�������� objs ֮ Updates )
	xx::String_v err;

	// ����������������ʵ��
	xx::MPtr<MonsterBase> CreateMonster(char const* classname);
};
