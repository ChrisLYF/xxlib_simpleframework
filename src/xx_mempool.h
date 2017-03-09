#pragma once
#include "xx_mempoolbase.h"

namespace xx
{

	/*
	// ʾ��:

	// headers ...

	// �ص�: type ����˳�� ������ ����ǰ���ں�. �˴��޹ص�����ν
	typedef xx::MemPool<C,B,A......> MP;

	// impls ...

	MP mp;
	auto a = mp.Create<A>(.....);
	*/


	// ���׿�ĺ����ڴ�������. �� 2^N �ߴ绮���ڴ������Ϊ, �� free ��ָ����� stack ���渴��
	// ���ڷ���������ڴ�, ���� �汾�� ������� ָ�� -8 ��( Alloc ). �����ж�ָ���Ƿ���ʧЧ
	// MPObject ����ʹ�� Create / Release ������������
	template<typename ... Types>
	struct MemPool : MemPoolBase
	{
		// �� Types ����Ϊ Tuple ������ std �ṩ�� tuple Ԫ��ϵ�в���
		typedef std::tuple<MPObject, Types...> Tuple;

		// ���͸���
		static const int typesSize = std::tuple_size<Tuple>::value;

		// ���� type ��Ӧ�� parent �� type id. 0: MPObject
		std::array<uint32_t, 1 + sizeof...(Types)> pids;


		MemPool(MemPool const&) = delete;
		MemPool& operator=(MemPool const &) = delete;
		MemPool()
		{
			FillPids(std::make_index_sequence<typesSize>());
		}

		/***********************************************************************************/
		// �ڴ����( Create / Release ϵ�� ). ����������� MPObject �Ķ���
		/***********************************************************************************/

		// �ò���������ͷ����� MemHeader_MPObject
		template<typename T, typename ...Args>
		T* Create(Args &&... args)
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
			p->typeId = (decltype(p->typeId))TupleIndexOf<T, Tuple>::value;
			p->tsFlags = 0;
			return new (p + 1) T(std::forward<Args>(args)...);
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
		void CreateTo(T*& outPtr, Args &&... args)
		{
			outPtr = Create<T>(std::forward<Args>(args)...);
		}
		template<typename T, typename ...Args>
		void CreateTo(MPtr<T>& outPtr, Args &&... args)
		{
			outPtr = CreateMPtr<T>(std::forward<Args>(args)...);
		}



		// ���� typeid �жϸ��ӹ�ϵ
		bool IsBaseOf(uint32_t baseTypeId, uint32_t typeId)
		{
			for (; typeId != baseTypeId; typeId = pids[typeId])
			{
				if (!typeId) return false;
			}
			return true;
		}

		// ���� ���� �жϸ��ӹ�ϵ
		template<typename BT, typename T>
		bool IsBaseOf()
		{
			return IsBaseOf(TupleIndexOf<BT, Tuple>::value, TupleIndexOf<T, Tuple>::value);
		}

		// �Խ�ָ�� p ת�� T* ����. ȡ�� dynamic_cast
		template<typename T>
		T* TryCast(MPObject* p)
		{
			return IsBaseOf(TupleIndexOf<T, Tuple>::value, p->typeId()) ? (T*)p : nullptr;
		}

	protected:
		/***********************************************************************************/
		// pids ���
		/***********************************************************************************/

		template<size_t... Indexs>
		void FillPids(std::index_sequence<Indexs...> const& idxs)
		{
			std::initializer_list<int>{ (FillPids(idxs, (std::tuple_element_t<Indexs, Tuple>*)nullptr), 0)... };
		}

		template<size_t... Indexs, typename T>
		void FillPids(std::index_sequence<Indexs...> const& idxs, T* const& ct)
		{
			uint16_t pid = 0;
			std::initializer_list<int>{ ((
				!std::is_same_v<std::tuple_element_t<Indexs, Tuple>, T>
				&& !pid 
				&& std::is_base_of_v<std::tuple_element_t<Indexs, Tuple>, T>
				? (pid = Indexs) : 0
				), 0)... };
			pids[TupleIndexOf<T, Tuple>::value] = pid;
			//std::cout << "tid = " << TupleIndexOf<T, Tuple>::value << ", t = " << typeid(T).name() << ", pid = " << pid << std::endl;
		}
	};
}
