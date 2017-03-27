struct MonsterBase;
struct MonsterFSMBase : FSMBase
{
	MonsterBase& ctx();
	MonsterFSMBase(SceneObjBase* owner);
};

//struct MonsterFSM_Idle : MonsterFSMBase
//{
//	int64_t sleepToTicks;
//	FSMBase* breakCond = nullptr;
//
//	MonsterFSM_Idle(SceneObjBase* owner);
//	void Init(int64_t ticks, FSMBase* breakCond);
//	virtual int Update() override;
//};
//
//struct MonsterFSM_Move : MonsterFSMBase
//{
//	int xInc;
//	int64_t toTicks;
//	FSMBase* breakCond;
//
//	MonsterFSM_Move(SceneObjBase* owner);
//	void Init(int xInc, int count, FSMBase* breakCond);
//	virtual int Update() override;
//};

struct MonsterFSM_Cast : MonsterFSMBase
{
	int skillIndex;

	MonsterFSM_Cast(SceneObjBase* owner);
	void Init(int skillIndex);
	virtual int Update() override;
};

struct MonsterFSM_AI : MonsterFSMBase
{
	int lineNumber = 0;
	MonsterFSM_AI(SceneObjBase* owner);
	virtual int Update() override;
};

//struct MonsterFSM_AlertCondition : MonsterFSMBase
//{
//	MonsterFSM_AlertCondition(SceneObjBase* owner);
//	virtual int Update() override;
//};

struct MonsterBase : SceneObjBase
{
	MonsterBase();
	~MonsterBase();

	xx::List_v<SkillBase*, true> skills;					// �ֵļ����б�( �ڹִ���ʱ��̶������� )( LUA ���ɼ� )
	int64_t skillsGcd = 0;									// ���ܵĹ���CD���Ʊ���( ���м��ܶ� check �� )( LUA ���ɼ� )
	int skillCursor = -1;									// AI �ͷż�����ʹ�õ�ѭ���α�( LUA ���ɼ� )

	// ģ�� cfg( ��ǰ�� 20 ticks Ϊ 1 �������� )			
	int cfg_alertInterval = 10;								// ������( ÿ������ ticks ִ��һ�ξ����ж� )
	int cfg_alertDistance = 0;								// �������( �ɵ�ǰ������İ뾶 )
	int cfg_moveTimespan = 60;								// ÿ�ƶ�һ������ʱ��
	int cfg_moveInterval = 40;								// ÿ�ƶ�һ�κ����Ϣʱ��
	float cfg_traceMaxDistance = 20;						// ׷ɱ��Զ����( �ɵ�ǰ������İ뾶 )
	int cfg_traceMaxTimespan = 600;							// ׷ɱ�ʱ��( �ӱ��ξ����е�׷ɱ��ʱ������� )
	xx::Range<float> cfg_traceKeepDistanceRange;			// ׷ɱʱ����˱��־���ķ�Χ( ������˱���С�����, �����Զ��֮. Զ��ֱ��ȡ�������ƶ�. ����Ʋ����Ͳ����� )
	//bool cfg_retreatAlert = false;						// ����ʱ����

	// cfg ���غ������
	int cfg_alertDistancePow2 = 0;							// cfg_alertDistance * cfg_alertDistance
	float cfg_traceMaxDistancePow2 = 0;						// cfg_traceMaxDistance * cfg_traceMaxDistance
	xx::Range<float> cfg_traceKeepDistanceRangePow2;		// cfg_traceKeepDistanceRange * cfg_traceKeepDistanceRange

	// ��ͻ���ͬ����ָ�������: ֻ��һ���ƶ���, ���� ����ʼ����, ����, �ٶ�( ����ÿ�� ) ����������. �ٶ�Ϊ0 ����ԭ����Ϣ. ���յ���һ��ָ��ǰ, �ͻ��˳���ģ��.
	// Ҳ����˵, �·����ݵ�ʱ��Ϊ: �ֵ���Ϊ�����ı�, ������Ҹ���ͬ��

