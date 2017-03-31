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
	cfg_alertInterval = 10;
	cfg_alertDistance = 20;
	cfg_moveTimespan = 10;
	cfg_moveInterval = 5;
	cfg_moveBackInterval = 5;
	cfg_traceMaxDistance = 30;
	cfg_traceMaxTimespan = 400;
	cfg_moveSpeed = 0.3f;
	cfg_radius = 1.0f;
	cfg_traceKeepDistanceRange = { 5.0f, 15.0f };
	cfg_enableTraceKeepDistanceRange = true;
	cfg_isInitiative = true;

	// ģ�� �������� ��������
	scene().Create<SkillFar>(this);

	// �����ּ��� cache ����
	cfg_moveSpeedPow2 = cfg_moveSpeed * cfg_moveSpeed;
	cfg_alertDistancePow2 = cfg_alertDistance * cfg_alertDistance;
	cfg_traceMaxDistancePow2 = cfg_traceMaxDistance * cfg_traceMaxDistance;

	// ��ʼ������ʱ����
	moveSpeed = 0;										// �ٶ�Ϊ 0 ��ʾ��Ϣ
	moveAngle = (uint8_t)scene().NextInteger(0, 256);	// ����Ƕ�

	xy = { (float)scene().NextDouble(0, scene().mapSize.w), (float)scene().NextDouble(0, scene().mapSize.h) };
	bornXY = xy;
	originalXY = xy;

	hp = 100;


	// ģ�� �������� �����ʼAI
	fsmAI->Init();
	SetFSM(fsmAI);

	// ���� LUA ���ʼ AI
	//SetFSM(CreateFSM<FSMLua>(this, "monster1.lua"));
}


