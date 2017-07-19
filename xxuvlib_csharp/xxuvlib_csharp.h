// xxuvlib_csharp.h

#pragma once

#pragma unmanaged
#include <xx_uv.h>
#include <xx_helpers.h>


/********************************************************************************************************************/
// MyUVAsync
/********************************************************************************************************************/

// �ɴ�����ݵ� async ���߳��ź���. ʹ�� UVWrapper �� CreateAsync ������. 

typedef void(*Func_OnFire)();
struct MyUVAsync : xx::UVAsync
{
	// ���������
	Func_OnFire onFire = nullptr;

	MyUVAsync(xx::UV* uv) : xx::UVAsync(uv) {}

	inline virtual void OnFire() override
	{
		assert(onFire);
		(*onFire)();
	}
};


/********************************************************************************************************************/
// MyUVTimer
/********************************************************************************************************************/

// �ɴ�����ݵ� timer. ʹ�� UVWrapper �� CreateTimer ������. Start ����. Release ����.

typedef void(*Func_OnTicks)();
struct MyUVTimer : xx::UVTimer
{
	// ���������
	Func_OnTicks onTicks = nullptr;

	MyUVTimer(xx::UV* uv) : xx::UVTimer(uv) {}

	inline virtual void OnFire() override
	{
		assert(onTicks);
		(*onTicks)();
	}
};


/********************************************************************************************************************/
// MyUVClientPeer
/********************************************************************************************************************/

// �ɴ�����ݵ� client peer. ʹ�� UVWrapper �� CreateClientPeer ������

typedef void(*Func_OnReceivePackage)(char* buf, int dataLen);
typedef void(*Func_OnConnect)();
typedef void(*Func_OnDisconnect)();
struct MyUVClientPeer : xx::UVClientPeer
{
	// ���������
	Func_OnReceivePackage onReceivePackage = nullptr;
	Func_OnConnect onConnect = nullptr;
	Func_OnDisconnect onDisconnect = nullptr;

	MyUVClientPeer(xx::UV* uv) : xx::UVClientPeer(uv) {}

	void OnReceivePackage(xx::BBuffer& bb) override
	{
		assert(onReceivePackage);
		(*onReceivePackage)(bb.buf, (int)bb.dataLen);
	}

	void OnConnect() override
	{
		assert(onConnect);
		(*onConnect)();
	}

	void OnDisconnect() override
	{
		this->xx::UVClientPeer::OnDisconnect();
		assert(onDisconnect);
		(*onDisconnect)();
	}
};



/********************************************************************************************************************/
// MyUVListener
/********************************************************************************************************************/

// �ɴ�����ݵ� listener. ʹ�� UVWrapper �� CreateListener ������

typedef xx::UVServerPeer*(*Func_OnCreatePeer)();
struct MyUVListener : xx::UVListener
{
	// ���������
	Func_OnCreatePeer onCreatePeer = nullptr;

	MyUVListener(xx::UV* uv, int port, int backlog) : xx::UVListener(uv, port, backlog) {}

	xx::UVServerPeer* OnCreatePeer() override
	{
		assert(onCreatePeer);
		return (*onCreatePeer)();
	}
};



/********************************************************************************************************************/
// MyUVServerPeer
/********************************************************************************************************************/

// �ɴ�����ݵ� listener. ʹ�� UVWrapper �� CreateListener ������

struct MyUVServerPeer;
typedef void(*Func_OnAccept)(char* addr, int addrLen);
struct MyUVServerPeer : xx::UVServerPeer
{
	// ���������
	Func_OnDisconnect onDisconnect = nullptr;
	Func_OnReceivePackage onReceivePackage = nullptr;

	MyUVServerPeer(MyUVListener* listener) : xx::UVServerPeer(listener)
	{
		GetPeerName();	// ��� peer name �� tmpStr, �Ա������ C# �����ڴ���֮��ģ�� Accept �¼�ʱ����
	}

	void OnReceivePackage(xx::BBuffer& bb) override
	{
		assert(onReceivePackage);
		(*onReceivePackage)(bb.buf, (int)bb.dataLen);
	}

	void OnDisconnect() override
	{
		assert(onDisconnect);
		(*onDisconnect)();
	}
};










#pragma managed

// ����ɾ����
#define SYSTEM_LIST_SWAP_REMOVE( listPtr, tarPtr, indexName )	\
do																\
{																\
	auto count_1 = listPtr->Count - 1;							\
	if (count_1 != tarPtr->indexName)							\
	{															\
		listPtr[tarPtr->indexName] = listPtr[count_1];			\
		listPtr[count_1]->indexName = tarPtr->indexName;		\
	}															\
	listPtr->RemoveAt(count_1);									\
																\
} while (false)


