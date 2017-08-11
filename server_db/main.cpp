#include "xx_uv.h"
#include "xx_helpers.h"
#include "pkg\PKG_class.h"
#include "db\DB_sqlite.h"
#include <mutex>
#include <thread>

struct Peer;
struct Service;
struct Listener;
struct TaskManager;
struct Dispacher;

/******************************************************************************/

struct Peer : xx::UVServerPeer
{
	Service* service;

	Peer(Listener* listener, Service* service);
	virtual void OnReceivePackage(xx::BBuffer & bb) override;
	virtual void OnDisconnect() override;
};

struct Listener : xx::UVListener
{
	Service* service;

	Listener(xx::UV* uv, int port, int backlog, Service* service);
	virtual xx::UVServerPeer * OnCreatePeer() override;
};

struct TaskManager : xx::Object
{
	Service* service;
	Dispacher* dispacher = nullptr;
	std::mutex tasksMutex;
	std::mutex resultsMutex;
	xx::Queue_v<std::function<void()>> tasks;
	xx::Queue_v<std::function<void()>> results;
	void AddTask(std::function<void()>&& f);
	void AddTask(std::function<void()> const& f);
	void AddResult(std::function<void()>&& f);
	void AddResult(std::function<void()> const& f);

	TaskManager(Service* service);
	~TaskManager();
	volatile bool running = true;
	volatile bool stoped = false;
	void ThreadProcess();
};
using TaskManager_v = xx::Dock<TaskManager>;

struct Service : xx::Object
{
	xx::MemPool sqlmp;		// �� SQLite ���乤���߳�ʹ�õ� mp
	xx::SQLite_v sqldb;		// SQLite �߶������ڴ��, �����̵߳ķ���

	xx::UV_v uv;
	Listener* listener = nullptr;
	TaskManager_v tm;

	Service();
	int Run();
};

struct Dispacher : xx::UVAsync
{
	TaskManager* tm;

	Dispacher(xx::UV* uv, TaskManager* tm);
	virtual void OnFire() override;
};


/******************************************************************************/

Listener::Listener(xx::UV* uv, int port, int backlog, Service* service)
	: xx::UVListener(uv, port, backlog)
	, service(service)
{
}

xx::UVServerPeer* Listener::OnCreatePeer()
{
	return mempool().Create<Peer>(this, service);
}


/******************************************************************************/


Dispacher::Dispacher(xx::UV* uv, TaskManager* tm)
	: xx::UVAsync(uv)
	, tm(tm)
{
}

void Dispacher::OnFire()
{
	while (!tm->results->Empty())
	{
		tm->results->Top()();
		tm->results->Pop();
	}
}

/******************************************************************************/

TaskManager::TaskManager(Service* service)
	: service(service)
	, tasks(mempool())
	, results(mempool())
{
	this->dispacher = service->uv->CreateAsync<Dispacher>(this);
	if (!this->dispacher) throw nullptr;
	std::thread t([this] { ThreadProcess(); });
	t.detach();
}

TaskManager::~TaskManager()
{
	running = false;
	while (!stoped) Sleep(1);
}

void TaskManager::AddTask(std::function<void()>&& f)
{
	std::lock_guard<std::mutex> lock(tasksMutex);
	tasks->Push(std::move(f));
}

void TaskManager::AddTask(std::function<void()> const& f)
{
	std::lock_guard<std::mutex> lock(tasksMutex);
	tasks->Push(std::move(f));
}

void TaskManager::AddResult(std::function<void()>&& f)
{
	{
		std::lock_guard<std::mutex> lock(resultsMutex);
		results->Push(std::move(f));
	}
	dispacher->Fire();
}

void TaskManager::AddResult(std::function<void()> const& f)
{
	{
		std::lock_guard<std::mutex> lock(resultsMutex);
		results->Push(std::move(f));
	}
	dispacher->Fire();
}

