#pragma once
namespace xx
{

	inline UV::UV()
		: listeners(mempool())
		, clientPeers(mempool())
		, timers(mempool())
	{
		loop = uv_default_loop();
		uv_idle_init(loop, &idler);
	}

	inline UV::~UV()
	{
		for (int i = (int)listeners->dataLen - 1; i >= 0; --i)
		{
			listeners->At(i)->Release();	// todo: ���� release ԭ��?
		}
		listeners->Clear();

		for (int i = (int)clientPeers->dataLen - 1; i >= 0; --i)
		{
			clientPeers->At(i)->Release();	// todo: ���� release ԭ��?
		}
		clientPeers->Clear();
	}

	inline int UV::EnableIdle()
	{
		return uv_idle_start(&idler, IdleCB);
	}

	inline void UV::DisableIdle()
	{
		uv_close((uv_handle_t*)&idler, nullptr);
	}

	inline void UV::OnIdle()
	{
	}

	inline void UV::Run()
	{
		uv_run(loop, UV_RUN_DEFAULT);
	}


	template<typename ListenerType>
	ListenerType* UV::CreateListener(int port, int backlog)
	{
		static_assert(std::is_base_of<UVListener, ListenerType>::value, "the ListenerType must inherit of UVListener.");
		return mempool().Create<ListenerType>(this, port, backlog);
	}

	template<typename ClientPeerType>
	ClientPeerType* UV::CreateClientPeer()
	{
		static_assert(std::is_base_of<UVClientPeer, ClientPeerType>::value, "the ClientPeerType must inherit of UVClientPeer.");
		return mempool().Create<ClientPeerType>(this);
	}

	template<typename TimerType>
	TimerType* UV::CreateTimer()
	{
		static_assert(std::is_base_of<UVTimer, TimerType>::value, "the TimerType must inherit of UVTimer.");
		return mempool().Create<TimerType>(this);
	}


	inline void UV::IdleCB(uv_idle_t* handle)
	{
		auto self = container_of(handle, UV, idler);
		self->OnIdle();
	}





	inline UVListener::UVListener(UV* uv, int port, int backlog)
		: uv(uv)
		, uv_listeners_index(uv->listeners->dataLen)
		, peers(mempool())
	{
		sockaddr_in addr;
		uv_ip4_addr("0.0.0.0", port, &addr);

		if (auto rtv = uv_tcp_init(uv->loop, &tcpServer))
		{
			throw rtv;
		}
		if (auto rtv = uv_tcp_bind(&tcpServer, (sockaddr const*)&addr, 0))
		{
			uv_close((uv_handle_t*)&tcpServer, nullptr);	// rollback
			throw rtv;
		}
		if (auto rtv = uv_listen((uv_stream_t*)&tcpServer, backlog, OnConnect))
		{
			uv_close((uv_handle_t*)&tcpServer, nullptr);	// rollback
			throw rtv;
		}

		uv->listeners->Add(this);
	}

