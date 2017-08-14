#include "xx_func.h"
#include "xx_uv.h"
#include "xx_helpers.h"
#include "pkg\PKG_class.h"
#include "db\DB_sqlite.h"


struct Peer;
struct Service;
struct Listener;
struct TaskManager;
struct Dispacher;

/******************************************************************************/

struct Peer : xx::UVServerPeer
{
	// ָ���������
	Service* service;

	Peer(Listener* listener);
	virtual void OnReceivePackage(xx::BBuffer & bb) override;
	virtual void OnDisconnect() override;
};

struct Listener : xx::UVListener
{
	// ָ���������
	Service* service;

	Listener(xx::UV* uv, int port, int backlog, Service* service);
	virtual xx::UVServerPeer * OnCreatePeer() override;
};

struct Worker : xx::UVWorker
{
	// ָ���������
	Service* service;
	Worker(Service* service);

	// �����հ�( �����̴߳��� ��������, �ڹ����߳���ִ�� )
	xx::Func<void()> task;

	// ������� & �������ձհ�( �ɹ����̴߳��� �������, �����߳���ִ�� )
	xx::Func<void()> result;

	// ������ձհ�( �����߳��и�ֵ, �ɹ����߳�ִ��������ϴεĽ�� )
	xx::Func<void()> resultKiller;

	// �ڹ����߳���ִ��
	virtual void OnWork() override;

	// �����߳���ִ��
	virtual void OnAfterWork(int status) override;
};

struct Service : xx::Object
{
	xx::MemPool sqlmp;			// �� SQLite ���乤���߳�ʹ�õ� mp
	xx::SQLite_v sqldb;			// SQLite �߶������ڴ��, �����̵߳ķ���
	DB::SQLiteManageFuncs fs;	// SQL ��伯

	xx::UV_v uv;
	Listener* listener = nullptr;

	xx::Dock<Worker> worker;
	xx::Queue_v<xx::Func<void()>> tasks;
	void AddTask(xx::Func<void()>&& fn);
	void SetResult(xx::Func<void()>&& fn);
	void SetResultKiller(xx::Func<void()>&& fn);

	Service();
	int Run();
};








/******************************************************************************/

inline Listener::Listener(xx::UV* uv, int port, int backlog, Service* service)
	: xx::UVListener(uv, port, backlog)
	, service(service)
{
}

inline xx::UVServerPeer* Listener::OnCreatePeer()
{
	return mempool().Create<Peer>(this);
}


/******************************************************************************/

inline Worker::Worker(Service* service)
	: UVWorker(&*service->uv)
	, service(service)
{
}

inline void Worker::OnWork()
{
	if (resultKiller)					// �������ϴδ����Ľ����
	{
		resultKiller();
		resultKiller = nullptr;
	}

	task();								// ִ������
}

inline void Worker::OnAfterWork(int status)
{
	if (result)							// ��ִ�н������
	{
		result();
		result = nullptr;
	}

	if (service->tasks->TryPop(task))	// ������к�������, ����ȡ����ִ��
	{
		Start();
	}
	else
	{
		task = nullptr;
	}
}

/******************************************************************************/

inline Service::Service()
	: sqldb(sqlmp, "data.db")
	, fs(*sqldb)
	, uv(mempool())
	, tasks(mempool())
	, worker(mempool(), this)
{
	//sqldb->Attach("log", "log.db");
	sqldb->SetPragmaJournalMode(xx::SQLiteJournalModes::WAL);
	sqldb->SetPragmaForeignKeys(true);
}

inline int Service::Run()
{
	// todo: ��� db ���ǲ����½���, �Ǿ�ִ�н���ű�

	listener = uv->CreateListener<Listener>(12345, 128, this);
	if (!listener) return -1;
	uv->Run();
	return 0;
}

inline void Service::AddTask(xx::Func<void()>&& fn)
{
	if (!worker->working)
	{
		assert(!tasks->Count());
		worker->task = std::move(fn);
		worker->Start();
	}
	else
	{
		tasks->Emplace(std::move(fn));
	}
}

inline void Service::SetResult(xx::Func<void()>&& fn)
{
	assert(!worker->result);
	worker->result = std::move(fn);
}

inline void Service::SetResultKiller(xx::Func<void()>&& fn)
{
	assert(!worker->resultKiller);
	worker->resultKiller = std::move(fn);
}

/******************************************************************************/

inline Peer::Peer(Listener* listener)
	: xx::UVServerPeer(listener)
	, service(listener->service)
{
}

inline void Peer::OnDisconnect() {}

#include <thread>

inline void Peer::OnReceivePackage(xx::BBuffer& bb)
{
	// todo: �յ���, ����, ����������ѹ����, ת����̨�߳�ִ��


	// ģ�⴫һ������
	struct Args : xx::Object
	{
		Args()
		{
			std::cout << "Args(), thread id = " << std::this_thread::get_id() << std::endl;
		}
		Args(Args&&) = default;
		xx::String_p un;
		~Args()
		{
			std::cout << "~Args(), thread id = " << std::this_thread::get_id() << std::endl;
		}
	};
	auto args = mempool().CreatePtr<Args>();
	// ��� args .........
	// ...

	// �� SQL �߳�ѹ�뺯��( service ��ָ��, ֱ�Ӹ���. peer ����ȫ��������, ���ݵ� result ������ʹ��. args ������ֵ�ƶ� )
	service->AddTask([service = this->service, peer = xx::Ref<Peer>(this), args = std::move(args)]()
	{
		std::cout << "Task, thread id = " << std::this_thread::get_id() << std::endl;

		// ִ�� SQL ���, �õ����
		// �ڼ�ֻ�ܴ� service->sqlmp �����ڴ�. rtv ����Ϊ���� Args �ļ������ƺ�����
		auto rtv = service->fs.SelectAccountByUsername(args->un);

		// �����߳�ȥ������, ���ý������( service, peer ֱ�Ӹ���, rtv �ƶ�, ˳�㽫 args �ƽ�ȥ�Ա���� )
		service->SetResult([service, peer, rtv = std::move(rtv), args = xx::Move(args)]
		{
			std::cout << "Result, thread id = " << std::this_thread::get_id() << std::endl;

			// handle( rtv )
			if (peer && peer->state == xx::UVPeerStates::Connected)			// ��� peer ������, �ط�
			{
				//peer->SendPackages
			}

			// �������߳�ȥ���ս��, ���ý�����պ���( �� rtv �ƽ�ȥ���� )
			service->SetResultKiller([rtv = xx::Move(rtv)]
			{
				std::cout << "ResultKiller, thread id = " << std::this_thread::get_id() << std::endl;
			});  			
		});
	});
}