	int alertIntervalTicks = 0;								// ��ǰ���� ticks
	int moveTimespanTicks = 0;								// ��ǰ�ƶ� ticks
	int moveIntervalTicks = 0;								// ��ǰ�ƶ���Ϣʱ�� ticks

	// ������Щ���ݻ�ͬ�����ͻ���
	uint8_t moveAngle = 0;									// ��ǰ�ƶ�����
	float moveSpeed = 0;									// ��ǰ�ƶ��ٶ�( ����ٶ���ָ 1 ֡�Ļ��ڽǶ�����Ӧ�� xz �����ĳ˷�ϵ�� )

	int hp = 50;											// �ֵ�Ѫ��( LUA ֻ�� )
	XY xy;													// �ֵĵ�ǰ����( LUA ֻ�� )
	XY bornXY;												// ���ֱ����õ� map ʱ�ĳ�ʼ����( for �س� )
	XY originalXY;											// �������뾯��״̬ʱ��������
	xx::MPtr<MonsterBase> target;							// ��ǰĿ��/����( LUA ֻ�� )

	//xx::List_v<FSMBase*, true> fsmConds;					// �洢�Ѵ��������������ж��� lua ״̬��( �Ա����Ŷ������ )

	// ��̬��������״̬( һ����ͨ�� Init ��ʼ������ )
	//xx::MemHeaderBox<MonsterFSM_Idle> fsmIdle;
	//xx::MemHeaderBox<MonsterFSM_Move> fsmMove;
	//xx::MemHeaderBox<MonsterFSM_AlertCondition> fsmAlertCondition;
	xx::MemHeaderBox<MonsterFSM_Cast> fsmCast;
	xx::MemHeaderBox<MonsterFSM_AI> fsmAI;

	float DistancePow2(MonsterBase* other);					// ������Ŀ��ľ����ƽ��ֵ
	xx::MPtr<MonsterBase> SearchTarget();					// ����Ŀ��, �����Ӿ���ҵз�������
	void SetTarget(MonsterBase* target);					// ���õ�ǰ target( �����ָ�������� )
	void Hurt(MonsterBase* attacker);						// ���û�� target ������ set target. ��������ִ�м�Ѫ��Ч��
	int TakeAvaliableSkillId();								// ѭ����������, �ҳ�һ�����õļ����±귵��. ��һ�ֶ��Ҳ����ͷ��� -1


	//// ����һ�� lua ����������һ�� �����ж���״̬��. ��� Update ���ط� 0 ���ʾִ�����.
	//// ������ʵ�� Idle , Move ʱ�� "����" Ч��, ģ���¼�����. 
	//// ״̬�� Update �ڼ��������Ľ��, ����Ҫ�� MonsterBase �мӽ�������������ڳ��� / ����.
	//xx::MPtr<FSMLua> LuaCondition(char const* luacode);

	//// ����״̬�������� fsmConds ��������.
	//template<typename T, typename ... Args>
	//xx::MPtr<T> Condition(Args&&...args);

	//// ����һ�� MonsterFSM_Idle ״̬���� Push. yield ����.
	//// ticks Ϊ idle ʱ��. cond Ϊ�� Update ���ط� 0 ʱ, ��Ϊ�������
	//void Idle(int64_t ticks, FSMBase* cond);

	//// ����һ�� MonsterFSM_Move ״̬���� Push. yield ����.
	//// xInc Ϊ x ÿ֡����ֵ. cond Ϊ�� Update ���ط� 0 ʱ, ��Ϊ�������
	//void Move(int xInc, int count, FSMBase* cond);

	// ����һ�� MonsterFSM_Cast ״̬���� Push. yield ����.
	// skillIndex Ϊ �����±�. �����ܲ�����ϻ���ֹʱ��״̬������
	void Cast(int skillIndex);
};


struct Monster1 : MonsterBase
{
	Monster1();
};
struct Monster2 : MonsterBase
{
	Monster2();
};