// δ���б��־���Ľ�ս��
Monster2::Monster2() : MonsterBase()
{
	// ģ�� ��������
	cfg_alertInterval = 10;
	cfg_alertDistance = 20;
	cfg_moveTimespan = 10;
	cfg_moveInterval = 5;
	cfg_moveBackInterval = 5;
	cfg_traceMaxDistance = 30;
	cfg_traceMaxTimespan = 400;
	cfg_moveSpeed = 0.4f;
	cfg_radius = 0.7f;
	cfg_traceKeepDistanceRange = { 0, 0 };
	cfg_enableTraceKeepDistanceRange = false;
	cfg_isInitiative = true;


	// ģ�� �������� ��������
	scene().Create<SkillNear>(this);

	// �����ּ��� cache ����
	cfg_moveSpeedPow2 = cfg_moveSpeed * cfg_moveSpeed;
	cfg_alertDistancePow2 = cfg_alertDistance * cfg_alertDistance;
	cfg_traceMaxDistancePow2 = cfg_traceMaxDistance * cfg_traceMaxDistance;

	// ��ʼ������ʱ����
	moveSpeed = 0;										// �ٶ�Ϊ 0 ��ʾ��Ϣ
	moveAngle = (uint8_t)scene().NextInteger(0, 256);	// ����Ƕ�

	xy = { (float)scene().NextDouble(0, scene().mapSize.w), (float)scene().NextDouble(0, scene().mapSize.h) };
	bornXY = xy;
	originalXY = xy;

	hp = 100;

	// ģ�� �������� �����ʼAI
	fsmAI->Init();
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
		if (d <= cfg_alertDistancePow2 && minDistance > d)
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




void MonsterBase::ToString(xx::String &str) const
{
	str.Append("{ id : ", pureVersionNumber(), ", hp : ", hp, ", xy : [ ");
	str.Reserve(str.dataLen + 100);
	str.dataLen += std::sprintf(str.buf + str.dataLen, "%.2f, %.2f", xy.x, xy.y);
	str.Append(" ] }");
}





MonsterFSM_AI::MonsterFSM_AI(SceneObjBase* owner) : FSMBase(owner) {}

void MonsterFSM_AI::Init()
{
	auto& c = *(MonsterBase*)owner;
	moveTicks = scene().ticks + c.cfg_moveInterval;		// ����������Ϣ
}

// �Ծ���һ��( ���Ϊ������, �� cd ���˵Ļ� ). �� idle / move ״̬���������Ŀ��Ҳ���� true
bool MonsterFSM_AI::Alert()
{
	auto& c = *(MonsterBase*)owner;
	auto& s = scene();
	if (c.target) return true;
	if (c.cfg_isInitiative && alertInterval <= s.ticks)
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
			auto& str = *s.err;	// ������ʱƴ�ӷ�������
			str.AppendFormat("mov {0} {1} {2} {3} {4} {5} {6} {7}", c.pureVersionNumber(), c.xy.x, 0, c.xy.y, c.cfg_radius, 0, 0, 0);
			s.udp->SetAddress("10.1.1.99", 6066);
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
			str.Append("act ", c.pureVersionNumber(), " idle");
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
		}

		// ��� idle ��ʱ�䵽��, ���е� move
		if (moveTicks <= s.ticks)
		{
			moveTicks = s.ticks + c.cfg_moveTimespan;
			CORO_YIELDTO(1);
		}

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

		bool needSync = false;

		// �մ� idle �й���? ȷ��δ�� moveTicks ʱ���ڵ��ƶ�����
		if (c.moveSpeed == 0.0f)
		{
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

			// ������
			if (c.moveSpeed != speed || c.moveAngle != angle)
			{
				c.moveSpeed = speed;
				c.moveAngle = angle;
				xyInc = xyMath.GetXyInc(c.moveAngle) * c.moveSpeed;

				needSync = true;
			}
		}

		// �ƶ�		// todo: ��Ҫ��� xy + inc ֮��ĵ��Ƿ�Ϸ�. ��Ҫ����ת xyInc ����
		c.xy.Add(xyInc);				// todo: ��� moveCount == 0 �ڼ��ƶ����赲, �����õ������ܷ��س�����

		if (needSync)
		{
			// todo: send move msg to client ?
			auto& str = *s.err;	// ������ʱƴ�ӷ�������
			str.AppendFormat("mov {0} {1} {2} {3} {4} {5} {6} {7}", c.pureVersionNumber(), c.xy.x, 0, c.xy.y, c.cfg_radius, xyInc.x * 20, 0, xyInc.y * 20);
			s.udp->SetAddress("10.1.1.99", 6066);
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
			str.Append("act ", c.pureVersionNumber(), " move");
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
		}

		// ��� move ��ʱ�䵽��, ���е� idle
		if (moveTicks <= s.ticks)
		{
			moveTicks = s.ticks + c.cfg_moveInterval;
			CORO_YIELDTO(0);
		}

		// ѭ��
		CORO_YIELDTO(1);
	}
	CORO_(2);													// ׷ɱ( �������ӵ���������Ŀ��ǰ���İ汾 )
	{
		// ���ܺ�ֱ
		if (castStunTicks > s.ticks)
		{
			CORO_YIELDTO(2);
		}

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
				if (c.moveSpeed != 0.0f)
				{
					auto& str = *s.err;	// ������ʱƴ�ӷ�������
					str.AppendFormat("mov {0} {1} {2} {3} {4} {5} {6} {7}", c.pureVersionNumber(), c.xy.x, 0, c.xy.y, c.cfg_radius, 0, 0, 0);
					s.udp->SetAddress("10.1.1.99", 6066);
					s.udp->Send(str.buf, str.dataLen);
					str.Clear();
					str.Append("act ", c.pureVersionNumber(), " cast_stun");
					s.udp->Send(str.buf, str.dataLen);
					str.Clear();
				}
				c.moveSpeed = 0.0f;

				castStunTicks = s.ticks + skill->cfg_castStunTimespan;
				CORO_YIELDTO(2);
			}
		}

		// �Ƚ��������浽�������ʱ����, �Ա�������жϱ仯 & ͬ�����ͻ���
		uint8_t angle = 0;
		auto speed = 0.0f;


		// �������Ŀ��� ���ĵ����pow2 ����
		auto tarDistPow2 = xyMath.GetDistancePow2(c.target->xy - c.xy);

		// Զ������ & pow2 Ԥ����
		float rf, rt, rfPow2, rtPow2;
		if (c.cfg_enableTraceKeepDistanceRange)						// ���־���
		{
			// Ϊ��������, ��������С��֡�ƶ�����, �����ƶ�
			rf = c.cfg_traceKeepDistanceRange.f + c.target->cfg_radius + c.cfg_moveSpeed;
			rt = c.cfg_traceKeepDistanceRange.t + c.target->cfg_radius - c.cfg_moveSpeed;
			if (rt < rf) rt = rf + c.cfg_moveSpeed * 4;				// �����ݲ�
			rfPow2 = rf * rf;
			rtPow2 = rt * rt;
		}
		else
		{
			rf = c.cfg_radius + c.target->cfg_radius + c.cfg_moveSpeed;
			rt = c.cfg_radius + c.target->cfg_radius + c.cfg_moveSpeed * 3;
			rfPow2 = rf * rf;
			rtPow2 = rt * rt;
		}

		if (rfPow2 <= tarDistPow2 && tarDistPow2 <= rtPow2)		// ��Χ��, ����Ҫ�ƶ�
		{
			angle = xyMath.GetAngle(c.target->xy - c.xy);
			speed = 0.0f;
		}
		else if (rfPow2 >= tarDistPow2)							// ��̫��, ����
		{
			angle = xyMath.GetAngle(c.xy - c.target->xy);
			speed = c.cfg_moveSpeed;
		}
		else													// ��̫Զ, �ӽ�
		{
			angle = xyMath.GetAngle(c.target->xy - c.xy);
			speed = c.cfg_moveSpeed;
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
			auto& str = *s.err;	// ������ʱƴ�ӷ�������
			str.AppendFormat("mov {0} {1} {2} {3} {4} {5} {6} {7}", c.pureVersionNumber(), c.xy.x, 0, c.xy.y, c.cfg_radius, xyInc.x * 20, 0, xyInc.y * 20);
			s.udp->SetAddress("10.1.1.99", 6066);
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
			str.Append("act ", c.pureVersionNumber(), " trace");
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();

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
			// ��������Ϊ������
			c.xy = c.bornXY;

			// ����� ���س����� ·�ϱ���, �ᵼ������Ŀ��, Ϊ���� Alert ����, ��������һ��
			c.target = nullptr;

			// ������Ϣ״̬
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
			auto& str = *s.err;	// ������ʱƴ�ӷ�������
			str.AppendFormat("mov {0} {1} {2} {3} {4} {5} {6} {7}", c.pureVersionNumber(), c.xy.x, 0, c.xy.y, c.cfg_radius, xyInc.x * 20, 0, xyInc.y * 20);
			s.udp->SetAddress("10.1.1.99", 6066);
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
			str.Append("act ", c.pureVersionNumber(), " turnback");
			s.udp->Send(str.buf, str.dataLen);
			str.Clear();
		}

		CORO_YIELDTO(3);
		// todo: ��ʱ���?  
	}
	CORO_END();
	return 0;
}
