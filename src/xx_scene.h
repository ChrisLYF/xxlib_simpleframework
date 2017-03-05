#pragma once
#include "xx_list.h"

// todo: ��Ϊ .h + .hpp, ȥ�������ռ�. ��Ϊ�������ļ� / sample ����. �� xx ���Ƴ�.
// .hpp ������ʱ��Ҫ����߱���Ϊ MP ���ڴ�� type
// CreateToWithoutTypeId(fsmStack) ��Ϊ�� id �İ汾

namespace xx
{
	// �� Update �麯������Ļ���
	struct UpdateBase : MPObject
	{
		// ���ط� 0 ��ʾ��ɱ. ����ͨ����������������, ����ͨ����ʾ����
		virtual int Update() = 0;
	};

	struct SceneBase;
	struct SceneObjBase;

	// ״̬������. ֻ���� SceneObjBase �ĺ�����������ɱ��. ������ֱ�� Release
	struct FSMBase : UpdateBase
	{
		SceneObjBase* owner;
	};

	// �������ӵĻ���( �������� ). ֻ���� SceneBase �ĺ�����������ɱ��. ������ֱ�� Release
	struct SceneObjBase : UpdateBase
	{
		// ָ�򳡾�����
		SceneBase* sceneBase;

		// λ�ڳ����������е��±�( for ����ʽ��ɾ )
		uint32_t sceneObjsIndex;

		// todo: �ṩһ�������������ϸ� FSM ��ִ�н��( ��������, �����˳�����, ����¼�����֮�� )
		List<FSMBase*>* fsmStack;
		FSMBase* currFSM = nullptr;
		FSMBase* deadFSM = nullptr;		// �ӳ�ɱ��

		SceneObjBase()
		{
			mempoolbase().CreateToWithoutTypeId(fsmStack);
		}

		template<typename MP, typename T, typename ...Args>
		T* CreateFSM(Args&&...args)
		{
			static_assert(std::is_base_of<FSMBase, T>::value, "the T must be inherit of FSMBase.");
			auto p = mempool<MP>().Create<T>(std::forward<Args>(args)...);
			p->owner = this;
			return p;
		}
		inline void SetFSM(FSMBase* fsm)
		{
			assert(fsm);
			assert(!deadFSM);
			deadFSM = currFSM;
			currFSM = fsm;
		}
		inline void PushFSM(FSMBase* fsm)
		{
			fsmStack->Add(currFSM);
			currFSM = fsm;
		}
		inline void PopFSM()
		{
			assert(!deadFSM);
			deadFSM = currFSM;
			currFSM = fsmStack->Top();
			fsmStack->Pop();
		}
		inline virtual int Update() override
		{
			auto rtv = currFSM->Update();
			assert(currFSM);				// ��������ɱ
			if (deadFSM)
			{
				deadFSM->Release();
				deadFSM = nullptr;
			}
			return rtv;
		}
		~SceneObjBase()
		{
			if (deadFSM)
			{
				deadFSM->Release();
				deadFSM = nullptr;
			}
			if (currFSM)
			{
				currFSM->Release();
				currFSM = nullptr;
			}
			while (fsmStack->dataLen)
			{
				fsmStack->Top()->Release();
				fsmStack->Pop();
			}
			fsmStack->Release();
		}
	};

	// ��������( �������ڴ��, ��������, LUA State )
	struct SceneBase : UpdateBase
	{
		// ���� SceneObjBase* ��Ψһ��������. ֻ���� SceneBase �� Create, Release ������
		List<SceneObjBase*, true>* objs;

		SceneBase()
		{
			mempoolbase().CreateToWithoutTypeId(objs);
		}
		~SceneBase()
		{
			if (objs)
			{
				objs->Release();
				objs = nullptr;
			}
		}

		template<typename MP, typename T, typename...Args>
		MPtr<T> Create(Args&&...args)
		{
			auto p = mempool<MP>().Create<T>(std::forward<Args>(args)...);
			p->sceneBase = this;
			p->sceneObjsIndex = (uint32_t)objs->dataLen;
			objs->AddDirect(p);
			return p;
		}

		template<typename MP, typename T, typename...Args>
		void CreateTo(MPtr<T>& tar, Args&&...args)
		{
			tar = Create<MP, T>(std::forward<Args>(args)...);
		}

		// ��Ŀ�����������, �� objs �Ƴ�. ʹ�ý����Ƴ���, �����Ƿ������������Ϊ
		inline bool Release(SceneObjBase* p)
		{
			auto idx = p->sceneObjsIndex;
			if (objs->SwapRemoveAt(idx))
			{
				objs->At(idx)->sceneObjsIndex = idx;
				return true;
			}
			return false;
		}
	};

}
