#include "main.h"
#include <thread>
#include <chrono>

/*************************************************************************************/

int main()
{
	PKG::AllTypesRegister();

	xx::MemPool mp;
	xx::UV_v uv(mp);
	auto c = mp.CreateMPtr<MyClient>(uv);	// c �Գ���, ��ɱ
	uv->Run();
	std::cout << "main: press any key to continue ..." << std::endl;
	std::cin.get();
	return 0;
}


/*************************************************************************************/

inline MyConnector::MyConnector(xx::UV* uv, MyClient* owner)
	: xx::UVClientPeer(uv)
	, owner(owner)
{}
inline MyConnector::~MyConnector()
{
	Cout("MyConnector: ~MyConnector()\n");
}
inline void MyConnector::OnReceivePackage(xx::BBuffer & bb)
{
	// �Խ��յ��İ������
	if (bb.ReadPackages(*recvPkgs) != 1)
	{
		Disconnect(true);
		return;
	}
}
inline void MyConnector::OnConnect()
{
	Cout("MyConnector: ", (state == xx::UVPeerStates::Connected
		? "state == xx::UVPeerStates::Connected\n"
		: "state != xx::UVPeerStates::Connected\n"));
	owner->connecting = false;
}


/*************************************************************************************/

inline MyTimer::MyTimer(xx::UV* uv, MyClient* owner)
	: xx::UVTimer(uv)
	, owner(owner)
{
	Start(0, 1);
}
inline void MyTimer::OnFire()
{
	if (owner->Update()) owner->Release();
}


/*************************************************************************************/

inline MyClient::MyClient(xx::UV* uv)
	: uv(uv)
{
	Cout("MyClient::MyClient(xx::UV* uv)\n");
	conn = uv->CreateClientPeer<MyConnector>(this);
	timer = uv->CreateTimer<MyTimer>(this);

	// Ԥ����������ݰ�
	mempool().CreateTo(pkgJoin);
	mempool().CreateTo(pkgJoin->username);
	mempool().CreateTo(pkgJoin->password);

	mempool().CreateTo(pkgMessage);
	mempool().CreateTo(pkgMessage->text);
}

inline MyClient::~MyClient()
{
	Cout("MyClient:~MyClient()\n");
	auto& mp = mempool();
	mp.SafeRelease(timer);
	mp.SafeRelease(conn);

	mp.SafeRelease(users);

	mp.SafeRelease(pkgJoin);
	mp.SafeRelease(pkgMessage);
}

