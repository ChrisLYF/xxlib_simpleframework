//#include <xx_mempool.h>
//#include <xx_bbuffer.h>
//
//namespace xx
//{
//	// �����С��ṩ���ֽ�����ɢ pop �Ĺ���
//	template<class OutType>
//	struct BBufferQueue
//	{
//		// todo: xx_queue
//
//		//std::deque<BBuffer> bufs;
//		size_t numPopBufs = 0;          // �� pop buf ����
//
//		size_t numPushLen = 0;          // �� push �ֽ��� for ͳ�� / ����ɶ��
//		size_t numPopLen = 0;           // �� pop �ֽ��� for ͳ�� / ����ɶ��
//
//		size_t bufIndex = 0;            // ���� buf ����( ��ȥ numPopBufs ���� bufs �±� )
//		size_t byteOffset = 0;          // ���� buf �Ĵ���������ʼ����
//
//		BufPool<bufLen> &bufPool;
//		BBufferQueue(BufPool<bufLen> &bufPool) : bufPool(bufPool) {}
//		~BBufferQueue()
//		{
//			Clear();
//		}
//
//
//		// ������ѹ���Ǹ��� ʣ��ռ仹�����ҷ����ڷ���״̬, ��ȡ����������������. �����Ǵӳ��з���
//		BufType Alloc(size_t maxDataLen)
//		{
//			assert(BufType::bufLen >= maxDataLen);
//			if (maxDataLen && bufs.size() + numPopBufs > bufIndex)
//			{
//				auto& p = bufs.back();
//				if (maxDataLen + p.dataLen() <= BufType::bufLen)
//				{
//					numPushLen -= p.dataLen();
//					auto r = std::move(bufs.back());
//					bufs.pop_back();
//					return r;
//				}
//			}
//			return bufPool.Alloc();
//		}
//		void Free(BufType &&p)
//		{
//			bufPool.Free(std::move(p));
//		}
//
//
//		template<typename ...PKGS>
//		void Push(PKGS&&...vs)
//		{
//			std::initializer_list<int>{ (
//				numPushLen += vs.dataLen(),
//				bufs.push_back(std::forward<PKGS>(vs)),
//				0)... };
//		}
//
//		// �´ε���ʱ��������һ�ε��ڴ� ��ȷ������ host �����ͳɹ�. Ҳ����˵ֻ�з��ͳɹ�֮������ٴε���.
//		// �������ʧ��, Ҫ����, outs ��ֵ�ǿ��Է���ʹ�õ�.
//		size_t PopTo(std::vector<OutType>& outs, size_t len)
//		{
//			outs.clear();
//			if (!len)
//			{
//				return 0;
//			}
//
//			if (bufIndex != numPopBufs)     // �����ڴ�
//			{
//				for (size_t i = 0; i < bufIndex - numPopBufs; i++)
//				{
//					bufPool.Free(std::move(bufs.front()));
//					bufs.pop_front();
//				}
//				numPopBufs = bufIndex;
//			}
//
//			auto maxIdx = min(64, bufs.size());    // ��˵��������ָ��һ��ֻ֧����� 64 ������
//			auto idx = bufIndex - numPopBufs;
//			if (idx >= maxIdx)
//			{
//				return 0;
//			}
//
//			auto bak_len = len;
//			while (len > 0 && idx < maxIdx)
//			{
//				auto& p = bufs[idx];
//				auto left = p.dataLen() - byteOffset;
//				if (left <= len)
//				{
//					len -= left;
//					++idx;
//					outs.push_back(BufQueue_MakeOutType<OutType>(p.buf() + byteOffset, left));
//					byteOffset = 0;
//				}
//				else
//				{
//					outs.push_back(BufQueue_MakeOutType<OutType>(p.buf() + byteOffset, len));
//					byteOffset += len;
//					len = 0;
//				}
//			}
//			bufIndex = idx + numPopBufs;
//			numPopLen += bak_len - len;
//			return bak_len - len;
//		}
//
//		void Clear()
//		{
//			for (auto& buf : bufs)
//			{
//				bufPool.Free(std::move(buf));
//			}
//			bufs.clear();
//			numPopBufs = numPopLen = bufIndex = byteOffset = 0;
//		}
//
//		bool Empty()
//		{
//			return bufs.empty();
//		}
//
//		size_t BytesCount()
//		{
//			return numPushLen - numPopLen;
//		}
//	};
//}
//
//int main()
//{
//
//	return 0;
//}