	inline UVListener::~UVListener()
	{
		for (int i = (int)peers->dataLen - 1; i >= 0; --i)
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
		, tmpStr(mempool())
	{
	}

	inline void UVPeer::OnReceive()
	{
		// ��ʵ�ֶ��� 2 �ֽڰ�ͷ�İ汾

		// ��� bbReceiveLeft û����, ��ֱ���� bbReceive �Ͻ��а��������ж�. 
		// ����ں�������, ���ϴ��� OnReceivePackage ����֮, ���ʣ�µ������Ƶ� bbReceiveLeft
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
		// ��� bbReceiveLeft ������, ���Դ� bbReceive ����һ����������
		// ���� OnReceivePackage ��������, ���� bbReceiveLeft û���ݵ�����
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
		auto self = container_of(handle, UVPeer, stream);
		if (suggested_size > self->bbReceive->bufLen)
		{
			self->bbReceive->Reserve((uint32_t)suggested_size);
		}
		buf->base = self->bbReceive->buf;
		buf->len = self->bbReceive->bufLen;
	}

	inline void UVPeer::CloseCB(uv_handle_t* handle)
	{
		auto self = container_of(handle, UVPeer, stream);
		self->Release();
	}

	inline void UVPeer::ReadCB(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf)
	{
		auto self = container_of(handle, UVPeer, stream);
		if (nread < 0)
		{
			/* Error or EOF */
			self->Disconnect(false);
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
		if (!uv_is_closing((uv_handle_t*)req->handle))
		{
			uv_close((uv_handle_t*)req->handle, CloseCB);
		}
		else
		{
			CloseCB((uv_handle_t*)req->handle);
		}
	}

	inline void UVPeer::SendCB(uv_write_t *req, int status)
	{
		auto self = container_of(req, UVPeer, writer);
		self->sending = false;
		if (status)
		{
			//std::cout << "Send error " << uv_strerror(status) << std::endl;
			self->Disconnect();	// todo: ��ԭ��?
		}
		else
		{
			self->Send();  // ������, ֱ������	// todo: ������ش���, �� last error?
		}
	}

	inline int UVPeer::Send()
	{
		assert(!sending);
		auto len = sendBufs->PopTo(*writeBufs, 65536);	// todo: ��д��. ���ֵ�����Ͻ�����
		if (len)
		{
			if (auto rtv = uv_write(&writer, (uv_stream_t*)&stream, writeBufs->buf, writeBufs->dataLen, SendCB)) return rtv;
			sending = true;
		}
		return 0;
	}

	inline BBuffer* UVPeer::GetSendBB(int const& capacity)
	{
		return sendBufs->PopLastBB(capacity);
	}

	inline int UVPeer::Send(BBuffer* const& bb)
	{
		//uv_is_writable check?
		//if (sendBufs->BytesCount() + bb.dataLen > sendBufLimit) return false;

		sendBufs->Push(bb);
		if (!sending) return Send();
		return 0;
	}

	inline void UVPeer::Disconnect(bool const& immediately)
	{
		// todo: save disconnect type ?
		if (immediately														// �����Ͽ�
			|| !sending && ((uv_stream_t*)&stream)->write_queue_size == 0	// û�������ڷ�
			|| uv_shutdown(&sreq, (uv_stream_t*)&stream, ShutdownCB))		// shutdown ʧ��
		{
			if (!uv_is_closing((uv_handle_t*)&stream))						// �� ���ڹ�
			{
				uv_close((uv_handle_t*)&stream, CloseCB);
			}
		}
	}

	int UVPeer::SetNoDelay(bool const& enable)
	{
		return uv_tcp_nodelay(&stream, enable ? 1 : 0);
	}

	int UVPeer::SetKeepAlive(bool const& enable, uint32_t const& delay)
	{
		return uv_tcp_keepalive(&stream, enable ? 1 : 0, delay);
	}


	String& UVPeer::GetPeerName()
	{
		sockaddr_in saddr;
		int len = sizeof(saddr);
		if (auto rtv = uv_tcp_getpeername(&stream, (sockaddr*)&saddr, &len))
		{
			tmpStr->Clear();
		}
		else
		{
			tmpStr->Reserve(16);
			rtv = uv_inet_ntop(AF_INET, &saddr.sin_addr, tmpStr->buf, tmpStr->bufLen);
			tmpStr->dataLen = (uint32_t)strlen(tmpStr->buf);
			tmpStr->Append(':', ntohs(saddr.sin_port));
		}
		return *tmpStr;
	}






	inline UVServerPeer::UVServerPeer(UVListener* listener)
		: UVPeer()
	{
		this->uv = listener->uv;
		this->listener = listener;
		if (auto rtv = uv_tcp_init(uv->loop, (uv_tcp_t*)&stream))
		{
			throw rtv;
		}
		listener_peers_index = listener->peers->dataLen;
		if (auto rtv = uv_accept((uv_stream_t*)&listener->tcpServer, (uv_stream_t*)&stream))
		{
			uv_close((uv_handle_t*)&stream, nullptr);	// rollback
			throw rtv;
		}
		if (auto rtv = uv_read_start((uv_stream_t*)&stream, AllocCB, ReadCB))
		{
			uv_close((uv_handle_t*)&stream, nullptr);	// rollback
			throw rtv;
		}
		listener->peers->Add(this);
	}
	inline UVServerPeer::~UVServerPeer()
	{
		// ����Ҫ��һ���˽��������ĸ�����
		if (!uv_is_closing((uv_handle_t*)&stream))
		{
			uv_close((uv_handle_t*)&stream, nullptr);
		}

		bbReceivePackage->buf = nullptr;
		bbReceivePackage->bufLen = 0;
		bbReceivePackage->dataLen = 0;
		bbReceivePackage->offset = 0;
		XX_LIST_SWAP_REMOVE(listener->peers, this, listener_peers_index);
	}








	inline UVClientPeer::UVClientPeer(UV* uv)
		: UVPeer()
	{
		this->uv = uv;
		uv_clientPeers_index = uv->clientPeers->dataLen;
		if (auto rtv = uv_tcp_init(uv->loop, (uv_tcp_t*)&stream))
		{
			throw rtv;
		}
		uv->clientPeers->Add(this);
	}

	inline UVClientPeer::~UVClientPeer()
	{
		// ����Ҫ��һ���˽��������ĸ�����
		if (!uv_is_closing((uv_handle_t*)&stream))
		{
			uv_close((uv_handle_t*)&stream, nullptr);
		}

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

	inline void UVClientPeer::ConnectCB(uv_connect_t* conn, int status)
	{
		auto self = container_of(conn, UVClientPeer, conn);
		self->connecting = false;
		self->lastStatus = status;
		if (status < 0)
		{
			self->connected = false;
			self->OnConnect();
		}
		else
		{
			self->connected = true;
			if (uv_read_start((uv_stream_t*)&self->stream, self->AllocCB, self->ReadCB))
			{
				self->connected = false;
				self->closing = true;
				uv_close((uv_handle_t*)&self->stream, self->ClientCloseCB);
			}
			self->OnConnect();
		}
	}

	inline void UVClientPeer::Disconnect(bool const& immediately)
	{
		closing = true;

		// todo: save disconnect type ?
		if (immediately														// �����Ͽ�
			|| !sending && ((uv_stream_t*)&stream)->write_queue_size == 0	// û�������ڷ�
			|| uv_shutdown(&sreq, (uv_stream_t*)&stream, ClientShutdownCB))	// shutdown ʧ��
		{
			// todo: ���ֵ� server��ɱ��ʱ,  closing ���ڷ���
			if (!uv_is_closing((uv_handle_t*)&stream))						// �� ���ڹ�
			{
				uv_close((uv_handle_t*)&stream, ClientCloseCB);
			}
			else
			{
				closing = false;
				if (connected)
				{
					connected = false;
					OnDisconnect();
				}
			}
		}
	}

	inline void UVClientPeer::ClientShutdownCB(uv_shutdown_t* req, int status)
	{
		if (!uv_is_closing((uv_handle_t*)req->handle))
		{
			uv_close((uv_handle_t*)req->handle, ClientCloseCB);
		}
		else
		{
			ClientCloseCB((uv_handle_t*)req->handle);
		}
	}

	inline void UVClientPeer::ClientCloseCB(uv_handle_t* handle)
	{
		auto self = container_of(handle, UVClientPeer, stream);
		self->closing = false;
		if (self->connected)
		{
			self->connected = false;
			self->OnDisconnect();
		}
	}







	inline UVTimer::UVTimer(UV* uv)
		: uv(uv)
	{
		uv_timers_index = uv->timers->dataLen;
		if (auto rtv = uv_timer_init(uv->loop, &timer_req))
		{
			throw rtv;
		}
		uv->timers->Add(this);
	}
	inline UVTimer::~UVTimer()
	{
		uv_close((uv_handle_t*)&timer_req, nullptr);
		XX_LIST_SWAP_REMOVE(uv->timers, this, uv_timers_index);
	}
	inline int UVTimer::Start(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS)
	{
		return uv_timer_start(&timer_req, TimerCB, timeoutMS, repeatIntervalMS);
	}
	inline void UVTimer::SetRepeat(uint64_t const& repeatIntervalMS)
	{
		uv_timer_set_repeat(&timer_req, repeatIntervalMS);
	}
	inline int UVTimer::Again()
	{
		return uv_timer_again(&timer_req);
	}
	inline int UVTimer::Stop()
	{
		return uv_timer_stop(&timer_req);
	}
	inline void UVTimer::TimerCB(uv_timer_t* handle)
	{
		auto self = container_of(handle, UVTimer, timer_req);
		self->OnFire();
	}
}
