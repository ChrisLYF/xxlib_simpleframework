#pragma once
#include <uv.h>
#include <xx_mempool.h>
#include <xx_bbqueue.h>
#include <assert.h>
#include <memory>
#include <iostream>

namespace xx
{
	struct UV;
	struct UVListener;
	struct UVPeer;
	struct UVServerPeer;
	struct UVClientPeer;

	struct UV : xx::MPObject									// ����ֻ�ܴ��� 1 ��ʵ��
	{
		xx::List_v<UVListener*> listeners;
		xx::List_v<UVClientPeer*> clientPeers;
		UV();
		~UV();
		void Run();
		template<typename ListenerType> ListenerType* CreateListener(int port, int backlog = SOMAXCONN);
		template<typename ClientType> ClientType* CreateClient();

		// uv's
		uv_loop_t* loop;
	};

	struct UVListener : xx::MPObject							// ��ǰΪ ipv4, ip Ϊ 0.0.0.0
	{
		UV* uv;
		uint32_t uv_listeners_index;
		xx::List_v<UVServerPeer*> peers;
		virtual UVServerPeer* OnCreatePeer() = 0;				// ��д���ṩ�������� peer ���͵ĺ���
		UVListener(UV* uv, int port, int backlog);
		~UVListener();

		// uv's
		uv_tcp_t tcpServer;
		static void OnConnect(uv_stream_t* server, int status);
	};

	struct UVPeer : xx::MPObject								// һЩ�������ݽṹ
	{
		UVPeer();

		UV* uv;
		BBuffer_v bbReceive;									// for ReadCB & OnReceive
		BBuffer_v bbReceiveLeft;								// ���� OnReceive ����ʱʣ�µ�����
		BBuffer_v bbReceivePackage;								// for OnReceivePackage ����, ���� bbReceive �� bbReceiveLeft ���ڴ�
		xx::BBQueue_v sendBufs;									// ���������ݶ���. ���� Send �������ǽ�����ѹ������, ��ȡ�ʵ����ȵ�һ��������
		xx::List_v<uv_buf_t> writeBufs;							// ���õ� uv д���� ������ݲ���
		bool sending = false;									// ���Ͳ������. ��ǰ�����ֻͬʱ��һ������, �ɹ��ص�ʱ�ż�������һ��

		virtual void OnReceive();								// Ĭ��ʵ��Ϊ��ȡ��( 2 byte���� + ���� ), ���ڴ����������� call OnReceivePackage
		virtual void OnReceivePackage(BBuffer const& bb) = 0;	// OnReceive ����һ����ʱ�������õ���

		int Send();												// �ڲ�����, ��ʼ���� sendBufs ��Ķ���
		int Send(BBuffer* const& bb);							// ���� �� ������ѹ������Ͷ���, ���������Ƿ�ɹ�( ʧ��ԭ������Ǵ������ݹ��� )
		void Disconnect(bool immediately = true);				// �Ͽ�( ���Ż� Release ). immediately Ϊ����� shutdown ģʽ( �ӳ�ɱ, �ܾ�����ȷ�����ݷ���ȥ )

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

	struct UVServerPeer : UVPeer								// ��ǰΪ ipv4, δ������ v4 v6 ͬʱ֧��
	{
		UVListener* listener;
		uint32_t listener_peers_index;

		UVServerPeer(UVListener* listener);
		~UVServerPeer();
	};

	struct UVClientPeer : UVPeer
	{
		uint32_t uv_clientPeers_index;
		bool connecting = false;
		bool connected = false;

		UVClientPeer(UV* uv);
		~UVClientPeer();
		int SetAddress(char const* ip, int port);
		int Connect();
		virtual void OnConnect();
		virtual void OnDisconnect(int status) = 0;

		// uv's
		sockaddr_in tarAddr;
		uv_connect_t conn;
		static void ConnectCB(uv_connect_t* conn, int status);
	};

	using UV_v = xx::MemHeaderBox<UV>;
}

#include "xx_uv.hpp"
