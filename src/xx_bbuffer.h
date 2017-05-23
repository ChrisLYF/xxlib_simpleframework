#pragma once
#include "xx_bytesutils.h"
#include "xx_mempool.h"
#include "xx_string.h"
#include "xx_list.h"
#include "xx_dict.h"

namespace xx
{
	/*************************************************************************/
	// BBufferRWSwitcher( GCC ��Ҫ������������д�������� )
	/*************************************************************************/

	template<typename T, typename ENABLE = void>
	struct BBufferRWSwitcher
	{
		static void Write(BBuffer* bb, T const& v);
		static int Read(BBuffer* bb, T& v);
	};

	// non MPObject
	template<typename T>
	struct BBufferRWSwitcher<T, std::enable_if_t<!std::is_pointer<T>::value && !IsMPtr<T>::value>>
	{
		static void Write(BBuffer* bb, T const& v);
		static int Read(BBuffer* bb, T& v);
	};

	// todo: Xxxxxx_v ��ֵ����д?
	// MPObject* || MPtr
	template<typename T>
	struct BBufferRWSwitcher<T, std::enable_if_t<std::is_pointer<T>::value && std::is_base_of<MPObject, typename std::remove_pointer<T>::type>::value || IsMPtr<T>::value>>
	{
		static void Write(BBuffer* bb, T const& v);
		static int Read(BBuffer* bb, T& v);
	};


	/*************************************************************************/
	// BBuffer ����
	/*************************************************************************/

	struct BBuffer : public List<char, 16>
	{
		typedef List<char, 16> BaseType;
		uint32_t offset = 0;													// ��ָ��ƫ����

		BBuffer(BBuffer const&o) = delete;
		BBuffer& operator=(BBuffer const&o) = delete;
		explicit BBuffer(uint32_t capacity = 0) : BaseType(capacity) {}
		BBuffer(BBuffer* bb);

		/*************************************************************************/
		// ·��Ϊͳһ�ӿ�ϵ��
		/*************************************************************************/

		template<typename T>
		void Write(T const& v)
		{
			BBufferRWSwitcher<T>::Write(this, v);
		}
		template<typename T>
		int Read(T &v)
		{
			return BBufferRWSwitcher<T>::Read(this, v);
		}


		/*************************************************************************/
		// ��ͳ pod ��дϵ��( ͨ�� bytesutils ������� / �ػ� ʵ�� )
		/*************************************************************************/

		template<typename ...TS>
		void WritePod(TS const& ...vs)
		{
			this->Reserve(this->dataLen + BBCalc(vs...));
			this->dataLen += BBWriteTo(this->buf + this->dataLen, vs...);
			assert(this->dataLen <= this->bufLen);
		}

		template<typename T, typename ...TS>
		int ReadPod(T &v, TS&...vs)
		{
			return BBReadFrom(buf, dataLen, offset, vs...);
		}

		int ReadPod() { return 0; }


		/*************************************************************************/
		//  MPObject �����дϵ��
		/*************************************************************************/

		Dict<void*, uint32_t>*						ptrStore = nullptr;		// ��ʱ��¼ key: ָ��, value: offset
		Dict<uint32_t, std::pair<void*, uint16_t>>*	idxStore = nullptr;		// ��ʱ��¼ key: ��offset, value: pair<ptr, typeId>

		void BeginWrite()
		{
			if (!ptrStore) this->mempool().CreateTo(ptrStore, 16);
			//else ptrStore->Clear();
		}
		void EndWrite()
		{
			assert(ptrStore);
			ptrStore->Clear();
		}
		void BeginRead()
		{
			if (!idxStore) this->mempool().CreateTo(idxStore, 16);
			//else idxStore->Clear();
		}
		void EndRead()
		{
			assert(idxStore);
			idxStore->Clear();
		}

		template<typename T>
		void WriteRef(T const& v)
		{
			BeginWrite();
			Write(v);
			EndWrite();
		}

		template<typename T>
		int ReadRef(T &v)
		{
			BeginRead();
			auto rtv = Read(v);
			EndRead();
			return rtv;
		}

		template<typename T>
		void WriteObj(T* const& v)
		{
			assert(ptrStore);
			if (!v)
			{
				WritePod((uint8_t)0);
				return;
			}
			WritePod(v->typeId());

			auto rtv = ptrStore->Add((void*)v, dataLen);
			WritePod(ptrStore->ValueAt(rtv.index));
			if (rtv.success)
			{
				v->ToBBuffer(*this);
			}
		}
		template<typename T>
		int ReadObj(T* &v)
		{
			assert(idxStore);

			// get typeid
			uint16_t tid;
			if (auto rtv = ReadPod(tid)) return rtv;

			// isnull ?
			if (tid == 0)
			{
				v = nullptr;
				return 0;
			}

			// get offset
			uint32_t ptr_offset = 0, bb_offset_bak = offset;
			if (auto rtv = ReadPod(ptr_offset)) return rtv;

			// fill or ref
			if (ptr_offset == bb_offset_bak)
			{
				// ensure inherit
				if (!mempool().IsBaseOf(TypeId<T>::value, tid)) return -2;

				// try get creator func
				auto f = MemPool::creators()[tid];
				assert(f);

				// try create & read from bb
				v = f(mempool(), this, ptr_offset);
				if (v == nullptr) return -3;
			}
			else
			{
				// try get ptr from dict
				typename std::remove_pointer_t<decltype(idxStore)>::ValueType val;
				if (!idxStore->TryGetValue(ptr_offset, val)) return -4;

				// inherit validate
				if (!mempool().IsBaseOf(TypeId<T>::value, std::get<1>(val))) return -2;

				// set val
				v = (T*)std::get<0>(val);
			}
			return 0;
		}

