struct MonsterBase;
struct MonsterFSM_AI : FSMBase
{
	int64_t alertInterval = 0;				// ��ǰ���� ticks. �������������
	int moveCount = 0;						// �ƶ��������� for ���κ����������ƶ� 1 ��
	int64_t moveTicks;						// ��ǰ�ƶ� / ��Ϣ ticks( ���캯���г�ʼ�� )
	XY xyInc;								// �ƶ�����
	int64_t castStunTicks = 0;				// ʹ�ü�����ɵĽ�ֱ ticks ������
	int64_t traceTicks = 0;					// ׷ɱ��ʱ ticks( ״̬�л�ʱ��ʼ�� )
	bool Alert();							// �Ծ���һ��( ��� cd ���˵Ļ� ), �������Ŀ��ͷ��� true

	MonsterFSM_AI(SceneObjBase* owner);		// ���캯�����������ڻ��޷���ȡ owner �ĳ�Ա( δ��ʼ�� )
	void Init();							// �� SetFSM ǰ�����Գ�ʼ�������һЩ����ֵ
	int lineNumber = 0;						// stackless Э���к�
	virtual int Update() override;
};

struct MonsterBase : SceneObjBase
{
	MonsterBase();
	~MonsterBase();
	virtual void ToString(xx::String &str) const override;

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
	bool cfg_enableTraceKeepDistanceRange;					// �Ƿ����� ���־���
	bool cfg_isInitiative = true;							// �Ƿ�Ϊ������( �������� )

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

	xx::MemHeaderBox<MonsterFSM_AI> fsmAI;					// ��ֵ���ͷ�ʽ������ʹ��

	float DistancePow2(MonsterBase* other);					// ������Ŀ��ľ����ƽ��ֵ
	xx::MPtr<MonsterBase> SearchTarget();					// ����Ŀ��, �����Ӿ���ҵз�������
	void SetTarget(MonsterBase* target);					// ���õ�ǰ target( �����ָ�������� )
	void Hurt(MonsterBase* attacker);						// ���û�� target ������ set target. ��������ִ�м�Ѫ��Ч��
	xx::MPtr<SkillBase> TakeAvaliableSkill();				// ѭ����������, �ҳ�һ�����õļ����±귵��. ��һ�ֶ��Ҳ����ͷ��ؿ�

};


struct Monster1 : MonsterBase
{
	Monster1();
};
struct Monster2 : MonsterBase
{
	Monster2();
};
