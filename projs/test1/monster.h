struct MonsterBase;
struct MonsterFSMBase : FSMBase
{
	MonsterBase& ctx();
	MonsterFSMBase(SceneObjBase* owner);
};

struct MonsterFSM_Idle : MonsterFSMBase
{
	int64_t sleepToTicks;
	FSMBase* breakCond = nullptr;

	MonsterFSM_Idle(SceneObjBase* owner);
	void Init(int64_t ticks, FSMBase* breakCond);
	virtual int Update() override;
};

struct MonsterFSM_Move : MonsterFSMBase
{
	int xInc;
	int64_t toTicks;
	FSMBase* breakCond;

	MonsterFSM_Move(SceneObjBase* owner);
	void Init(int xInc, int count, FSMBase* breakCond);
	virtual int Update() override;
};

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

struct MonsterFSM_AlertCondition : MonsterFSMBase
{
	MonsterFSM_AlertCondition(SceneObjBase* owner);
	virtual int Update() override;
};

struct MonsterBase : SceneObjBase
{
	MonsterBase();
	~MonsterBase();

	xx::List_v<SkillBase*, true> skills;		// �ֵļ����б�( �ڹִ���ʱ��̶������� )( LUA ���ɼ� )
	int64_t skillsGcd = 0;						// ���ܵĹ���CD���Ʊ���( ���м��ܶ� check �� )( LUA ���ɼ� )
	int skillCursor = -1;						// AI �ͷż�����ʹ�õ�ѭ���α�( LUA ���ɼ� )

	int cfg_watchDistance = 0;					// �Ӿ�( ���䷶Χ )( ģ��config, �ھ������г�ʼ�� )
	// todo: ���־������, ׷����Χ���, ���������

	int hp = 100;								// �ֵĳ�ʼѪ��( LUA ֻ�� )
	int x = 0;									// �ֵ�����( ��������ƺ�������ȡΪ����һ�ֻ��� )( LUA ֻ�� )
	int originalX = 0;							// �������뾯��״̬ʱ, ��ֵ���ڴ��ԭʼ x �Ա�س�ʱʹ��
	xx::MPtr<MonsterBase> target;				// ��ǰĿ��/����( LUA ֻ�� )

	xx::List_v<FSMBase*, true> fsmConds;		// �洢�Ѵ��������������ж��� lua ״̬��( �Ա����Ŷ������ )

	// ��̬��������״̬( һ����ͨ�� Init ��ʼ������ )
	xx::MemHeaderBox<MonsterFSM_Idle> fsmIdle;
	xx::MemHeaderBox<MonsterFSM_Move> fsmMove;
	xx::MemHeaderBox<MonsterFSM_Cast> fsmCast;
	xx::MemHeaderBox<MonsterFSM_AI> fsmAI;
	xx::MemHeaderBox<MonsterFSM_AlertCondition> fsmAlertCondition;

	int Distance(MonsterBase* other);			// ������Ŀ��ľ���
	xx::MPtr<MonsterBase> SearchTarget();		// ����Ŀ��, �����Ӿ���ҵз�������
	void SetTarget(MonsterBase* target);		// ���õ�ǰ target( �����ָ�������� )
	void Hurt(MonsterBase* attacker);			// ����������һ���¼�֪ͨ������, ��������ִ�м�Ѫ��Ч��
	int AnyAvaliableSkillId();					// ѭ����������, �ҳ�һ�����õļ����±귵��. ��һ�ֶ��Ҳ����ͷ��� -1


	// ����һ�� lua ����������һ�� �����ж���״̬��. ��� Update ���ط� 0 ���ʾִ�����.
	// ������ʵ�� Idle , Move ʱ�� "����" Ч��, ģ���¼�����. 
	// ״̬�� Update �ڼ��������Ľ��, ����Ҫ�� MonsterBase �мӽ�������������ڳ��� / ����.
	xx::MPtr<FSMLua> LuaCondition(char const* luacode);

	// ����״̬�������� fsmConds ��������.
	template<typename T, typename ... Args>
	xx::MPtr<T> Condition(Args&&...args);

	// ����һ�� MonsterFSM_Idle ״̬���� Push. yield ����.
	// ticks Ϊ idle ʱ��. cond Ϊ�� Update ���ط� 0 ʱ, ��Ϊ�������
	void Idle(int64_t ticks, FSMBase* cond);

	// ����һ�� MonsterFSM_Move ״̬���� Push. yield ����.
	// xInc Ϊ x ÿ֡����ֵ. cond Ϊ�� Update ���ط� 0 ʱ, ��Ϊ�������
	void Move(int xInc, int count, FSMBase* cond);

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
