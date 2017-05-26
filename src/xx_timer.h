#pragma once
#include "xx_mempool.h"

namespace xx
{
	// todo: ToString support ?

	struct TimerManager;
	struct TimerBase : MPObject
	{
		// TimerManager �� Register ʱ������г�Ա
		TimerManager* timerManager = nullptr;	// ָ�������
		TimerBase* prevTimer = nullptr;			// ָ��ͬһ ticks �µ���һ timer
		TimerBase* nextTimer = nullptr;			// ָ��ͬһ ticks �µ���һ timer
		int	timerssIndex = -1;					// λ�ڹ����� timerss ������±�

		virtual void Execute() = 0;				// ʱ�䵽���Ҫִ�е�����( ִ�к��߼��ϻᴥ�� RemoveFromManager ��Ϊ )
		void RemoveFromManager();				// �ӹ��������Ƴ�( �ᴥ�� Release ��Ϊ )
	};

	// �����ڴ�ص� timer ������, ֻ֧�ִ���̳��� TimerBase �ķº�����
	struct TimerManager : MPObject
	{
		TimerBase**				timerss;		// timer ��������
		int						timerssLen;		// timer �������� ����
		int						cursor = 0;		// �����α�

		// �趨ʱ���̶ܿ�
		explicit TimerManager(int timerssLen = 60)
		{
			Init(timerssLen);
		}

		~TimerManager()
		{
			Dispose();
		}
		inline void Dispose()
		{
			if (timerss)
			{
				Clear();
				mempool().Free(timerss);
				timerss = nullptr;
			}
		}
		inline void Clear()
		{
			// �������� ticks ���� Release
			for (int i = 0; i < timerssLen; ++i)
			{
				auto t = timerss[i];
				while (t)
				{
					auto nt = t->nextTimer;	// �Ȱ�ָ����һ�ڵ��ָ��ȡ��, ������� Release �˿��ܾ�ȡ������
					t->Release();			// ���� / ����
					t = nt;					// ����������һ�ڵ�
				};
			}
		}

		// ��ָ�� interval ���� timers ��������һ�� timer( ���ӳ� )
		inline void AddDirect(int interval, TimerBase* t)
		{
			assert(t && interval >= 0 && interval < timerssLen);

			// ���ζ�λ�� timers �±�
			interval += cursor;
			if (interval >= timerssLen) interval -= timerssLen;

			// �������� & ������Ϣ
			t->timerManager = this;
			t->prevTimer = nullptr;
			t->timerssIndex = interval;
			if (timerss[interval])				// �о�������
			{
				t->nextTimer = timerss[interval];
				timerss[interval]->prevTimer = t;
			}
			else
			{
				t->nextTimer = nullptr;
			}
			timerss[interval] = t;				// ��Ϊ����ͷ
		}

		// ��ָ�� interval ���� timers ��������һ�� timer
		inline void Add(int interval, TimerBase* t)
		{
			t->AddRef();
			AddDirect(interval, t);
		}

		// �Ƴ�һ�� timer
		inline void Remove(TimerBase* t)
		{
			assert(t->timerManager && (t->prevTimer || timerss[t->timerssIndex] == t));
			if (t->nextTimer) t->nextTimer->prevTimer = t->prevTimer;
			if (t->prevTimer) t->prevTimer->nextTimer = t->nextTimer;
			else timerss[t->timerssIndex] = t->nextTimer;
			t->timerManager = nullptr;
			t->Release();
		}

		// ���� ��ǰcursor �� timers ֮�� cursor++
		inline void Update()
		{
			// ������ǰ ticks ����ִ��
			auto t = timerss[cursor];
			while (t)
			{
				t->Execute();					// ִ��

				//t->timerManager = nullptr;
				auto nt = t->nextTimer;
				t->Release();
				// if (nt) nt->prevTimer = nullptr;
				t = nt;
			};

			timerss[cursor] = 0;				// �������ͷ
			cursor++;							// �����α�
			if (cursor == timerssLen) cursor = 0;
		}

		// ������� ticks �� timerss
		void Update(int ticks)
		{
			for (int i = 0; i < ticks; ++i) Update();
		}


	protected:
		inline void Init(int timerssLen)
		{
			assert(timerssLen);
			this->timerssLen = timerssLen;
			auto bytesCount = sizeof(TimerBase*) * timerssLen;
			timerss = (TimerBase**)mempool().Alloc(bytesCount);
			memset(timerss, 0, bytesCount);
		}
	};

	inline void TimerBase::RemoveFromManager()
	{
		assert(this->timerManager);
		this->timerManager->Remove(this);
	}

}
