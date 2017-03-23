
struct MonsterFSM_Idle : FSMBase
{
	int64_t sleepToTicks;
	FSMLua* breakCond;

	MonsterFSM_Idle(SceneObjBase* owner);
	void Init(int64_t ticks, FSMLua* breakCond);
	virtual int Update() override;
};

struct MonsterFSM_Move : FSMBase
{
	int xInc;
	FSMLua* breakCond;

	MonsterFSM_Move(SceneObjBase* owner);
	void Init(int xInc, FSMLua* breakCond);
	virtual int Update() override;
};

struct MonsterFSM_Cast : FSMBase
{
	int skillIndex;

	MonsterFSM_Cast(SceneObjBase* owner);
	void Init(int skillIndex);
	virtual int Update() override;
};

struct MonsterBase : SceneObjBase
{
	MonsterBase();
	~MonsterBase();

	xx::MPtr<MonsterBase> target;			// ��ǰĿ��/����
	xx::List_v<SkillBase*, true> skills;	// �ֵļ����б�( �ڹִ���ʱ��̶������� )
	int64_t skillsGcd = 0;					// ���ܵĹ���CD���Ʊ���( Ҳ����˵���м��ܶ���ȥ check �� )
	int skillCursor = -1;					// AI �ͷż�����ʹ�õ�ѭ���α�

	int cfg_watchDistance = 0;				// �Ӿ�( ���䷶Χ )( ģ��config, �ھ������г�ʼ�� )

	int hp = 100;							// �ֵĳ�ʼѪ��
	int x = 0;								// �ֵ�����( ��������ƺ�������ȡΪ����һ�ֻ��� )

	// Ԥ���� fsms ����
	xx::MemHeaderBox<MonsterFSM_Idle> fsmIdle;
	xx::MemHeaderBox<MonsterFSM_Move> fsmMove;
	xx::MemHeaderBox<MonsterFSM_Cast> fsmCast;

	// �洢�Ѵ��������������ж��� lua ״̬��( �Ա����Ŷ������ )
	xx::List_v<FSMLua*, true> conds;


	// ������Ŀ��ľ���
	int Distance(MonsterBase* other);		

	// ����Ŀ��, �����Ӿ���ҵз�, �ҵ������� target ������ true. �Ҳ������� false
	bool SearchTarget();


	// todo: �����¼���� target ����

	// todo: ��Ҫһ���� lua ����Է����ȡ�� �¼�֪ͨ����.
	// todo: �о�����Ϊ lua �ṩ  GetMessageCount   , GetMessageByIndex   ֮��ĺ���
	// todo: ������ lua �Ϳ��Ա��� ����һ������.  Message ��ĳ�����Ҫ��һ�����


	// ѭ����������, �ҳ�һ�����õļ����±귵��. ��һ�ֶ��Ҳ����ͷ��� -1
	int ChooseOneAvaliableSkill();

	// ����һ�� lua ����������һ�� �����ж���״̬��. ��� Update ���ط� 0 ���ʾִ�����.
	// ������ʵ�� Idle , Move ʱ�� "����" Ч��, ģ���¼�����. 
	// ״̬�� Update �ڼ��������Ľ��, ����Ҫ�� MonsterBase �мӽ�������������ڳ��� / ����.
	xx::MPtr<FSMLua> Cond(char const* luacode);

	// ����һ�� MonsterFSM_Idle ״̬���� Push. yield ����.
	// ticks Ϊ idle ʱ��. cond Ϊ�� Update ���ط� 0 ʱ, ��Ϊ�������
	void Idle(int64_t ticks, FSMLua* cond);

	// ����һ�� MonsterFSM_Move ״̬���� Push. yield ����.
	// xInc Ϊ x ÿ֡����ֵ. cond Ϊ�� Update ���ط� 0 ʱ, ��Ϊ�������
	void Move(int xInc, FSMLua* cond);

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