		template<typename T>
		void WriteObj(MPtr<T> const& v)
		{
			Write(v.Ensure());
		}
		template<typename T>
		int ReadObj(MPtr<T> &v)
		{
			T* t;
			auto rtv = Read(t);
			v = t;
			return rtv;
		}



		/*************************************************************************/
		//  �������ߺ���
		/*************************************************************************/

		// ��ָ��λ�õ�����( ��Ӱ�� offset )
		template<typename ...TS>
		int ReadAt(uint32_t const& pos, TS&...vs)
		{
			if (pos >= dataLen) return -1;
			auto bak = offset;
			offset = pos;
			if (auto rtv = Read(vs...)) return rtv;
			offset = bak;
			return 0;
		}

		// �ӵ�ǰλ�ö����ݵ��º� offset ƫ��ָ������
		template<typename ...TS>
		int ReadJump(uint32_t const& len, TS&...vs)
		{
			if (offset + len > dataLen) return -1;
			auto bak = offset;
			if (auto rtv = Read(vs...)) return rtv;
			offset = bak + len;
			return 0;
		}

		// ֱ��׷��д��һ�� buf ( ������¼���� )
		void WriteBuf(char const* buf, uint32_t const& len)
		{
			this->Reserve(this->dataLen + len);
			std::memcpy(this->buf + this->dataLen, buf, len);
			this->dataLen += len;
		}

		// ׷��һ��ָ�����ȵĿռ�, ���ص�ǰ dataLen
		uint32_t WriteSpace(uint32_t const& len)
		{
			auto rtv = this->dataLen;
			this->Reserve(this->dataLen + len);
			this->dataLen += len;
			return rtv;
		}

		// �� pos λ��д��һ�� buf ( ������¼���� ). dataLen ���ܳŴ�.
		void WriteBufAt(uint32_t const& pos, char const* buf, uint32_t const& len)
		{
			assert(pos < this->dataLen);
			auto bak = this->dataLen;		// ����ԭʼ���ݳ���, ��ʼ׷��. ׷����֮��, �Ա�ԭʼ���ݳ���. ���û����, ��Ҫ��ԭ.
			this->dataLen = pos;
			WriteBuf(buf, len);
			if (this->dataLen < bak) this->dataLen = bak;
		}

		// �� pos λ���� Write ����. dataLen ���ܳŴ�.
		template<typename ...TS>
		void WriteAt(uint32_t const& pos, TS const&...vs)
		{
			assert(pos < this->dataLen);
			auto bak = this->dataLen;
			this->dataLen = pos;
			Write(vs...);
			if (this->dataLen < bak) this->dataLen = bak;
		}

		/*************************************************************************/
		// ʵ�� ToString �ӿ�
		/*************************************************************************/

		// �ȼ�����
		virtual void ToString(String &str) const override
		{
			str.Append("{ \"len\" : ", dataLen, ", \"data\" : [ ");
			for (size_t i = 0; i < dataLen; i++)
			{
				str.Append((int)(uint8_t)buf[i], ", ");
			}
			if (dataLen) str.dataLen -= 2;
			str.Append(" ] }");
		}

		/*************************************************************************/
		// ʵ�� BBuffer �ӿ�
		/*************************************************************************/

		virtual void ToBBuffer(BBuffer &bb) const override;

		virtual int FromBBuffer(BBuffer &bb) override;
	};

	/*************************************************************************/
	// BBufferRWSwitcher
	/*************************************************************************/

	template<typename T>
	void BBufferRWSwitcher<T, std::enable_if_t<!std::is_pointer<T>::value && !IsMPtr<T>::value>>::Write(BBuffer* bb, T const& v)
	{
		bb->WritePod(v);
	}
	template<typename T>
	int BBufferRWSwitcher<T, std::enable_if_t<!std::is_pointer<T>::value && !IsMPtr<T>::value>>::Read(BBuffer* bb, T& v)
	{
		return bb->ReadPod(v);
	}
	template<typename T>
	void BBufferRWSwitcher<T, std::enable_if_t<std::is_pointer<T>::value && std::is_base_of<MPObject, typename std::remove_pointer<T>::type>::value || IsMPtr<T>::value>>::Write(BBuffer* bb, T const& v)
	{
		bb->WriteObj(v);
	}
	template<typename T>
	int BBufferRWSwitcher<T, std::enable_if_t<std::is_pointer<T>::value && std::is_base_of<MPObject, typename std::remove_pointer<T>::type>::value || IsMPtr<T>::value>>::Read(BBuffer* bb, T& v)
	{
		return bb->ReadObj(v);
	}


