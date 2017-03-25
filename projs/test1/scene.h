struct Scene : MP
{
	// ֡�������
	static const int64_t msPerFrame = 1000 / 20;
	int64_t ticks = 0;
	
	// ���� scene �õ��������������
	xx::Random_v rnd;

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

	// ������ lua �����
	int NextInteger(int min, int max);
	double NextDouble(double min, double max);

	// ������ lua ��ɨ monsters
	uint32_t Monsters_Count();
	xx::MPtr<MonsterBase> Monsters_At(uint32_t index);
};
