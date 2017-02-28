#pragma once
#include "xx_mptr.h"

namespace xx
{
	// ֧�� MemPool ���඼Ӧ�ôӸû�������
	struct MPObject
	{
		virtual ~MPObject() {}

		// �ӳ�
		inline void AddRef()
		{
			++refCount();
		}

		// ���� �� ���� + ���ձ�Ұ( �����ʵ���� xx_mempoolbase.h ��β�� )
		void Release();

		/***********************************************************************************/
		// �����Ƿ���ͷ�����ݵĸ��� helpers
		/***********************************************************************************/

		inline MemHeader_MPObject& memHeader() { return *(MemHeader_MPObject*)((char*)this - sizeof(MemHeader_MPObject)); }
		inline MemHeader_MPObject& memHeader() const { return *(MemHeader_MPObject*)((char*)this - sizeof(MemHeader_MPObject)); }
		inline MemPoolBase* const& mempool() const { return memHeader().mempool; }
		inline MemPoolBase*& mempool() { return memHeader().mempool; }
		inline uint64_t const& versionNumber() const { return memHeader().versionNumber; }
		inline uint64_t pureVersionNumber() const { return versionNumber() & 0x00FFFFFFFFFFFFFFu; }
		inline uint32_t& refCount() { return memHeader().refCount; }
		inline uint32_t const& refCount() const { return memHeader().refCount; }
		inline uint32_t const& typeId() const { return memHeader().typeId; }
	};


	/***********************************************************************************/
	// type_traits
	/***********************************************************************************/

	template<typename T>
	struct IsMPObject
	{
		static const bool value = false;
	};
	template<typename T>
	struct IsMPObject<T*>
	{
		static const bool value = !std::is_void<T>::value && std::is_base_of<MPObject, T>::value;
	};
	template<typename T>
	struct IsMPObject<MPtr<T>>
	{
		static const bool value = true;
	};


	// ɨ�����б����Ƿ��� MPObject* �� MPtr<MPObject> ����
	template<typename ...Types>
	struct ExistsMPObject
	{
		template<class Tuple, size_t N> struct TupleScaner {
			static constexpr bool Exec()
			{
				return IsMPObject< std::tuple_element_t<N - 1, Tuple> >::value ? true : TupleScaner<Tuple, N - 1>::Exec();
			}
		};
		template<class Tuple> struct TupleScaner<Tuple, 1> {
			static constexpr bool Exec()
			{
				return IsMPObject< std::tuple_element_t<0, Tuple> >::value;
			}
		};
		static constexpr bool value = TupleScaner<std::tuple<Types...>, sizeof...(Types)>::Exec();
	};

}