inline int MyClient::Update()
{
	auto currMS = xx::GetCurrentMS();
	XX_CORO_BEGIN();
	{
		conn->SetAddress("127.0.0.1", 12345);
		connecting = true;
		if (auto rtv = conn->Connect())
		{
			Cout("MyClient: conn->Connect() error! rtv = ", rtv, '\n');
			return -1;
		}

		lastMS = currMS;		// ���ü�ʱ��Ϊ��һ���ڳ�ʱ�ж���׼��
	}
	XX_CORO_(1);
	{
		if (!connecting)
		{
			XX_CORO_GOTO(2);
		}
		if (currMS - lastMS < 5000 && conn->state != xx::UVPeerStates::Connected)
		{
			XX_CORO_YIELDTO(1);
		}
		else
		{
			Cout("MyClient: connect to server timeout!\n");
			return -1;
		}
	}
	XX_CORO_(2);
	{
		if (conn->state != xx::UVPeerStates::Connected)
		{
			Cout("MyClient: can't connect to server!\n");
			return -1;
		}
	}
	XX_CORO_(3);
	{
		Cout("MyClient: connected!\n");

		pkgJoin->username->Assign("a");
		pkgJoin->password->Assign("a");

		if (auto rtv = conn->SendPackages(pkgJoin))
		{
			Cout("MyClient: conn->SendPackages(pkgJoin) error! rtv = ", rtv, '\n');
			return -1;
		}

		lastMS = currMS;		// ���ü�ʱ��Ϊ��һ���ڳ�ʱ�ж���׼��
	}
	XX_CORO_(4);
	{
		if (conn->state != xx::UVPeerStates::Connected)
		{
			Cout("MyClient: wait conn Recv disconnected!\n");
			return -1;
		}
		else if (conn->recvPkgs->dataLen)
		{
			XX_CORO_GOTO(5);
		}
		if (currMS - lastMS < 5000)
		{
			XX_CORO_YIELDTO(4);
		}
		else
		{
			Cout("MyClient: wait recv timeout!\n");
			return -1;
		}
	}
	XX_CORO_(5);
	{
		assert(conn->recvPkgs->dataLen);

		// �ȴ���� 1 ����, ��Ȼ�� JoinSuccess �� JoinFail
		auto& o_ = conn->recvPkgs->Top();
		auto& typeId = o_->typeId();
		switch (typeId)
		{
		case xx::TypeId_v<PKG::Server_Client::JoinSuccess>:
		{
			auto o = (PKG::Server_Client::JoinSuccess*)o_;

			// todo: У���յ�������

			// ת���յ�������
			self = o->self;			// תΪ MPtr
			users = o->users;		// ��Ϊ��������, ���а����ü�����Ϊ1, �պ�
			o->self = nullptr;		// �������������ߵ�����
			o->users = nullptr;		// �������������ߵ�����

			// ��ʾ���յ�������
			Cout("MyClient: recv msg == PKG::Server_Client::JoinSuccess!\n"
				"users->dataLen = ", users->dataLen, "\n"
				"self->id = ", self->id, '\n');

			// for ���� ������ 1 ����
			i = 1;
			lastMS = currMS;		// ���ü�ʱ��Ϊ��һ���ڳ�ʱ�ж���׼��
			XX_CORO_GOTO(6);
		}
		case xx::TypeId_v<PKG::Server_Client::JoinFail>:
		{
			auto o = (PKG::Server_Client::JoinFail*)o_;
			Cout("MyClient: recv msg == PKG::Server_Client::JoinFail!\n    reason = ", o->reason, '\n');
			return -1;
		}
		default:
		{
			Cout("MyClient: recv unhandled msg! typeId = ", typeId, '\n');
			return -1;
		}
		}
	}
	XX_CORO_(6);
	{
		if (conn->state != xx::UVPeerStates::Connected)
		{
			Cout("MyClient: wait conn Recv disconnected! ( joined )\n");
			return -1;
		}

		// ��������ʣ�µİ�
		for (; i < (int)conn->recvPkgs->dataLen; ++i)
		{
			auto& o_ = conn->recvPkgs->At(i);
			auto& typeId = o_->typeId();
			switch (typeId)
			{
			case xx::TypeId_v<PKG::Server_Client::PushJoin>:
			{
				auto o = (PKG::Server_Client::PushJoin*)o_;
				Cout("MyClient: recv msg == PKG::Server_Client::PushJoin! id = ", o->id, '\n');
				break;
			}
			case xx::TypeId_v<PKG::Server_Client::PushLogout>:
			{
				auto o = (PKG::Server_Client::PushLogout*)o_;
				Cout("MyClient: recv msg == PKG::Server_Client::PushLogout! id & reason = ", o->id, ' ', o->reason, '\n');
				break;
			}
			case xx::TypeId_v<PKG::Server_Client::PushMessage>:
			{
				auto o = (PKG::Server_Client::PushMessage*)o_;
				Cout("MyClient: recv msg == PKG::Server_Client::PushMessage! id & text = ", o->id, ' ', o->text, '\n');
				break;
			}
			default:
			{
				Cout("MyClient: recv unhandled msg! typeId = ", typeId, '\n');
				return -1;
			}
			}
		}

		// ����Ѿ�������İ�, ׼����һ�ν���
		conn->ReleaseRecvPkgs();

		// todo: ͨ������һ���߳�, �� Console �����ַ��Բ��� PKG::Client_Server::Message ��Ϣ

		// ÿ���ӷ�һ�ε�ǰ ms
		if (currMS - lastMS > 1000)
		{
			lastMS = currMS;

			// ��ģ��һ�¼�������
			pkgMessage->text->Clear();
			pkgMessage->text->Append("currMS = ", currMS);

			// �� Message
			if (auto rtv = conn->SendPackages(pkgMessage))
			{
				Cout("MyClient: conn->SendPackages(pkgMessage) error! rtv = ", rtv, '\n');
				return -1;
			}
		}

		// ���Ⱥ������ĵ���
		i = 0;
		XX_CORO_YIELDTO(6);
	}
	XX_CORO_END();
	return -1;
}

