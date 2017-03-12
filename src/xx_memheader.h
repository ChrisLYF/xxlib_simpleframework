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
	};

	struct MemPoolBase;

	// ���� MemPool ����� ��� MPObject ��������ڴ涼��������һ��ͷ��
	struct MemHeader_MPObject : public MemHeader_VersionNumber
	{
		// �������ϣ���� MPObject �����Է�ָ�뷽ʽʹ��ʱ, ���ڱ���ǰ�����ṩ�ڴ�ͷ�ռ�ʱʹ��( ʵ���� xx_mempoolbase.h �� )
		MemHeader_MPObject(MemPoolBase& mempoolbase);

		// ���ھֲ���������ڴ��( ʹ�� MP::Get(this) �����õ��ڴ��. MP ���ڴ�ص� type )
		MemPoolBase *mempoolbase;

		// ���� 0 ������ Dispose
		uint32_t refCount = 1;

		// MemPool ����ʱ�������ID
		uint16_t typeId = 0;

		// ��ǰ������ ToString ������( ���Ǵ�ո�����ֵ )
		uint16_t tsFlags = 0;
	};

	// Ϊ MPObject �����ڴ�ͷ�Ա��ṩֵ���͵�����ṹ( ����ģʽ�� mh ��������ڴ��ָ������, �������汾��, ���͵���Ϣ )
	template<typename T>
	struct MPStruct
	{
		MemHeader_MPObject mh;
		T instance;
		template<typename ...Args>
		MPStruct(MemPoolBase& mempoolbase, Args&& ... args)
			: mh(mempoolbase)
			, instance(std::forward<Args>(args)...)
		{
		}
		T const* const& operator->() const
		{
			return &instance;
		}
		T* const& operator->()
		{
			return &instance;
		}
		T& operator*()
		{
			return instance;
		}
		T const& operator*() const
		{
			return instance;
		}
	};
}
