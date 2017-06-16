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
		{
			mempool().CreateTo(ptrStore);
			mempool().CreateTo(idxStore);
		}

		BBQueue(BBQueue &&o)
			: BaseType((BaseType&&)o)
		{
		}

		~BBQueue()
		{
			Clear();
			mempool().SafeRelease(ptrStore);
			mempool().SafeRelease(idxStore);
		}

		// ����, �Ƶ��´����� BB ��, Push ʱ�ƻ�.
		Dict<void*, uint32_t>*						ptrStore = nullptr;
		Dict<uint32_t, std::pair<void*, uint16_t>>*	idxStore = nullptr;

		// ����һ�� BBuffer ����( ����乫�� ptrStore & idxStore )
		BBuffer* CreateBB(uint32_t const& capacity = 0)
		{
			auto bb = mempool().Create<BBuffer>(capacity);
			std::swap(bb->ptrStore, ptrStore);
			std::swap(bb->idxStore, idxStore);
			return bb;
		}

		// �������β�����ڣ��� �����ڷ���״̬ �� ���ü���Ϊ 1( ��Ⱥ�� ), �ͷ��������ڼ������. �����õ�ǰ�ڴ���½�һ������.
		BBuffer* PopLastBB(uint32_t const& capacity = 0)
		{
			if (Count() + numPopBufs > bufIndex && Last()->refCount() == 1)
			{
				numPushLen -= Last()->dataLen;
				auto bb = Last();
				PopLast();
				std::swap(bb->ptrStore, ptrStore);
				std::swap(bb->idxStore, idxStore);
				return bb;
			}
			return CreateBB(capacity);
		}

		// ���������� bb ѹ������й�( ��ͬ����Ӧ��ͳ����ֵ, PopTo ���Զ� Release ), ֮�󲻿����ټ������� bb
		void Push(BBuffer* const& bb)
		{
			numPushLen += bb->dataLen;
			std::swap(bb->ptrStore, ptrStore);
			std::swap(bb->idxStore, idxStore);
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
					Top()->Release();
					Pop();
				}
				numPopBufs = bufIndex;
			}

			auto maxIdx = MIN(maxBufsCount, Count());		// ��˵��������ָ��һ��ֻ֧����� 64 ������
			auto idx = uint32_t(bufIndex - numPopBufs);
			if (idx >= maxIdx) return 0;						// û������Ҫ��

			auto bak_len = len;
			while (len > 0 && idx < maxIdx)
			{
				auto& bb = At(idx);
				auto left = bb->dataLen - byteOffset;
				if (left <= len)
				{
					len -= left;
					++idx;
					outBufs.Add(BufMaker<T>::Make(bb->buf + byteOffset, left));
					byteOffset = 0;
				}
				else
				{
					outBufs.Add(BufMaker<T>::Make(bb->buf + byteOffset, len));
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
			while (!Empty())
			{
				Top()->Release();
				Pop();
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
