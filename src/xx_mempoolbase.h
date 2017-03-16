#pragma once
#include "xx_mpobject.h"
#include "xx_string.h"
#include <array>
#include <vector>
#include <cstring>
#include <allocators>

namespace xx
{
	// �ڴ�ػ���. �ṩ�����޹ص��ڴ���� / ���չ���
	struct MemPoolBase
	{
		// ������. ָ����һָ����ڴ�����ܿ��汾������д
		struct PtrStack
		{
			void* header = nullptr;

			bool TryPop(void*& output)
			{
				if (!header) return false;
				output = header;
				header = *(void**)((char*)header + sizeof(MemHeader_VersionNumber));
				return true;
			}

			void Push(void* const& v)
			{
				*(void**)((char*)v + sizeof(MemHeader_VersionNumber)) = header;
				header = v;
			}
		};

		// ���鳤�Ⱥ������� 2^n ������䳤�ȹ���
		std::array<PtrStack, sizeof(size_t) * 8> ptrstacks;

		// �����汾��( ÿ�δ���ʱ�� ++ ����� )
		uint64_t versionNumber = 0;

		// ���ڴ����
		~MemPoolBase()
		{
			void* p;
			for (auto& stack : ptrstacks)
			{
				while (stack.TryPop(p)) std::free(p);
			}
		}




		/***********************************************************************************/
		// �ڴ����( malloc / free ϵ��. ��Ҫ��һЩ������Ĵ���ʹ�� )
		/***********************************************************************************/

		// todo: ����ֵ����ʹ�÷�ʽ, �����ʽ��ֵ�ƶ�����? ��� xxxxx_v (�ر��� String_v ) �� dict / tostring ֮������? ��ֵ���츴�ƺ���?


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
			if (!ptrstacks[idx].TryPop(rtv)) rtv = malloc(siz);

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
			ptrstacks[h->mpIndex].Push(h);											// ���
			h->versionNumber = 0;												// ��հ汾��
		}

		inline void InitMemHeader(MPObject* p) noexcept
		{

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

	};


	/***********************************************************************************/
	// һЩ����Ҫ�õ� MemPool �Ĺ��ܹ�ʵ��д������
	/***********************************************************************************/

	inline void MPObject::Release() noexcept
	{
		mempoolbase().Release(this);
	}

	inline MemHeader_MPObject::MemHeader_MPObject(MemPoolBase& mempoolbase)
		: mempoolbase(&mempoolbase)
	{
		this->versionNumber = 0;
	}





	/***********************************************************************************/
	// �� MemPool ʵ��һ������ stl ��׼�� allocator
	/***********************************************************************************/

	/*
	
	xx::MemPoolAllocator<int> a(mp);
	std::vector<int, decltype(a)> vec(a);


struct LuaTable;
typedef std::variant< int, std::string > LuaTable_KT;
typedef std::variant< int, std::string, LuaTable, LuaTable* > LuaTable_VT;
typedef xx::MemPoolAllocator<std::pair<LuaTable_KT, LuaTable_VT>> LuaTable_AT;
typedef std::map<LuaTable_KT, LuaTable_VT, std::less<LuaTable_KT>, LuaTable_AT> LuaTable_BT;
// typedef std::unordered_map<LuaTable_KT, LuaTable_VT, std::hash<LuaTable_KT>, std::equal_to<LuaTable_KT>, LuaTable_AT> LuaTable_BT;
struct LuaTable : LuaTable_BT
{
	LuaTable(LuaTable_AT const& a) : LuaTable_BT(a) {}
	LuaTable(LuaTable_AT && a) : LuaTable_BT(std::move(a)) {}
};

int main()
{
	xx::MemPoolBase mpb;
	LuaTable t((typename LuaTable_AT)(mpb));
	t["a"] = 1;
	t[1] = "a";
	t[2] = &t;
	t["newtable"] = LuaTable(mpb);
	std::get<LuaTable>(t["newtable"])[1] = 123;



	*/

	template <class T>
	struct MemPoolAllocator : public std::allocator<T>
	{
		typedef std::allocator<T> base_type;

		template<class Other>
		struct rebind
		{
			typedef MemPoolAllocator<Other> other;
		};

		xx::MemPoolBase* mpb = nullptr;

		MemPoolAllocator() = delete;
		MemPoolAllocator(xx::MemPoolBase& mpb) : mpb(&mpb) {}
		MemPoolAllocator(MemPoolAllocator<T> const& o) : mpb(o.mpb) {}
		MemPoolAllocator(MemPoolAllocator<T> && o) : mpb(o.mpb) {}
		MemPoolAllocator<T>& operator=(MemPoolAllocator<T> const& o) { mpb = o.mpb; return (*this); }
		template<class O> MemPoolAllocator(MemPoolAllocator<O> const& o) : mpb(o.mpb) {}
		template<class O> MemPoolAllocator<T>& operator=(MemPoolAllocator<O> const& o) { mpb = o.mpb; return (*this); }

		size_type max_size() const
		{
			return static_cast<size_type>(-1) / sizeof(value_type);
		}

		pointer allocate(size_type count)
		{
			assert(mpb);
			void* p = mpb->Alloc(count * sizeof(T));
			if (!p) throw std::bad_alloc();
			return static_cast<pointer>(p);
		}

		void deallocate(pointer ptr, size_type count)
		{
			assert(mpb);
			mpb->Free(ptr);
		}
	};

}
