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

	struct MemPool;

	// ���� MemPool ����� ��� MPObject ��������ڴ涼��������һ��ͷ��
	struct MemHeader_MPObject : public MemHeader_VersionNumber
	{
		// �������ϣ���� MPObject �����Է�ָ�뷽ʽʹ��ʱ, ���ڱ���ǰ�����ṩ�ڴ�ͷ�ռ�ʱʹ��( ʵ���� xx_mempoolbase.h �� )
		MemHeader_MPObject(MemPool& mempool)
			: mempool(&mempool)
		{
			this->versionNumber = 0;
			this->refCount = (uint32_t)-1;											// �� Release
		}
		MemHeader_MPObject(MemHeader_MPObject&& o)
			: mempool(o.mempool)
			, refCount(o.refCount)
			, typeId(o.typeId)
			, tsFlags(o.tsFlags)
		{
			this->versionNumber = o.versionNumber;
			o.versionNumber = 0;
			o.mempool = nullptr;
			o.refCount = 0;
			o.typeId = 0;
			o.tsFlags = 0;
		}

		// ���ھֲ���������ڴ��( ʹ�� MP::Get(this) �����õ��ڴ��. MP ���ڴ�ص� type )
		MemPool *mempool;

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
	struct MemHeaderBox
	{
		MemHeader_MPObject mh;
		T instance;

		template<typename ...Args>
		MemHeaderBox(MemPool& mempool, Args&& ... args)
			: mh(mempool)
			, instance(std::forward<Args>(args)...)
		{
		}

		MemHeaderBox(MemHeaderBox const&) = delete;
		MemHeaderBox& operator=(MemHeaderBox const&) = delete;
		MemHeaderBox(MemHeaderBox&& o)
			: mh(std::move(o.mh))
			, instance(std::move(o.instance))
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
		operator T*()
		{
			return &instance;
		}
	};


	template<typename T>
	struct IsMemHeaderBox
	{
		static const bool value = false;
	};

	template<typename T>
	struct IsMemHeaderBox<MemHeaderBox<T>>
	{
		static const bool value = true;
	};
	template<typename T>
	constexpr bool IsMemHeaderBox_v = IsMemHeaderBox<T>::value;

}
