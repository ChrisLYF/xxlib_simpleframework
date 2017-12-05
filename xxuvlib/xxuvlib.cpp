#include "xxloglib.h"
#include "xx_logger.h"

#ifdef _WIN32

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

#endif


// �����ļ��� ������־ db �ļ�. �ɹ����� Logger ��ָ�� ctx. ʧ�ܷ��� ��
XXLOGLIB_API void* xxlogNew(char const* fn)
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
XXLOGLIB_API void xxlogWriteAll(void* ctx, int level, long long time, char const* machine, char const* service, char const* instanceId, char const* title, long long opcode, char const* desc)
{
	((xx::Logger*)ctx)->WriteAll((xx::LogLevel)level, time, machine, service, instanceId, title, opcode, desc);
}

// ��¼��־( ��� WriteAll ȱʧ�Ĳ��� ��ȥ��Ĭ��ֵ ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
XXLOGLIB_API void xxlogWrite(void* ctx, int level, char const* title, long long opcode, char const* desc)
{
	((xx::Logger*)ctx)->Write((xx::LogLevel)level, title, opcode, desc);
}

// ����һЩ�����ױ仯����Ŀ��Ĭ��ֵ, �Ա�ʹ�� Write ʱ�ܷ���д����Щ���� ������Ҫ�ٴ�
XXLOGLIB_API void xxlogSetDefaultValue(void* ctx, char const* machine, char const* service, char const* instanceId)
{
	((xx::Logger*)ctx)->SetDefaultValue(machine, service, instanceId);
}

// ���� Logger
XXLOGLIB_API long long xxlogGetCounter(void* ctx)
{
	return ((xx::Logger*)ctx)->counter;
}


// ���� Logger
XXLOGLIB_API void xxlogDelete(void* ctx)
{
	delete (xx::Logger*)ctx;
}

