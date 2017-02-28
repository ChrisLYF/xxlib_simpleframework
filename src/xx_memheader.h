#pragma once
#include <cstdint>

namespace xx
{
	// ���� MemPool ������ڴ涼��������һ��ͷ��
	struct MemHeader_VersionNumber
	{
		// �����汾��, ��Ϊʶ��ǰָ���Ƿ���Ч����Ҫ��ʶ
		union
		{
			uint64_t versionNumber;
			struct
			{
				uint8_t vnArea[7];
				const uint8_t mpIndex;		// ���� / ����ʱ���ڴ������ �±�( ָ�� versionNumber �����λ�ֽ� )
			};
		};

		// ͨ��һ��ָ��������ͷ��
		static MemHeader_VersionNumber& Visit(void* p)
		{
			return *((MemHeader_VersionNumber*)p - 1);
		}
	};

	struct MemPoolBase;

	// ���� MemPool ����� ��� MPObject ��������ڴ涼��������һ��ͷ��
	struct MemHeader_MPObject : public MemHeader_VersionNumber
	{
		// ���ھֲ���������ڴ��( ʹ�� MP::Get(this) �����õ��ڴ��. MP ���ڴ�ص� type )
		MemPoolBase *mempool;

		// ���� 0 ������ Dispose
		uint32_t refCount = 1;

		// MemPool ����ʱ�������ID
		uint32_t typeId = 0;

		// ͨ��һ��ָ��������ͷ��
		static MemHeader_MPObject& Visit(void* p)
		{
			return *((MemHeader_MPObject*)p - 1);
		}
	};
}
