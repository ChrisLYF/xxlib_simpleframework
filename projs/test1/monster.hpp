MonsterBase::MonsterBase()
	: skills(scene())
	//, fsmConds(scene())
	//, fsmIdle(scene(), this)
	//, fsmMove(scene(), this)
	, fsmCast(scene(), this)
	, fsmAI(scene(), this)
	//, fsmAlertCondition(scene(), this)
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


// ���б��־����Զ�̹�
Monster1::Monster1() : MonsterBase()
{
	// ģ�� ��������
	cfg_alertInterval = 20;								// ��������ɨ��ÿ 1 �뷢��һ��
	cfg_alertDistance = 10;								// ������� 10 ��
	cfg_moveTimespan = 20;								// ÿ���ƶ������ 1 ��
	cfg_moveInterval = 20;								// �ƶ�֮����Ϣ 1 ��
	cfg_traceMaxDistance = 30;							// ׷ɱ�� 30 ��ʱ����׷ɱ
	cfg_traceMaxTimespan = 400;							// ׷ɱ 20 �����׷ɱ
	cfg_traceKeepDistanceRange = { 5.0f, 15.0f };		// ׷ɱʱ���־��� 5 ~ 15 ��
	cfg_moveSpeed = 0.2f;								// ÿ֡�ƶ� 0.2 ��, ÿ���ƶ� 4 ��
	cfg_alertDistancePow2 = cfg_alertDistance * cfg_alertDistance;
	cfg_traceMaxDistancePow2 = cfg_traceMaxDistance * cfg_traceMaxDistance;
	cfg_traceKeepDistanceRangePow2 = cfg_traceKeepDistanceRange * cfg_traceKeepDistanceRange;

	alertInterval = 0;									// �������������
	moveTicks = scene().ticks + cfg_moveInterval;		// ����������Ϣ
	moveSpeed = 0;										// �ٶ�Ϊ 0 ��ʾ��Ϣ
	moveAngle = (uint8_t)scene().NextInteger(0, 256);	// ����Ƕ�

	xy = { scene().NextDouble(0, 100), scene().NextDouble(0, 100) };
	bornXY = xy;
	originalXY = xy;

	hp = 100;

	// ģ�� �������� ��������
	scene().Create<SkillFar>(this);


	// ģ�� �������� �����ʼAI
	SetFSM(fsmAI);

	// ���� LUA ���ʼ AI
	//SetFSM(CreateFSM<FSMLua>(this, "monster1.lua"));
}


// δ���б��־���Ľ�ս��
Monster2::Monster2() : MonsterBase()
{
	// ģ�� ��������
	cfg_alertInterval = 20;								// ��������ɨ��ÿ 1 �뷢��һ��
	cfg_alertDistance = 10;								// ������� 10 ��
	cfg_moveTimespan = 20;								// ÿ���ƶ������ 1 ��
	cfg_moveInterval = 20;								// �ƶ�֮����Ϣ 1 ��
	cfg_traceMaxDistance = 30;							// ׷ɱ�� 30 ��ʱ����׷ɱ
	cfg_traceMaxTimespan = 400;							// ׷ɱ 20 �����׷ɱ
	cfg_traceKeepDistanceRange = { 5.0f, 15.0f };		// ׷ɱʱ���־��� 5 ~ 15 ��
	cfg_moveSpeed = 0.1f;								// ÿ֡�ƶ� 0.1 ��, ÿ���ƶ� 2 ��
	cfg_alertDistancePow2 = cfg_alertDistance * cfg_alertDistance;
	cfg_traceMaxDistancePow2 = cfg_traceMaxDistance * cfg_traceMaxDistance;
	cfg_traceKeepDistanceRangePow2 = cfg_traceKeepDistanceRange * cfg_traceKeepDistanceRange;

	alertInterval = 0;									// �������������
	moveTicks = scene().ticks + cfg_moveInterval;		// ����������Ϣ
	moveSpeed = 0;										// �ٶ�Ϊ 0 ��ʾ��Ϣ
	moveAngle = (uint8_t)scene().NextInteger(0, 256);	// ����Ƕ�

	xy = { scene().NextDouble(0, 100), scene().NextDouble(0, 100) };
	bornXY = xy;
	originalXY = xy;

	hp = 100;

	// ģ�� �������� ��������
	scene().Create<SkillNear>(this);

	// ģ�� �������� �����ʼAI
	SetFSM(fsmAI);

	// ���� LUA ���ʼ AI
	//SetFSM(CreateFSM<FSMLua>(this, "monster2.lua"));
}



float MonsterBase::DistancePow2(MonsterBase* other)
{
	return xyMath.GetDistancePow2(this->xy - other->xy);
}

