#ifndef _MY_UTILS_H_
#define _MY_UTILS_H_

namespace Utils
{

    // �ļ�����( ��������·��, ����ｨĿ¼. Ҳ���� check �ļ��Ƿ����. ֱ�Ӵ����򸲸� )
    bool fileCopy( string const & src, string const & dest );

    // ����ļ��Ƿ����
    bool fileExists( string const & fn );

    // ɾ�ļ�, �ɹ����� true
    bool fileDelete( string const & fn );

    // �ļ����� �� �ƶ�
    bool fileRename( string const & src, string const & dest );

    // ���ļ�д�� buf, �ɹ����� true
    bool fileWriteBytes( string const & fn, char const * buf, int len );

    // ȡ�ļ��ߴ�( ����һ��δ���򿪵��ļ��� )
    int fileMoveSeekEnd( string const & fn );

    // �ƶ��ļ�ָ�뵽��β�� �������ļ��ߴ� ( ����һ���Ѿ��򿪵��ļ���� )
    int fileMoveSeekEnd( FILE* f );

    // �����ļ��ߴ� ( ����һ���Ѿ��򿪵��ļ���� )
    int fileGetSize( FILE* f );

    // �и��ļ��ߴ�Ϊָ��ֵ( �����Ѵ򿪵��ļ���� ). ���������س���ʱ�����һ����Ч����ȥ��
    bool fileCut( FILE* f, uint size );

    // �򵥷�װ FileUtils �Ĺ��ܺ���
    string fileGetFullPath( string const & fn );

    // path: �е��ļ���, ����Ŀ¼����( ����� / )
    string filePathCutFileName( string const & fn );

    // �򵥷�װȡ�ļ����ݵĺ���
    Data fileGetData( string const & fn );

    // �򵥷�װȡ�ı��ļ����ݵĺ���
    string fileGetText( string const & fn );

    // ���� IOS ƽ̨����, ���Ŀ¼���ļ�, ������ "���Զ����ݵ� iCloud" ������
    void fileSkipBackup( string const & path );

    // �� plist �ֵ��� ȡ�ò���ͼ�ļ���
    string getTextureFileNameFromPlistDict( ValueMap const & plistDict );

    //// ����λ�� atlas �ļ������е� ͼƬ�ļ����б�
    //vector<string> getFileNamesByAtlas( string const & data );

    // �� plist �ļ��� ȡ�ò���ͼ�ļ���
    string getTextureFileNameFromPlistFile( string const & fn );

    // �ִ�֮���滻( ֱ�Ӹ�Դ�� )
    void simpleReplace( string &s, char src, char dest );

    // �ִ�֮���滻( �����滻֮��Ĵ� )
    string simpleReplace( string &s, char src, string const & dest );

    // ��Ŀ¼( ����һ��һ������ ). �ɹ� �� �Ѵ��� ������ true
    bool createDirectory( char const * tarPath );

    // ȷ�� Ŀ��Ŀ¼ һ������( ���û��, ��һ��һ������ )
    // ����:
    //      fullPath: λ�� "��д����" �� "Ŀ¼ȫ·��"( ��Ҫ: ���ܺ����ļ��� )
    bool ensureDirectory( string fullPath );

    // �򵥵��� hash �ĺ���
    uint getHashCode( char const * buf, int len );

    // �򵥵��и��ִ�
    vector<string> split( string const & s, string const & delim );

    // �ж��Ƿ����  λ�� scrollview, clipping region node �еĸ��ڵ�
    // ���ж� ���ڵ�ɼ���,  ��ǰ������Ƿ�λ�ڡ�����ʾ����, ����� btn �� btn , ���Ƿ� enable
    bool ensureTouch( Node * btn, Touch * touch );

    // �ж��Ƿ������ڲ�����
    bool isTouchInside( Node * o, Touch * touch );




    // ��������, �ڳ� {} ��Χʱ�Զ�ִ�� lambda
    class ScopeGuard
    {
    public:
        typedef function<void()> FT;

        // ���� f
        ScopeGuard( FT f );

        // ִ�� f
        ~ScopeGuard();

        // ִ�� f, ������ʱ����ִ�� f
        void runAndCancel();

        // ִ�� f
        void run();

        // ������ʱ����ִ�� f
        void cancel();

        // ���� f
        void set( FT f );

    private:
        FT _f;
        ScopeGuard( const ScopeGuard & );             // delete
        ScopeGuard &operator=( const ScopeGuard & );  // delete
    };





    /************************************************************************/
    /* ������ fill string buffer ���. Σ�� ������                          */
    /************************************************************************/

