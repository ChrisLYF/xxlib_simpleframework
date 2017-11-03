#ifndef _NETCORE_H_
#define _NETCORE_H_

class Hosts;
class NetClient;
struct NetContext
{
    NetContext( uint32 ipv4, uint16 port, int timeoutMS, NetClient * client );

    uint32      ip;
    uint16      port;
    int         timeoutMS;
    NetClient*  client;
};


class NetEngine
{
public:
    MAKE_INSTANCE_H( NetEngine );

    // ��ʼ��ƽ̨ socket
    NetEngine();

    // ����ʼ��ƽ̨ socket
    ~NetEngine();

    // for �����������¼� ƽ̨������ ���� ���õ�ǰ������������
    static void setNetType( NetTypes e );

    // �õ���ǰ������������
    NetTypes getNetType();

    // �� close ���� NetClient ��ͬ���ȴ������
    void shutdown();

    // Ϊ client ���������߳�
    bool makeWorker( NetClient * token, const char * name, uint16 port, int timeoutMS = -1 );

    // �� close ���� NetClient
    void closeAll();

    // ���� ���� Socket��
    void update();

    friend NetClient;
private:
    // ͨ�����Ӳ����������Խ��� tcp ����
    static bool connect( NetContext * ctx );

    // client �����̺߳���
    void * workerProcess( NetContext * ctx );

    ThreadSafeQueue<NetClient*>     _clients;
    std::atomic<int>                _numWorkers;
    static NetTypes                 _netType;
};



#endif
