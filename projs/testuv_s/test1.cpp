#include <xx_mempool.h>
#include <xx_bbuffer.h>
#include <xx_queue.h>

namespace xx
{
	// �����С��ṩ���ֽ�����ɢ pop �Ĺ���
	// ��ֱ��ʹ�� BBuffer ��ʵ��, ָ�뷽ʽʹ��, �����ü���ɾ��
	struct BBufferQueue : protected Queue<BBuffer*>
	{
		typedef Queue<BBuffer*> BaseType;
		size_t numPopBufs = 0;          // �� pop buf ����

		size_t numPushLen = 0;          // �� push �ֽ��� for ͳ�� / ����ɶ��
		size_t numPopLen = 0;           // �� pop �ֽ��� for ͳ�� / ����ɶ��

		size_t bufIndex = 0;            // ���� buf ����( ��ȥ numPopBufs ���� bufs �±� )
		size_t byteOffset = 0;          // ���� buf �Ĵ���������ʼ����

		BBufferQueue(BBufferQueue const& o) = delete;
		BBufferQueue& operator=(BBufferQueue const& o) = delete;

		explicit BBufferQueue(uint32_t capacity = 8)
			: BaseType(capacity)
		{}

		BBufferQueue(BBufferQueue &&o)
			: BaseType((BaseType&&)o)
		{}

		~BBufferQueue()
		{
			Clear();
		}

		// todo: Add

		// �������β��( ���ڵĻ� )�������ڷ���״̬ �� ���ü���Ϊ 1, �ͷ�����( �����ڼ������ ). ���򷵻ؿ�.
		// ʹ�����������֮��, ��Ҫ���� numPushLen ��ֵ. ʾ��:
		/*
		auto buf = q->TryRefLastBB();
		auto bak_dataLen = 0;
		if (buf)
		{
			bak_dataLen = buf->dataLen;
		}
		else
		{
			buf = mp.Create<BBuffer>();
		}

		// fill buf

		if (bak_dataLen)
		{
			q->numPushLen += buf->dataLen - bak_dataLen;
		}
		else
		{
			q->Add(buf);
		}
		*/
		BBuffer* const& TryRefLastBB()
		{
			if (this->Count() + numPopBufs > bufIndex && this->Last()->refCount() == 1)
			{
				return this->Last();
			}
			return nullptr;
		}

		// ����ָ���ֽڳ��ȵ�ָ������( [ { buf, len }, ... ] ��ʽ )
		// �´ε���ʱ��������һ�ε��ڴ� ��ȷ�����ݳ��е����ͳɹ�. Ҳ����˵ֻ�з��ͳɹ�֮������ٴε��øú���.
		// �������ʧ��, Ҫ����, outBufs ��ֵ�ǿ��Է���ʹ�õ�.
		template<typename T, uint32_t maxBufsCount = 64>
		size_t PopBytesTo(List<T>* const& outBufs, size_t len)
		{
			//outBufs->Clear();
			//if (!len)
			//{
			//	return 0;
			//}

			//if (bufIndex != numPopBufs)     // �����ڴ�
			//{
			//	for (size_t i = 0; i < bufIndex - numPopBufs; i++)
			//	{
			//		bufPool.Free(std::move(bufs.front()));
			//		bufs.pop_front();
			//	}
			//	numPopBufs = bufIndex;
			//}

			//auto maxIdx = min(maxBufsCount, bufs.size());    // ��˵��������ָ��һ��ֻ֧����� 64 ������
			//auto idx = bufIndex - numPopBufs;
			//if (idx >= maxIdx)
			//{
			//	return 0;
			//}

			//auto bak_len = len;
			//while (len > 0 && idx < maxIdx)
			//{
			//	auto& p = bufs[idx];
			//	auto left = p.dataLen() - byteOffset;
			//	if (left <= len)
			//	{
			//		len -= left;
			//		++idx;
			//		outBufs->Add({ p.buf() + byteOffset, left });
			//		byteOffset = 0;
			//	}
			//	else
			//	{
			//		outBufs->Add({ p.buf() + byteOffset, len });
			//		byteOffset += len;
			//		len = 0;
			//	}
			//}
			//bufIndex = idx + numPopBufs;
			//numPopLen += bak_len - len;
			//return bak_len - len;
		}

		size_t BytesCount()
		{
			return numPushLen - numPopLen;
		}
	};
}

int main()
{

	return 0;
}