    // �� ԭ���������� תΪ�ִ� д�� buf, ������䳤��
    int fillCore( char * buf, char v );
    int fillCore( char * buf, int v );
    int fillCore( char * buf, uint v );
    int fillCore( char * buf, float v );
    int fillCore( char * buf, double v );
    int fillCore( char * buf, string const & s );
    template<int len>
    int fillCore( char * buf, char const ( &s )[ len ] )
    {
        memcpy( buf, s, len - 1 );
        return len - 1;
    }

    // �� wchar��ֻ֧�ֵ� ucs2��תΪ "\uXXXX" �� 16���� �ִ� ��̬ д�� buf
    void fillU16( char * buf, ushort c );

    // �� s: utf8 ֻ֧�ֵ� ucs2����ת��Ϊ json �ִ���� buf ( " -> \"  ,  ���ַ�-> \uXXXX ), ����ת����ĳ�
    int fillToJson( char * buf, int bufLen, char const * s, int sLen );

    // �� 1�� utf8 ��ʽ���ַ� תΪ 1 �� wchar ��䵽 buf. ������䳤��
    int fillWChar( ushort * buf, char const * s );

    // �� 1 �� wchar תΪ utf8 ��䵽 buf, ������䳤��
    int fillUtf8( char * buf, ushort c );

