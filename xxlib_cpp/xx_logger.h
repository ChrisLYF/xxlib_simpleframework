#pragma once
#include "xx_sqlite.h"
#include "xx_queue.h"
#include <mutex>
#include <thread>

namespace xx
{
	// ��Ӧ log ��־��
	struct Log;
	// ��־����
	enum class LogLevel : int32_t
	{
		Off = 0,
		Fatal = 1,
		Error = 2,
		Warning = 3,
		Info = 4,
		Debug = 5,
		All = 6,
	};

	// ��Ӧ log ��־��
	struct Log : Object
	{
		// ��������
		int64_t id = 0;
		// ��־����
		LogLevel level = (LogLevel)0;
		// ����ʱ��
		int64_t time = 0;
		// ������
		String_p machine;
		// ����/������
		String_p service;
		// ����/ʵ��id
		String_p instanceId;
		// ����
		String_p title;
		// ��������
		int64_t opcode = 0;
		// ��־��ϸ
		String_p desc;

		typedef Log ThisType;
		typedef Object BaseType;
		Log();
		Log(Log const&) = delete;
		Log& operator=(Log const&) = delete;
		virtual void ToString(String &str) const override;
		virtual void ToStringCore(String &str) const override;
	};
	using Log_p = Ptr<Log>;
	using Log_v = Dock<Log>;


	inline Log::Log()
	{
	}

	inline void Log::ToString(String &str) const
	{
		if (tsFlags())
		{
			str.Append("[ \"***** recursived *****\" ]");
			return;
		}
		else tsFlags() = 1;

		str.Append("{ \"type\" : \"Log\"");
		ToStringCore(str);
		str.Append(" }");

		tsFlags() = 0;
	}
	inline void Log::ToStringCore(String &str) const
	{
		this->BaseType::ToStringCore(str);
		str.Append(", \"id\" : ", this->id);
		str.Append(", \"level\" : ", this->level);
		str.Append(", \"time\" : ", this->time);
		str.Append(", \"machine\" : ", this->machine);
		str.Append(", \"service\" : ", this->service);
		str.Append(", \"instanceId\" : ", this->instanceId);
		str.Append(", \"title\" : ", this->title);
		str.Append(", \"opcode\" : ", this->opcode);
		str.Append(", \"desc\" : ", this->desc);
	}

	template<>
	struct MemmoveSupport<Log_v>
	{
		static const bool value = true;
	};



	struct LogFuncs
	{
		SQLite& db;
		LogFuncs(SQLite& db) : db(db) {}

		SQLiteQuery_p query_CreateTable_log;
		// �� log ��
		void CreateTable_log()
		{
			auto& q = query_CreateTable_log;

			if (!q)
			{
				q = db.CreateQuery(R"=-=(
CREATE TABLE [log](
    [id] INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL UNIQUE, 
    [level] INT NOT NULL,
    [time] INTEGER NOT NULL, 
    [machine] TEXT(250) NOT NULL, 
    [service] TEXT(50) NOT NULL, 
    [instanceId] TEXT(50) NOT NULL, 
    [title] TEXT(250) NOT NULL,
    [opcode] INTEGER NOT NULL,
    [desc] TEXT NOT NULL
);)=-=");
			}
			q->Execute();
		}


		SQLiteQuery_p query_InsertLog;
		// ����һ�� log. time ���� DateTime.Now.Ticks
		void InsertLog
		(
			LogLevel const& level,
			int64_t const& time,
			String_p const& machine,
			String_p const& service,
			String_p const& instanceId,
			String_p const& title,
			int64_t const& opcode,
			String_p const& desc
		)
		{
			auto& q = query_InsertLog;

			if (!q)
			{
				q = db.CreateQuery(R"=-=(
insert into [log] ([level], [time], [machine], [service], [instanceId], [title], [opcode], [desc]) 
values (?, ?, ?, ?, ?, ?, ?, ?))=-=");
			}
			q->SetParameters(level, time, machine, service, instanceId, title, opcode, desc);
			q->Execute();
		}



		xx::SQLiteQuery_p query__InsertLog;
		// ����һ�� log. time ���� DateTime.Now.Ticks
		void InsertLog
		(
			LogLevel const& level,
			int64_t const& time,
			char const* const& machine,
			char const* const& service,
			char const* const& instanceId,
			char const* const& title,
			int64_t const& opcode,
			char const* const& desc
		)
		{
			auto& q = query__InsertLog;

			if (!q)
			{
				q = db.CreateQuery(R"=-=(
insert into [log] ([level], [time], [machine], [service], [instanceId], [title], [opcode], [desc]) 
values (?, ?, ?, ?, ?, ?, ?, ?))=-=");
			}
			q->SetParameters(level, time, machine, service, instanceId, title, opcode, desc);
			q->Execute();
		}
	};





	struct Logger
	{
		MemPool mp;
		SQLite_v db;
		LogFuncs funcs;