using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;


/********************************************************************************************************************/
// MPtrWrapper
/********************************************************************************************************************/

public ref class MPtrWrapper abstract
{
protected:
	System::IntPtr^ ptr = nullptr;
	uint64_t versionNumber = 0;
public:

	virtual void InitPtr(xx::MPObject* ptr)
	{
		this->ptr = gcnew System::IntPtr(ptr);
		versionNumber = ptr->versionNumber();
	}

	bool Ensure()
	{
		return versionNumber != 0 && ptr->ToPointer() != nullptr && ((xx::MPObject*)ptr->ToPointer())->versionNumber() == versionNumber;
	}

	void AssertEnsure()
	{
		System::Diagnostics::Debug::Assert(versionNumber != 0 && ptr->ToPointer() != nullptr && ((xx::MPObject*)ptr->ToPointer())->versionNumber() == versionNumber);
	}

	~MPtrWrapper()
	{
		AssertEnsure();
		((xx::MPObject*)ptr->ToPointer())->Release();
		this->ptr = nullptr;
		versionNumber = 0;
	}
};
ref class UVWrapper;

/********************************************************************************************************************/
// UVClientPeerWrapper
/********************************************************************************************************************/

public enum class UVPeerStates
{
	Disconnected,
	Connecting,
	Connected,
	Disconnecting
};

public delegate void(Delegate_OnConnect)();
public delegate void(Delegate_OnDisconnect)();
public delegate void(Delegate_OnReceivePackage)(char* buf, int dataLen);

public ref class UVPeerWrapper abstract : MPtrWrapper
{
protected:
	Delegate_OnConnect^ onConnect;
	Delegate_OnDisconnect^ onDisconnect;
	Delegate_OnReceivePackage^ onReceivePackage;
	void OnReceivePackageCore(char* buf, int dataLen)
	{
		System::IntPtr bufPtr(buf);
		auto data = gcnew array<System::Byte>(dataLen);
		Marshal::Copy(bufPtr, data, 0, dataLen);
		OnReceivePackage(data);
	}
public:
	virtual void OnConnect() = 0;
	virtual void OnDisconnect() = 0;
	virtual void OnReceivePackage(array<Byte>^ data) = 0;

	UVPeerWrapper()
	{
		onReceivePackage = gcnew Delegate_OnReceivePackage(this, &UVPeerWrapper::OnReceivePackageCore);
		onConnect = gcnew Delegate_OnConnect(this, &UVPeerWrapper::OnConnect);
		onDisconnect = gcnew Delegate_OnDisconnect(this, &UVPeerWrapper::OnDisconnect);
	}
	//~UVPeerWrapper();

	virtual void InitPtr(xx::MPObject* ptr) override = 0;

	int Disconnect(bool immediately)
	{
		AssertEnsure();
		auto p = (MyUVServerPeer*)ptr->ToPointer();
		if (p->state != xx::UVPeerStates::Connected) return -1;
		p->Disconnect(immediately);
		return 0;
	}
	bool Send(array<System::Byte>^ buf, int offset, int dataLen)
	{
		if (buf->Length < offset + dataLen) throw gcnew System::IndexOutOfRangeException();
		AssertEnsure();
		auto p = (MyUVServerPeer*)ptr->ToPointer();
		if (p->state != xx::UVPeerStates::Connected) return false;

		// ���һ�������õ� bb( ���������Ѿ��������� )
		auto bb = p->GetSendBB(dataLen);

		// ����׷��. δ�����Ƕ��Ⱪ¶ BBuffer ԭ������ �Ӷ�ʵ�� 0 copy
		pin_ptr<System::Byte> pin = &buf[0];
		unsigned char* pinbuf = pin;
		bb->WriteBuf((char*)pinbuf + offset, dataLen);

		// ����( ѹ�����Ͷ��� )
		return p->Send(bb) == 0;
	}
	property UVPeerStates State
	{
		UVPeerStates get()
		{
			AssertEnsure();
			auto p = (MyUVServerPeer*)ptr->ToPointer();
			return (UVPeerStates)(int)p->state;
		}
	}
	property String^ PeerName
	{
		String^ get()
		{
			AssertEnsure();
			auto p = (MyUVServerPeer*)ptr->ToPointer();
			auto& s = p->GetPeerName();
			return gcnew String((const char*)s.buf, 0, s.dataLen);
		}
	}
	// ������Կɲ���Ҫ Ensure ֱ���� �Է��� �ж� peer ��״̬
	property bool Alive
	{
		bool get()
		{
			return Ensure() && State == UVPeerStates::Connected;
		}
	}
};



