#pragma once
#ifdef _WIN32
	#include <SDKDDKVer.h>
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#ifdef xxloglib_EXPORTS
	#define XXLOGLIB_API __declspec(dllexport)
	#else
	#define XXLOGLIB_API __declspec(dllimport)
	#endif
#else
	#define XXLOGLIB_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

	// �����ļ��� ������־ db �ļ�. �ɹ����� Logger ��ָ�� ctx. ʧ�ܷ��� ��
	XXLOGLIB_API void* xxlogNew(char const* fn);

	// ֱ�Ӵ�������־����, ��¼��־. ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
	XXLOGLIB_API void xxlogWriteAll(void* ctx, int level, long long time, char const* machine, char const* service, char const* instanceId, char const* title, long long opcode, char const* desc);

	// ��¼��־( ��� WriteAll ȱʧ�Ĳ��� ��ȥ��Ĭ��ֵ ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
	XXLOGLIB_API void xxlogWrite(void* ctx, int level, char const* title, long long opcode, char const* desc);

	// ����һЩ�����ױ仯����Ŀ��Ĭ��ֵ, �Ա�ʹ�� Write ʱ�ܷ���д����Щ���� ������Ҫ�ٴ�
	XXLOGLIB_API void xxlogSetDefaultValue(void* ctx, char const* machine, char const* service, char const* instanceId);

	// ��ȡ��д���¼��( �ѽ������� ��Ҳ�п�����Ϊ��������δ�ɹ��ύ��д�̶����ֲ�һ�� )
	XXLOGLIB_API long long xxlogGetCounter(void*);

	// ���� Logger
	XXLOGLIB_API void xxlogDelete(void*);

#ifdef __cplusplus
}
#endif
