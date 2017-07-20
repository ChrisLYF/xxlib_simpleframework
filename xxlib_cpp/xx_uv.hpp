#pragma once
namespace xx
{

	inline UV::UV()
		: listeners(mempool())
		, clientPeers(mempool())
		, timers(mempool())
		, asyncs(mempool())
	{
		//loop = uv_default_loop();
		if (auto r = uv_loop_init(&loop)) throw r;
		uv_idle_init(&loop, &idler);
	}

	inline UV::~UV()
	{
		for (int i = (int)asyncs->dataLen - 1; i >= 0; --i)
		{
			asyncs->At(i)->Release();
		}
		asyncs->Clear();

		for (int i = (int)timers->dataLen - 1; i >= 0; --i)
		{
			timers->At(i)->Release();
		}
		timers->Clear();

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

		uv_loop_close(&loop);
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
		uv_run(&loop, UV_RUN_DEFAULT);
	}

	inline void UV::Stop()
	{
		uv_stop(&loop);
	}

	template<typename ListenerType, typename ...Args>
	ListenerType* UV::CreateListener(int port, int backlog, Args &&... args)
	{
		static_assert(std::is_base_of<UVListener, ListenerType>::value, "the ListenerType must inherit of UVListener.");
		return mempool().Create<ListenerType>(this, port, backlog, std::forward<Args>(args)...);
	}

	template<typename ClientPeerType, typename ...Args>
	ClientPeerType* UV::CreateClientPeer(Args &&... args)
	{
		static_assert(std::is_base_of<UVClientPeer, ClientPeerType>::value, "the ClientPeerType must inherit of UVClientPeer.");
		return mempool().Create<ClientPeerType>(this, std::forward<Args>(args)...);
	}

	template<typename TimerType, typename ...Args>
	TimerType* UV::CreateTimer(Args &&... args)
	{
		static_assert(std::is_base_of<UVTimer, TimerType>::value, "the TimerType must inherit of UVTimer.");
		return mempool().Create<TimerType>(this, std::forward<Args>(args)...);
	}