public ref class UVClientPeerWrapper abstract : UVPeerWrapper
{
public:
	UVWrapper^ creator = nullptr;				// �ӳ��Է� uv �� this ������

	// λ�ڸ��������±�
	int uv_clientPeers_index = -1;

	~UVClientPeerWrapper();

	virtual void InitPtr(xx::MPObject* ptr) override;

	int SetAddress(String^ ip, int port)
	{
		if (String::IsNullOrWhiteSpace(ip)) return -1;
		AssertEnsure();
		auto p = (MyUVClientPeer*)ptr->ToPointer();
		auto ipPtr = Marshal::StringToHGlobalAnsi(ip);
		return p->SetAddress((const char*)ipPtr.ToPointer(), port);
	}

	int Connect()
	{
		AssertEnsure();
		auto p = (MyUVClientPeer*)ptr->ToPointer();
		return p->Connect();
	}
};


/********************************************************************************************************************/
// UVServerPeerWrapper
/********************************************************************************************************************/

ref class UVListenerWrapper;
public ref class UVServerPeerWrapper abstract : UVPeerWrapper
{
public:
	UVListenerWrapper^ creator = nullptr;				// �ӳ��Է� listener �� this ������

	// λ�ڸ��������±�
	int listener_serverPeers_index = -1;

	~UVServerPeerWrapper();

	virtual void InitPtr(xx::MPObject* ptr) override;
};

/********************************************************************************************************************/
// UVListenerWrapper
/********************************************************************************************************************/

public delegate xx::UVServerPeer*(Delegate_OnCreatePeer)();
public ref class UVListenerWrapper abstract : MPtrWrapper
{
public:
	UVWrapper^ creator = nullptr;				// �ӳ��Է� uv �� this ������
	System::Collections::Generic::List<UVServerPeerWrapper^>^ serverPeers;
protected:
	Delegate_OnCreatePeer^ onCreatePeer;
	xx::UVServerPeer* OnCreatePeerCore()
	{
		auto spWrapper = OnCreatePeer();
		System::Diagnostics::Debug::Assert(spWrapper != nullptr);
		auto spPtr = (xx::UVServerPeer*)ptr->ToPointer();
		System::Diagnostics::Debug::Assert(spPtr != nullptr);
		return spPtr;
	}
public:
	virtual UVServerPeerWrapper^ OnCreatePeer() = 0;	// return CreateServerPeer<T>();

	// λ�ڸ��������±�
	int uv_listeners_index = -1;

	UVListenerWrapper()
	{
		onCreatePeer = gcnew Delegate_OnCreatePeer(this, &UVListenerWrapper::OnCreatePeerCore);
		serverPeers = gcnew List<UVServerPeerWrapper^>();
	}
	~UVListenerWrapper();
	virtual void InitPtr(xx::MPObject* ptr) override;

	// ������ OnCreatePeer �ڲ����� server peer ����
	generic<typename T> where T : UVServerPeerWrapper
		T CreateServerPeer(T wrapper)
	{
		System::Diagnostics::Debug::Assert(wrapper->creator == nullptr);
		wrapper->creator = this;
		auto t = ((xx::MPObject*)ptr->ToPointer())->mempool().Create<MyUVServerPeer>((MyUVListener*)ptr->ToPointer());
		if (t == nullptr) return T();

		wrapper->InitPtr(t);
		wrapper->OnConnect();
		return wrapper;
	}
	generic<typename T> where T : UVServerPeerWrapper, gcnew()
		T CreateServerPeer()
	{
		return CreateServerPeer(gcnew T());
	}
};


/********************************************************************************************************************/
// UVTimerWrapper
/********************************************************************************************************************/

public delegate void(Delegate_OnTicks)();
public ref class UVTimerWrapper abstract : MPtrWrapper
{
public:
	UVWrapper^ creator = nullptr;				// �ӳ��Է� uv �� this ������
protected:
	Delegate_OnTicks^ onTicks;
public:
	virtual void OnCreated() = 0;
	virtual void OnTicks() = 0;

	// λ�ڸ��������±�
	int uv_timers_index = -1;

	UVTimerWrapper()
	{
		onTicks = gcnew Delegate_OnTicks(this, &UVTimerWrapper::OnTicks);
	}
	~UVTimerWrapper();
	virtual void InitPtr(xx::MPObject* ptr) override;
	int Start(System::UInt64 timeoutMS, UInt64 repeatIntervalMS)
	{
		AssertEnsure();
		auto p = (MyUVTimer*)ptr->ToPointer();
		return p->Start(timeoutMS, repeatIntervalMS);
	}
};


