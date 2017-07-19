#pragma once
#include <uv.h>
#include "xx_mempool.h"
#include "xx_bbqueue.h"
#include <assert.h>
#include <memory>
#include <functional>

namespace xx
{
	struct UV;
	struct UVListener;
	struct UVPeer;
	struct UVServerPeer;
	struct UVClientPeer;
	struct UVTimer;
	struct UVAsync;

	struct UV : MPObject											// ����ֻ�ܴ��� 1 ��ʵ��
	{
		List_v<UVListener*> listeners;
		List_v<UVClientPeer*> clientPeers;
		List_v<UVTimer*> timers;
		List_v<UVAsync*> asyncs;

		UV();
		~UV();
		int EnableIdle();
		void DisableIdle();
		void Run();
		void Stop();
		virtual void OnIdle();
		template<typename ListenerType, typename ...Args>
		ListenerType* CreateListener(int port, int backlog, Args &&... args);
		template<typename ClientPeerType, typename ...Args>
		ClientPeerType* CreateClientPeer(Args &&... args);
		template<typename TimerType, typename ...Args>
		TimerType* CreateTimer(Args &&... args);
		template<typename AsyncType, typename ...Args>
		AsyncType* CreateAsync(Args &&... args);

		// uv's
		uv_loop_t loop;
		uv_idle_t idler;
		static void IdleCB(uv_idle_t* handle);
	};

	struct UVListener : MPObject									// ��ǰΪ ipv4, ip Ϊ 0.0.0.0
	{
		UV* uv;
		uint32_t uv_listeners_index;
		List_v<UVServerPeer*> peers;
		virtual UVServerPeer* OnCreatePeer() = 0;					// ��д���ṩ�������� peer ���͵ĺ���
		UVListener(UV* uv, int port, int backlog);
		~UVListener();

		// uv's
		uv_tcp_t tcpServer;
		static void OnConnect(uv_stream_t* server, int status);
	};

	enum class UVPeerStates
	{
		Disconnected,
		Connecting,
		Connected,
		Disconnecting
	};

	struct UVPeer : MPObject										// һЩ�������ݽṹ
	{
		UVPeer();
		~UVPeer();

		UV* uv;
		BBuffer_v bbReceive;										// for ReadCB & OnReceive
		BBuffer_v bbReceiveLeft;									// ���� OnReceive ����ʱʣ�µ�����
		BBuffer_v bbReceivePackage;									// for OnReceivePackage ����, ���� bbReceive �� bbReceiveLeft ���ڴ�

		BBQueue_v sendBufs;											// ���������ݶ���. ���� Send �������ǽ�����ѹ������, ��ȡ�ʵ����ȵ�һ��������
		List_v<uv_buf_t> writeBufs;									// ���õ� uv д���� ������ݲ���

		bool sending = false;										// ���Ͳ������. ��ǰ�����ֻͬʱ��һ������, �ɹ��ص�ʱ�ż�������һ��
		UVPeerStates state;											// ����״̬( server peer ��ʼΪ Connected, client peer Ϊ Disconnected )

		virtual void OnReceive();									// Ĭ��ʵ��Ϊ��ȡ��( 2 byte���� + ���� ), ���ڴ����������� call OnReceivePackage
		virtual void OnReceivePackage(BBuffer& bb) = 0;				// OnReceive ����һ����ʱ�������õ���
		virtual void OnDisconnect();								// Client Peer �������� init stream ������ �Ա���Է���ʹ�� client peer

		BBuffer* GetSendBB(int const& capacity = 0);				// ��ȡ�򴴽�һ�������õ� BBuffer( ��������Ѿ��в������� ), ��Ҫ�Լ�����, ���괫�� Send( �����Ƿ�Ͽ� )
		int Send(BBuffer* const& bb);								// ������"����"�����Ͷ���, ������������, ���������Ƿ�ɹ�( 0 ��ʾ�ɹ� )( ʧ��ԭ������Ǵ������ݹ��� )
		virtual void Disconnect(bool const& immediately = true);	// �Ͽ�( ���Ż� Release ). immediately Ϊ����� shutdown ģʽ( �ӳ�ɱ, �ܾ�����ȷ�����ݷ���ȥ )