		static const int nameLenLimit = 200;
		String_v machine;
		String_v service;
		String_v instanceId;

		// Ϊÿ�� queue ����һ�� mp��ר�ã�����ֻ��Ҫ���л�ʱ lock
		MemPool mp1;
		Queue_v<Log_p> logMsgs1;		// �л�ʹ��

		MemPool mp2;
		Queue_v<Log_p> logMsgs2;		// �л�ʹ��

		Queue<Log_p>* logMsgs;		// ָ��ǰ����ʹ�õ� logMsgs
		std::mutex mtx;


		int64_t counter = 0;
		bool disposing = false;

		Logger(char const* const& fn)
			: db(mp, fn)
			, funcs(*db)
			, machine(mp, nameLenLimit)
			, service(mp, nameLenLimit)
			, instanceId(mp, nameLenLimit)
			, logMsgs1(mp1)
			, logMsgs2(mp2)
			, logMsgs(&*logMsgs1)
		{
			if (!db->TableExists("log")) funcs.CreateTable_log();
			std::thread t([this]
			{
				Queue<Log_p>* backMsgs;

				while (!disposing)
				{
					// �л�ǰ��̨����( ���������. û�о� sleep һ�¼���ɨ )
					{
						std::lock_guard<std::mutex> lg(mtx);

						if (!logMsgs->Count()) goto LabEnd;

						if (logMsgs == &*logMsgs1)
						{
							backMsgs = &*logMsgs1;
							logMsgs = &*logMsgs2;
						}
						else
						{
							backMsgs = &*logMsgs2;
							logMsgs = &*logMsgs1;
						}
					}

					// ��ʼ��������( ���ڼ�ǰ̨���Լ������� )
					try
					{
						db->BeginTransaction();
						while (!backMsgs->Empty())
						{
							auto& o = *backMsgs->Top();
							if (o.id)
							{
								funcs.InsertLog(o.level, o.time, o.machine, o.service, o.instanceId, o.title, o.opcode, o.desc);
							}
							else
							{
								machine->Assign(o.machine);
								service->Assign(o.service);
								instanceId->Assign(o.instanceId);
							}
							backMsgs->Pop();
							++counter;
						}
						db->EndTransaction();
					}
					catch (...)
					{
						// �ƺ�ֻ�ܺ��Դ���
						std::cout << "logdb insert error! errNO = " << db->lastErrorCode << " errMsg = " << db->lastErrorMessage << std::endl;
					}
				LabEnd:
					Sleep(50);
				}
				disposing = false;
			});
			t.detach();
		}

		Logger(Logger const&) = delete;
		Logger operator=(Logger const&) = delete;

		~Logger()
		{
			disposing = true;
			while (disposing) Sleep(1);
			std::cout << "~Logger()" << std::endl;
		}

		template<typename MachineType, typename ServiceType, typename InstanceIdType, typename TitleType, typename DescType>
		void WriteAll(LogLevel const& level, int64_t const& time
			, MachineType const& machine, ServiceType const& service, InstanceIdType const& instanceId
			, TitleType const& title, int64_t const& opcode, DescType const& desc)
		{
			std::lock_guard<std::mutex> lg(mtx);
			auto& mp = logMsgs->mempool();
			auto o = mp.CreatePtr<Log>();

			o->id = 2;             // ���������ͨ�� WaitAll д���
			o->level = level;
			o->time = time;
			o->machine.Create(mp, machine);
			o->service.Create(mp, service);
			o->instanceId.Create(mp, instanceId);
			o->title.Create(mp, title);
			o->opcode = opcode;
			o->desc.Create(mp, desc);

			logMsgs->Emplace(std::move(o));
		}

		template<typename TitleType, typename DescType>
		void Write(LogLevel level, TitleType const& title, int64_t const& opcode, DescType const& desc)
		{
			std::lock_guard<std::mutex> lg(mtx);
			auto& mp = logMsgs->mempool();
			auto o = mp.CreatePtr<Log>();

			o->id = 1;             // ���������ͨ�� Write д���
			o->level = level;

			o->time = GetNowDateTimeTicks();
			o->machine = &*machine;
			o->service = &*service;
			o->instanceId = &*instanceId;

			o->title.Create(mp, title);
			o->opcode = opcode;
			o->desc.Create(mp, desc);

			logMsgs->Emplace(std::move(o));
		}

		template<typename MachineType, typename ServiceType, typename InstanceIdType>
		void SetDefaultValue(MachineType const& machine, ServiceType const& service, InstanceIdType const& instanceId)
		{
			std::lock_guard<std::mutex> lg(mtx);
			auto& mp = logMsgs->mempool();
			auto o = mp.CreatePtr<Log>();

			o->id = 0;				// ���������ͨ�� SetDefaultValue д���
			o->machine.Create(mp, machine);
			o->service.Create(mp, service);
			o->instanceId.Create(mp, instanceId);

			logMsgs->Emplace(std::move(o));
		}
	};
}