    // �� buf ��������� ���� toString �� ������һ������˶����ֽ�
    template<typename T0>
    int fill( char * buf, T0 const & v0 )
    {
        int offset = fillCore( buf, v0 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1>
    int fill( char * buf, T0 const & v0, T1 const & v1 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        offset += fillCore( buf + offset, v4 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        offset += fillCore( buf + offset, v4 );
        offset += fillCore( buf + offset, v5 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        offset += fillCore( buf + offset, v4 );
        offset += fillCore( buf + offset, v5 );
        offset += fillCore( buf + offset, v6 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6, T7 const & v7 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        offset += fillCore( buf + offset, v4 );
        offset += fillCore( buf + offset, v5 );
        offset += fillCore( buf + offset, v6 );
        offset += fillCore( buf + offset, v7 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6, T7 const & v7, T8 const & v8 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        offset += fillCore( buf + offset, v4 );
        offset += fillCore( buf + offset, v5 );
        offset += fillCore( buf + offset, v6 );
        offset += fillCore( buf + offset, v7 );
        offset += fillCore( buf + offset, v8 );
        buf[ offset ] = '\0';
        return offset;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    int fill( char * buf, T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6, T7 const & v7, T8 const & v8, T9 const & v9 )
    {
        int offset = fillCore( buf, v0 );
        offset += fillCore( buf + offset, v1 );
        offset += fillCore( buf + offset, v2 );
        offset += fillCore( buf + offset, v3 );
        offset += fillCore( buf + offset, v4 );
        offset += fillCore( buf + offset, v5 );
        offset += fillCore( buf + offset, v6 );
        offset += fillCore( buf + offset, v7 );
        offset += fillCore( buf + offset, v8 );
        offset += fillCore( buf + offset, v9 );
        buf[ offset ] = '\0';
        return offset;
    }



    inline int toStringGetMaxLength( char v ) { return 4; }
    inline int toStringGetMaxLength( int v ) { return 11; }
    inline int toStringGetMaxLength( uint v ) { return 10; }
    inline int toStringGetMaxLength( float v ) { return 17; }
    inline int toStringGetMaxLength( double v ) { return 20; }
    inline int toStringGetMaxLength( string const & s ) { return s.size(); }

    template<typename T0>
    std::string toString( T0 const & v0 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 ) );
        s.resize( fill( (char*)s.data(), v0 ) );
        return s;
    }
    template<typename T0, typename T1>
    std::string toString( T0 const & v0, T1 const & v1 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 ) 
                  + toStringGetMaxLength( v2 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 ) 
                  + toStringGetMaxLength( v2 ) 
                  + toStringGetMaxLength( v3 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  + toStringGetMaxLength( v2 )
                  + toStringGetMaxLength( v3 )
                  + toStringGetMaxLength( v4 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3, v4 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  + toStringGetMaxLength( v2 )
                  + toStringGetMaxLength( v3 )
                  + toStringGetMaxLength( v4 )
                  + toStringGetMaxLength( v5 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3, v4, v5 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  + toStringGetMaxLength( v2 )
                  + toStringGetMaxLength( v3 )
                  + toStringGetMaxLength( v4 )
                  + toStringGetMaxLength( v5 )
                  + toStringGetMaxLength( v6 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3, v4, v5, v6 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6, T7 const & v7 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  + toStringGetMaxLength( v2 )
                  + toStringGetMaxLength( v3 )
                  + toStringGetMaxLength( v4 )
                  + toStringGetMaxLength( v5 )
                  + toStringGetMaxLength( v6 )
                  + toStringGetMaxLength( v7 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3, v4, v5, v6, v7 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6, T7 const & v7, T8 const & v8 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  + toStringGetMaxLength( v2 )
                  + toStringGetMaxLength( v3 )
                  + toStringGetMaxLength( v4 )
                  + toStringGetMaxLength( v5 )
                  + toStringGetMaxLength( v6 )
                  + toStringGetMaxLength( v7 )
                  + toStringGetMaxLength( v8 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3, v4, v5, v6, v7, v8 ) );
        return s;
    }
    template<typename T0, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8, typename T9>
    std::string toString( T0 const & v0, T1 const & v1, T2 const & v2, T3 const & v3, T4 const & v4, T5 const & v5, T6 const & v6, T7 const & v7, T8 const & v8, T9 const & v9 )
    {
        std::string s;
        s.resize( toStringGetMaxLength( v0 )
                  + toStringGetMaxLength( v1 )
                  + toStringGetMaxLength( v2 )
                  + toStringGetMaxLength( v3 )
                  + toStringGetMaxLength( v4 )
                  + toStringGetMaxLength( v5 )
                  + toStringGetMaxLength( v6 )
                  + toStringGetMaxLength( v7 )
                  + toStringGetMaxLength( v8 )
                  + toStringGetMaxLength( v9 )
                  );
        s.resize( fill( (char*)s.data(), v0, v1, v2, v3, v4, v5, v6, v7, v8, v9 ) );
        return s;
    }






    /************************************************************************/
    /* �����ִ��������                                                     */
    /************************************************************************/

    void trim( string& s );








    /************************************************************************/
    /* utf8 ��ش���                                                        */
    /************************************************************************/

    // �õ����� ָ������ �İ�Ǹ����� utf8 �����и��( ĩβ�İ���ֽ��������� )
    int getCutIndex( char const * s, int numHalfChars );

    // ͳ�� utf8 ��������( ���ַ��� 1 �� )
    int getWCharCount( char const * s );

    // ͳ�� utf8 ���� ���ַ� �ĸ���
    int getOnlyWCharCount( char const * s );

    // ͳ�� utf8 ��������( ���ַ��� 2 �� )
    int getCharCount( char const * s );












    /************************************************************************/
    /* ��ݻ������                                                         */
    /************************************************************************/

    // ͼƬ����Ϊ ���� �ṹ�� [ ] �� [v] ����ͼƴ��
    Sprite * createSwitchButtonSprite( string const & fileName );

    // ���ݴ���ڵ�� pos, contentSize, �ڽڵ�������ʾ����( ��ǰֻ֧��ê��Ϊ 0.5, 0.5 )
    Node * drawRect( Node * n );
    Node * drawRichTextRect( Node * n );

    // ����һ������ק�ľ���
    Sprite * createDragableSprite( string const & fileName );









    /************************************************************************/
    /* ������Щ���Ʒ����Ƴ����Ĵ���                                       */
    /************************************************************************/



    // ����Ϊ map ֮�������� key ֵ��ģ��( ���� )
    template<typename T>
    class Hash
    {
    public:
        typedef Hash<T> CT;
        Hash<T>( T && t ) : _t( t )
        {
            // todo: type trits check, 
            // ��� T ��������������( ����������������ö�� ), size С�ڵ��� sizeof(_hash) ��ֱ�Ӵ�, ��ĵ� char* ��
            // ��� T ����, ��ʵ���� int hash() ����, ��֮. ������ӲתΪ char* ����.
            // ��� T ��ָ��, ����תΪ��, ͬ�ϴ���
            _hash = Utils::getHashCode( (char*)&t, sizeof( t ) );
        }
        bool operator<( CT const & other ) const
        {
            if( this->_hash < other._hash ) return true;
            else if( this->_hash == other._hash )
            {
                // todo: type trits check, ��� T ��ָ��, תΪ���� ...,  ��Ҫ����ʵ�� operator < 
                return *this < other;
            }
            return false;
        }
        T   _t;
        int _hash;
    };



    //// �򵥵İ������ַ��и��ִ� ������ pair<char*, size_t> ָ��, ���� �� ( ��û�� )
    //inline vector<pair<char const *, size_t>> splite( string const &s, char sc )
    //{
    //    auto p = s.c_str();                             // ������
    //    vector<pair<char const *, size_t>> ss;
    //    ss.reserve( 16 );                               // ��������
    //    int prev = 0;
    //    size_t i = 0;
    //    for( ; i < s.size(); ++i )
    //    {
    //        if( p[ i ] == sc )
    //        {
    //            ss.push_back( make_pair( p + prev, i - prev ) );
    //            prev = i + 1;
    //        }
    //    }
    //    if( i < s.size() )
    //    {
    //        ss.push_back( make_pair( p + i, ss.size() - i ) );
    //    }
    //    return ss;
    //}


}

#endif