	template<typename AsyncType, typename ...Args>
	AsyncType* UV::CreateAsync(Args &&... args)
	{
		static_assert(std::is_base_of<UVAsync, AsyncType>::value, "the AsyncType must inherit of UVAsync.");
		return mempool().Create<AsyncType>(this, std::forward<Args>(args)...);
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

		if (auto rtv = uv_tcp_init(&uv->loop, &tcpServer))
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
		, recvPkgs(mempool())
	{
	}

	inline UVPeer::~UVPeer()
	{
		// ����Ҫ��һ���˽��������ĸ�����( ��֪�ᵼ�»ص�����, ����ʱ��������, Ӧ������ )
		if (!uv_is_closing((uv_handle_t*)&stream))
		{
			uv_close((uv_handle_t*)&stream, nullptr);
		}

		bbReceivePackage->buf = nullptr;
		bbReceivePackage->bufLen = 0;
		bbReceivePackage->dataLen = 0;
		bbReceivePackage->offset = 0;

		if (recvPkgs->dataLen) bbReceivePackage->ReleasePackages(*recvPkgs);
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

	inline void UVPeer::CloseCB(uv_handle_t* handle)
	{
		auto self = container_of(handle, UVPeer, stream);
		self->state = UVPeerStates::Disconnected;
		self->Clear();
		self->OnDisconnect();	// ���ﲻ���� self->Release(); ��Ҫ�Լ� Release / Dispose
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

	inline void UVPeer::OnDisconnect()
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

	inline void UVPeer::Clear()
	{
		bbReceiveLeft->Clear();
		sendBufs->Clear();
	}

	inline BBuffer* UVPeer::GetSendBB(int const& capacity)
	{
		return sendBufs->PopLastBB(capacity);
	}

	inline int UVPeer::Send(BBuffer* const& bb)
	{
		sendBufs->Push(bb);			// ѹ��, �ӹܲ��ƽ��������ֵ�		//if (sendBufs->BytesCount() + bb.dataLen > sendBufLimit) return false;
		if (state != UVPeerStates::Connected) return -1;
		if (!sending) return Send();
		return 0;
	}

	inline void UVPeer::Disconnect(bool const& immediately)
	{
		state = UVPeerStates::Disconnecting;

		// todo: save reason ?
		if (immediately														// �����Ͽ�
			|| !sending && ((uv_stream_t*)&stream)->write_queue_size == 0	// û�������ڷ�
			|| uv_shutdown(&sreq, (uv_stream_t*)&stream, ShutdownCB))		// shutdown ʧ��
		{
			if (!uv_is_closing((uv_handle_t*)&stream))						// �� ���ڹ�
			{
				uv_close((uv_handle_t*)&stream, CloseCB);
			}
			else
			{
				state = UVPeerStates::Disconnected;
				Clear();
				OnDisconnect();
			}
		}
	}

	inline int UVPeer::SetNoDelay(bool const& enable)
	{
		return uv_tcp_nodelay(&stream, enable ? 1 : 0);
	}

	inline int UVPeer::SetKeepAlive(bool const& enable, uint32_t const& delay)
	{
		return uv_tcp_keepalive(&stream, enable ? 1 : 0, delay);
	}

	inline String& UVPeer::GetPeerName()
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

	inline void UVPeer::ReleaseRecvPkgs()
	{
		bbReceivePackage->ReleasePackages(*recvPkgs);
	}

	template<typename T>
	int UVPeer::SendCore(T const& pkg)
	{
		auto bb = GetSendBB();
		auto b = bb->WritePackage(pkg);
		if (!b) return -1;
		return Send(bb);
	}
	template<typename T, typename ...TS>
	int UVPeer::SendCore(T const& pkg, TS const& ... pkgs)
	{
		if (auto rtv = SendCore(pkg)) return rtv;
		return SendCore(pkgs...);
	}
	template<typename ...TS>
	int UVPeer::SendPackages(TS const& ... pkgs)
	{
		return SendCore(pkgs...);
	}

	template<typename T>
	void UVPeer::SendCombineCore(BBuffer& bb, T const& pkg)
	{
		bb.WriteRoot(pkg);
	}
	template<typename T, typename ...TS>
	void UVPeer::SendCombineCore(BBuffer& bb, T const& pkg, TS const& ... pkgs)
	{
		bb.WriteRoot(pkg);
		SendCombineCore(bb, pkgs...);
	}
	template<typename ...TS>
	int UVPeer::SendCombine(TS const& ... pkgs)
	{
		auto bb = GetSendBB();
		bb->BeginWritePackage();
		SendCombineCore(*bb, pkgs...);
		if (bb->EndWritePackage()) return -1;
		return Send(bb);
	}






	inline UVServerPeer::UVServerPeer(UVListener* listener)
		: UVPeer()
	{
		state = UVPeerStates::Connected;
		this->uv = listener->uv;
		this->listener = listener;
		if (auto rtv = uv_tcp_init(&uv->loop, (uv_tcp_t*)&stream))
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
		XX_LIST_SWAP_REMOVE(listener->peers, this, listener_peers_index);
	}








	inline UVClientPeer::UVClientPeer(UV* uv)
		: UVPeer()
	{
		state = UVPeerStates::Disconnected;
		this->uv = uv;
		uv_clientPeers_index = uv->clientPeers->dataLen;
		if (auto rtv = uv_tcp_init(&uv->loop, (uv_tcp_t*)&stream))
		{
			throw rtv;
		}
		uv->clientPeers->Add(this);
	}

	inline UVClientPeer::~UVClientPeer()
	{
		XX_LIST_SWAP_REMOVE(uv->clientPeers, this, uv_clientPeers_index);
	}

	inline int UVClientPeer::SetAddress(char const* ip, int port)
	{
		return uv_ip4_addr(ip, port, &tarAddr);
	}

	inline int UVClientPeer::Connect()
	{
		if (state != UVPeerStates::Disconnected) return -1;
		state = UVPeerStates::Connecting;
		if (auto rtv = uv_tcp_connect(&conn, &stream, (sockaddr*)&tarAddr, ConnectCB))
		{
			state = UVPeerStates::Disconnected;
			return rtv;
		}
		return 0;
	}

	inline void UVClientPeer::ConnectCB(uv_connect_t* conn, int status)
	{
		auto self = container_of(conn, UVClientPeer, conn);
		if (!self->versionNumber()) return;			// �����������������µĻص�( ��ǰ����ȷ����û������ CB Ҳ��Ҫ���� )

		self->lastStatus = status;
		if (status < 0)
		{
			self->state = UVPeerStates::Disconnected;
			self->OnConnect();
		}
		else
		{
			self->state = UVPeerStates::Connected;
			if (uv_read_start((uv_stream_t*)&self->stream, self->AllocCB, self->ReadCB))
			{
				self->state = UVPeerStates::Disconnecting;
				uv_close((uv_handle_t*)&self->stream, self->CloseCB);
			}
			self->OnConnect();
		}
	}

	inline void UVClientPeer::OnDisconnect()
	{
		auto rtv = uv_tcp_init(&uv->loop, (uv_tcp_t*)&stream);
		assert(!rtv);
	}







	inline UVTimer::UVTimer(UV* uv)
		: uv(uv)
	{
		uv_timers_index = uv->timers->dataLen;
		if (auto rtv = uv_timer_init(&uv->loop, &timer_req))
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








	inline UVAsync::UVAsync(UV* uv)
		: uv(uv)
	{
		uv_asyncs_index = uv->asyncs->dataLen;
		if (auto rtv = uv_async_init(&uv->loop, &async_req, AsyncCB))
		{
			throw rtv;
		}
		uv->asyncs->Add(this);
	}

	inline UVAsync::~UVAsync()
	{
		uv_close((uv_handle_t*)&async_req, nullptr);
		XX_LIST_SWAP_REMOVE(uv->asyncs, this, uv_asyncs_index);
	}

	inline void UVAsync::Fire()
	{
		uv_async_send(&async_req);
	}

	inline void UVAsync::AsyncCB(uv_async_t* handle)
	{
		auto self = container_of(handle, UVAsync, async_req);
		self->OnFire();
	}
}