		int SetNoDelay(bool const& enable);							// ���� tcp �ӳٷ����Ի������ݵĹ���
		int SetKeepAlive(bool const& enable, uint32_t const& delay);// ���� tcp ���ֻ�Ծ��ʱ��

		String_v tmpStr;
		String& GetPeerName();

		int Send();													// �ڲ�����, ��ʼ���� sendBufs ��Ķ���
		void Clear();												// �ڲ�����, �ڶϿ�֮�������շ���ػ���

		// ����ʹ�õ�һЩ��չ( ��ǰ����ֱ��ӳ�䵽 C# )
		List_v<MPObject*> recvPkgs;									// ���� OnReceivePackage ʱ�� bb.ReadPackages(*recvPkgs) �������. ���� bb.ReleasePackages �ͷ�.
	protected:
		template<typename T>
		int SendCore(T const& pkg);
		template<typename T, typename ...TS>
		int SendCore(T const& pkg, TS const& ... pkgs);
		template<typename T>
		void SendCombineCore(BBuffer& bb, T const& pkg);
		template<typename T, typename ...TS>
		void SendCombineCore(BBuffer& bb, T const& pkg, TS const& ... pkgs);
	public:
		template<typename ...TS>
		int SendPackages(TS const& ... pkgs);						// �﷨��, ��ͬ��д���е� Send ���ÿ������. �ᷢ�� pkgs ������ [head] + [data]
		template<typename ...TS>
		int SendCombine(TS const& ... pkgs);						// ���������Ͻ�������ϲ��� 1 �� [head] + [data] �е� [data] ����

		// uv's
		uv_tcp_t stream;
		uv_shutdown_t sreq;
		uv_write_t writer;

		static void AllocCB(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void CloseCB(uv_handle_t* stream);
		static void ReadCB(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
		static void ShutdownCB(uv_shutdown_t* req, int status);
		static void SendCB(uv_write_t *req, int status);
	};

	struct UVServerPeer : UVPeer									// ��ǰΪ ipv4, δ������ v4 v6 ͬʱ֧��
	{
		UVListener* listener;
		uint32_t listener_peers_index;

		UVServerPeer(UVListener* listener);
		~UVServerPeer();
	};

	struct UVClientPeer : UVPeer
	{
		uint32_t uv_clientPeers_index;
		int lastStatus = 0;											// ���״̬( ����, �Ͽ� )

		UVClientPeer(UV* uv);
		~UVClientPeer();											// �������ʱ connected Ϊ true ���ʾ�Ѷ���

		int SetAddress(char const* ip, int port);
		int Connect();
		virtual void OnConnect() = 0;								// lastStatus �� 0 �� connected Ϊ false ��ʾû����
		virtual void OnDisconnect() override;						// �����дʱ���Ҫ���� client peer, ������������������� init stream

		// uv's
		sockaddr_in tarAddr;
		uv_connect_t conn;
		static void ConnectCB(uv_connect_t* conn, int status);
	};

	struct UVTimer : MPObject
	{
		UV* uv;
		uint32_t uv_timers_index;

		UVTimer(UV* uv);
		~UVTimer();

		virtual void OnFire() = 0;
		int Start(uint64_t const& timeoutMS, uint64_t const& repeatIntervalMS);
		void SetRepeat(uint64_t const& repeatIntervalMS);
		int Again();
		int Stop();

		// uv's
		uv_timer_t timer_req;
		static void TimerCB(uv_timer_t* handle);
	};

	struct UVAsync : MPObject
	{
		UV* uv;
		uint32_t uv_asyncs_index;

		UVAsync(UV* uv);
		~UVAsync();

		void Fire();
		virtual void OnFire() = 0;

		// uv's
		uv_async_t async_req;
		static void AsyncCB(uv_async_t* handle);
	};

	using UV_v = MemHeaderBox<UV>;

	template<>
	struct BufMaker<uv_buf_t, void>
	{
		static uv_buf_t Make(char* buf, uint32_t len)
		{
			uv_buf_t rtv;
			rtv.base = buf;
			rtv.len = len;
			return rtv;
		}
	};

}

#include "xx_uv.hpp"
