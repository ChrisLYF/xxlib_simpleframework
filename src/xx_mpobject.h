#pragma once
#include "xx_mptr.h"

namespace xx
{
	struct String;


	// ֧�� MemPool ���඼Ӧ�ôӸû�������
	struct MPObject
	{
		virtual ~MPObject() {}

		// �ӳ�
		inline void AddRef()
		{
			++refCount();
		}

		/*
		if (!tsFlags()) return; else tsFlags() = 1;
		// str.append( .......... )........
		tsFlags() = 0;
		*/
		virtual void ToString(String &str) const;

		// ���� �� ���� + ���ձ�Ұ( �����ʵ���� xx_mempoolbase.h ��β�� )
		void Release();

		/***********************************************************************************/
		// �����Ƿ���ͷ�����ݵĸ��� helpers
		/***********************************************************************************/

		inline MemHeader_MPObject& memHeader() { return *((MemHeader_MPObject*)this - 1); }
		inline MemHeader_MPObject& memHeader() const { return *((MemHeader_MPObject*)this - 1); }

		inline uint64_t const& versionNumber() const { return memHeader().versionNumber; }
		inline uint64_t pureVersionNumber() const { return versionNumber() & 0x00FFFFFFFFFFFFFFu; }

		inline uint32_t& refCount() { return memHeader().refCount; }
		inline uint32_t const& refCount() const { return memHeader().refCount; }

		inline uint16_t const& typeId() const { return memHeader().typeId; }

		inline uint16_t& tsFlags() { return memHeader().tsFlags; }
		inline uint16_t const& tsFlags() const { return memHeader().tsFlags; }

		inline MemPoolBase const& mempoolbase() const { return *(memHeader().mempoolbase); }
		inline MemPoolBase& mempoolbase() { return *(memHeader().mempoolbase); }

		template<typename T> T& mempool()
		{
			static_assert(std::is_base_of<MemPoolBase, T>::value, "the T must be a MemPool<...>");
			return *(T*)(memHeader().mempoolbase);
		}
	};


	/***********************************************************************************/
	// type_traits
	/***********************************************************************************/

	template<typename T>
	struct IsMPObject
	{
		static const bool value = !std::is_void<T>::value && std::is_base_of<MPObject, T>::value;
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
	template<typename T>
	constexpr bool IsMPObject_v = IsMPObject<T>::value;


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
