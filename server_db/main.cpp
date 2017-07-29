#include "xx_uv.h"
#include "xx_helpers.h"
#include "pkg\PKG_class.h"
#include "db\DB_class.h"
#include <xx_sqlite.h>
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
};

struct Listener : xx::UVListener
{
	Service* service;

	Listener(xx::UV* uv, int port, int backlog, Service* service);
	virtual xx::UVServerPeer * OnCreatePeer() override;
};

struct TaskManager : xx::MPObject
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

struct Service : xx::MPObject
{
	xx::UV_v uv;
	xx::SQLite_v sqldb;
	Listener* listener = nullptr;
	xx::MemHeaderBox<TaskManager> tm;

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
	: uv(mempool())
	, sqldb(mempool(), "data.db")
	, tm(mempool(), this)
{
	sqldb->SetPragmas(xx::SQLiteJournalModes::WAL);
}

int Service::Run()
{
	// todo: ��� db ���ǲ����½���, �Ǿ�ִ�н���ű�
	// todo: ��ȥдһ�������� for sqlite

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

void Peer::OnReceivePackage(xx::BBuffer& bb)
{
	// todo: �յ���, ����, ����������ѹ����, ת����̨�߳�ִ��
	// SQLite �߶������ڴ��, �����̵߳ķ���

	// �ڴ��������: 
	// 1. uv�߳� ���� SQL�߳� ��Ҫ������, �󽫴�����ѹ�� tasks
	// 2. SQL�߳� ִ���ڼ�, ���乩 uv�߳� ��������������, �󽫴�����ѹ�� results
	// 3. uv�߳� ��ȡ�������, ���� ��1��������ڴ�, �� ��2�����ڴ���պ���ѹ�� tasks
	// 4. tasks ִ�е�2��������ڴ����

	
	service->tm->AddTask([service = this->service, peer = xx::MPtr<Peer>(this)/*, args*/]	// ת�� SQL �߳�
	{
		// ִ�� SQL ���, �õ����
		// auto rtv = sqlfs.execxxxxx( args... )

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




namespace xx
{
	struct SQLiteString;

	// ��������ģ��
	template<typename T, typename ENABLE = void>
	struct SQLiteAppend
	{
		static uint32_t Exec(SQLiteString* ss, T const &in, bool ignoreReadOnly)
		{
			assert(false);
			return 0;
		}
	};

	struct SQLiteString : String
	{
		SQLiteString() : String() {}

		// ���ڲ�������
		template<typename T>
		void SQLAppend(T const& v)
		{
			static_assert(std::is_arithmetic<T>::value, "");
			Append(v);
		}

		// �����ִ��ᱻת��
		void SQLAppend(char const* const& v)
		{
			if (!v)
			{
				Append("null");
				return;
			}
			// todo: ת��
			//Append("'" + v.Replace("'", "''") + "'");
		}

		void SQLAppend(String* const& v)
		{
		}

		// ���� BLOB
		void SQLAppend(BBuffer* const& v)
		{
			if (!v)
			{
				Append("null");
				return;
			}
			// todo: �� v ת�� 0xXX ( ����д���һ���о�, ������Ҫת? )
		}

		// List ������
		template<typename T>
		void SQLAppend(List<T>* const& os)
		{
			assert(os && os->dataLen);
			for (int i = 0; i < os->dataLen; ++i)
			{
				SQLAppend(os[i]);
				Append(", ");
			};
			dataLen -= 2;
		}


		// ���ڲ�����
		template<typename T>
		void SQLAppend(T* const& v, bool const& ignoreReadOnly)
		{
			assert(v);
			static_assert(IsMPObject_v<T>, "");
			SQLiteAppend<T>::Exec(this, *v, ignoreReadOnly);
		}
	};
}

// ģ��������
namespace xx
{
	template<>
	struct SQLiteAppend<DB::Account, void>
	{
		static inline void Exec(SQLiteString* ss, DB::Account const &in, bool ignoreReadOnly)
		{
			ss->Append("(");
			if (!ignoreReadOnly)
			{
				ss->Append(in.id);
				ss->Append(", ");
			}
			ss->SQLAppend(in.username);
			ss->Append(", ");
			ss->SQLAppend(in.password);
			ss->Append(", ");
			ss->dataLen -= 2;
			ss->Append(")");
		}
	};
}

namespace DB
{
	// ģ�����ɵĺ�������. ����ִ����Ҫ try

	// todo: ȥ�� try ��Ϊִ������ж� lastErrorNumber

	


	struct SQLiteFuncs		// interface name
	{
		xx::SQLite* sqlite;
		xx::MemPool& mp;

		SQLiteFuncs(xx::SQLite* sqlite) : sqlite(sqlite), mp(sqlite->mempool()) {}
		~SQLiteFuncs()
		{
			if (query_CreateAccountTable) query_CreateAccountTable->Release();
			if (query_AddAccount) query_AddAccount->Release();
			if (query_GetAccountByUsername) query_GetAccountByUsername->Release();
		}

		xx::SQLiteQuery* query_CreateAccountTable = nullptr;
		void CreateAccountTable()
		{
			auto& q = query_CreateAccountTable;
			if (!q)
			{
				q = sqlite->CreateQuery(R"=-=(
create table [account]
(
    [id] integer primary key autoincrement, 
    [username] text(64) not null unique, 
    [password] text(64) not null
)
)=-=");
			}
			if (!q) throw - 1;

			if (!q->Execute())
			{
				throw - 3;
			}
		}


		xx::SQLiteQuery* query_AddAccount = nullptr;
		void AddAccount(char const* const& username, char const* const& password)
		{
			auto& q = query_AddAccount;
			if (!q)
			{
				q = sqlite->CreateQuery(R"=-=(
insert into [account] ([username], [password])
values (?, ?)
)=-=");
			}
			if (!q) throw - 1;
			if (q->SetParameters(username, password))
			{
				throw - 2;
			}
			if (!q->Execute())
			{
				throw - 3;
			}
		}

		xx::SQLiteQuery* query_GetAccountByUsername = nullptr;
		Account* GetAccountByUsername(char const* const& username)
		{
			auto& q = query_GetAccountByUsername;
			if (!q)
			{
				q = sqlite->CreateQuery(R"=-=(
select [id], [username], [password]
  from [account]
 where [username] = ?
)=-=");
			}
			if (!q) throw - 1;
			if (q->SetParameters(username))
			{
				throw - 2;
			}
			Account* rtv = nullptr;
			if (!q->Execute([&](xx::SQLiteReader& sr)
			{
				rtv = mp.Create<Account>();
				rtv->id = sr.ReadInt64(0);
				mp.CreateTo(rtv->username, sr.ReadString(1));
				mp.CreateTo(rtv->password, sr.ReadString(2));
			}))
			{
				throw - 3;
			}
			return rtv;
		}

	};
}


int main()
{
	PKG::AllTypesRegister();

	xx::MemPool mp;
	xx::SQLite_v sql(mp, "data.db");
	DB::SQLiteFuncs sqlfs(sql);

	auto r = sql->TableExists("account");
	if (r < 0)
	{
		sql->Cout("sql exec err! last errno = ", sql->lastErrorCode, ", last errmsg = ", sql->lastErrorMessage, "\n");
		goto LabEnd;
	}
	else if (r == 0)
	{
		try
		{
			sqlfs.CreateAccountTable();
			sqlfs.AddAccount("a", "1");
			sqlfs.AddAccount("b", "2");
		}
		catch (int errCode)
		{
			sql->Cout("errCode = ", errCode);
			goto LabEnd;
		}
	}
	else
	{
		try
		{
			auto a = sqlfs.GetAccountByUsername("a");
			if (a == nullptr)
			{
				sql->Cout("can't find account a!\n");
			}
			else
			{
				sql->Cout("found account a! id = ", a->id, " password = ", a->password, "\n");
			}
		}
		catch (int errCode)
		{
			sql->Cout("errCode = ", errCode);
			goto LabEnd;
		}
	}

	//xx::MemHeaderBox<Service> s(mp);
	//s->Run();

LabEnd:
	std::cin.get();
	return 0;
}
