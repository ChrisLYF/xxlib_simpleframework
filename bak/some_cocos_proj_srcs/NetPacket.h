#ifndef _NETPACKET_H_
#define _NETPACKET_H_

// ���賤�ȵĶ��������
class NetPacket : public Ref
{
public:
    NetPacket();
    explicit NetPacket( int maxPkgLen );
    ~NetPacket();
    NetPacket( NetPacket const & other );
    NetPacket( NetPacket && other );
    NetPacket & operator=( NetPacket const & other );
    NetPacket & operator=( NetPacket && other );

    static NetPacket* create( int maxPkgLen = 0 );

    // ֱ��ϵ�У���û���κ�Խ���ʧ���ж�.

    // ��������˵��
    // d: direct
    // w: write
    // r: read
    // u: unsigned
    // 4, 2, 1: bytes
    // f: float
    // d: double
    // s: string

    void        dwu4( uint32 v );
    void        dwu2( uint16 v );
    void        dwu1( uint8  v );
    void        dw4( int32   v );
    void        dw2( int16   v );
    void        dw1( int8    v );
    void        dwf( float   v );
    void        dwd( double  v );
    void        dws( const char * v );

    uint32      dru4();
    uint16      dru2();
    uint8       dru1();
    int32       dr4();
    int16       dr2();
    int8        dr1();
    float       drf();
    double      drd();
    string      drs();

    // �� read offset
    void        clearRo();

    // �� write offset
    void        clearWo();

    // �� read & write offset
    void        clear();

    // ����ʣ���д����( �� offset ���� )
    int         owfree( int offset ) const;

    // ����ʣ��ɶ�����( �� offset ���� )
    int         orfree( int offset ) const;

    // ����ʣ���д����
    int         wfree() const;

    // ����ʣ��ɶ�����
    int         rfree() const;

    // �����ڴ� dump string( �� prepare һ�� )
    string      dump();

    // ���� read offset
    int         roffset() const;

    // ���� write offset
    int         woffset() const;

    // ���� read offset
    void        roffset( int v );

    // ���� write offset
    void        woffset( int v );

    // ���� opcode
    uint16      opcode() const;

    // ���� opcode
    void        opcode( uint16 v );

    // ���� buffer ����
    int         bufLen() const;

    // �õ� buffer ָ��( ��ӳ�䵽 LUA )
    char *      bufData() const;

    // ���� �� ����
    int         pkgLen() const;

    // ���� �� ����ָ��
    char *      pkgData() const;

    // ���� ������ָ��
    char *      data() const;

    // ���� ���ݳ���
    int         dataLen() const;

    // ����ǰ��׼��( �ڰ�ͷ�� ��� ������ )
    void        prepare();

    // ������ C++ �з���ʹ�õĺ���( ��ӳ�䵽 LUA )

    template<typename T>
    NetPacket & operator<<( T v );

    template<typename T>
    NetPacket & operator>>( T & v );

    template<typename T>
    void dw( T const & v );

    template<typename T>
    void dr( T & v );

protected:

#pragma pack( push, 1 )
    // ���౻���ڽ� _buf ǿתΪ����ָ�뷽��������Լ���������
    struct Header
    {
        int32 pkgLen;
        uint16 opcode;
    };
#pragma pack( pop )

    // �� _buf ͷӲתΪ Header �ṹ��������
    inline Header & header() { return *reinterpret_cast<Header*>( bufData() ); }
    inline Header const & header() const { return *reinterpret_cast<Header*>( bufData() ); }

    int         _ro, _wo;               // ��д offset ֵ
    int         _bufLen;                // �����¼��ֵΪʵ�� buffer �� - sizeof(Header)
    char *      _buf;                   // ָ����������ʼ��ַ���� sizeof(Header) ��Ϊʵ��ָ����ʼ��ַ
};




template<typename T>
NetPacket & NetPacket::operator<<( T v )
{
    BufferHelper::write( _buf, _wo, v );
    return *this;
}

template<typename T>
NetPacket & NetPacket::operator>>( T & v )
{
    BufferHelper::read( _buf, v, _ro );
    return *this;
}

template<typename T>
void NetPacket::dr( T & v )
{
    BufferHelper::read( _buf, v, _ro );
}

template<typename T>
void NetPacket::dw( T const & v )
{
    BufferHelper::write( _buf, _wo, v );
}




#endif
