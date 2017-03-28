MonsterBase::MonsterBase()
	: skills(scene())
	, fsmAI(scene(), this)
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
	cfg_moveBackInterval = 5;							// ����ƶ� 5 ���� 1 �γ����㷽����ƶ�
	cfg_traceMaxDistance = 30;							// ׷ɱ�� 30 ��ʱ����׷ɱ
	cfg_traceMaxTimespan = 400;							// ׷ɱ 20 �����׷ɱ
	cfg_moveSpeed = 0.2f;								// ÿ֡�ƶ� 0.2 ��, ÿ���ƶ� 4 ��
	cfg_radius = 0.5f;									// ��ͶӰ�������Բ�뾶 0.5 ��
	cfg_traceKeepDistanceRange = { 5.0f, 15.0f };		// ׷ɱʱ���־��� 5 ~ 15 ��
	cfg_enableTraceKeepDistanceRange = true;			// ���ñ��־���

	// ģ�� �������� ��������
	scene().Create<SkillFar>(this);

	// ���ݱ�������ʩ�ŷ�Χ ���� ���־���. 
	assert(cfg_traceKeepDistanceRange.f <= cfg_traceKeepDistanceRange.t);
	for (auto& skill : *this->skills)
	{
		if (skill->cfg_distanceRange.f > cfg_traceKeepDistanceRange.f)
		{
			cfg_traceKeepDistanceRange.f = skill->cfg_distanceRange.f;
		}
	}
	// ��Сֵ����: Ϊȷ����Һ͹ֵ�ģ�;�����Ҫ�ص���ʾ, ��С���־��뼴Ϊ�ֵİ뾶
	if (cfg_traceKeepDistanceRange.f < cfg_radius) cfg_traceKeepDistanceRange.f = cfg_radius;
	if (cfg_traceKeepDistanceRange.t < cfg_radius) cfg_traceKeepDistanceRange.t = cfg_radius;

	// �����ּ��� cache ����
	cfg_moveSpeedPow2 = cfg_moveSpeed * cfg_moveSpeed;
	cfg_alertDistancePow2 = cfg_alertDistance * cfg_alertDistance;
	cfg_traceMaxDistancePow2 = cfg_traceMaxDistance * cfg_traceMaxDistance;
	cfg_traceKeepDistanceRangePow2 = cfg_traceKeepDistanceRange * cfg_traceKeepDistanceRange;

	// ��ʼ������ʱ����
	moveSpeed = 0;										// �ٶ�Ϊ 0 ��ʾ��Ϣ
	moveAngle = (uint8_t)scene().NextInteger(0, 256);	// ����Ƕ�

	xy = { (float)scene().NextDouble(0, 100), (float)scene().NextDouble(0, 100) };
	bornXY = xy;
	originalXY = xy;

	hp = 100;


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
	cfg_moveBackInterval = 5;							// ����ƶ� 5 ���� 1 �γ����㷽����ƶ�
	cfg_traceMaxDistance = 30;							// ׷ɱ�� 30 ��ʱ����׷ɱ
	cfg_traceMaxTimespan = 400;							// ׷ɱ 20 �����׷ɱ
	cfg_moveSpeed = 0.1f;								// ÿ֡�ƶ� 0.1 ��, ÿ���ƶ� 2 ��
	cfg_radius = 0.3f;									// ��ͶӰ�������Բ�뾶 0.3 ��
	cfg_traceKeepDistanceRange = { 0.0f, 0.0f };		// �����ñ��־���
	cfg_enableTraceKeepDistanceRange = false;			// �����ñ��־���


	// ģ�� �������� ��������
	scene().Create<SkillNear>(this);

	// ���ݱ�������ʩ�ŷ�Χ ���� ���־���. 
	assert(cfg_traceKeepDistanceRange.f <= cfg_traceKeepDistanceRange.t);
	for (auto& skill : *this->skills)
	{
		if (skill->cfg_distanceRange.f > cfg_traceKeepDistanceRange.f)
		{
			cfg_traceKeepDistanceRange.f = skill->cfg_distanceRange.f;
		}
	}
	// ��Сֵ����: Ϊȷ����Һ͹ֵ�ģ�;�����Ҫ�ص���ʾ, ��С���־��뼴Ϊ�ֵİ뾶
	if (cfg_traceKeepDistanceRange.f < cfg_radius) cfg_traceKeepDistanceRange.f = cfg_radius;
	if (cfg_traceKeepDistanceRange.t < cfg_radius) cfg_traceKeepDistanceRange.t = cfg_radius;

	// �����ּ��� cache ����
	cfg_moveSpeedPow2 = cfg_moveSpeed * cfg_moveSpeed;
	cfg_alertDistancePow2 = cfg_alertDistance * cfg_alertDistance;
	cfg_traceMaxDistancePow2 = cfg_traceMaxDistance * cfg_traceMaxDistance;
	cfg_traceKeepDistanceRangePow2 = cfg_traceKeepDistanceRange * cfg_traceKeepDistanceRange;

	// ��ʼ������ʱ����

	moveSpeed = 0;										// �ٶ�Ϊ 0 ��ʾ��Ϣ
	moveAngle = (uint8_t)scene().NextInteger(0, 256);	// ����Ƕ�

	xy = { (float)scene().NextDouble(0, 100), (float)scene().NextDouble(0, 100) };
	bornXY = xy;
	originalXY = xy;

	hp = 100;

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


