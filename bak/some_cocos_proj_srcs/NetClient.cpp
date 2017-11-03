#include "Precompile.h"




int NetClient::sendCore( const char * buf, int len )
{
    if( len < 1 ) return 0;
    int n = 0;
    while( len )
    {              // һֱ����ֱ�� ���� �� ����
        int r = ::send( _socket, buf, len, 0 );
        if( r == -1 )
        {
            int e = NetUtils::getSockErrNo();
            if( e != EAGAIN &&
                e != EWOULDBLOCK )     // other errors: close socekt
                close();
            CCLOG("\t!!!! SEND ERROR: %u", e);
            break;
        }
        len -= r;
        n += r;
    }
    return n;       // �����ѷ����ĳ���
}

bool NetClient::send( const char * buf, int len )
{
    if( isClosed() ) return false;
    if( !_sendBuf.empty() )
    {           // �������û��������ݣ�����ѹ���������
        //if( _sendBuf.size() + len > (int)NetConfigs::MaxWriteBufferSize )
        //{
        //    close();
        //    return false;               // ������������ͻ��泤�����ƾ͹����ӣ�����ʧ��
        //}
        _sendBuf.push( buf, len );   // ��������ѹ��ȥ
        CCLOG("\tSEND BUFFER: %u", _sendBuf.size());
        return true;
    }
    auto n = sendCore( buf, len );
    if( n < len )                // �������û�з��꣬��ʣ�µ�ѹ������ͻ���
    {
        _sendBuf.push( buf + n, len - n );
        CCLOG("\tSEND BUFFER: %u", _sendBuf.size());
    }
    else
    {
        CCLOG("\tSEND SYS!!!: %u", n);
    }
    return true;
}

bool NetClient::send( NetPacket *p )
{
    if( isClosed() ) return false;
    CCLOG("<<<<<<<<<<<<< SEND: %u", p->opcode());
    p->prepare();
    return send( p->pkgData(), p->pkgLen() );
}

void NetClient::sendLeft()
{
    if( isClosed() ) return;
    int total = _sendBuf.size() < (int)NetConfigs::MaxSendSizePerFrame ? _sendBuf.size() : (int)NetConfigs::MaxSendSizePerFrame;
    char buf[ (int)NetConfigs::BufferSize ];
    while( total )
    {
        int len = _sendBuf.copy( buf, (int)NetConfigs::BufferSize );
        int n = sendCore( buf, len );
        _sendBuf.pop( n );
        if( n < len ) break;
        total -= n;
    }
}

int NetClient::receive()
{
    // �����ԣ�ִ�е��˴�ʱ��_readBuf ����Ϊ�գ��ʣ�recv �����ʣ��ռ�
    assert( _readBuf.wfree() );

    int n, e = 0;

    do
    {
        n = ::recv( _socket, _readBuf.data() + _readBuf.woffset(), _readBuf.wfree(), 0 );
        if (n == -1) 
        {
            e = NetUtils::getSockErrNo();
            if (e == EINTR) continue;
            CCLOG( "SOCKET ERROR: %d", e);

            if( e != EAGAIN &&
                e != EWOULDBLOCK )
                close();
            return -1;
        }
        else if (n == 0)
        {
            CCLOG("recv data connect closed!");
            close();
            return -1;
        }
        else if (n > 0)
        {
            CCLOG("\trecv data: %u", n);
            _readBuf.woffset( _readBuf.woffset() + n );
            onReceived( _readBuf );
            return 0;
        }
        else
        {
            e = NetUtils::getSockErrNo();
            CCLOG( "SOCKET ERROR: %d", e);

            if( e != EAGAIN &&
                e != EWOULDBLOCK )
            {
                close();
                return -1;
            }
        }
        break;
    }
    while (true);

	return 0;
}

void NetClient::setState( SocketStates s )
{
    std::lock_guard<std::mutex> g( _mutex );
    _state = s;
}


bool NetClient::isAlive() const
{
    return _state == SocketStates::Alive;
}

bool NetClient::isClosed() const
{
    return _state >= SocketStates::Closed;
}

bool NetClient::isDead() const
{
    return _state == SocketStates::Dead;
}



NetClient::NetClient()
: _luaFN( 0 )
, _port( 0 )
, _timeoutMS( 0 )
, _socket( 0 )
, _state( SocketStates::Unknown )
, _sendBuf( (int)NetConfigs::MaxWriteBufferSize )
, _threadRunning( false )
{
    NetEngine::getInstance()->_clients.push( this );
}

