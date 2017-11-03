#ifndef _NETCLIENT_H_
#define _NETCLIENT_H_

typedef pair<NetEvents, NetPacket*> NetClientEventType;

// ʵ��ָ����պ�����ȷ���������Ժ�ص��¼�
class NetClient : public Ref
{
public:
    // ���÷� client ע�ᵽ NetEngine �� _clients
    NetClient();

    // ���÷� client ��ע�ᵽ NetEngine �� _clients
    ~NetClient();

    static NetClient* create();

    // �����Ƿ�ͨ
    bool isAlive() const;

    // �����Ƿ�ر�
    bool isClosed() const;

    // �����Ƿ�����
    bool isDead() const;

    // ȡ��ǰ����״̬
    SocketStates getState() const;

    // ������
    bool send( const char * buf, int len ); 

    // ����
    bool send( NetPacket * p );

    // ������û�����( ��ͨ��λ��ÿ frame ���� call )
    void sendLeft();

    // ���������������
    void setParms( string ipv4, uint16 port, int timeoutMS = 8000 );

    // �Դ�������( ʹ�� setParms ���õĲ��� )��ʧ�ܽ����� false
    bool connect();

    // ����������Ϣ �� �Դ������ӣ�ʧ�ܽ����� false
    bool connect( string ipv4, uint16 port, int timeoutMS = 8000 );

    // �ر�����
    void close();

    // ע�� lua �հ��¼��ص�
    void registerNetEventHandler( LUA_FUNCTION fn );

    // ��ע�� lua �հ��ص�
    void unregisterNetEventHandler();

    // ���¼�����
    void clear();

    friend NetEngine;
    friend Hosts;
protected:
    // ǿ����������
    int sendCore( const char * buf, int len );

    // ������. � _readBuf, ���� onReceived
    int receive();

    // ������ÿ frame ���¼��� lua
    void update();

    // lock �޸� state
    void setState( SocketStates s );

    std::mutex      _mutex;
    Socket_t        _socket;
    SocketStates    _state;
    ListBuffer      _sendBuf;
    NetPacket       _readBuf;

    // �����¼�������
    void onConnected();
    void onConnectFailed();
    void onClosed();
    void onReceived( NetPacket & p );  // p ��Ϊ readBuf, ��Ҫ����δ��������ݳ������� woffset

    string                              _ipv4;
    uint16                              _port;
    int                                 _timeoutMS;
    LUA_FUNCTION                        _luaFN;
    ThreadSafeQueue<NetClientEventType> _evnts;

    bool                                _threadRunning;
};


#endif