xx::MPtr<SkillBase> MonsterBase::TakeAvaliableSkill()
{
	// ���ж� gcd. ��� gcd ��ֱ�ӷ��� -1. ������ gcd ��������������
	if (skillsGcd > scene().ticks) return nullptr;

	// ѭ���ƶ� skillCursor, ����⼼���Ƿ����. ����ƶ�һȦ. ���ص�1�����ֵĿ��ü���
	for (int i = 0; i < (int)skills->dataLen; ++i)
	{
		skillCursor++;
		if (skillCursor >= (int)skills->dataLen) skillCursor = 0;
		auto& skill = skills->At(skillCursor);
		if (skill->Avaliable()) return skill;
	}
	return nullptr;
}

MonsterFSM_AI::MonsterFSM_AI(SceneObjBase* owner) : FSMBase(owner)
{
	auto& c = *(MonsterBase*)owner;
	moveTicks = scene().ticks + c.cfg_moveInterval;		// ����������Ϣ
}

// �Ծ���һ��( ��� cd ���˵Ļ� ). ˳��Ѱ�����Ҳ������
bool MonsterFSM_AI::Alert()
{
	auto& c = *(MonsterBase*)owner;
	auto& s = scene();
	if (c.target) return true;
	if (alertInterval <= s.ticks)
	{
		auto tar = c.SearchTarget();
		if (tar)
		{
			c.target = tar;
			c.originalXY = c.xy;
			return true;
		}
	}
	return false;
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
}

#define CORO_YIELDTO(n)		\
{							\
	lineNumber = n;			\
	return 0;				\
}

#define CORO_GOTO(n)		\
goto Label##n



