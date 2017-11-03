#ifndef _BUFFERHELPER_H_
#define _BUFFERHELPER_H_


namespace BufferHelper
{
    // �� v תΪ 16 ���� push �� s
    void convertHex( string & s, char v );

    // �� v תΪ 16 ���� push �� s
    void convertHex( string & s, int v );

    // �� buf תΪ ascii ��ʾЧ�� push �� s ( dump ����ר�� )
    void dumpAscII( string & s, char const * buf, int len );

    // �� buf תΪ 16 ���� push �� s �� return s
    string dump( string & s, char const * const & buf, int len );

    // set ϵ�н�ֱ���� buf �� offset ƫ�Ƶ�ַ��д�� v �����ݣ����޸� offset

    void set( char * const & buf, int const & offset, char const * const & v, int const & len );

    void set( char * const & buf, int const & offset, string const & v );

    template<typename T>
    void set( char * const & buf, int const & offset, T const & v )
    {
#ifdef __IA
        *(T*)( buf + offset ) = v;
#else
        auto b = buf + offset;
        auto p = (char*)&v;
        if( sizeof(v) >= 1  ) {
            b[ 0 ] = p[ 0 ];
        }
        if( sizeof(v) >= 2 ) {
            b[ 1 ] = p[ 1 ];
        }
        if( sizeof(v) >= 4 ) {
            b[ 2 ] = p[ 2 ];
            b[ 3 ] = p[ 3 ];
        }
        if( sizeof(v) == 8 ) {
            b[ 4 ] = p[ 4 ];
            b[ 5 ] = p[ 5 ];
            b[ 6 ] = p[ 6 ];
            b[ 7 ] = p[ 7 ];
        }
#endif
    }

    // write ϵ�н��� char* �� offset ��ʼ��ַ��д�� v �����ݣ�ͬʱ�޸� offset����ʵ��˳��д���Ч��

    void write( char * const & buf, int & offset, char const * const & v, int const & len );

    void write( char * const & buf, int & offset, string const & v );

    template<typename T>
    void write( char * const & buf, int & offset, T const & v )
    {
        set( buf, offset, v );
        offset += sizeof( v );
    }

    // read ϵ�н�ֱ�Ӵ� buf �� offset ƫ�Ƶ�ַ����������д�� v

    void get( char * const & v, char const * const & buf, int const & offset, int const & len );

    void get( string & v, char const * const & buf, int const & offset );

    template<typename T>
    void get( T & v, char const * const & buf, int const & offset )
    {
#ifdef __IA
        v = *(T*)( buf + offset );
#else
        auto b = buf + offset;
        auto p = (char*)&v;
        if( sizeof(v) >= 1 ) {
            p[ 0 ] = b[ 0 ];
        }
        if( sizeof(v) >= 2 ) {
            p[ 1 ] = b[ 1 ];
        }
        if( sizeof(v) >= 4 ) {
            p[ 2 ] = b[ 2 ];
            p[ 3 ] = b[ 3 ];
        }
        if( sizeof(v) >= 8 ) {
            p[ 4 ] = b[ 4 ];
            p[ 5 ] = b[ 5 ];
            p[ 6 ] = b[ 6 ];
            p[ 7 ] = b[ 7 ];
        }
#endif
    }

    // read ϵ�н��� buf �� offset ƫ�Ƶ�ַ����������д�� v��ͬʱ�޸� offset����ʵ��˳���ȡ��Ч��

    void read( char * const & v, char const * const & buf, int & offset, int const & len );

    void read( string & v, char const * const & buf, int & offset );

    template<typename T>
    void read( T & v, char const * const & buf, int & offset )
    {
        get( v, buf, offset );
        offset += sizeof( v );
    }

};

#endif