xx::MPtr<MonsterBase> MonsterBase::SearchTarget()
{
	MonsterBase* r = nullptr;
	auto minDistance = std::numeric_limits<float>::max();
	for (auto& m : *scene().monsters)
	{
		if (m == this) continue;
		auto d = DistancePow2(m);
		if (d <= cfg_alertDistance && minDistance > d)
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


int MonsterBase::TakeAvaliableSkillId()
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

//xx::MPtr<FSMLua> MonsterBase::LuaCondition(char const* luacode)
//{
//	auto fsm = scene().Create<FSMLua>(this, luacode, false);
//	fsmConds->AddDirect(fsm);
//	return fsm;
//}
//
//template<typename T, typename ... Args>
//xx::MPtr<T> MonsterBase::Condition(Args&&...args)
//{
//	auto fsm = scene().Create<T>(this, std::forward<Args>(args)...);
//	fsmConds->AddDirect(fsm);
//	return fsm;
//}

//void MonsterBase::Idle(int64_t ticks, FSMBase* cond)
//{
//	fsmIdle->Init(ticks, cond);
//	PushFSM(fsmIdle);
//}
//
//void MonsterBase::Move(int xInc, int count, FSMBase* cond)
//{
//	fsmMove->Init(xInc, count, cond);
//	PushFSM(fsmMove);
//}

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





//MonsterFSM_Idle::MonsterFSM_Idle(SceneObjBase* owner) : MonsterFSMBase(owner)
//{
//}
//void MonsterFSM_Idle::Init(int64_t ticks, FSMBase* breakCond)
//{
//	this->sleepToTicks = scene().ticks + ticks;
//	this->breakCond = breakCond;
//}
//int MonsterFSM_Idle::Update()
//{
//	// ʱ�䵽�������� Update ���ط� 0, ��ջ
//	if (scene().ticks >= sleepToTicks
//		|| (breakCond && breakCond->Update())) Pop();
//	return 0;
//}
//
//
//
//
//MonsterFSM_Move::MonsterFSM_Move(SceneObjBase* owner) : MonsterFSMBase(owner)
//{
//}
//void MonsterFSM_Move::Init(int xInc, int count, FSMBase* breakCond)
//{
//	this->xInc = xInc;
//	this->toTicks = scene().ticks + count;
//	this->breakCond = breakCond;
//}
//int MonsterFSM_Move::Update()
//{
//	if (scene().ticks >= toTicks
//		|| (breakCond && breakCond->Update())) Pop();
//	ctx().x += xInc;
//	return 0;
//}




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
	Pop();
	return 0;
}



//MonsterFSM_AlertCondition::MonsterFSM_AlertCondition(SceneObjBase* owner) : MonsterFSMBase(owner) {}
//
//// �йֽ��뷶Χ����������, ��� target ���Ժ󷵻ط� 0 ( ֪ͨʹ���������˳� )
//int MonsterFSM_AlertCondition::Update()
//{
//	assert(!ctx().target);
//	auto tar = ctx().SearchTarget();
//	if (tar)
//	{
//		ctx().target = tar;
//		ctx().originalX = ctx().x;
//		return 1;						// ��Ϊ����ʹ��, ͨ�� Update ����ֵ���ﵽ�����֪��Ŀ��
//	}
//	return 0;
//}




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
}

#define CORO_YIELD(n)		\
{							\
	lineNumber = n;			\
	return 0;				\
}

#define CORO_GOTO(n)		\
goto Label##n


//// ������ idle / move, ���ֹֽ��뷶Χ�Ͳ�ͣ׷ɱ�ӽ�������ѭ���ż���, ֱ��Ŀ��ʧЧʱ����ԭ��
//int MonsterFSM_AI::Update()
//{
//	auto& c = ctx();
//	auto& s = scene();
//	CORO_BEGIN();
//	{
//		if (c.target) CORO_GOTO(1);					// �����Ŀ��( ���������ำ�� ), �е�׷ɱ״̬
//		
//		auto v = s.NextInteger(-1, 2);				// �������ֵ -1, 0, 1 �������� idle ���� move( v ֵͬʱ�����ƶ����� )
//		if (v == 0)
//		{
//			c.Idle(s.NextInteger(1, 3), c.fsmAlertCondition);		// ѹ�� Idle ״̬��
//		}
//		else
//		{
//			c.Move(v, s.NextInteger(1, 3), c.fsmAlertCondition);	// ѹ�� Move ״̬��
//		}
//		CORO_YIELD(0);								// �´ν���ʱ�� CORO_BEGIN ����ʼִ��
//	}
//	CORO_(1);
//	{
//		// �����Ŀ��, �������ǰ�� idle �������ж��������, Ŀ�걻����, ��׷ɱ( ������Ŀ���ƶ�, �м��ܷ�ʱ�ͷż��� )
//		if (c.target)
//		{
//			auto skillid = c.AnyAvaliableSkillId();
//			if (skillid == -1)
//			{
//				int xInc = c.target->x > c.x ? 1 : -1;
//				c.Move(xInc, 3, nullptr);
//			}
//			else
//			{
//				c.Cast(skillid);
//			}
//			CORO_YIELD(1);							// �´ν���ʱ�� CORO_(1) ����ʼִ��
//		}
//		else
//		{
//			// ���Ŀ�궪ʧ( ����? ������? ), ִ�лس�����( ���� move ������ target ˲���¼�ĵ����� )
//			auto d = c.originalX - c.x;
//			c.Move(d > 0 ? 1 : -1, std::abs(d), nullptr);	// �������س�
//
//			CORO_YIELD(0);							// �´ν���ʱ�� CORO_BEGIN ����ʼִ��
//		}
//	}
//	CORO_END();
//	return 0;
//}

MonsterFSM_AI::MonsterFSM_AI(SceneObjBase* owner) : MonsterFSMBase(owner) {}

// �ϸ��ղ߻������� AI ����
int MonsterFSM_AI::Update()
{
	auto& c = ctx();
	auto& s = scene();
	CORO_BEGIN();
	{
		assert(c.moveSpeed == 0);
		if (c.alertInterval <= s.ticks)
		{
			//	assert(!ctx().target);
			//	auto tar = ctx().SearchTarget();
			//	if (tar)
			//	{
			//		ctx().target = tar;
			//		ctx().originalX = ctx().x;
			//		return 1;						// ��Ϊ����ʹ��, ͨ�� Update ����ֵ���ﵽ�����֪��Ŀ��
			//	}
			//	return 0;

		}
	}
	CORO_(1);
	{
		assert(c.moveSpeed != 0);
	}
	CORO_END();
	return 0;
}