// �ϸ��ղ߻������� AI ����. ������ Idle, Move, Track, TurnBack ��״̬
int MonsterFSM_AI::Update()
{
	auto& c = *(MonsterBase*)owner;
	auto& s = scene();
	CORO_BEGIN();														// idle
	{
		// ����һ��. ����ɹ�( ѡ�� target ), �е�׷ɱ����
		if (Alert())
		{
			// ��¼��ʼ׷ɱ��ʱ�� for ��ʱ���
			traceTicks = s.ticks + c.cfg_traceMaxTimespan;
			CORO_GOTO(2);
		}

		// �ж�Ҫ��Ҫ������
		if (c.moveSpeed != 0.0f)
		{
			c.moveSpeed = 0.0f;
			// todo: send set pos msg to client ?
		}

		// ��� idle ��ʱ�䵽��, ���е� move
		if (moveTicks <= s.ticks) CORO_YIELDTO(1);

		// ѭ��
		CORO_YIELDTO(0);
	}
	CORO_(1);															// ����ƶ�
	{
		// ����һ��. ����ɹ�( ѡ�� target ), �е�׷ɱ����
		if (Alert())
		{
			// ��¼��ʼ׷ɱ��ʱ�� for ��ʱ���
			traceTicks = s.ticks + c.cfg_traceMaxTimespan;
			CORO_GOTO(2);
		}

		auto speed = c.cfg_moveSpeed;
		uint8_t angle;

		if (++moveCount >= c.cfg_moveBackInterval)
		{
			// ÿ cfg_moveBackInterval ���ƶ�֮��, ����һ���ƶ����������Գ������, ��ȷ���ֲ��������̫Զ
			moveCount = 0;
			angle = xyMath.GetAngle(c.bornXY - c.xy);
		}
		else
		{
			// �����ǰ���Ƕ�
			angle = (uint8_t)s.NextInteger(0, 256);
		}

		bool needSync = false;
		// ������
		if (c.moveSpeed != speed || c.moveAngle != angle)
		{
			c.moveSpeed = speed;
			c.moveAngle = angle;
			xyInc = xyMath.GetXyInc(c.moveAngle) * c.moveSpeed;

			needSync = true;
		}

		// �ƶ�		// todo: ��Ҫ��� xy + inc ֮��ĵ��Ƿ�Ϸ�. ��Ҫ����ת xyInc ����
		c.xy.Add(xyInc);				// todo: ��� moveCount == 0 �ڼ��ƶ����赲, �����õ������ܷ��س�����

		if (needSync)
		{
			// todo: send move msg to client ?
		}


		// ��� move ��ʱ�䵽��, ���е� idle
		if (moveTicks <= s.ticks) CORO_YIELDTO(0);

		// ѭ��
		CORO_YIELDTO(1);
	}
	CORO_(2);													// ׷ɱ( �������ӵ���������Ŀ��ǰ���İ汾 )
	{
		// Ŀ��ʧЧ, ׷����Զ�����޶�, ��ʱ, �������س�����( ��������״̬ )
		if (!c.target
			|| traceTicks <= s.ticks
			|| xyMath.GetDistancePow2(c.bornXY - c.xy) > c.cfg_traceMaxDistancePow2)
		{
			CORO_GOTO(3);
		}

		// �ж��Ƿ��п��ü���. �о���
		if (auto skill = c.TakeAvaliableSkill())
		{
			skill->Cast();
			// send msg to client?
			if (skill->cfg_castStunTimespan)
			{
				castStunTicks = s.ticks + skill->cfg_castStunTimespan;
				CORO_GOTO(4);
			}
		}

		// �Ƚ��������浽�������ʱ����, �Ա�������жϱ仯 & ͬ�����ͻ���
		uint8_t angle = 0;
		auto speed = 0.0f;

		// ���־���

		// �������Ŀ��ľ���pow2����
		auto tarDistPow2 = xyMath.GetDistancePow2(c.target->xy - c.xy);

		// �ж� ���� �Ƿ��� С�� ��С���־���, �����ƶ�
		if (c.cfg_traceKeepDistanceRangePow2.f > tarDistPow2)
		{
			angle = xyMath.GetAngle(c.xy - c.target->xy);
			speed = c.cfg_moveSpeed;
		}
		// �ж� ���� �Ƿ����Դ��� ��С���־���, �ƶ�( ��ν���Դ�����ָ�������һ֡���ƶ����� )
		else if ((c.cfg_enableTraceKeepDistanceRange && c.cfg_traceKeepDistanceRangePow2.t > tarDistPow2)
			|| ((c.cfg_traceKeepDistanceRange.f + c.cfg_moveSpeed) * (c.cfg_traceKeepDistanceRange.f + c.cfg_moveSpeed) < tarDistPow2))
		{
			angle = xyMath.GetAngle(c.target->xy - c.xy);
			speed = c.cfg_moveSpeed;
		}
		// ����Ҫ�ƶ�
		else
		{
			angle = xyMath.GetAngle(c.target->xy - c.xy);
			speed = 0.0f;
		}

		bool needSync = false;
		// ������
		if (c.moveSpeed != speed || c.moveAngle != angle)
		{
			c.moveSpeed = speed;
			c.moveAngle = angle;
			xyInc = xyMath.GetXyInc(angle) * speed;

			needSync = true;
		}

		// �ƶ�					
		c.xy.Add(xyInc);

		if (needSync)
		{
			// todo: send move msg to client ?
		}

		// ѭ��
		CORO_YIELDTO(2);
	}
	CORO_(3);															// ���س�����
	{
		uint8_t angle = xyMath.GetAngle(c.bornXY - c.xy);
		auto speed = c.cfg_moveSpeed;

		// �ж��Ƿ��Ѿ��ص��˳�����( ����С��һ֡���ƶ����� )
		if (xyMath.GetDistancePow2(c.bornXY - c.xy) <= c.cfg_moveSpeedPow2)
		{
			// ��������Ϊ������, ������Ϣ״̬
			c.xy = c.bornXY;
			CORO_GOTO(0);
		}

		bool needSync = false;
		// ������
		if (c.moveSpeed != speed || c.moveAngle != angle)
		{
			c.moveSpeed = speed;
			c.moveAngle = angle;
			xyInc = xyMath.GetXyInc(c.moveAngle) * c.cfg_moveSpeed;

			needSync = true;
		}

		// �ƶ�					
		c.xy.Add(xyInc);					// todo: ����

		if (needSync)
		{
			// todo: send move msg to client ?
		}

		CORO_YIELDTO(3);
		// todo: ��ʱ���?  
	}
	CORO_(4)															// ���ܺ�ֱ
	{
		if (castStunTicks > s.ticks)
		{
			CORO_YIELDTO(4);
		}
		else
		{
			CORO_YIELDTO(2);
		}
	}
	CORO_END();
	return 0;
}
