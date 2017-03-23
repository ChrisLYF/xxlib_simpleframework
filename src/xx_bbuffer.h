#pragma once
#include "xx_bytesutils.h"
#include "xx_mempool.h"
#include "xx_list.h"

namespace xx
{
	struct BBuffer : public List<char, false, 16>
	{
		typedef List<char, false, 16> BaseType;
		uint32_t offset = 0;												// ��ָ��ƫ����

		explicit BBuffer(uint32_t capacity = 0) : BaseType(capacity) {}
		BBuffer(BBuffer const&o) = delete;
		BBuffer& operator=(BBuffer const&o) = delete;

		/*************************************************************************/
		// ��ͳ pod ��дϵ��( ͨ�� bytesutils ������� / �ػ� ʵ�� )
		/*************************************************************************/

		template<typename ...TS>
		void Write(TS const& ...vs)
		{
			this->Reserve(this->dataLen + BBCalc(vs...));
			this->dataLen += BBWriteTo(this->buf + this->dataLen, vs...);
			assert(this->dataLen <= this->bufLen);
		}

		template<typename T>
		int Read(T &v)
		{
			return BBReadFrom(buf, dataLen, offset, v);
		}

		template<typename T, typename ...TS>
		int Read(T &v, TS&...vs)
		{
			if (auto rtv = Read<T>(v)) return rtv;
			return Read(vs...);
		}
		int Read() { return 0; }

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

	};


	/*************************************************************************/
	// ʵ��ֵ����ʹ����������
	/*************************************************************************/

	using BBuffer_v = MemHeaderBox<BBuffer>;

}
