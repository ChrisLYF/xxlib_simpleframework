struct MonsterBase;

struct Range
{
	int from, to;
	inline bool Test(int n)
	{
		assert(from <= to);
		return n >= from && n <= to;
	}
};

struct SkillBase : UpdateBase
{
	// ģ������
	static const int cfg_gcd = 3;

	Range cfg_distance;					// ʩ�ž��뷶Χ
	int cfg_damage;						// �˺�ֵ
	int cfg_cd;							// ˲������ӳ�ʱ��( �������� )

	// runtime
	int64_t cd = 0;						// ���ڼ��� cd �� timer
	MonsterBase* owner;					// ���ܹ����ĸ�������
	SkillBase(MonsterBase* owner);

	virtual bool Avaliable();			// ���ص�ǰ�Ƿ���õı�־λ
	virtual void Cast();				// �ͷŸü���, ���� cd, ��Ŀ��Ѫ
	inline virtual int Update() override { return 0; }
};

// ��սƬ��, ������
// ��ʱ������ʩ��, ��������ͷŷ�Χ���޹ֵĻ������κ�����...
// ���߼��ϸü�����ʵ����Χ�ڵ��˲���, �ṩ�� avaliable ���ѵ�ʩ����ʾ
// Ҳ����˵, ��Ծ��弼��, ������һϵ�� test ����: ���赱ǰ�ͷ��������, �����ܵõ���������Ч
// ����˼��, ��Ч��Ϊ: 1. �����б�.  2. ��������   3. ���˺���....
// ���༼���� AI ����, avaliable ûɶ����. ��Ҫ��һ��׷���ü��ܵ� �ͷ� �Լ۱�.
struct SkillNear : SkillBase
{
	SkillNear(MonsterBase* owner);
};

// Զ�̵�ɱ, ��ʩ�ŵ�ʱ����Ҫ owner ����Ŀ��
// Ҳ����˵�ֵ����ϻ���һ�� ���� ����, ���ֵĽ����������Чʱ, 
// ��ǰ���ܿ��õ�����֮һ������( ��һ����ͨ���� cd �ѵ� )
struct SkillFar : SkillBase
{
	SkillFar(MonsterBase* owner);
};