/********************************************************************************************************************/
// UVAsyncWrapper
/********************************************************************************************************************/

public delegate void(Delegate_OnFire)();
public ref class UVAsyncWrapper abstract : MPtrWrapper
{
public:
	UVWrapper^ creator = nullptr;				// �ӳ��Է� uv �� this ������
protected:
	Delegate_OnFire^ onFire;
public:
	virtual void OnFire() = 0;

	// λ�ڸ��������±�
	int uv_asyncs_index = -1;

	UVAsyncWrapper()
	{
		onFire = gcnew Delegate_OnFire(this, &UVAsyncWrapper::OnFire);
	}
	~UVAsyncWrapper();
	virtual void InitPtr(xx::MPObject* ptr) override;

	void Fire();
};

/********************************************************************************************************************/
// UVWrapper
/********************************************************************************************************************/

public ref class UVWrapper
{
public:
	xx::MemPool* mp;
	xx::UV* uv;
	List<UVClientPeerWrapper^>^ clientPeers;
	List<UVListenerWrapper^>^ listeners;
	List<UVTimerWrapper^>^ timers;
	List<UVAsyncWrapper^>^ asyncs;

	UVWrapper()
	{
		mp = new xx::MemPool();
		uv = mp->Create<xx::UV>();
		clientPeers = gcnew List<UVClientPeerWrapper^>();
		listeners = gcnew List<UVListenerWrapper^>();
		timers = gcnew List<UVTimerWrapper^>();
		asyncs = gcnew List<UVAsyncWrapper^>();
	}
	~UVWrapper()
	{
		// todo: foreach clientPeers, listeners, timers, syncs Dispose ?

		uv->Stop();
		uv->Release();
		uv = nullptr;
		delete mp;
		mp = nullptr;
	}


	/***************************************************************************************/
	// CreateTimer

	generic<typename T> where T : UVTimerWrapper
		T CreateTimer(T wrapper)
	{
		System::Diagnostics::Debug::Assert(wrapper->creator == nullptr);
		wrapper->creator = this;

		auto t = uv->CreateTimer<MyUVTimer>();
		if (t == nullptr) return T();

		wrapper->InitPtr(t);
		wrapper->OnCreated();
		return wrapper;
	}
	generic<typename T> where T : UVTimerWrapper, gcnew()
		T CreateTimer()
	{
		return CreateTimer(gcnew T());
	}


	/***************************************************************************************/
	// CreateAsync

	generic<typename T> where T : UVAsyncWrapper
		T CreateAsync(T wrapper)
	{
		System::Diagnostics::Debug::Assert(wrapper->creator == nullptr);
		wrapper->creator = this;

		auto t = uv->CreateAsync<MyUVAsync>();
		if (t == nullptr) return T();

		wrapper->InitPtr(t);
		return wrapper;
	}
	generic<typename T> where T : UVAsyncWrapper, gcnew()
		T CreateAsync()
	{
		return CreateAsync(gcnew T());
	}


	/***************************************************************************************/
	// CreateClientPeer

	generic<typename T> where T : UVClientPeerWrapper
		T CreateClientPeer(T wrapper)
	{
		System::Diagnostics::Debug::Assert(wrapper->creator == nullptr);
		wrapper->creator = this;

		auto t = uv->CreateClientPeer<MyUVClientPeer>();
		if (t == nullptr) return T();

		wrapper->InitPtr(t);
		return wrapper;
	}
	generic<typename T> where T : UVClientPeerWrapper, gcnew()
		T CreateClientPeer()
	{
		return CreateClientPeer(gcnew T());
	}


	/***************************************************************************************/
	// CreateListener

	generic<typename T> where T : UVListenerWrapper
		T CreateListener(T wrapper, int port, int backlog)
	{
		System::Diagnostics::Debug::Assert(wrapper->creator == nullptr);
		wrapper->creator = this;

		auto t = uv->CreateListener<MyUVListener>(port, backlog);
		if (t == nullptr) return T();

		wrapper->InitPtr(t);
		return wrapper;
	}
	generic<typename T> where T : UVListenerWrapper, gcnew()
		T CreateListener(int port, int backlog)
	{
		return CreateListener(gcnew T(), port, backlog);
	}


	/***************************************************************************************/
	// Run

	void Run()
	{
		uv->Run();
	}

	void Stop()
	{
		uv->Stop();
	}
};