	/*************************************************************************/
	// ʵ��ֵ����ʹ����������
	/*************************************************************************/

	using BBuffer_v = MemHeaderBox<BBuffer>;


	/*************************************************************************/
	// ʵ�ָ������л��ӿ�
	/*************************************************************************/

	template<typename T, typename PT>
	void MemPool::Register()
	{
		// �游 pid
		assert(!pids()[TypeId<T>::value]);
		pids()[TypeId<T>::value] = TypeId<PT>::value;

		// ��ִ�й��캯��֮ǰ�õ�ָ�� ���� bb. ���캯��ִ��ʧ��ʱ�� bb �Ƴ�
		creators()[TypeId<T>::value] = [](MemPool* mp, BBuffer* bb, uint32_t ptrOffset) ->void*
		{
			// �����ֵ�ռλ, ���䵽ʵ��ָ����滻
			auto addResult = bb->idxStore->Add(ptrOffset, std::make_pair(nullptr, TypeId<T>::value));

			// ���д��� ������ Create ����С��
			auto siz = sizeof(T) + sizeof(MemHeader_MPObject);
			auto idx = Calc2n(siz);
			if (siz > (size_t(1) << idx)) siz = size_t(1) << ++idx;

			void* rtv;
			if (!mp->ptrstacks[idx].TryPop(rtv)) rtv = malloc(siz);
			if (!rtv) return nullptr;

			auto p = (MemHeader_MPObject*)rtv;
			p->versionNumber = (++mp->versionNumber) | ((uint64_t)idx << 56);
			p->mempool = mp;
			p->refCount = 1;
			p->typeId = TypeId<T>::value;
			p->tsFlags = 0;

			auto t = (T*)(p + 1);
			bb->idxStore->ValueAt(addResult.index).first = t;	// �滻����ʵ�ֵ�
			try
			{
				new (t) T(bb);
			}
			catch (...)
			{
				bb->idxStore->RemoveAt(addResult.index);		// ���ֵ��Ƴ�( �����Ͻ����Բ���, ����ʧ�ܳ�ȥ��� clear )
				mp->ptrstacks[idx].Push(p);
				p->versionNumber = 0;
				return nullptr;
			}
			return t;
		};
	};

	// todo: ÿ�����͵Ķ�д�ӿ�����

	BBuffer::BBuffer(BBuffer* bb)
	{
		// todo
	}
	void BBuffer::ToBBuffer(BBuffer &bb) const
	{
		// todo
	}

	int BBuffer::FromBBuffer(BBuffer &bb)
	{
		// todo
		return 0;
	}


	// List ���л� ·����
	template<typename T, uint32_t reservedHeaderLen, typename ENABLE = void>
	struct ListBBSwitcher
	{
		static void ToBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb);
		static int FromBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb);
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb);
	};

	// ���� 1 �ֽڳ��ȵ� ��ֵ ��ö�� �� float( ��Щ����ֱ�� memcpy )
	template<typename T, uint32_t reservedHeaderLen>
	struct ListBBSwitcher<T, reservedHeaderLen, std::enable_if_t< sizeof(T) == 1 || (std::is_floating_point<T>::value && sizeof(T) == 4) >>
	{
		static void ToBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb)
		{
			// todo
		}
		static int FromBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb)
		{
			// todo
			return 0;
		}
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb)
		{
			// todo
		}
	};

	// ��������( ֻ�� foreach һ������ )
	template<typename T, uint32_t reservedHeaderLen>
	struct ListBBSwitcher<T, reservedHeaderLen, std::enable_if_t< sizeof(T) != 1 && std::is_floating_point<T>::value && sizeof(T) != 4 >>
	{
		static void ToBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb)
		{
			// todo
		}
		static int FromBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb)
		{
			// todo
			return 0;
		}
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* const& list, BBuffer &bb)
		{
			// todo
		}
	};


	template<typename T, uint32_t reservedHeaderLen>
	List<T, reservedHeaderLen>::List(BBuffer* bb)
	{
		ListBBSwitcher<T, reservedHeaderLen>::CreateFromBBuffer(this, *bb);
	}

	template<typename T, uint32_t reservedHeaderLen>
	void List<T, reservedHeaderLen>::ToBBuffer(BBuffer &bb) const
	{
		ListBBSwitcher<T, reservedHeaderLen>::ToBBuffer(this, *bb);
	}

	template<typename T, uint32_t reservedHeaderLen>
	int List<T, reservedHeaderLen>::FromBBuffer(BBuffer &bb)
	{
		return ListBBSwitcher<T, reservedHeaderLen>::FromBBuffer(this, *bb);
	}

}
