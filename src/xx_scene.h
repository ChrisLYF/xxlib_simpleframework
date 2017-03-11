/*

MP �������뺬��������Щ����

typedef xx::MemPool<
.............
SceneObjBase,
FSMBase,
xx::List<FSMBase*>, 
xx::List<SceneObjBase*, true> 
................> MP;

*/

#ifndef SCENE_TYPE_NAME
#define SCENE_TYPE_NAME Scene
#endif

struct SCENE_TYPE_NAME;
struct SceneObjBase;

// �� Update �麯������Ļ���
struct UpdateBase : xx::MPObject
{
	SCENE_TYPE_NAME& scene();
	SCENE_TYPE_NAME& scene() const;
	// ���ط� 0 ��ʾ��ɱ. ����ͨ����������������, ����ͨ����ʾ����
	virtual int Update() = 0;
};


// ״̬������. ֻ���� SceneObjBase �ĺ���������. ������ֱ�� Release. ��ɱ���� PopFSM �� Update ���ط� 0
struct FSMBase : UpdateBase
{
	SceneObjBase* owner;
	FSMBase(SceneObjBase* owner) : owner(owner) {}
};

// �������ӵĻ���( �������� )
struct SceneObjBase : UpdateBase
{
	// λ�� scene ��ĳ�����е��±� ( List / Links / Dict )
	uint32_t sceneContainerIndex = -1;

	// todo: �ṩһ�������������ϸ� FSM ��ִ�н��( ��������, �����˳�����, ����¼�����֮�� )
	xx::List<FSMBase*>* fsmStack;
	FSMBase* currFSM = nullptr;
	FSMBase* deadFSM = nullptr;		// �ӳ�ɱ��

	SceneObjBase();
	~SceneObjBase();

	template<typename T, typename ...Args>
	T* CreateFSM(Args&&...args);
	void SetFSM(FSMBase* fsm);
	void PushFSM(FSMBase* fsm);
	void PopFSM();
	virtual int Update() override;
};