UVListenerWrapper::~UVListenerWrapper()
{
	// �Ӹ��������Ƴ�
	SYSTEM_LIST_SWAP_REMOVE(creator->listeners, this, uv_listeners_index);
}
void UVListenerWrapper::InitPtr(xx::MPObject* ptr)
{
	System::Diagnostics::Debug::Assert(this->ptr == nullptr);
	this->MPtrWrapper::InitPtr(ptr);
	auto p = (MyUVListener*)ptr;
	p->onCreatePeer = Func_OnCreatePeer(Marshal::GetFunctionPointerForDelegate(onCreatePeer).ToPointer());

	// �Ž��������Լӳ�
	uv_listeners_index = creator->listeners->Count;
	creator->listeners->Add(this);
}



UVTimerWrapper::~UVTimerWrapper()
{
	// �Ӹ��������Ƴ�
	SYSTEM_LIST_SWAP_REMOVE(creator->timers, this, uv_timers_index);
}
void UVTimerWrapper::InitPtr(xx::MPObject* ptr)
{
	System::Diagnostics::Debug::Assert(this->ptr == nullptr);
	this->MPtrWrapper::InitPtr(ptr);
	auto p = (MyUVTimer*)ptr;
	p->onTicks = Func_OnTicks(Marshal::GetFunctionPointerForDelegate(onTicks).ToPointer());

	// �Ž��������Լӳ�
	uv_timers_index = creator->timers->Count;
	creator->timers->Add(this);
}



UVAsyncWrapper::~UVAsyncWrapper()
{
	// �Ӹ��������Ƴ�
	SYSTEM_LIST_SWAP_REMOVE(creator->asyncs, this, uv_asyncs_index);
}
void UVAsyncWrapper::InitPtr(xx::MPObject* ptr)
{
	System::Diagnostics::Debug::Assert(this->ptr == nullptr);
	this->MPtrWrapper::InitPtr(ptr);
	auto p = (MyUVAsync*)ptr;
	p->onFire = Func_OnFire(Marshal::GetFunctionPointerForDelegate(onFire).ToPointer());

	// �Ž��������Լӳ�
	uv_asyncs_index = creator->asyncs->Count;
	creator->asyncs->Add(this);
}
void UVAsyncWrapper::Fire()
{
	AssertEnsure();
	auto p = (MyUVAsync*)ptr->ToPointer();
	p->Fire();
}





UVClientPeerWrapper::~UVClientPeerWrapper()
{
	// �Ӹ��������Ƴ�
	SYSTEM_LIST_SWAP_REMOVE(creator->clientPeers, this, uv_clientPeers_index);
}
void UVClientPeerWrapper::InitPtr(xx::MPObject* ptr)
{
	System::Diagnostics::Debug::Assert(this->ptr == nullptr);
	this->MPtrWrapper::InitPtr(ptr);
	auto p = (MyUVClientPeer*)ptr;
	p->onReceivePackage = Func_OnReceivePackage(Marshal::GetFunctionPointerForDelegate(onReceivePackage).ToPointer());
	p->onConnect = Func_OnConnect(Marshal::GetFunctionPointerForDelegate(onConnect).ToPointer());
	p->onDisconnect = Func_OnDisconnect(Marshal::GetFunctionPointerForDelegate(onDisconnect).ToPointer());

	// �Ž��������Լӳ�
	uv_clientPeers_index = creator->clientPeers->Count;
	creator->clientPeers->Add(this);
}




UVServerPeerWrapper::~UVServerPeerWrapper()
{
	// �Ӹ��������Ƴ�
	SYSTEM_LIST_SWAP_REMOVE(creator->serverPeers, this, listener_serverPeers_index);
}

void UVServerPeerWrapper::InitPtr(xx::MPObject* ptr)
{
	System::Diagnostics::Debug::Assert(this->ptr == nullptr);
	this->MPtrWrapper::InitPtr(ptr);
	auto p = (MyUVServerPeer*)ptr;
	p->onReceivePackage = Func_OnReceivePackage(Marshal::GetFunctionPointerForDelegate(onReceivePackage).ToPointer());
	p->onDisconnect = Func_OnDisconnect(Marshal::GetFunctionPointerForDelegate(onDisconnect).ToPointer());

	// �Ž��������Լӳ�
	listener_serverPeers_index = creator->serverPeers->Count;
	creator->serverPeers->Add(this);
}
