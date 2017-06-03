#pragma once
namespace xx
{

	inline UV::UV()
		: listeners(mempool())
		, clients(mempool())
	{
		loop = uv_default_loop();
	}

	inline UV::~UV()
	{
		// todo release container items
	}

	inline void UV::Run()
	{
		uv_run(loop, UV_RUN_DEFAULT);
	}



	template<typename ListenerType>
	ListenerType* UV::CreateListener(int port, int backlog)
	{
		return mempool().Create<ListenerType>(this, port, backlog);
	}

	template<typename ClientType>
	ClientType* UV::CreateClient()
	{
		return mempool().Create<ClientType>(this);
	}






	inline UVListener::UVListener(UV* uv, int port, int backlog)
		: uv(uv)
		, uv_listeners_index(uv->listeners->dataLen)
		, peers(mempool())
	{
		struct sockaddr_in addr;
		uv_ip4_addr("0.0.0.0", port, &addr);

		if (uv_tcp_init(uv->loop, &tcpServer)) throw - 1;
		if (uv_tcp_bind(&tcpServer, (const struct sockaddr*) &addr, 0)) throw - 2;
		if (uv_listen((uv_stream_t*)&tcpServer, backlog, OnConnect)) throw - 3;

		uv->listeners->Add(this);
	}

	inline UVListener::~UVListener()
	{
		// todo release peers?

		XX_LIST_SWAP_REMOVE(uv->listeners, this, uv_listeners_index);
	}

	inline void UVListener::OnConnect(uv_stream_t* server, int status)
	{
		auto self = container_of(server, UVListener, tcpServer);
		if (status)
		{
			// todo: connect error log
			return;
		}
		self->OnCreatePeer();
	}






	inline UVServerPeer::UVServerPeer(UVListener* listener)
		: uv(listener->uv)
		, listener(listener)
		, listener_peers_index(listener->peers->dataLen)
		, bbReceive(mempool())
		, bbReceiveLeft(mempool())
		, bbReceivePackage(mempool())
	{
		if (uv_tcp_init(uv->loop, (uv_tcp_t*)&stream)) throw - 1;

		listener->peers->Add(this);

		if (uv_accept((uv_stream_t*)&listener->tcpServer, (uv_stream_t*)&stream))
		{
			uv_close((uv_handle_t*)&stream, CloseCB);
			return;
		}

		if (uv_read_start((uv_stream_t*)&stream, AllocCB, ReadCB))
		{
			uv_close((uv_handle_t*)&stream, CloseCB);
			return;
		}
	}

	inline UVServerPeer::~UVServerPeer()
	{
		bbReceivePackage->buf = nullptr;
		bbReceivePackage->bufLen = 0;
		bbReceivePackage->dataLen = 0;
		bbReceivePackage->offset = 0;

		XX_LIST_SWAP_REMOVE(listener->peers, this, listener_peers_index);
	}

