#pragma once
namespace xx
{

	inline UV::UV()
		: listeners(mempool())
		, clientPeers(mempool())
	{
		loop = uv_default_loop();
	}

	inline UV::~UV()
	{
		for (int i = (int)listeners->dataLen; i >= 0; --i)
		{
			listeners->At(i)->Release();	// todo: ���� release ԭ��?
		}
		listeners->Clear();

		// todo: more release
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
		for (int i = (int)peers->dataLen; i >= 0; --i)
		{
			peers->At(i)->Release();	// todo: ���� release ԭ��?
		}
		peers->Clear();

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






	inline UVPeer::UVPeer()
		: bbReceive(mempool())
		, bbReceiveLeft(mempool())
		, bbReceivePackage(mempool())
		, sendBufs(mempool())
		, writeBufs(mempool())
	{
	}


	inline UVServerPeer::UVServerPeer(UVListener* listener)
		: UVPeer()
	{
		this->uv = listener->uv;
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



	inline void UVPeer::OnReceive()
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
			bbReceiveLeft->offset = 0;
			uint16_t dataLen = 0;

			// �ж�ͷ����. ���������, �����ܲ��ܲ���
			if (bbReceiveLeft->offset + sizeof(dataLen) > bbReceiveLeft->dataLen)
			{
				// ������ٴ����ͷ
				auto left = bbReceiveLeft->offset + sizeof(dataLen) - bbReceiveLeft->dataLen;

				// ���ʣ�����ݳ����޷�����, ׷�Ӹ��յ������ݺ��˳�
				if (bbReceive->offset + left > bbReceive->dataLen)
				{
					bbReceiveLeft->Write(bbReceive->buf[bbReceive->offset]);	// ����ֻ���ܲ�1�ֽڲ����ͷ( ��ͬ )
					return;
				}
				else
				{
					bbReceiveLeft->Write(bbReceive->buf[bbReceive->offset++]);
				}
			}

			// ����ͷ, �õ�����
			dataLen = bbReceiveLeft->buf[bbReceiveLeft->offset] + (bbReceiveLeft->buf[bbReceiveLeft->offset + 1] << 8);
			bbReceiveLeft->offset += 2;

			// �ж�����������. ���������, �����ܲ��ܲ���
			if (bbReceiveLeft->offset + dataLen > bbReceiveLeft->dataLen)
			{
				// ������ٴ���������
				auto left = bbReceiveLeft->offset + dataLen - bbReceiveLeft->dataLen;

				// ���ʣ�����ݳ����޷�����, �ƶ�ʣ�����ݵ�ͷ����׷�Ӹ��յ������ݺ��˳�
				if (bbReceive->offset + left > bbReceive->dataLen)
				{
					bbReceiveLeft->WriteBuf(bbReceive->buf + bbReceive->offset, bbReceive->dataLen - bbReceive->offset);
					return;
				}
				// ����ֻ���뵱ǰ��������
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
			if (bbReceive->dataLen > bbReceive->offset) goto LabBegin;
		}
	}

	inline void UVPeer::AllocCB(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)
	{
		auto self = container_of(handle, UVServerPeer, stream);
		self->bbReceive->Reserve((uint32_t)suggested_size);
		buf->base = self->bbReceive->buf;
		buf->len = self->bbReceive->bufLen;
	}

	inline void UVPeer::CloseCB(uv_handle_t* handle)
	{
		auto self = container_of(handle, UVServerPeer, stream);
		self->Release();
	}

	inline void UVPeer::ReadCB(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
	{
		auto self = container_of(handle, UVServerPeer, stream);
		if (nread < 0)
		{
			/* Error or EOF */
			self->Disconnect();
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

	inline void UVPeer::ShutdownCB(uv_shutdown_t* req, int status)
	{
		uv_close((uv_handle_t*)req->handle, CloseCB);
	}
	inline void UVPeer::SendCB(uv_write_t *req, int status)
	{
		auto self = container_of(req, UVServerPeer, writer);
		self->sending = false;
		if (status)
		{
			std::cout << "Send error " << uv_strerror(status) << std::endl;
			self->Disconnect();
		}
		else
		{
			self->Send();  // ������, ֱ������	// todo: ������ش���, �� last error?
		}
	}
	inline int UVPeer::Send()
	{
		assert(!sending);
		auto len = sendBufs->PopTo(*writeBufs, 4096);	// todo: ��д��. ���ֵ�����Ͻ�����
		if (len)
		{
			if (auto rtv = uv_write(&writer, (uv_stream_t*)&stream, writeBufs->buf, (unsigned int)writeBufs->dataLen, SendCB)) return rtv;
			sending = true;
			return 0;
		}
	}
	inline int UVPeer::Send(BBuffer* const& bb)
	{
		//uv_is_writable check?
		//if (sendBufs->BytesCount() + bb.dataLen > sendBufLimit) return false;
		sendBufs->Push(bb);
		if (!sending) return Send();
		return 0;
	}

	inline void UVPeer::Disconnect(bool immediately)
	{
		// todo: save disconnect type ?
		if (immediately														// �����Ͽ�
			|| !sending && ((uv_stream_t*)&stream)->write_queue_size == 0	// û�������ڷ�
			|| uv_shutdown(&sreq, (uv_stream_t*)&stream, ShutdownCB))		// shutdown ʧ��
		{
			if (!uv_is_closing((uv_handle_t*)&socket))
			{
				uv_close((uv_handle_t*)&socket, CloseCB);
			}
		}
	}






	inline UVClientPeer::UVClientPeer(UV* uv)
		: UVPeer()
	{
		this->uv = uv;
		// todo
	}

	inline UVClientPeer::~UVClientPeer()
	{
		bbReceivePackage->buf = nullptr;
		bbReceivePackage->bufLen = 0;
		bbReceivePackage->dataLen = 0;
		bbReceivePackage->offset = 0;
		XX_LIST_SWAP_REMOVE(uv->clientPeers, this, uv_clientPeers_index);
	}

	inline int UVClientPeer::SetAddress(char const* ip, int port)
	{
		return uv_ip4_addr(ip, 12345, &tarAddr);
	}

	inline int UVClientPeer::Connect()
	{
		assert(!connecting);
		connecting = true;
		return uv_tcp_connect(&conn, &stream, (sockaddr*)&tarAddr, ConnectCB);
	}

	inline void UVClientPeer::OnConnect()
	{
		if (uv_read_start((uv_stream_t*)&stream, AllocCB, ReadCB))
		{
			uv_close((uv_handle_t*)&stream, CloseCB);
			return;
		}
	}

	inline void UVClientPeer::ConnectCB(uv_connect_t* conn, int status)
	{
		auto self = container_of(conn, UVClientPeer, conn);
		self->connecting = false;
		if (status < 0)
		{
			self->connected = false;
			self->OnDisconnect(status);
		}
		else
		{
			self->connected = true;
			self->OnConnect();
		}
	}
}
