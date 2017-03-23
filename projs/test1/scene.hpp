
int Scene::Update()
{
	if (co) if (auto rtv = xx::Lua_Resume(co, err)) return rtv;

	for (auto i = (int)monsters->dataLen - 1; i >= 0; --i)
	{
		auto& o = monsters->At(i);
		auto r = o->Update();
		if (r)
		{
			// todo: r < 0 log ?
			o->Release();
		}
	}
	// more for ....

	return 0;
}

int Scene::Run()
{
#ifdef _WIN32
	timeBeginPeriod(1);
#endif
	int64_t accumulatMS = 0;
	xx::Stopwatch sw;
	while (true)
	{
		auto durationMS = sw();
		if (durationMS > msPerFrame)
		{
			// todo: ��־����֡
			durationMS = msPerFrame;
		}

		accumulatMS += durationMS;
		bool executed = false;
		while (accumulatMS >= msPerFrame)
		{
			executed = true;
			accumulatMS -= msPerFrame;

			if (auto rtv = Update()) return rtv;
			++ticks;
		}
		if (!executed)
		{
			Sleep(1);     // ʡ�� cpu
						  // todo: ��־����֡
		}
	}
#ifdef _WIN32
	timeEndPeriod(1);
#endif
	return 0;
}


Scene::~Scene()
{
	while (monsters->dataLen)
	{
		monsters->Top()->Release();
	}

	co = nullptr;
	if (L)
	{
		lua_close(L);
		L = nullptr;
	}
}


void Scene::LoadLuaFile(char const* fn)
{
	assert(fn);

	// ����Э��( ��Э����ֱ���� local self = scene ������ )
	co = xx::Lua_RegisterCoroutine(L, this);

	// ����Э�� lua ����
	luaL_loadfile(co, fn);
	xx::LuaFunc<MP, Scene*>::Push(co, this);
	xx::Lua_Resume(co, err, 1);
}

Scene::Scene()
	: monsters(*this)
	, err(*this)
{
	// more...

	L = xx::Lua_NewState(*this);

	// LuaBind: Scene
	xx::Lua_PushMetatable<MP, Scene>(L);
	xxLua_BindField(MP, L, Scene, ticks, false);
	xxLua_BindFunc(MP, L, Scene, CreateMonster, false);
	lua_pop(L, 1);

	// LuaBind: MonsterBase
	xx::Lua_PushMetatable<MP, MonsterBase>(L);
	xxLua_BindField(MP, L, MonsterBase, x, true);
	lua_pop(L, 1);

	// more bind here...

	// �ɸ���Ҷ, �𼶸�������Ԫ�ص���һ��( ������ )
	xx::Lua_CloneParentMetatables(*this, L);

	// set global scene
	xx::Lua_SetGlobal<MP>(L, "scene", this);
}

xx::MPtr<MonsterBase> Scene::CreateMonster(char const* classname)
{
	if (!std::strcmp(classname, "Monster1"))
	{
		return Create<Monster1>();
	}
	else if (!std::strcmp(classname, "Monster2"))
	{
		return Create<Monster2>();
	}
	return nullptr;
}
