#pragma once
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifdef xxloglib_EXPORTS
#define XXLOGLIB_API __declspec(dllexport)
#else
#define XXLOGLIB_API __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	// �����ļ��� ������־ db �ļ�. �ɹ����� Logger ��ָ�� ctx. ʧ�ܷ��� ��
	XXLOGLIB_API void* xxlogNew(char const* fn);

	// ֱ�Ӵ�������־����, ��¼��־. ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
	XXLOGLIB_API char const* xxlogWriteAll(void* ctx, int level, long long time
		, char const* machine, char const* service, char const* instanceId
		, char const* title, long long opcode, char const* desc);

	// ��¼��־( ��� WriteAll ȱʧ�Ĳ��� ��ȥ��Ĭ��ֵ ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
	XXLOGLIB_API char const* xxlogWrite(void* ctx, int level, char const* title, long long opcode, char const* desc);

	// ����һЩ�����ױ仯����Ŀ��Ĭ��ֵ, �Ա�ʹ�� Write ʱ�ܷ���д����Щ���� ������Ҫ�ٴ�
	XXLOGLIB_API void xxlogSetDefaultValue(void* ctx, char const* machine, char const* service, char const* instanceId);

	// ��ʼ��д( ��������, �ϲ�д������, ����������, �������������𻵵Ŀ����� ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
	XXLOGLIB_API char const* xxlogBeginTrans(void* ctx);

	// ������д( �ύ���� ). ���� nullptr ��ʾ�ɹ�( ʧ�ܷ��ش�����Ϣ )
	XXLOGLIB_API char const* xxlogEndTrans(void* ctx);

	// todo: ���ü�¼������ʱ�Զ�ɾ

	// ���� Logger
	XXLOGLIB_API void xxlogDelete(void*);

#ifdef __cplusplus
}
#endif