/******************************************************************************/

struct Ctx
{
	xx::Func<void()> f1;
	xx::Func<void()> f2;
	xx::Func<void()> f3;
};
struct Foo
{
	bool val;
	Foo(bool val = false) : val(val)
	{
		std::cout << "Foo() val = " << (val ? "true" : "false") << std::endl;
	}
	Foo(Foo const& o) = delete;
	Foo(Foo && o)
		: val(false)
	{
		std::swap(val, o.val);
		std::cout << "Foo(Foo && o)" << std::endl;
	}
	~Foo()
	{
		std::cout << "~Foo() val = " << (val ? "true" : "false") << std::endl;
	}
};

int main()
{
	{
		Ctx ctx;
		{
			Foo foo(true);
			ctx.f1 = [&ctx, foo = xx::Move(foo)]()mutable
			{
				std::cout << "f1()" << std::endl;
				ctx.f2 = [&ctx, foo = xx::Move(foo)]()mutable
				{
					std::cout << "f2()" << std::endl;
					ctx.f3 = [&ctx, foo = xx::Move(foo)]()mutable
					{
						std::cout << "f3()" << std::endl;
					};
					std::cout << "~f2()" << std::endl;
				};
				std::cout << "~f1()" << std::endl;
			};
			std::cout << "1111111111" << std::endl;
			ctx.f1();
			ctx.f1 = nullptr;
			std::cout << "2222222222" << std::endl;
			ctx.f2();
			ctx.f2 = nullptr;
			std::cout << "3333333333" << std::endl;
			ctx.f3();
			ctx.f3 = nullptr;
			std::cout << "4444444444" << std::endl;
		}
		std::cout << "5555555555" << std::endl;
	}

	return 0;
}




//
//
//// ���Խ��, UV_THREADPOOL_SIZE=1 �������, ÿ����ִ�� 100 ���
//
//struct MyUV : xx::UV
//{
//	MyUV() : UV()
//	{
//		Cout("MyUV()\n");
//	}
//	~MyUV()
//	{
//		Cout("~MyUV()\n");
//	}
//};
//
//struct Worker : xx::UVWorker
//{
//	int i1 = 0, i2 = 0;
//	Worker(xx::UV* uv) : UVWorker(uv)
//	{
//		Cout("Worker()\n");
//	}
//	~Worker()
//	{
//		Cout("~Worker()\n");
//	}
//	virtual void OnWork() override
//	{
//		Cout("OnWork before sleep\n");
//		++i1;
//		Sleep(2000000);
//		Cout("OnWork sleeped\n");
//	}
//	virtual void OnAfterWork(int status) override
//	{
//		Cout("OnAfterWork\n");
//		++i2;
//		if (i2 < 10000000) Start();
//	}
//};
//
//struct Timer : xx::UVTimer
//{
//	Worker& w;
//	Timer(xx::UV* uv, Worker& w) : UVTimer(uv), w(w)
//	{
//		Start(1000, 1000);
//	}
//	virtual void OnFire() override
//	{
//		Cout("i1 = ", w.i1, ", i2 = ", w.i2, "\n");
//		uv->Stop();
//	}
//};
//
//int main()
//{
//	// todo: �� Worker ��������Ĵ���
//	xx::MemPool mp;
//	{
//		xx::Dock<MyUV> uv(mp);
//		auto w = uv->CreateWorker<Worker>();
//		w->Start();
//		auto t = uv->CreateTimer<Timer>(*w);
//		uv->Run();
//		mp.Cout("uv stoped.\n");
//	}
//	std::cin.get();
//	return 0;
//}




// todo: �յ���, ����, ����������ѹ����, ת����̨�߳�ִ��
// SQLite �߶������ڴ��, �����̵߳ķ���

// �ڴ���� & ��������( ���� SQLite ֻռ 1 ��, ʹ�� mempool ����� ): 
// 1. uv�߳� ���� SQL�߳� ��Ҫ������, �󽫴�����ѹ�� tasks
// 2. SQL�߳� ִ���ڼ�, ���乩 uv�߳� ��������������, �󽫴�����ѹ�� results
// 3. uv�߳� ��ȡ�������, ���� ��1��������ڴ�, �� ��2�����ڴ���պ���ѹ�� tasks
// 4. tasks ִ�е�2��������ڴ����

// �������̵� �� work thread �Ĳ���:
// 1. ��һ���߳�˽�� tasks ���м� mempool, ���Ϊ TLS
// 2. �������̵� 2 �� ��¼˽�� tasks ָ�� �����´���
// 3. �������̵� 3 �� �� ˽�� tasks ѹ�ڴ���պ���
// 4. �����̳߳���ɨ���� tasks ����, ��ɨ˽�� tasks, ɨ����ִ��

// ���׷�������������, �����ڴ�ػ���ıȽϴ�, �����ʵ���. 

