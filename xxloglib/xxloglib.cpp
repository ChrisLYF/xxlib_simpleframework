#include "xxloglib.h"
#include "xx_logger.h"

BOOL APIENTRY DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}




// �����ļ��� ������־ db �ļ�. �ɹ����� Logger ��ָ�� ctx. ʧ�ܷ��� ��
XXLOGLIB_API void* xxlog2New(char const* fn)
{
	try
	{
		return new xx::Logger(fn);
	}
	catch (...)
	{
		return nullptr;
	}
}

// todo: ���� char* תΪ uint16_t* ��תΪ utf8, �������� C# �Ǳ���, C# �Ǳ�ֱ�� string

// ֱ�Ӵ�������־����, ��¼��־. ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API void xxlog2WriteAll(void* ctx, int level, long long time, char const* machine, char const* service, char const* instanceId, char const* title, long long opcode, char const* desc)
{
	((xx::Logger*)ctx)->WriteAll((xx::LogLevel)level, time, machine, service, instanceId, title, opcode, desc);
}

// ��¼��־( ��� WriteAll ȱʧ�Ĳ��� ��ȥ��Ĭ��ֵ ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API void xxlog2Write(void* ctx, int level, char const* title, long long opcode, char const* desc)
{
	((xx::Logger*)ctx)->Write((xx::LogLevel)level, title, opcode, desc);
}

// ����һЩ�����ױ仯����Ŀ��Ĭ��ֵ, �Ա�ʹ�� Write ʱ�ܷ���д����Щ���� ������Ҫ�ٴ�
XXLOGLIB_API void xxlog2SetDefaultValue(void* ctx, char const* machine, char const* service, char const* instanceId)
{
	((xx::Logger*)ctx)->SetDefaultValue(machine, service, instanceId);
}

// ���� Logger
XXLOGLIB_API long long xxlog2GetCounter(void* ctx)
{
	return ((xx::Logger*)ctx)->counter;
}


// ���� Logger
XXLOGLIB_API void xxlog2Delete(void* ctx)
{
	delete (xx::Logger*)ctx;
}









// ר��дһ�ݹ� C# ��, C# �Լ������̺߳��ڴ����
struct Logger
{
	xx::MemPool mp;
	xx::SQLite_v db;
	xx::LogFuncs funcs;
	xx::String_v machine;
	xx::String_v service;
	xx::String_v instanceId;

	Logger(char const* const& fn)
		: db(mp, fn)
		, funcs(*db)
		, machine(mp)
		, service(mp)
		, instanceId(mp)
	{
		if (!db->TableExists("log")) funcs.CreateTable_log();
	}
};

// �����ļ��� ������־ db �ļ�. �ɹ����� Logger ��ָ�� ctx. ʧ�ܷ��� ��
XXLOGLIB_API void* xxlogNew(char const* fn)
{
	try
	{
		return new Logger(fn);
	}
	catch (...)
	{
		return nullptr;
	}
}

// ֱ�Ӵ�������־����, ��¼��־. ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API char const* xxlogWriteAll(void* ctx, int level, long long time, char const* machine, char const* service, char const* instanceId, char const* title, long long opcode, char const* desc)
{
	auto& self = *(Logger*)ctx;
	auto str = "";
	try
	{
		self.funcs.InsertLog((xx::LogLevel)level, time, (machine ? machine : str), (service ? service : str), (instanceId ? instanceId : str), (title ? title : str), opcode, (desc ? desc : str));
	}
	catch (...)
	{
		return self.db->lastErrorMessage;
	}
	return nullptr;
}

// ��¼��־( ��� WriteAll ȱʧ�Ĳ��� ��ȥ��Ĭ��ֵ ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API char const* xxlogWrite(void* ctx, int level, char const* title, long long opcode, char const* desc)
{
	auto& self = *(Logger*)ctx;
	return xxlogWriteAll(ctx, level, xx::GetNowDateTimeTicks(), self.machine->buf, self.service->buf, self.instanceId->buf, title, opcode, desc);
}

// ����һЩ�����ױ仯����Ŀ��Ĭ��ֵ, �Ա�ʹ�� Write ʱ�ܷ���д����Щ���� ������Ҫ�ٴ�
XXLOGLIB_API void xxlogSetDefaultValue(void* ctx, char const* machine, char const* service, char const* instanceId)
{
	auto& self = *(Logger*)ctx;
	self.machine->Assign(machine); self.machine->C_str();
	self.service->Assign(service); self.service->C_str();
	self.instanceId->Assign(instanceId); self.instanceId->C_str();
}

// ��ʼ��д( ��������, �ϲ�д������, ����������, �������������𻵵Ŀ����� ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API char const* xxlogBeginTrans(void* ctx)
{
	auto& self = *(Logger*)ctx;
	try
	{
		self.db->BeginTransaction();
	}
	catch (...)
	{
		return self.db->lastErrorMessage;
	}
	return nullptr;
}

// ������д( �ύ���� ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API char const* xxlogEndTrans(void* ctx)
{
	auto& self = *(Logger*)ctx;
	try
	{
		self.db->EndTransaction();
	}
	catch (...)
	{
		return self.db->lastErrorMessage;
	}
	return nullptr;
}

// ���� Logger
XXLOGLIB_API void xxlogDelete(void* ctx)
{
	delete (Logger*)ctx;
}
