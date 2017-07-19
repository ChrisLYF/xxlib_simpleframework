#pragma once
#include "xx_uv.h"
#include "xx_helpers.h"
#include "pkg_cpp\PKG_class.h"

struct MyClient;

struct MyConnector : xx::UVClientPeer
{
	MyClient* owner;
	MyConnector(xx::UV* uv, MyClient* owner);
	~MyConnector();
	virtual void OnReceivePackage(xx::BBuffer & bb) override;
	virtual void OnConnect() override;
};


struct MyTimer : xx::UVTimer
{
	MyClient* owner;
	MyTimer(xx::UV* uv, MyClient* owner);
	virtual void OnFire() override;
};


struct MyClient : xx::MPObject
{
	xx::UV* uv;
	MyConnector* conn;
	MyTimer* timer;
	MyClient(xx::UV* uv, char const* un, char const* pw);
	~MyClient();

	// Ԥ�������㷢��������
	PKG::Client_Server::Join* pkgJoin;
	PKG::Client_Server::Message* pkgMessage;

	// ����ʱ������
	xx::MPtr<PKG::UserInfo> self;
	xx::List<PKG::UserInfo*>* users = nullptr;

	int lineNumber = 0;					    // stackless Э���к�
	int Update();

	int64_t lastMS = 0;						// ���ڸ��ֳ�ʱ�ж�
	bool connecting = false;				// ���� OnConnect �ص��ı�־
	int i = 0;								// for ����

};