NetClient::~NetClient()
{
    clear();
    close();
    while( _threadRunning ) sleepMS( 100 );
    NetEngine::getInstance()->_clients.erase( [ this ]( NetClient*& c ) { return c == this; } );
}

void NetClient::setParms( string ipv4, uint16 port, int timeoutMS /*= 8000 */ )
{
    _ipv4 = ipv4;
    _port = port;
    _timeoutMS = timeoutMS;
}

bool NetClient::connect()
{
    return NetEngine::getInstance()->makeWorker( this, _ipv4.c_str(), _port, _timeoutMS );
}

bool NetClient::connect( string ipv4, uint16 port, int timeoutMS /*= 8000 */ )
{
    setParms( ipv4, port, timeoutMS );
    return connect();
}

void NetClient::close()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WP8) || (CC_TARGET_PLATFORM == CC_PLATFORM_WINRT)
    {
        std::lock_guard<std::mutex> g( _mutex );
        if( isClosed() ) return;
        _state = SocketStates::Closed;
    }
    while (_threadRunning) sleepMS(100);
    {
        std::lock_guard<std::mutex> g( _mutex );
        NetUtils::closeSocket( _socket );
        _sendBuf.clear();
    }
#else
    std::lock_guard<std::mutex> g( _mutex );
    if( isClosed() ) return;
    NetUtils::closeSocket( _socket );
    _state = SocketStates::Closed;
    _sendBuf.clear();
#endif
}

void NetClient::clear()
{
    _evnts.clear( []( NetClientEventType & e )
    {
        if( e.second ) delete e.second;
    } );
}

void NetClient::registerNetEventHandler( LUA_FUNCTION fn )
{
    _luaFN = fn;
}

void NetClient::unregisterNetEventHandler()
{
    _luaFN = 0;
}

void NetClient::update()
{
    sendLeft();
    if( _luaFN )
    {
        NetClientEventType e;
        while( _evnts.pop( e ) )
        {
            LuaHelper::push( (int)e.first );
            LuaHelper::push( e.second, "NetPacket" );
            LuaHelper::executeFunction( _luaFN, 2 );
            if( e.second ) delete e.second;
        }
    }
}

void NetClient::onConnected()
{
    _evnts.push( make_pair( NetEvents::Connected, ( NetPacket* )nullptr ) );
}

void NetClient::onConnectFailed()
{
    _evnts.push( make_pair( NetEvents::ConnectFailed, ( NetPacket* )nullptr ) );
}

void NetClient::onClosed()
{
    _evnts.push( make_pair( NetEvents::Closed, ( NetPacket* )nullptr ) );
}

void NetClient::onReceived( NetPacket & p )
{
    // ��� p.size() >= 4 ���ж�ͷ 4 �ֽڵ�����( ���� )����һ�� check p.size()
    // ������㳤�ȣ��� new NetPacket, copy �� push �� ThreadSafeQueue���ٽ�����������Ƶ�ͷ����ͬ�� woffset
    while( p.woffset() >= 4 )
    {                            // ��ǰ��ͷǰ 4 �ֽ�Ϊ���ܳ�������ȷ�����չ� 4 �ֽ��ٽ�һ���жϰ��Ƿ��ѽ�����
        int len = 0;
        BufferHelper::get( len, p.data(), 0 );              // ȡ���ܳ�
        if( p.woffset() < len ) break;                      // �����Ȳ���һ�������˳�ѭ��

        auto np = new NetPacket( len );                     // ���������
        uint16 opcode = 0;
        BufferHelper::get( opcode, p.data(), 4 );           // ȡ opcode
        np->opcode( opcode );                                 // �� opcode
        memcpy( np->data(), p.data() + 6, len - 6 );        // �������ݲ���
        np->woffset( len - 6 );                            // �����ݳ�
        _evnts.push( std::make_pair( NetEvents::ReceivedPacket, np ) ); // ѹ��һ���������¼�

        CCLOG(">>>>>>>>>>>> RECV: %u", np->opcode()); 

        // �Ƴ��Ѵ����İ�
        p.woffset( p.woffset() - len );
        if( p.woffset() ) memmove( p.data(), p.data() + len, p.woffset() );
    }
}

SocketStates NetClient::getState() const
{
    return _state;
}

NetClient* NetClient::create()
{
    auto rtv = new NetClient();
    if( !rtv ) return nullptr;
    rtv->autorelease();
    return rtv;
}
