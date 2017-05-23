#pragma once
#include "xx_mempool.h"
#include <cstring>

namespace xx
{
	struct BBuffer;

	// ���� std vector / .net List �ļ�����
	// reservedHeaderLen Ϊ���� buf �ڴ����ǰ��ճ�һ���ڴ治��, Ҳ����ʼ��, ���ݲ�����( Ϊ����ͷ�����ݴ������ )
	template<typename T, uint32_t reservedHeaderLen = 0>
	struct List : public MPObject
	{
		typedef T ChildType;
		T*			buf;
		uint32_t    bufLen;
		uint32_t    dataLen;

		explicit List(uint32_t const& capacity = 0)
		{
			if (capacity == 0)
			{
				buf = nullptr;
				bufLen = 0;
			}
			else
			{
				auto bufByteLen = Round2n((reservedHeaderLen + capacity) * sizeof(T) + sizeof(MemHeader_VersionNumber)) - sizeof(MemHeader_VersionNumber);
				buf = (T*)mempool().Alloc((uint32_t)bufByteLen) + reservedHeaderLen;
				bufLen = uint32_t(bufByteLen / sizeof(T)) - reservedHeaderLen;
			}
			dataLen = 0;
		}
		~List()
		{
			Clear(true);
		}
		List(List const&o) = delete;
		List(List &&o)
			: buf(o.buf)
			, bufLen(o.bufLen)
			, dataLen(o.dataLen)
		{
			o.buf = nullptr;
			o.bufLen = 0;
			o.dataLen = 0;
		}
		List& operator=(List const&o) = delete;
		List(BBuffer* bb);


		void Reserve(uint32_t const& capacity)
		{
			if (capacity <= bufLen) return;

			auto newBufByteLen = Round2n(reservedHeaderLen + capacity * sizeof(T) + sizeof(MemHeader_VersionNumber)) - sizeof(MemHeader_VersionNumber);
			auto newBuf = (T*)mempool().Alloc((uint32_t)newBufByteLen) + reservedHeaderLen;

			if (std::is_trivial<T>::value || MemmoveSupport_v<T>)
			{
				memcpy(newBuf, buf, dataLen * sizeof(T));
			}
			else
			{
				for (uint32_t i = 0; i < dataLen; ++i)
				{
					new (&newBuf[i]) T((T&&)buf[i]);
					buf[i].~T();
				}
			}
			// memcpy(newBuf - reservedHeaderLen, buf - reservedHeaderLen, reservedHeaderLen * sizeof(T));

			if (buf) mempool().Free(buf - reservedHeaderLen);
			buf = newBuf;
			bufLen = uint32_t(newBufByteLen / sizeof(T)) - reservedHeaderLen;
		}

		// ��������ǰ�ĳ���
		uint32_t Resize(uint32_t const& len)
		{
			if (len == dataLen) return dataLen;
			else if (len < dataLen && !std::is_trivial<T>::value)
			{
				for (uint32_t i = len; i < dataLen; ++i)
				{
					buf[i].~T();
				}
			}
			else // len > dataLen
			{
				Reserve(len);
				if (!std::is_pod<T>::value)
				{
					for (uint32_t i = dataLen; i < len; ++i)
					{
						new (buf + i) T();
					}
				}
			}
			auto rtv = dataLen;
			dataLen = len;
			return rtv;
		}

		T const& operator[](uint32_t const& idx) const
		{
			assert(idx < dataLen);
			return buf[idx];
		}
		T& operator[](uint32_t const& idx)
		{
			assert(idx < dataLen);
			return buf[idx];
		}

		T const& At(uint32_t const& idx) const
		{
			assert(idx < dataLen);
			return buf[idx];
		}
		T& At(uint32_t const& idx)
		{
			assert(idx < dataLen);
			return buf[idx];
		}

		uint32_t Count() const
		{
			return dataLen;
		}

		T& Top()
		{
			assert(dataLen > 0);
			return buf[dataLen - 1];
		}
		void Pop()
		{
			assert(dataLen > 0);
			--dataLen;
			buf[dataLen].~T();
		}
		T const& Top() const
		{
			assert(dataLen > 0);
			return buf[dataLen - 1];
		}
		bool TryPop(T& output)
		{
			if (!dataLen) return false;
			output = (T&&)buf[--dataLen];
			buf[dataLen].~T();
			return true;
		}

		void Clear(bool const& freeBuf = false)
		{
			if (!buf) return;
			if (dataLen)
			{
				for (uint32_t i = dataLen - 1; i != (uint32_t)-1; --i)
				{
					buf[i].~T();
				}
				dataLen = 0;
			}
			if (freeBuf)
			{
				mempool().Free(buf - reservedHeaderLen);
				buf = nullptr;
				bufLen = 0;
			}
		}

		// �Ƴ�ָ��������Ԫ��. Ϊ��������, ���ܲ����ڴ��ƶ�
		void RemoveAt(uint32_t const& idx)
		{
			assert(idx < dataLen);
			--dataLen;
			if (std::is_trivial<T>::value || MemmoveSupport_v<T>)
			{
				buf[idx].~T();
				memmove(buf + idx, buf + idx + 1, (dataLen - idx) * sizeof(T));
			}
			else
			{
				for (uint32_t i = idx; i < dataLen; ++i)
				{
					buf[i] = (T&&)buf[i + 1];
				}
				buf[dataLen].~T();
			}
		}

		// �������Ҳ��Ƴ�
		void Remove(T const& v)
		{
			for (uint32_t i = 0; i < dataLen; ++i)
			{
				if (EqualsTo(v, buf[i]))
				{
					RemoveAt(i);
					return;
				}
			}
		}

