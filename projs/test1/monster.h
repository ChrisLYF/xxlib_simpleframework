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
	int64_t alertInterval = 0;				// ��ǰ���� ticks. �������������
	int moveCount = 0;						// �ƶ��������� for ���κ����������ƶ� 1 ��
	int64_t moveTicks;						// ��ǰ�ƶ� / ��Ϣ ticks( ���캯���г�ʼ�� )
	XY xyInc;								// �ƶ�����
	XY targetXyBak;							// ����Ŀ�������Է����ƶ��ж�
	int64_t castStunTicks = 0;				// ʹ�ü�����ɵĽ�ֱ ticks ������

	bool Alert();							// �Ծ���һ��( ��� cd ���˵Ļ� ), �������Ŀ��ͷ��� true
	bool stateIsChanged = false;			// �����ж��Ƿ�Ϊ�״�move��idle

	MonsterFSM_AI(SceneObjBase* owner);
	int lineNumber = 0;						// stackless Э���к�
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
	int skillCursor = -1;									// AI �ͷż�����ʹ�õ�ѭ���α�( LUA ���ɼ� )
	int64_t skillsGcd = 0;									// ���ܵĹ��� CD ticks( LUA ���ɼ� )

	// ģ�� cfg( ��ǰ�� 20 ticks Ϊ 1 �������� )			
	int cfg_alertInterval;									// ������( ÿ������ ticks ִ��һ�ξ����ж� )
	int cfg_moveTimespan;									// ÿ�ƶ�һ������ʱ��
	int cfg_moveInterval;									// ÿ�ƶ�һ�κ����Ϣʱ��
	int cfg_moveBackInterval;								// ÿ���ٴ��ƶ���Ϊ֮��, ��Ȼ��һ�����Գ�������ƶ�
	int cfg_traceMaxTimespan;								// ׷ɱ�ʱ��( �ӱ��ξ����е�׷ɱ��ʱ������� )
	float cfg_traceMaxDistance;								// ׷ɱ��Զ����( �ɵ�ǰ������İ뾶 )
	float cfg_alertDistance;								// �������( �ɵ�ǰ������İ뾶 )
	xx::Range<float> cfg_traceKeepDistanceRange;			// ׷ɱʱ����˱��־���ķ�Χ( ������˱���С�����, �����Զ��֮. Զ��ֱ��ȡ�������ƶ�. ����Ʋ����Ͳ����� )
	float cfg_moveSpeed;									// ÿ֡�ƶ�����
	float cfg_radius;										// ����뾶( AI ��Ŀ���ƶ�ʱ �����������ص�Ϊ��С���뱣�� )
	//bool cfg_retreatAlert = false;						// ����ʱ����

	// cfg ���غ������
	float cfg_moveSpeedPow2;								// cfg_moveSpeedPow2 * cfg_moveSpeedPow2
	float cfg_alertDistancePow2;							// cfg_alertDistance * cfg_alertDistance
	float cfg_traceMaxDistancePow2;							// cfg_traceMaxDistance * cfg_traceMaxDistance
	xx::Range<float> cfg_traceKeepDistanceRangePow2;		// cfg_traceKeepDistanceRange * cfg_traceKeepDistanceRange

	// ��ͻ���ͬ����ָ�������: ֻ��һ���ƶ���, ���� ����ʼ����, ����, �ٶ�( ����ÿ�� ) ����������. �ٶ�Ϊ0 ����ԭ����Ϣ. ���յ���һ��ָ��ǰ, �ͻ��˳���ģ��.
	// Ҳ����˵, �·����ݵ�ʱ��Ϊ: �ֵ���Ϊ�����ı�, ������Ҹ���ͬ��

	// ������ 2 ����ͬ�����ͻ���
	uint8_t moveAngle;										// ��ǰ�ƶ�����
	float moveSpeed;										// ��ǰ�ƶ��ٶ�( ����ٶ���ָ 1 ֡�Ļ��ڽǶ�����Ӧ�� xz �����ĳ˷�ϵ�� ). �ͻ���ͨ�����ֵ���жϹֵ�ǰ�Ƿ������ƶ�.

	XY xy;													// �ֵĵ�ǰ����( LUA ֻ�� )
	XY bornXY;												// ���ֱ����õ� map ʱ�ĳ�ʼ����( for �س� )
	XY originalXY;											// �������뾯��״̬ʱ��������
	int hp;													// �ֵ�Ѫ��( LUA ֻ�� )
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

	//// ����һ�� MonsterFSM_Cast ״̬���� Push. yield ����.
	//// skillIndex Ϊ �����±�. �����ܲ�����ϻ���ֹʱ��״̬������
	//void Cast(int skillIndex);
};


struct Monster1 : MonsterBase
{
	Monster1();
};
struct Monster2 : MonsterBase
{
	Monster2();
};
