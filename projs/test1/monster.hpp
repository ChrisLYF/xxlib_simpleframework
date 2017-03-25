MonsterBase::MonsterBase()
	: skills(scene())
	, fsmConds(scene())
	, fsmIdle(scene(), this)
	, fsmMove(scene(), this)
	, fsmCast(scene(), this)
	, fsmAI(scene(), this)
	, fsmAlertCondition(scene(), this)
{
	auto& ms = *scene().monsters;
	sceneContainerIndex = (uint32_t)ms.dataLen;
	ms.Add(this);
}

MonsterBase::~MonsterBase()
{
	// Create ʱ��� throw exception ���ܵ�������������
	if (sceneContainerIndex < 0) return;

	// �� �������� �����Ƴ�, ͬ���±�
	auto& ms = *scene().monsters;
	auto& buf = ms.buf;
	auto& dataLen = ms.dataLen;
	--dataLen;
	if (dataLen == this->sceneContainerIndex) return;	// last one
	else
	{
		buf[this->sceneContainerIndex] = buf[dataLen];
		buf[dataLen]->sceneContainerIndex = this->sceneContainerIndex;
	}
}



Monster1::Monster1() : MonsterBase()
{
	// ģ�� ��������
	this->cfg_watchDistance = 30;

	// ģ�� �������� ��������
	scene().Create<SkillNear>(this);
	scene().Create<SkillFar>(this);


	// ģ�� �������� �����ʼAI
	SetFSM(fsmAI);

	// ���� LUA ���ʼ AI
	//SetFSM(CreateFSM<FSMLua>(this, "monster1.lua"));
}



Monster2::Monster2() : MonsterBase()
{
	// ģ�� ��������
	this->cfg_watchDistance = 20;

	// ģ�� �������� ��������
	scene().Create<SkillNear>(this);
	scene().Create<SkillFar>(this);

	// ģ�� �������� �����ʼAI
	SetFSM(fsmAI);

	// ���� LUA ���ʼ AI
	//SetFSM(CreateFSM<FSMLua>(this, "monster2.lua"));
}



int MonsterBase::Distance(MonsterBase* other)
{
	return std::abs(this->x - other->x);
}

xx::MPtr<MonsterBase> MonsterBase::SearchTarget()
{
	MonsterBase* r = nullptr;
	int minDistance = std::numeric_limits<int>::max();
	for (auto& m : *scene().monsters)
	{
		if (m == this) continue;
		auto d = Distance(m);
		if (d <= cfg_watchDistance && minDistance > d)
		{
			minDistance = d;
			r = m;
		}
	}
	return r;
}

void MonsterBase::Hurt(MonsterBase* attacker)
{
	// ��ǰ������ AI ����, ֻ����׸����𹥻���Ŀ���� target ��¼
	if (target) return;
	target = attacker;
}

void MonsterBase::SetTarget(MonsterBase* target)
{
	this->target = target;
}


int MonsterBase::AnyAvaliableSkillId()
{
	// ���ж� gcd. ��� gcd ��ֱ�ӷ��� -1. ������ gcd ��������������
	if (skillsGcd > scene().ticks) return -1;

	// ѭ���ƶ� skillCursor, ����⼼���Ƿ����. ����ƶ�һȦ. ���ص�1�����ֵĿ��ü���
	for (int i = 0; i < (int)skills->dataLen; ++i)
	{
		skillCursor++;
		if (skillCursor >= (int)skills->dataLen) skillCursor = 0;
		if (skills->At(skillCursor)->Avaliable()) return skillCursor;
	}
	return -1;
}

xx::MPtr<FSMLua> MonsterBase::LuaCondition(char const* luacode)
{
	auto fsm = scene().Create<FSMLua>(this, luacode, false);
	fsmConds->AddDirect(fsm);
	return fsm;
}

template<typename T, typename ... Args>
xx::MPtr<T> MonsterBase::Condition(Args&&...args)
{
	auto fsm = scene().Create<T>(this, std::forward<Args>(args)...);
	fsmConds->AddDirect(fsm);
	return fsm;
}

void MonsterBase::Idle(int64_t ticks, FSMBase* cond)
{
	assert(cond);
	fsmIdle->Init(ticks, cond);
	PushFSM(fsmIdle);
}

void MonsterBase::Move(int xInc, int count, FSMBase* cond)
{
	assert(cond);
	fsmMove->Init(xInc, count, cond);
	PushFSM(fsmMove);
}

void MonsterBase::Cast(int skillIndex)
{
	fsmCast->Init(skillIndex);
	PushFSM(fsmCast);
}





