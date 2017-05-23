#pragma once
#include "xx_mpobject.h"
#include <array>

namespace xx
{

	/*
	// ʾ��:
	template<> struct TypeId<T> { static const uint16_t value = 2; };
	template<> struct TypeId<T> { static const uint16_t value = 3; };
	template<> struct TypeId<T> { static const uint16_t value = 4; };
	...

	MemPool::Register< T, PT >();
	MemPool::Register< T, PT >();
	MemPool::Register< T, PT >();
	...

	MemPool mp;
	auto o = mp.Create<T>(.....);

	*/


	struct BBuffer;

	// ���׿�ĺ����ڴ�������. �� 2^N �ߴ绮���ڴ������Ϊ, �� free ��ָ����� stack ���渴��
	// ���ڷ���������ڴ�, ���� �汾�� ������� ָ�� -8 ��( Alloc ). �����ж�ָ���Ƿ���ʧЧ
	// MPObject ����ʹ�� Create / Release ������������
	struct MemPool
	{
		// ������. ָ����һָ����ڴ�����ܿ��汾������д
		struct PtrStack
		{
			void* header = nullptr;

			inline bool TryPop(void*& output)
			{
				if (!header) return false;
				output = header;
				header = *(void**)((char*)header + sizeof(MemHeader_VersionNumber));
				return true;
			}

			inline void Push(void* const& v)
			{
				*(void**)((char*)v + sizeof(MemHeader_VersionNumber)) = header;
				header = v;
			}
		};

		// ���鳤�Ⱥ������� 2^n ������䳤�ȹ���
		std::array<PtrStack, sizeof(size_t) * 8> ptrstacks;

		// �����汾��( ÿ�δ���ʱ�� ++ ����� )
		uint64_t versionNumber = 0;


		MemPool(MemPool const&) = delete;
		MemPool& operator=(MemPool const &) = delete;
		MemPool() {}
		~MemPool()
		{
			// ���ڴ����
			void* p;
			for (auto& stack : ptrstacks)
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
			if (!ptrstacks[idx].TryPop(p)) p = std::malloc(siz);

			auto h = (MemHeader_VersionNumber*)p;								// ָ���ڴ�ͷ
			h->versionNumber = (++versionNumber) | ((uint64_t)idx << 56);		// �������±긽�����λ��, Free Ҫ��
			return h + 1;														// ָ�� header ��������򷵻�
		}

		inline void Free(void* p)
		{
			if (!p) return;
			auto h = (MemHeader_VersionNumber*)p - 1;							// ָ���ڴ�ͷ
			assert(h->versionNumber && h->mpIndex < ptrstacks.size());				// �����Ͻ� free ��ʱ����汾�Ų�Ӧ���� 0. ����������ظ� Free
			ptrstacks[h->mpIndex].Push(h);											// ���
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
			assert(h->versionNumber && h->mpIndex < ptrstacks.size());
			auto oldSize = (size_t(1) << h->mpIndex) - sizeof(MemHeader_VersionNumber);
			if (oldSize >= newSize) return p;

			auto np = Alloc(newSize);
			memcpy(np, p, MIN(oldSize, dataLen));
			Free(p);
			return np;
		}

		/***********************************************************************************/
		// �ڴ����( Create / Release ϵ�� ). ����������� MPObject �Ķ���
		/***********************************************************************************/

		// �ò���������ͷ����� MemHeader_MPObject
		template<typename T, typename ...Args>
		T* Create(Args &&... args) noexcept
		{
			static_assert(std::is_base_of<MPObject, T>::value, "the T must be inerit of MPObject.");

			// ���д��� ������ Alloc ����С��
			auto siz = sizeof(T) + sizeof(MemHeader_MPObject);
			auto idx = Calc2n(siz);
			if (siz > (size_t(1) << idx)) siz = size_t(1) << ++idx;

			void* rtv;
			if (!ptrstacks[idx].TryPop(rtv)) rtv = malloc(siz);
			if (!rtv) return nullptr;

			auto p = (MemHeader_MPObject*)rtv;
			p->versionNumber = (++versionNumber) | ((uint64_t)idx << 56);
			p->mempool = this;
			p->refCount = 1;
			p->typeId = TypeId<T>::value;
			p->tsFlags = 0;

			auto t = (T*)(p + 1);
			try
			{
				new (t) T(std::forward<Args>(args)...);
			}
			catch (...)
			{
				ptrstacks[idx].Push(p);											// ���
				p->versionNumber = 0;											// ��հ汾��
				return nullptr;
			}
			return t;
		}

