#pragma once
#include "xx_mempool.h"
#include "xx_bbuffer.h"
#include "xx_queue.h"

namespace xx
{
	// �����С��ṩ���ֽ�����ɢ pop �Ĺ���
	// ��ֱ��ʹ�� BBuffer ��ʵ��, ָ�뷽ʽʹ��, �����ü���ɾ��
	struct BBQueue : protected Queue<BBuffer*>
	{
		typedef Queue<BBuffer*> BaseType;
		uint32_t numPopBufs = 0;          // �� pop buf ����

		uint32_t numPushLen = 0;          // �� push �ֽ��� for ͳ�� / ����ɶ��
		uint32_t numPopLen = 0;           // �� pop �ֽ��� for ͳ�� / ����ɶ��

		uint32_t bufIndex = 0;            // ���� buf ����( ��ȥ numPopBufs ���� bufs �±� )
		uint32_t byteOffset = 0;          // ���� buf �Ĵ���������ʼ����

		BBQueue(BBQueue const& o) = delete;
		BBQueue& operator=(BBQueue const& o) = delete;

		explicit BBQueue(uint32_t capacity = 8)
			: BaseType(capacity)
		{}

		BBQueue(BBQueue &&o)
			: BaseType((BaseType&&)o)
		{}

		~BBQueue()
		{
			Clear();
		}

		// �������β�����ڣ��� �����ڷ���״̬ �� ���ü���Ϊ 1( ��Ⱥ�� ), �ͷ��������ڼ������. ���򷵻ؿ�.
		BBuffer* PopLastBB()
		{
			if (this->Count() + numPopBufs > bufIndex && this->Last()->refCount() == 1)
			{
				numPushLen -= this->Last()->dataLen;
				auto bb = this->Last();
				this->PopLast();
				return bb;
			}
			return nullptr;
		}

		// ���������� bb ѹ������й�( ��ͬ����Ӧ��ͳ����ֵ, PopTo ���Զ� Release ), ֮�󲻿����ټ������� bb
		void Push(BBuffer* const& bb)
		{
			numPushLen += bb->dataLen;
			this->BaseType::Push(bb);
		}

		// ����ָ���ֽڳ��ȵ�ָ������( [ { len, bufPtr,  }, ... ] ��ʽ ), ����ʵ�ʵ����ֽ���
		// �´ε���ʱ��������һ�ε��ڴ� ��ȷ�����ݳ��е����ͳɹ�. Ҳ����˵ֻ�з��ͳɹ�֮������ٴε��øú���.
		// �������ʧ��, Ҫ����, outBufs ��ֵ�ǿ��Է���ʹ�õ�.
		template<typename T, uint32_t maxBufsCount = 64>
		uint32_t PopTo(List<T>& outBufs, uint32_t len)
		{
			outBufs.Clear();
			if (!len) return 0;

			if (bufIndex != numPopBufs)							// �����ڴ�
			{
				for (uint32_t i = 0; i < bufIndex - numPopBufs; i++)
				{
					this->At(0)->Release();
					this->Pop();
				}
				numPopBufs = bufIndex;
			}

			auto maxIdx = MIN(maxBufsCount, this->Count());		// ��˵��������ָ��һ��ֻ֧����� 64 ������
			auto idx = uint32_t(bufIndex - numPopBufs);
			if (idx >= maxIdx) return 0;						// û������Ҫ��

			auto bak_len = len;
			while (len > 0 && idx < maxIdx)
			{
				auto& bb = this->At(idx);
				auto left = bb->dataLen - byteOffset;
				if (left <= len)
				{
					len -= left;
					++idx;
					outBufs.Add({ left, bb->buf + byteOffset });
					byteOffset = 0;
				}
				else
				{
					outBufs.Add({ len, bb->buf + byteOffset });
					byteOffset += len;
					len = 0;
				}
			}
			bufIndex = idx + numPopBufs;
			numPopLen += bak_len - len;
			return bak_len - len;
		}

		// �����������, �ͷ����� bb
		void Clear()
		{
			while (!this->Empty())
			{
				this->Top()->Release();
				this->Pop();
			}
			numPopBufs = 0;
			numPopLen = 0;
			bufIndex = 0;
			byteOffset = 0;
		}

		// ��ȡ��ǰ���ж����ֽڵ����ݴ���
		uint32_t BytesCount()
		{
			return numPushLen - numPopLen;
		}
	};




	/*************************************************************************/
	// ֵ����ʹ����̬��װ
	/*************************************************************************/

	using BBQueue_v = MemHeaderBox<BBQueue>;

	template<>
	struct MemmoveSupport<MemHeaderBox<BBQueue>>
	{
		static const bool value = true;
	};

}
