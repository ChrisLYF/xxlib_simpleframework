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
	struct UVServerPeer;
	struct UVClient;

	struct UV : xx::MPObject									// ����ֻ�ܴ��� 1 ��ʵ��
	{
		xx::List_v<UVListener*> listeners;
		xx::List_v<UVClient*> clients;
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

	struct UVServerPeer : xx::MPObject							// ��ǰΪ ipv4, δ������ v4 v6 ͬʱ֧��
	{
		UV* uv;
		UVListener* listener;
		uint32_t listener_peers_index;

		UVServerPeer(UVListener* listener);
		~UVServerPeer();

		BBuffer_v bbReceive;									// for ReadCB & OnReceive
		BBuffer_v bbReceiveLeft;								// ���� OnReceive ����ʱʣ�µ�����
		virtual void OnReceive();								// Ĭ��ʵ��Ϊ��ȡ��( 2 byte���� + ���� ), ���ڴ����������� call OnReceivePackage

		BBuffer_v bbReceivePackage;								// for OnReceivePackage ����, ���� bbReceive �� bbReceiveLeft ���ڴ�
		virtual void OnReceivePackage(BBuffer const& bb) = 0;	// OnReceive ����һ����ʱ�������õ���

		// Send, Disconnect, virtual OnDisconnect
		uv_tcp_t stream;
		uv_shutdown_t sreq;

		uv_write_t writer;
		bool writing = false;									// д�������. ��ǰ�����ֻ����ͬʱдһ������, ����д�ɹ��ص�ʱ�ż���д��һ��
		xx::BBQueue_v sendBufs;									// ���������ݶ���. ���� Send �������ǽ�����ѹ������, ��ȡ�ʵ����ȵ�һ��������
		xx::List_v<uv_buf_t> writeBufs;							// ���õ� uv д���� ������ݲ���
		void Write();
		void Kill();

		static void AllocCB(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);
		static void CloseCB(uv_handle_t* stream);
		static void ReadCB(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf);
		static void ShutdownCB(uv_shutdown_t* req, int status);
		static void WriteCB(uv_write_t *req, int status);
	};

	struct UVClient : xx::MPObject
	{
		UV* uv;
		uint32_t uv_clients_index;
		UVClient(UV* uv);
		~UVClient();
		bool SetAddress(/* addr */);
		bool Connect();
		// bool Disconnect(/* timeout */);
		// Send, virtual OnReceive, virtual OnDisconnect
	};

	using UV_v = xx::MemHeaderBox<UV>;
}

#include "xx_uv.hpp"