MonsterBase& MonsterFSMBase::ctx()
{
	return *(MonsterBase*)owner;
}
MonsterFSMBase::MonsterFSMBase(SceneObjBase* owner) : FSMBase(owner)
{
}





MonsterFSM_Idle::MonsterFSM_Idle(SceneObjBase* owner) : MonsterFSMBase(owner)
{
}
void MonsterFSM_Idle::Init(int64_t ticks, FSMBase* breakCond)
{
	this->sleepToTicks = scene().ticks + ticks;
	this->breakCond = breakCond;
}
int MonsterFSM_Idle::Update()
{
	if (sleepToTicks <= scene().ticks) return 1;
	return breakCond ? breakCond->Update() : 0;
}




MonsterFSM_Move::MonsterFSM_Move(SceneObjBase* owner) : MonsterFSMBase(owner)
{
}
void MonsterFSM_Move::Init(int xInc, int count, FSMBase* breakCond)
{
	this->xInc = xInc;
	this->toTicks = scene().ticks + count;
	this->breakCond = breakCond;
}
int MonsterFSM_Move::Update()
{
	if (scene().ticks >= this->toTicks) return 1;
	ctx().x += xInc;
	return breakCond ? breakCond->Update() : 0;
}




MonsterFSM_Cast::MonsterFSM_Cast(SceneObjBase* owner) : MonsterFSMBase(owner)
{
}
void MonsterFSM_Cast::Init(int skillIndex)
{
	this->skillIndex = skillIndex;
}
int MonsterFSM_Cast::Update()
{
	assert(skillIndex >= 0
		&& (int)ctx().skills->dataLen > skillIndex
		&& ctx().skills->At(skillIndex)->Avaliable());
	ctx().skills->At(skillIndex)->Cast();
	return 0;
}



MonsterFSM_AlertCondition::MonsterFSM_AlertCondition(SceneObjBase* owner) : MonsterFSMBase(owner) {}

// �йֽ��뷶Χ����������, ��� target ���Ժ󷵻ط� 0 ( ֪ͨʹ���������˳� )
int MonsterFSM_AlertCondition::Update()
{
	assert(!ctx().target);
	auto tar = ctx().SearchTarget();
	if (tar)
	{
		ctx().target = tar;
		ctx().originalX = ctx().x;
		return 1;
	}
	return 0;
}




#define CORO_BEGIN()		\
switch (lineNumber)			\
{							\
case 0:						\
Label0:						\
{

#define CORO_(n)			\
}							\
lineNumber = n;				\
return 0;					\
case n:						\
Label##n:					\
{

#define CORO_END()			\
}							\
}							\
return 0

#define CORO_YIELD(n)		\
{							\
	lineNumber = n;			\
	return 0;				\
}

#define CORO_GOTO(n)		\
goto Label##n


// ������ idle / move, ���ֹֽ��뷶Χ�� Cast
int MonsterFSM_AI::Update()
{
	CORO_BEGIN();
	{
		// �������ֵ -1, 0, 1 �������� move ���� idle
		assert(!ctx().target);
		auto& cond = ctx().fsmAlertCondition;
		auto v = scene().rnd->Next(-1, 2);
		assert(v >= -1 && v <= 1);
		if (v == 0)
		{
			ctx().Idle(scene().rnd->Next(1, 3), cond);
		}
		else
		{
			ctx().Move(v, scene().rnd->Next(1, 3), cond);
		}
		CORO_YIELD(0);
	}
	CORO_(1);
	{
		// �����Ŀ��, �������ǰ�� idle �������ж��������, Ŀ�걻����, ��׷ɱ( ������Ŀ���ƶ�, �м��ܷ�ʱ�ͷż��� )
		if (ctx().target)
		{
			auto skillid = ctx().AnyAvaliableSkillId();
			if (skillid == -1)
			{
				int xInc = ctx().target->x > ctx().x ? 1 : -1;
				ctx().Move(xInc, 3, nullptr);
			}
			else
			{
				ctx().Cast(skillid);
			}
			CORO_YIELD(1);
		}
		else
		{
			// ���Ŀ�궪ʧ( ����? ������? ), ִ�лس�����( ���� move ������ target ˲���¼�ĵ����� )
			auto d = ctx().originalX - ctx().x;
			ctx().Move(d > 0 ? 1 : -1, std::abs(d), nullptr);
			CORO_YIELD(0);
		}
	}
	CORO_END();
}

MonsterFSM_AI::MonsterFSM_AI(SceneObjBase* owner) : MonsterFSMBase(owner) {}

