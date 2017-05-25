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

	// !( MPObject* || MPtr || MemHeaderBox )
	template<typename T>
	struct BBufferRWSwitcher<T, std::enable_if_t< !(std::is_pointer<T>::value && IsMPObject_v<T> || IsMPtr_v<T> || IsMemHeaderBox_v<T>) >>
	{
		static void Write(BBuffer* bb, T const& v);
		static int Read(BBuffer* bb, T& v);
	};

	// MPObject* || MPtr
	template<typename T>
	struct BBufferRWSwitcher<T, std::enable_if_t< std::is_pointer<T>::value && IsMPObject_v<T> || IsMPtr<T>::value >>
	{
		static void Write(BBuffer* bb, T const& v);
		static int Read(BBuffer* bb, T& v);
	};

	// MemHeaderBox
	template<typename T>
	struct BBufferRWSwitcher<T, std::enable_if_t< IsMemHeaderBox_v<T> >>
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
		BBuffer(BBuffer* bb) : BaseType(bb) {}

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
		void WritePods(TS const& ...vs)
		{
			this->Reserve(this->dataLen + BBCalc(vs...));
			this->dataLen += BBWriteTo(this->buf + this->dataLen, vs...);
			assert(this->dataLen <= this->bufLen);
		}

		template<typename T, typename ...TS>
		int ReadPods(T &v, TS&...vs)
		{
			return BBReadFrom(this->buf, this->dataLen, this->offset, v, vs...);
		}

		int ReadPods() { return 0; }


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

		// һ��ˬ write
		template<typename T>
		void WriteRoot(T const& v)
		{
			BeginWrite();
			Write(v);
			EndWrite();
		}
		// һ��ˬ read
		template<typename T>
		int ReadRoot(T &v)
		{
			BeginRead();
			auto rtv = Read(v);
			EndRead();
			return rtv;
		}

		template<typename T>
		void WritePtr(T* const& v)
		{
			assert(ptrStore);
			if (!v)
			{
				WritePods((uint8_t)0);
				return;
			}
			WritePods(v->typeId());

			auto rtv = ptrStore->Add((void*)v, dataLen);
			WritePods(ptrStore->ValueAt(rtv.index));
			if (rtv.success)
			{
				v->ToBBuffer(*this);
			}
		}
		template<typename T>
		int ReadPtr(T* &v)
		{
			assert(idxStore);

			// get typeid
			uint16_t tid;
			if (auto rtv = ReadPods(tid)) return rtv;

			// isnull ?
			if (tid == 0)
			{
				v = nullptr;
				return 0;
			}

			// get offset
			uint32_t ptr_offset = 0, bb_offset_bak = offset;
			if (auto rtv = ReadPods(ptr_offset)) return rtv;

			// fill or ref
			if (ptr_offset == bb_offset_bak)
			{
				// ensure inherit
				if (!mempool().IsBaseOf(TypeId<T>::value, tid)) return -2;

				// try get creator func
				auto f = MemPool::creators()[tid];
				assert(f);

				// try create & read from bb
				v = (T*)f(&mempool(), this, ptr_offset);
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
		void WritePtr(MPtr<T> const& v)
		{
			Write(v.Ensure());
		}
		template<typename T>
		int ReadPtr(MPtr<T> &v)
		{
			T* t;
			auto rtv = Read(t);
			v = t;
			return rtv;
		}



		template<typename T>
		void WriteBox(MemHeaderBox<T> const& v)
		{
			v->ToBBuffer(*this);
		}
		template<typename T>
		int ReadBox(MemHeaderBox<T> &v)
		{
			return v->FromBBuffer(*this);
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

	// !( MPObject* || MPtr || MemHeaderBox )

	template<typename T>
	void BBufferRWSwitcher<T, std::enable_if_t< !(std::is_pointer<T>::value && IsMPObject_v<T> || IsMPtr_v<T> || IsMemHeaderBox_v<T>) >>::Write(BBuffer* bb, T const& v)
	{
		bb->WritePods(v);
	}
	template<typename T>
	int BBufferRWSwitcher<T, std::enable_if_t< !(std::is_pointer<T>::value && IsMPObject_v<T> || IsMPtr_v<T> || IsMemHeaderBox_v<T>) >>::Read(BBuffer* bb, T& v)
	{
		return bb->ReadPods(v);
	}

	// MPObject* || MPtr

	template<typename T>
	void BBufferRWSwitcher<T, std::enable_if_t< std::is_pointer<T>::value && IsMPObject_v<T> || IsMPtr<T>::value >>::Write(BBuffer* bb, T const& v)
	{
		bb->WritePtr(v);
	}
	template<typename T>
	int BBufferRWSwitcher<T, std::enable_if_t< std::is_pointer<T>::value && IsMPObject_v<T> || IsMPtr<T>::value >>::Read(BBuffer* bb, T& v)
	{
		return bb->ReadPtr(v);
	}

	// MemHeaderBox

	template<typename T>
	void BBufferRWSwitcher<T, std::enable_if_t< IsMemHeaderBox_v<T> >>::Write(BBuffer* bb, T const& v)
	{
		bb->WriteBox(v);
	}
	template<typename T>
	int BBufferRWSwitcher<T, std::enable_if_t< IsMemHeaderBox_v<T> >>::Read(BBuffer* bb, T& v)
	{
		return bb->ReadBox(v);
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


	void BBuffer::ToBBuffer(BBuffer &bb) const
	{
		bb.Reserve(bb.dataLen + 5 + this->dataLen);
		bb.Write(this->dataLen);
		if (!this->dataLen) return;
		memcpy(bb.buf + bb.dataLen, this->buf, this->dataLen);
		bb.dataLen += this->dataLen;
	}

	int BBuffer::FromBBuffer(BBuffer &bb)
	{
		uint32_t len = 0;
		if (auto rtv = bb.Read(len)) return rtv;
		if (bb.offset + len > bb.dataLen) return -1;
		this->Resize(len);
		if (len == 0) return 0;
		memcpy(this->buf, bb.buf + bb.offset, len);
		bb.offset += len;
		return 0;
	}


	// List ���л� ·����
	template<typename T, uint32_t reservedHeaderLen, typename ENABLE = void>
	struct ListBBSwitcher
	{
		static void ToBBuffer(List<T, reservedHeaderLen> const* list, BBuffer &bb);
		static int FromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb);
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb);
	};

	// ���� 1 �ֽڳ��ȵ� ��ֵ ��ö�� �� float( ��Щ����ֱ�� memcpy )
	template<typename T, uint32_t reservedHeaderLen>
	struct ListBBSwitcher<T, reservedHeaderLen, std::enable_if_t< sizeof(T) == 1 || std::is_same<float, typename std::decay<T>::type>::value >>
	{
		static void ToBBuffer(List<T, reservedHeaderLen> const* list, BBuffer &bb)
		{
			bb.Reserve(bb.dataLen + 5 + list->dataLen);
			bb.Write(list->dataLen);
			if (!list->dataLen) return;
			memcpy(bb.buf + bb.dataLen, list->buf, list->dataLen);
			bb.dataLen += list->dataLen;
		}
		static int FromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb)
		{
			uint32_t len = 0;
			if (auto rtv = bb.Read(len)) return rtv;
			if (bb.offset + len > bb.dataLen) return -1;
			list->Resize(len);
			if (len == 0) return 0;
			memcpy(list->buf, bb.buf + bb.offset, len);
			bb.offset += len;
			return 0;
		}
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb)
		{
			uint32_t len = 0;
			if (auto rtv = bb.ReadPods(len)) throw rtv;
			if (bb.offset + len * sizeof(T) > bb.dataLen) throw - 1;
			if (len == 0) return;
			list->Reserve(len);
			memcpy(list->buf, bb.buf + bb.offset, len * sizeof(T));
			bb.offset += len * sizeof(T);
			list->dataLen = len;
		}
	};

	// ����� MPObject* / MPtr ( ֻ�� foreach һ������, �� MemHeaderBox )
	template<typename T, uint32_t reservedHeaderLen>
	struct ListBBSwitcher<T, reservedHeaderLen, std::enable_if_t< !(sizeof(T) == 1 || std::is_same<float, typename std::decay<T>::type>::value) && !(IsMPtr_v<T> || (std::is_pointer<T>::value && IsMPObject_v<T>)) >>
	{
		static void ToBBuffer(List<T, reservedHeaderLen> const* list, BBuffer &bb)
		{
			bb.Write(list->dataLen);
			if (!list->dataLen) return;
			for (uint32_t i = 0; i < list->dataLen; ++i)
			{
				bb.Write(list->At(i));
			}
		}
		static int FromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb)
		{
			uint32_t len = 0;
			if (auto rtv = bb.Read(len)) return rtv;
			list->Resize(len);
			if (len == 0) return 0;
			for (uint32_t i = 0; i < len; ++i)
			{
				if (auto rtv = bb.Read(list->At(i)))
				{
					list->Resize(i);
					return rtv;
				}
			}
			return 0;
		}
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb)
		{
			uint32_t len = 0;
			if (auto rtv = bb.Read(len)) throw rtv;
			list->Resize(len);
			if (len == 0) return;
			for (uint32_t i = 0; i < len; ++i)
			{
				if (auto rtv = bb.Read(list->At(i)))
				{
					list->Clear(true);
					throw rtv;
				}
			}
		}
	};

	// ���� MPObject* / MPtr, �����л�ʧ�ܺ����� Release �ѳɹ������� items
	template<typename T, uint32_t reservedHeaderLen>
	struct ListBBSwitcher<T, reservedHeaderLen, std::enable_if_t< IsMPtr_v<T> || (std::is_pointer<T>::value && IsMPObject_v<T>) >>
	{
		static void ToBBuffer(List<T, reservedHeaderLen> const* list, BBuffer &bb)
		{
			bb.Write(list->dataLen);
			if (!list->dataLen) return;
			for (uint32_t i = 0; i < list->dataLen; ++i)
			{
				bb.Write(list->At(i));
			}
		}
		static int FromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb)
		{
			uint32_t len = 0;
			if (auto rtv = bb.Read(len)) return rtv;
			list->Resize(len);
			if (len == 0) return 0;
			for (uint32_t i = 0; i < len; ++i)
			{
				if (auto rtv = bb.Read(list->At(i)))
				{
					for (uint32_t j = i - 1; j != (uint32_t)-1; --j)
					{
						if (list->At(j)) list->At(j)->Release();
					}
					list->Resize(0);
					return rtv;
				}
			}
			return 0;
		}
		static void CreateFromBBuffer(List<T, reservedHeaderLen>* list, BBuffer &bb)
		{
			uint32_t len = 0;
			if (auto rtv = bb.Read(len)) throw rtv;
			list->Resize(len);
			if (len == 0) return;
			for (uint32_t i = 0; i < len; ++i)
			{
				if (auto rtv = bb.Read(list->At(i)))
				{
					for (uint32_t j = i - 1; j != (uint32_t)-1; --j)
					{
						if (list->At(j)) list->At(j)->Release();
					}
					list->Clear(true);
					throw rtv;
				}
			}
		}
	};

	template<typename T, uint32_t reservedHeaderLen>
	List<T, reservedHeaderLen>::List(BBuffer* bb)
		: buf(nullptr)
		, bufLen(0)
		, dataLen(0)
	{
		ListBBSwitcher<T, reservedHeaderLen>::CreateFromBBuffer(this, *bb);
	}

	template<typename T, uint32_t reservedHeaderLen>
	void List<T, reservedHeaderLen>::ToBBuffer(BBuffer &bb) const
	{
		ListBBSwitcher<T, reservedHeaderLen>::ToBBuffer(this, bb);
	}

	template<typename T, uint32_t reservedHeaderLen>
	int List<T, reservedHeaderLen>::FromBBuffer(BBuffer &bb)
	{
		return ListBBSwitcher<T, reservedHeaderLen>::FromBBuffer(this, bb);
	}

}