void TaskManager::ThreadProcess()
{
	std::function<void()> func;
	while (running)
	{
		bool gotFunc = false;
		{
			std::lock_guard<std::mutex> lock(tasksMutex);
			gotFunc = tasks->TryPop(func);
		}
		if (gotFunc) func();
		else Sleep(1);
	}
	stoped = true;
}

/******************************************************************************/

Service::Service()
	: sqldb(sqlmp, "data.db")
	, uv(mempool())
	, tm(mempool(), this)
{
	sqldb->Attach("log", "log.db");
	sqldb->SetPragmaJournalMode(xx::SQLiteJournalModes::WAL);
	sqldb->SetPragmaForeignKeys(true);
}

int Service::Run()
{
	// todo: ��� db ���ǲ����½���, �Ǿ�ִ�н���ű�

	listener = uv->CreateListener<Listener>(12345, 128, this);
	if (!listener) return -1;
	uv->Run();
	return 0;
}

/******************************************************************************/

Peer::Peer(Listener* listener, Service* service)
	: xx::UVServerPeer(listener)
	, service(service)
{
}

void Peer::OnDisconnect() {}

void Peer::OnReceivePackage(xx::BBuffer& bb)
{
	// todo: �յ���, ����, ����������ѹ����, ת����̨�߳�ִ��
	// SQLite �߶������ڴ��, �����̵߳ķ���

	// �ڴ���� & ��������( ���� SQLite ֻռ 1 ��, ʹ�� �Լ��� mp ����� ): 
	// 1. uv�߳� �� uvmp ���� SQL�߳� ��Ҫ������, �󽫴�����ѹ�� tasks
	// 2. SQL�߳� ִ���ڼ�, �� sqlmp ���乩 uv�߳� ��������������, �󽫴�����ѹ�� results
	// 3. uv�߳� ��ȡ������ݲ�����, ���� ��1��������ڴ�, �� ��2�����ڴ���պ���ѹ�� tasks
	// 4. tasks ִ�е�2��������ڴ����

	service->tm->AddTask([service = this->service, peer = xx::Ref<Peer>(this)/*, args*/]	// ת�� SQL �߳�
	{
		// ִ�� SQL ���, �õ����
		DB::SQLiteManageFuncs fs(*service->sqldb);
	// auto rtv = fs.execxxxxx( args... )

	service->tm->AddResult([service, peer/*, args, rtv */]	// ת�� uv �߳�
	{
		// args->Release();
		// handle( rtv )
		if (peer && peer->state == xx::UVPeerStates::Connected)	// ��� peer ������, ��һЩ�ط�����
		{
			//mp->SendPackages
		}
		// service->tm->AddTask([rtv]{ rtv->Release(); })	// ת�� SQL �߳�
	});
	});
}

/******************************************************************************/



// ���Խ��, UV_THREADPOOL_SIZE=1 �������, ÿ����ִ�� 100 ���
struct Worker
{
	int i1 = 0, i2 = 0;
	uv_loop_t* loop;
	uv_work_t req;
	Worker(uv_loop_t* loop) : loop(loop) {}
	void Exec()
	{
		uv_queue_work(loop, &req, Worker::WorkCB, Worker::AfterWorkCB);
	}
	static void WorkCB(uv_work_t* w)
	{
		auto self = container_of(w, Worker, req);
		++self->i1;
	}
	static void AfterWorkCB(uv_work_t* w, int status)
	{
		auto self = container_of(w, Worker, req);
		++self->i2;
		if (self->i2 < 10000000)		self->Exec();			
	}
};

int main()
{
	// todo: ���� uv_work
	xx::MemPool mp;
	auto loop = uv_default_loop();
	Worker w(loop);
	w.Exec();
	uv_timer_t t;
	t.data = &w;
	uv_timer_init(loop, &t);
	uv_timer_start(&t, [](auto tp)
	{
		auto& worker = *((Worker*)tp->data);
		std::cout << "i1 = " << worker.i1 << ", i2 = " << worker.i2 << std::endl;
	}, 0, 1000);
	uv_run(loop, UV_RUN_DEFAULT);
	return 0;
}











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

