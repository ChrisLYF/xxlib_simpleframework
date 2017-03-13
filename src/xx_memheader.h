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
				uint8_t mpIndex;		// ���� / ����ʱ���ڴ������ �±�( ָ�� versionNumber �����λ�ֽ� )
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



	/************************************************************************************/
	// ֵ����ʹ�����
	/************************************************************************************/

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
		T const* operator->() const
		{
			return &instance;
		}
		T* operator->()
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


	template<typename T>
	struct IsMPStruct
	{
		static const bool value = false;
	};

	template<typename T>
	struct IsMPStruct<MPStruct<T>>
	{
		static const bool value = true;
	};
	template<typename T>
	constexpr bool IsMPStruct_v = IsMPStruct<T>::value;

}
