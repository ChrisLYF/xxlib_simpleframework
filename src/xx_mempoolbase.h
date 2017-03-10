#pragma once
#include "xx_podstack.h"
#include "xx_mpobject.h"
#include "xx_string.h"
#include <array>
#include <vector>
#include <cstring>

namespace xx
{
	// todo: �����ڴ������� counter. ÿ����/���ڴ�ʱͬ��

	// �ڴ�ػ���. �ṩ�����޹ص��ڴ���� / ���չ���
	struct MemPoolBase
	{
		// todo: ��ʵ�ָ�����濴���᲻�����
		// stacks ����
		std::array<PodStack<void*>, sizeof(size_t) * 8> stacks;

		// �����汾��( ÿ�δ���ʱ�� ++ ����� )
		uint64_t versionNumber = 0;

		// ���ڴ����
		~MemPoolBase()
		{
			void* p;
			for (auto& stack : stacks)
			{
				while (stack.TryPop(p)) std::free(p);
			}
		}

		/***********************************************************************************/
		// �ڴ����( malloc / free ϵ��. ��Ҫ��һЩ������Ĵ���ʹ�� )
		/***********************************************************************************/

		// �ò���������ͷ��������� MemHeader_VersionNumber ������, ����ƫ�ƺ��ָ��
		// ����ڴ������ʵ� size ����: Round2n(capacity * sizeof(T) + sizeof(MemHeader_VersionNumber)) - sizeof(MemHeader_VersionNumber)
		inline void* Alloc(size_t siz)
		{
			assert(siz);
			siz += sizeof(MemHeader_VersionNumber);								// �ճ����� MemHeader_VersionNumber �ĵض�
			auto idx = Calc2n(siz);
			if (siz > (size_t(1) << idx)) siz = size_t(1) << ++idx;

			void* p;
			if (!stacks[idx].TryPop(p)) p = std::malloc(siz);

			auto h = (MemHeader_VersionNumber*)p;								// ָ���ڴ�ͷ
			h->versionNumber = (++versionNumber) | ((uint64_t)idx << 56);		// �������±긽�����λ��, Free Ҫ��
			return h + 1;														// ָ�� header ��������򷵻�
		}

		inline void Free(void* p)
		{
			if (!p) return;
			auto h = (MemHeader_VersionNumber*)p - 1;							// ָ���ڴ�ͷ
			assert(h->versionNumber && h->mpIndex < stacks.size());				// �����Ͻ� free ��ʱ����汾�Ų�Ӧ���� 0. ����������ظ� Free
			stacks[h->mpIndex].Push(h);											// ���
			h->versionNumber = 0;												// ��հ汾��
		}

		// dataLen ��ʾҪ���ƶ����ֽ������µ��ڴ�. �������� p ��ԭʼ����
		inline void* Realloc(void *p, size_t newSize, size_t dataLen = -1)
		{
			if (!newSize)
			{
				Free(p);
				return nullptr;
			}
			if (!p) return Alloc(newSize);

			auto h = (MemHeader_VersionNumber*)p - 1;
			assert(h->versionNumber && h->mpIndex < stacks.size());
			auto oldSize = (size_t(1) << h->mpIndex) - sizeof(MemHeader_VersionNumber);
			if (oldSize >= newSize) return p;

			auto np = Alloc(newSize);
			memcpy(np, p, std::min(oldSize, dataLen));
			Free(p);
			return np;
		}

		/***********************************************************************************/
		// �ڴ����( Create / Release ϵ�� ). ����������� MPObject �Ķ���
		/***********************************************************************************/

		// �ò���������ͷ����� MemHeader_MPObject
		template<typename T, typename ...Args>
		T* CreateWithoutTypeId(Args &&... args) noexcept
		{
			static_assert(std::is_base_of<MPObject, T>::value, "the T must be inerit of MPObject.");

			// ���д��� ������ Alloc ����С��
			auto siz = sizeof(T) + sizeof(MemHeader_MPObject);
			auto idx = Calc2n(siz);
			if (siz > (size_t(1) << idx)) siz = size_t(1) << ++idx;

			void* rtv;
			if (!stacks[idx].TryPop(rtv)) rtv = malloc(siz);

			auto p = (MemHeader_MPObject*)rtv;
			p->versionNumber = (++versionNumber) | ((uint64_t)idx << 56);
			p->mempoolbase = this;
			p->refCount = 1;
			p->typeId = -1;// (decltype(p->typeId))TupleIndexOf<T, Tuple>::value;

			auto t = (T*)(p + 1);
			try
			{
				new (t) T(std::forward<Args>(args)...);
			}
			catch (...)
			{
				this->Release(t);
				return nullptr;
			}
			return t;
		}

		// �ͷ��� Create ��������
		inline void Release(MPObject* p) noexcept
		{
			if (!p) return;
			assert(p->versionNumber());
			if (p->refCount() == 0 || --p->refCount()) return;
			p->~MPObject();

			auto h = (MemHeader_MPObject*)p - 1;								// ָ���ڴ�ͷ
			assert(h->versionNumber);											// �����Ͻ� free ��ʱ����汾�Ų�Ӧ���� 0. ����������ظ� Free
			stacks[h->mpIndex].Push(h);											// ���
			h->versionNumber = 0;												// ��հ汾��
		}

		/***********************************************************************************/
		// helpers
		/***********************************************************************************/

		template<typename T, typename ...Args>
		MPtr<T> CreateMPtrWithoutTypeId(Args &&... args) noexcept
		{
			return CreateWithoutTypeId<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename ...Args>
		void CreateToWithoutTypeId(T*& outPtr, Args &&... args) noexcept
		{
			outPtr = CreateWithoutTypeId<T>(std::forward<Args>(args)...);
		}
		template<typename T, typename ...Args>
		void CreateToWithoutTypeId(MPtr<T>& outPtr, Args &&... args) noexcept
		{
			outPtr = CreateMPtrWithoutTypeId<T>(std::forward<Args>(args)...);
		}

		//// �����ж����ڴ��ڳ�
		//uint64_t FreeMem()
		//{
		//	uint64_t count = 0;
		//	for (size_t i = 0; i < stacks.size(); ++i)
		//	{
		//		count += stacks[i].dataLen * (2 << i);
		//	}
		//	return count;
		//}
	};


	/***********************************************************************************/
	// �������Ҫ�õ� MemPool �Ĺ��ܹ�ʵ��д������
	/***********************************************************************************/
	inline void MPObject::Release() noexcept
	{
		mempoolbase().Release(this);
	}
}
