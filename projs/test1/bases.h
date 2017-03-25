/*

MP �������뺬��������Щ����

typedef xx::MemPool<
.............
Scene,
SceneObjBase,
FSMBase,
xx::List<FSMBase*>, 
................> MP;

*/


struct Scene;
struct SceneObjBase;

// �� Update �麯������Ļ���
struct UpdateBase : xx::MPObject
{
	Scene& scene();							// �﷨��
	Scene& scene() const;
	virtual int Update() = 0;				// ���ط� 0 ��ʾ��ɱ. ����ͨ����������������, ����ͨ����ʾ����
};

// ״̬������. ֻ���� SceneObjBase �ĺ���������. ������ֱ�� Release. ��ɱ���� PopFSM �� Update ���ط� 0
struct FSMBase : UpdateBase
{
	SceneObjBase* owner;
	void Pop();
	FSMBase(SceneObjBase* owner) : owner(owner) {}
};

// �������ӵĻ���( �������� )
struct SceneObjBase : UpdateBase
{
	uint32_t sceneContainerIndex = -1;		// λ�� scene ��ĳ�����е��±� ( List / Links / Dict )

	xx::List_v<FSMBase*> fsmStack;

	FSMBase* currFSM = nullptr;
	FSMBase* deadFSM = nullptr;		// �ӳ�ɱ��
	// todo: �ṩһ�������������ϸ� FSM ��ִ�н��( ��������, �����˳�����, ����¼�����֮�� )

	SceneObjBase();
	~SceneObjBase();

	template<typename T, typename ...Args>
	T* CreateFSM(Args&&...args);
	void SetFSM(FSMBase* fsm);
	void PushFSM(FSMBase* fsm);
	void PopFSM();
	virtual int Update() override;
};
