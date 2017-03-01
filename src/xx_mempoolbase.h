#pragma once
#include "xx_podstack.h"
#include "xx_mpobject.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstring>

namespace xx
{
	// �ڴ�ػ���. �ṩ�����޹ص��ڴ���� / ���չ���
	struct MemPoolBase
	{
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
		// ����ڴ������ʵ� size ����: Round2n(capacity * sizeof(T) + 8) - 8
		inline void* Alloc(size_t siz)
		{
			siz += sizeof(MemHeader_VersionNumber);								// �ճ����� MemHeader_VersionNumber �ĵض�
			auto idx = Calc2n(siz);
			if (siz > (size_t(1) << idx)) siz = size_t(1) << ++idx;

			void* rtv;
			if (!stacks[idx].TryPop(rtv)) rtv = malloc(siz);

			auto p = (MemHeader_VersionNumber*)rtv;
			p->versionNumber = (++versionNumber) | ((uint64_t)idx << 56);		// �������±긽�����λ��, Free Ҫ��
			return p + 1;														// ָ�� header ��������򷵻�
		}

		inline void Free(void* p)
		{
			if (!p) return;
			auto& h = MemHeader_VersionNumber::Visit(p);						// ָ���ڴ�ͷ
			assert(h.versionNumber);											// �����Ͻ� free ��ʱ����汾�Ų�Ӧ���� 0. ����������ظ� Free
			stacks[h.mpIndex].Push(&h);											// ���
			h.versionNumber = 0;												// ��հ汾��
		}

		// dataLen ��ʾҪ���ƶ����ֽ������µ��ڴ�. �������� p ��ԭʼ����
		inline void* Realloc(void *p, size_t newSize, size_t dataLen = std::numeric_limits<size_t>::max())
		{
			assert(!p || (p && dataLen));
			auto rtv = Alloc(newSize);
			if (p)
			{
				auto oldSize = (size_t(1) << ((MemHeader_VersionNumber*)p - 1)->mpIndex) - sizeof(MemHeader_VersionNumber);
				memcpy(rtv, p, std::min(oldSize, dataLen));
				Free(p);
			}
			return rtv;
		}

		/***********************************************************************************/
		// �ڴ����( Create / Release ϵ�� ). ����������� MPObject �Ķ���
		/***********************************************************************************/

		// Create<T> ������ MemPool ����

		// �ͷ��� Create ��������
		inline void Release(MPObject* p)
		{
			if (!p) return;
			assert(p->versionNumber());
			if (p->refCount() == 0 || --p->refCount()) return;
			p->~MPObject();

			auto& h = MemHeader_MPObject::Visit(p);								// ָ���ڴ�ͷ
			assert(h.versionNumber);											// �����Ͻ� free ��ʱ����汾�Ų�Ӧ���� 0. ����������ظ� Free
			stacks[h.mpIndex].Push(&h);											// ���
			h.versionNumber = 0;												// ��հ汾��
		}

	};


	/***********************************************************************************/
	// �������Ҫ�õ� MemPool �Ĺ��ܹ�ʵ��д������
	/***********************************************************************************/
	inline void MPObject::Release()
	{
		mempoolbase().Release(this);
	}
}