	inline void UVServerPeer::OnReceive()
	{
		// ��ʵ�ֶ��� 2 �ֽڰ�ͷ�İ汾

		// ��� bbReceiveLeft û����, ��ֱ���� bbReceive �Ͻ��а��������ж�. ����ں�������, ���ϴ��� OnReceivePackage ����֮, ���ʣ�µ������Ƶ� bbReceiveLeft
		if (!bbReceiveLeft->dataLen)
		{
			// ��ʼ����
		LabBegin:
			uint16_t dataLen = 0;

			// �ж�ͷ����. ���������, ��ʣ������׷�ӵ� bbReceiveLeft ���˳�
			if (bbReceive->dataLen < bbReceive->offset + sizeof(dataLen))
			{
				bbReceiveLeft->Write(bbReceive->buf[bbReceive->offset++]);		// ����ֻ������1�ֽ�
				return;
			}

			// ����ͷ
			dataLen = bbReceive->buf[bbReceive->offset] + (bbReceive->buf[bbReceive->offset + 1] << 8);
			bbReceive->offset += 2;

			// ��������������㹻, ��һ�� OnReceivePackage ���ظ�����ͷ + ���ݵĹ���
			if (bbReceive->offset + dataLen <= bbReceive->dataLen)
			{
				bbReceivePackage->buf = bbReceive->buf + bbReceive->offset;
				bbReceivePackage->bufLen = dataLen;
				bbReceivePackage->dataLen = dataLen;
				bbReceivePackage->offset = 0;

				OnReceivePackage(*bbReceivePackage);

				// �����Ѵ���������ݶβ�������������
				bbReceive->offset += dataLen;
				if (bbReceive->dataLen > bbReceive->offset) goto LabBegin;
			}
			// ����ʣ������׷�ӵ� bbReceiveLeft ���˳�
			else
			{
				bbReceiveLeft->WriteBuf(bbReceive->buf + bbReceive->offset, bbReceive->dataLen - bbReceive->offset);
			}
		}
		// ��� bbReceiveLeft ������, ���Դ� bbReceive ����һ���������ݴ��� OnReceivePackage ��������, ���� bbReceiveLeft û���ݵ�����
		else
		{
			uint16_t dataLen = 0;

			// �ж�ͷ����. ���������, �����ܲ��ܲ���
			if (bbReceiveLeft->offset + sizeof(dataLen) > bbReceiveLeft->dataLen)
			{
				// ������ٴ����ͷ
				auto left = bbReceiveLeft->offset + sizeof(dataLen) - bbReceiveLeft->dataLen;

				// ���ʣ�����ݳ����޷�����, �ƶ�ʣ�����ݵ�ͷ����׷�Ӹ��յ������ݺ��˳�
				if (bbReceive->offset + left > bbReceive->dataLen) goto LabEnd;
				else
				{
					bbReceiveLeft->Write(bbReceive->buf[bbReceive->offset++]);		// ����ֻ���ܲ�1�ֽڲ����ͷ
				}
			}

			// ����ͷ, �õ�����
			dataLen = bbReceiveLeft->buf[bbReceiveLeft->offset] + (bbReceiveLeft->buf[bbReceiveLeft->offset + 1] << 8);
			bbReceiveLeft->offset += 2;
			auto offset_bak = bbReceiveLeft->offset;

			// �ж�����������. ���������, �����ܲ��ܲ���
			if (bbReceiveLeft->offset + dataLen > bbReceiveLeft->dataLen)
			{
				// ������ٴ���������
				auto left = bbReceiveLeft->offset + dataLen - bbReceiveLeft->dataLen;

				// ���ʣ�����ݳ����޷�����, �ƶ�ʣ�����ݵ�ͷ����׷�Ӹ��յ������ݺ��˳�
				if (bbReceive->offset + left > bbReceive->dataLen) goto LabEnd;
				else
				{
					bbReceiveLeft->WriteBuf(bbReceive->buf + bbReceive->offset, left);
					bbReceive->offset += left;
				}
			}

			// �����������㹻, ��һ�� OnReceivePackage
			bbReceivePackage->buf = bbReceiveLeft->buf + bbReceiveLeft->offset;
			bbReceivePackage->bufLen = dataLen;
			bbReceivePackage->dataLen = dataLen;
			bbReceivePackage->offset = 0;

			OnReceivePackage(*bbReceivePackage);

			// ��� bbReceiveLeft �е�����, �������ʣ������, ���� bbReceive �������μ���. 
			bbReceiveLeft->dataLen = 0;
			bbReceiveLeft->offset = 0;
			if (bbReceive->dataLen > bbReceive->offset) goto LabBegin;

		LabEnd:
			// �ƶ� bbReceiveLeft ʣ�����ݵ�ͷ��
			auto left = bbReceiveLeft->dataLen - bbReceiveLeft->offset;
			memmove(bbReceiveLeft->buf, bbReceiveLeft->buf + bbReceiveLeft->offset, left);
			bbReceiveLeft->dataLen = left;
			bbReceiveLeft->offset = 0;
			// ׷�� bbReceive ʣ�����ݵ� bbReceiveLeft
			bbReceiveLeft->WriteBuf(bbReceive->buf + bbReceive->offset, bbReceive->dataLen - bbReceive->offset);
		}
	}

	void UVServerPeer::AllocCB(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		auto self = container_of(handle, UVServerPeer, stream);
		self->bbReceive->Reserve((uint32_t)suggested_size);
		buf->base = self->bbReceive->buf;
		buf->len = self->bbReceive->bufLen;
	}

	void UVServerPeer::CloseCB(uv_handle_t* handle)
	{
		auto self = container_of(handle, UVServerPeer, stream);
		self->Release();
	}

	void UVServerPeer::ReadCB(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
	{
		auto self = container_of(handle, UVServerPeer, stream);
		if (nread < 0)
		{
			/* Error or EOF */
			auto rtv = uv_shutdown(&self->sreq, handle, ShutdownCB);
			assert(!rtv);
			return;
		}
		if (nread == 0)
		{
			/* Everything OK, but nothing read. */
			return;
		}
		assert(buf->base == self->bbReceive->buf && buf->len == self->bbReceive->bufLen);
		self->bbReceive->dataLen = (uint32_t)nread;
		self->bbReceive->offset = 0;
		self->OnReceive();
	}

	void UVServerPeer::ShutdownCB(uv_shutdown_t* req, int status)
	{
		uv_close((uv_handle_t*)req->handle, CloseCB);
	}







	inline UVClient::UVClient(UV* uv)
	{

	}
	inline UVClient::~UVClient()
	{

	}
	bool UVClient::SetAddress(/*, addr */)
	{
		return false;
	}
	bool UVClient::Connect()
	{
		return false;
	}

}