	protected:

		// ��Խ����
		void FastAdd(T const& v)
		{
			new (buf + dataLen) T(v);
			++dataLen;
		}

		// ��Խ����( ��ֵ�� )
		void FastAdd(T&& v)
		{
			new (buf + dataLen) T((T&&)v);
			++dataLen;
		}

	public:

		// �ƶ� / �������һ�����
		template<typename...VS>
		void AddMulti(VS&&...vs)
		{
			static_assert(sizeof...(vs), "lost args ??");
			Reserve(dataLen + sizeof...(vs));
			std::initializer_list<int>{ (FastAdd(std::forward<VS>(vs)), 0)... };
		}

		// ׷��һ�� item( ��ֵ�� )
		void Add(T&& v)
		{
			Emplace((T&&)v);
		}
		// ׷��һ�� item
		void Add(T const& v)
		{
			Emplace(v);
		}

		// �ò���ֱ�ӹ���һ��( ��ǰ��֧��ֱ�Ӿ��� mempool Create MPObject* ) ���ӳ�
		template<typename...Args>
		T& Emplace(Args &&... args)
		{
			Reserve(dataLen + 1);
			auto& p = buf[dataLen++];
			new (&p) T(std::forward<Args>(args)...);
			return p;
		}

		void InsertAt(uint32_t const& idx, T&& v)
		{
			EmplaceAt(idx, (T&&)v);
		}
		void InsertAt(uint32_t const& idx, T const& v)
		{
			EmplaceAt(idx, v);
		}

		// �ò���ֱ�ӹ���һ����ָ��λ��( ��ǰ��֧�־��� mempool ���� MPObject* )
		template<typename...Args>
		T& EmplaceAt(uint32_t idx, Args&&...args)
		{
			Reserve(dataLen + 1);
			if (idx < dataLen)
			{
				if (std::is_trivial<T>::value || MemmoveSupport_v<T>)
				{
					memmove(buf + idx + 1, buf + idx, (dataLen - idx) * sizeof(T));
				}
				else
				{
					new (buf + dataLen) T((T&&)buf[dataLen - 1]);
					for (uint32_t i = dataLen - 1; i > idx; --i)
					{
						buf[i] = (T&&)buf[i - 1];
					}
					buf[idx].~T();
				}
			}
			else idx = dataLen;
			++dataLen;
			new (buf + idx) T(std::forward<Args>(args)...);
			return buf[idx];
		}
		void AddRange(T const* items, uint32_t count)
		{
			Reserve(dataLen + count);
			if (std::is_trivial<T>::value || MemmoveSupport_v<T>)
			{
				std::memcpy(buf, items, count * sizeof(T));
			}
			else
			{
				for (uint32_t i = 0; i < count; ++i)
				{
					new (&buf[dataLen + i]) T((T&&)items[i]);
				}
			}
			dataLen += count;
		}



		// ����ҵ��ͷ�������. �Ҳ��������� uint32_t(-1)
		uint32_t Find(T const& v)
		{
			for (uint32_t i = 0; i < dataLen; ++i)
			{
				if (EqualsTo(v, buf[i])) return i;
			}
			return uint32_t(-1);
		}

		// ����ҵ��ͷ�������. �Ҳ��������� uint32_t(-1)
		uint32_t Find(std::function<bool(T&)> cond)
		{
			for (uint32_t i = 0; i < dataLen; ++i)
			{
				if (cond(buf[i])) return i;
			}
			return uint32_t(-1);
		}

		// �����Ƿ����
		bool Exists(std::function<bool(T&)> cond)
		{
			return Find(cond) != uint32_t(-1);
		}

		bool TryFill(T& out, std::function<bool(T&)> cond)
		{
			auto idx = Find(cond);
			if (idx == uint32_t(-1)) return false;
			out = buf[idx];
			return true;
		}


		// handler ���� false �� foreach ��ֹ
		void ForEach(std::function<bool(T&)> handler)
		{
			for (uint32_t i = 0; i < dataLen; ++i)
			{
				if (handler(buf[i])) return;
			}
		}




		// ֻ��Ϊ���� for( auto c : 
		struct Iter
		{
			T *ptr;
			bool operator!=(Iter const& other) { return ptr != other.ptr; }
			Iter& operator++()
			{
				++ptr;
				return *this;
			}
			T& operator*() { return *ptr; }
		};
		Iter begin()
		{
			return Iter{ buf };
		}
		Iter end()
		{
			return Iter{ buf + dataLen };
		}
		Iter begin() const
		{
			return Iter{ buf };
		}
		Iter end() const
		{
			return Iter{ buf + dataLen };
		}



		/*************************************************************************/
		// ʵ�� ToString �ӿ�
		/*************************************************************************/

		// ʵ�ִ����� xx_string.h

		virtual void ToString(String &str) const override;

		/*************************************************************************/
		// ʵ�� BBuffer �ӿ�
		/*************************************************************************/

		// ʵ�ִ����� xx_bbuffer.h

		virtual void ToBBuffer(BBuffer &bb) const override;

		virtual int FromBBuffer(BBuffer &bb) override;
	};


	/*************************************************************************/
	// ֵ����ʹ����̬��װ
	/*************************************************************************/

	template<typename T, uint32_t reservedHeaderLen = 0>
	using List_v = MemHeaderBox<List<T, reservedHeaderLen>>;


	template<typename T, uint32_t reservedHeaderLen>
	struct MemmoveSupport<MemHeaderBox<List<T, reservedHeaderLen>>>
	{
		static const bool value = true;
	};
}