		// �ͷ��� Create ��������
		inline void Release(MPObject* p) noexcept
		{
			if (!p || p->refCount() > 0x7FFFFFFF) return;						// �����ָ�� ������ MemHeaderBox ������ִ�� Release ����
			assert(p->versionNumber() && p->refCount());						// �����Ͻ� free ��ʱ����汾�Ų�Ӧ���� 0, ���ü��������� 0. ������� Release ��������
			if (--p->refCount()) return;
			p->~MPObject();

			auto h = (MemHeader_MPObject*)p - 1;								// ָ���ڴ�ͷ
			ptrstacks[h->mpIndex].Push(h);										// ���
			h->versionNumber = 0;												// ��հ汾��
		}

		/***********************************************************************************/
		// helpers
		/***********************************************************************************/

		template<typename T, typename ...Args>
		MPtr<T> CreateMPtr(Args &&... args)
		{
			return Create<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename ...Args>
		bool CreateTo(T*& outPtr, Args &&... args)
		{
			outPtr = Create<T>(std::forward<Args>(args)...);
			return outPtr != nullptr;
		}
		template<typename T, typename ...Args>
		bool CreateTo(MPtr<T>& outPtr, Args &&... args)
		{
			outPtr = CreateMPtr<T>(std::forward<Args>(args)...);
			return outPtr.pointer != nullptr;
		}









		/***********************************************************************************/
		// ���ʹ������
		/***********************************************************************************/

		// ���� type ��Ӧ�� parent �� type id. 1 : MPObject
		inline static std::array<uint16_t, std::numeric_limits<uint16_t>::max()>& pids()
		{
			static std::array<uint16_t, std::numeric_limits<uint16_t>::max()> _pids;
			return _pids;
		}

		// �� typeId �����л����캯����ӳ��
		inline static std::array<void*(*)(MemPool*, BBuffer*, uint32_t), 1 << sizeof(uint16_t) * 8>& creators()
		{
			static std::array<void*(*)(MemPool*, BBuffer*, uint32_t), 1 << sizeof(uint16_t) * 8> _creators;
			return _creators;
		}

		// ע�����͵ĸ��ӹ�ϵ. ˳�����ɴ�������. MPObject ����Ҫע��. T ��Ҫ�ṩ��Ӧ���캯�� for �����л�
		template<typename T, typename PT>
		static void Register();							// ʵ���� xx_buffer.h β��
		//{
		//	assert(!pids()[TypeId<T>::value]);
		//	pids()[TypeId<T>::value] = TypeId<PT>::value;
		//	creators()[TypeId<T>::value] = [](MemPool* mp, BBuffer* bb) 
		//	{
		//		// todo: ��Ҫ�����ﵥ��ʵ�� Create, �Ա���ִ�й��캯��֮ǰ�õ�ָ�� ���� bb. ���캯��ִ��ʧ��ʱ�� bb �Ƴ�
		//		return (MPObject*)mp->Create<T>(bb);
		//	};
		//}

		// ���� typeid �жϸ��ӹ�ϵ
		inline static bool IsBaseOf(uint32_t baseTypeId, uint32_t typeId)
		{
			for (; typeId != baseTypeId; typeId = pids()[typeId])
			{
				if (!typeId || typeId == pids()[typeId]) return false;
			}
			return true;
		}

		// ���� ���� �жϸ��ӹ�ϵ
		template<typename BT>
		static bool IsBaseOf(uint32_t typeId)
		{
			return IsBaseOf(TupleIndexOf<BT, Tuple>::value, typeId);
		}

		// ���� ���� �жϸ��ӹ�ϵ
		template<typename BT, typename T>
		static bool IsBaseOf()
		{
			return IsBaseOf(TupleIndexOf<BT, Tuple>::value, TupleIndexOf<T, Tuple>::value);
		}

		// �Խ�ָ�� p ת�� T* ����. ȡ�� dynamic_cast
		template<typename T>
		static T* TryCast(MPObject* p)
		{
			return IsBaseOf(TupleIndexOf<T, Tuple>::value, p->typeId()) ? (T*)p : nullptr;
		}

	};


	/***********************************************************************************/
	// һЩ����Ҫ�õ� MemPool �Ĺ��ܹ�ʵ��д������
	/***********************************************************************************/

	inline void MPObject::Release() noexcept
	{
		mempool().Release(this);
	}
}
