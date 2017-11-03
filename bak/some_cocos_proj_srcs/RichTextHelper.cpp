#include "Precompile.h"

namespace RichTextHelper
{
    string _cmdTexts[] =
    {
        "Paragraph",
        "ParagraphEnd",
        "UnderLine",
        "UnderLineEnd",
        "Button",
        "ButtonEnd",
        "Font",
        "Color",
        "Offset",
        "Image",
        "Align",
        "Space",
        "LineHeight",
        "Variant"
    };

    unordered_map<string, RTCmds> _cmdsMap;
    void initCmdsMap()
    {
        _cmdsMap.insert( make_pair( "p", RTCmds::Paragraph ) );
        _cmdsMap.insert( make_pair( "paragraph", RTCmds::Paragraph ) );

        _cmdsMap.insert( make_pair( "/p", RTCmds::ParagraphEnd ) );
        _cmdsMap.insert( make_pair( "/paragraph", RTCmds::ParagraphEnd ) );

        _cmdsMap.insert( make_pair( "u", RTCmds::UnderLine ) );
        _cmdsMap.insert( make_pair( "ul", RTCmds::UnderLine ) );
        _cmdsMap.insert( make_pair( "underline", RTCmds::UnderLine ) );

        _cmdsMap.insert( make_pair( "/u", RTCmds::UnderLineEnd ) );
        _cmdsMap.insert( make_pair( "/ul", RTCmds::UnderLineEnd ) );
        _cmdsMap.insert( make_pair( "/underline", RTCmds::UnderLineEnd ) );

        _cmdsMap.insert( make_pair( "b", RTCmds::Button ) );
        _cmdsMap.insert( make_pair( "btn", RTCmds::Button ) );
        _cmdsMap.insert( make_pair( "button", RTCmds::Button ) );

        _cmdsMap.insert( make_pair( "/b", RTCmds::ButtonEnd ) );
        _cmdsMap.insert( make_pair( "/btn", RTCmds::ButtonEnd ) );
        _cmdsMap.insert( make_pair( "/button", RTCmds::ButtonEnd ) );

        _cmdsMap.insert( make_pair( "f", RTCmds::Font ) );
        _cmdsMap.insert( make_pair( "font", RTCmds::Font ) );

        _cmdsMap.insert( make_pair( "c", RTCmds::Color ) );
        _cmdsMap.insert( make_pair( "color", RTCmds::Color ) );

        _cmdsMap.insert( make_pair( "o", RTCmds::Offset ) );
        _cmdsMap.insert( make_pair( "offset", RTCmds::Offset ) );

        _cmdsMap.insert( make_pair( "i", RTCmds::Image ) );
        _cmdsMap.insert( make_pair( "img", RTCmds::Image ) );
        _cmdsMap.insert( make_pair( "image", RTCmds::Image ) );

        _cmdsMap.insert( make_pair( "a", RTCmds::Align ) );
        _cmdsMap.insert( make_pair( "align", RTCmds::Align ) );

        _cmdsMap.insert( make_pair( "s", RTCmds::Space ) );
        _cmdsMap.insert( make_pair( "space", RTCmds::Space ) );

        _cmdsMap.insert( make_pair( "h", RTCmds::Height ) );
        _cmdsMap.insert( make_pair( "height", RTCmds::Height ) );
        _cmdsMap.insert( make_pair( "lh", RTCmds::Height ) );
        _cmdsMap.insert( make_pair( "lineheight", RTCmds::Height ) );

        _cmdsMap.insert( make_pair( "v", RTCmds::Variant ) );
        _cmdsMap.insert( make_pair( "var", RTCmds::Variant ) );
        _cmdsMap.insert( make_pair( "variant", RTCmds::Variant ) );
    }



    // �� 0 ~ 9, a ~ f, A ~ F �� ASCII ӳ�����Ӧ�� int ֵ
    char const _hex_int[ 256 ] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        +0, +1, +2, +3, +4, +5, +6, +7, +8, +9, -1, -1, -1, -1, -1, -1,
        -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    };

    // �Խ� string( buf, len ) �ִ� ��ȫƥ�� תΪ float, ʧ�ܷ��� false
    bool tryParse( char const* buf, int len, float& outValue )
    {
        string tmp( buf, len );
        char* p;
        outValue = std::strtod( tmp.c_str(), &p );  // ò�� vs2012 strtof ������
        return p == tmp.c_str() + len;
    }

    // �Խ� string( buf, len ) �ִ� ��ȫƥ�� תΪ double, ʧ�ܷ��� false
    bool tryParse( char const* buf, int len, double& outValue )
    {
        string tmp( buf, len );
        char* p;
        outValue = strtod( tmp.c_str(), &p );
        return p == tmp.c_str() + len;
    }

    // �Խ� string( buf, len ) �ִ� ��ȫƥ�� תΪ int, ʧ�ܷ��� false
    bool tryParse( char const* buf, int len, int& outValue )
    {
        string tmp( buf, len );
        char* p;
        outValue = strtol( tmp.c_str(), &p, 10 );
        return p == tmp.c_str() + len;
    }

    // �Խ� string( buf, len ) �ִ� ��ȫƥ�� תΪ string, ʧ�ܷ��� false
    bool tryParse( char const* buf, int len, string& outValue )
    {
        if( len == 0 ) return false;
        outValue.assign( buf, len );
        return true;
    }

    // �Խ� string( buf, len ) �ִ� ��ȫƥ�� תΪ Color3B, ʧ�ܷ��� false
    bool tryParse( char const* buf, int len, Color3B& outValue )    // �� 6 �ֽڳ��� 16 ����תΪ��ɫ
    {
        if( len != 6 ) return false;
        auto b0 = _hex_int[ buf[ 0 ] ];
        auto b1 = _hex_int[ buf[ 1 ] ];
        auto b2 = _hex_int[ buf[ 2 ] ];
        auto b3 = _hex_int[ buf[ 3 ] ];
        auto b4 = _hex_int[ buf[ 4 ] ];
        auto b5 = _hex_int[ buf[ 5 ] ];
        if( b0 == -1 || b1 == -1 ||
            b2 == -1 || b3 == -1 ||
            b4 == -1 || b5 == -1 ) return false;
        outValue.r = ( b0 << 4 ) + b1;
        outValue.g = ( b2 << 4 ) + b3;
        outValue.b = ( b4 << 4 ) + b5;
        return true;
    }

    // ���ؼ���������ĸת��Сд( ������ / �� )
    void toLower( char* s, int len )
    {
        auto p = (size_t*)s;
        int count = len / sizeof( size_t );
        size_t mask = (size_t)0x2020202020202020;
        for( int i = 0; i < count; ++i ) p[ i ] |= mask;
        for( int i = count * sizeof( size_t ); i < len; ++i ) s[ i ] |= 0x20;
    }

    // ɨ�� �ո��Ʊ����з� ����
    int scanSpaces( char const* s, int len )
    {
        int i = 0;
        for( ; i < len; ++i )
        {
            auto c = s[ i ];
            if( c != ' ' &&
                c != '\r' &&
                c != '\t' &&
                c != '\n' ) return i;
        }
        return i;
    }

    // ɨ�� ָ�� ����( ���� �� ��СдӢ����ĸ �� / ʱ���� )
    int scanCommand( char const* s, int len )
    {
        int i = 0;
        for( ; i < len; ++i )
        {
            auto c = s[ i ];
            if( !( (c >= 'a' && c <= 'z')
                || (c >= 'A' && c <= 'Z')
                || c == '/' ) ) break;
        }
        return i;
    }

    // ɨ�� ���� ����( ���� ���� ] �ո�� ʱ���� )
    int scanParameter( char const* s, int len )
    {
        int i = 0;
        for( ; i < len; ++i )
        {
            auto c = s[ i ];
            if( c == '>' ||
                c == ',' ||
                c == ' ' ||
                c == '\r' ||
                c == '\t' ||
                c == '\n' ) return i;
        }
        return i;
    }

    // ɨ�� �ı�, ������䵽 outBuf �ı�������䵽 outLen, ���� ɨ�賤��
    // ����������� ��ֵ���� ������ <ָ�� �� \n ʱ��ֹ�� << ת�屻�����ı�, \r ���� )
    int scanText( char const* s, int len, char* outBuf, int& outLen )
    {
        outLen = 0;
        int i = 0;
        for( ; i < len; ++i )
        {
            auto c = s[ i ];
            if( c == '\n' ) return i;
            if( c == '<' )
            {
                if( i == len - 1 ) return -i;
                if( s[ i + 1 ] != '<' ) return i;
                else ++i;
            }
            else if( c == '\r' )
            {
                // ����
            }
            else
            {
                outBuf[ outLen++ ] = c;
            }
        }
        return i;
    }

    // �����ո�
    void jumpSpaces( Analyzer& ctx )
    {
        if( int rtv = scanSpaces( ctx.s, ctx.len ) )
        {
            ctx.s += rtv;
            ctx.len -= rtv;
        }
    }

    // �����Ҽ�����( ������ ��� �����пո� ), ��ʽ���Խ��������� false
    bool jumpRightAngleBrackets( Analyzer& ctx )
    {
        jumpSpaces( ctx );
        if( ctx.s[ 0 ] != '>' )
        {
            ctx.setError_MissRightAngleBrackets();
            return false;
        }
        ++ctx.s;
        --ctx.len;
        return true;
    }

    // ��������( ���� ǰ�� �����пո� ), ��ʽ���Խ��������� false
    bool jumpComma( Analyzer& ctx )
    {
        jumpSpaces( ctx );
        if( ctx.s[ 0 ] != ',' )
        {
            ctx.setError_MissComma();
            return false;
        }
        ++ctx.s;
        --ctx.len;
        jumpSpaces( ctx );
        return true;
    }

    // ȡ������, 0 ������������ false
    bool getParmLen( Analyzer& ctx, int& outLen )
    {
        outLen = scanParameter( ctx.s, ctx.len );
        if( outLen == 0 )
        {
            ctx.setError_MissParameter();
            return false;
        }
        return true;
    }

    // ȡ int ����. ��ʽ���Խ��������� false
    bool getParm( Analyzer& ctx, int& outVal )
    {
        int parmLen;
        if( !getParmLen( ctx, parmLen ) ) return false;
        if( !tryParse( ctx.s, parmLen, outVal ) )
        {
            ctx.setError_WrongParameterFormat( "Integer" );
            return false;
        }
        ctx.s += parmLen;
        ctx.len -= parmLen;
        return true;
    }

    // ȡ float ����. ��ʽ���Խ��������� false
    bool getParm( Analyzer& ctx, float& outVal )
    {
        int parmLen;
        if( !getParmLen( ctx, parmLen ) ) return false;
        if( !tryParse( ctx.s, parmLen, outVal ) )
        {
            ctx.setError_WrongParameterFormat( "Float" );
            return false;
        }
        ctx.s += parmLen;
        ctx.len -= parmLen;
        return true;
    }

    // ȡ string ����. ��ʽ���Խ��������� false
    bool getParm( Analyzer& ctx, string& outVal )
    {
        int parmLen;
        if( !getParmLen( ctx, parmLen ) ) return false;
        if( !tryParse( ctx.s, parmLen, outVal ) )
        {
            ctx.setError_WrongParameterFormat( "String" );
            return false;
        }
        ctx.s += parmLen;
        ctx.len -= parmLen;
        return true;
    }

    // ȡ Color3B ����. ��ʽ���Խ��������� false
    bool getParm( Analyzer& ctx, Color3B& outVal )
    {
        int parmLen;
        if( !getParmLen( ctx, parmLen ) ) return false;
        if( !tryParse( ctx.s, parmLen, outVal ) )
        {
            ctx.setError_WrongParameterFormat( "Color" );
            return false;
        }
        ctx.s += parmLen;
        ctx.len -= parmLen;
        return true;
    }


    void handleText( Analyzer& ctx, char const* s, int len )
    {
        ctx.addText( string( s, len ) );
    }

    bool handleParagraph( Analyzer& ctx, int stackSize )
    {
        float LW = 0;     // �п�
        if( *ctx.s != '>' )
        {
            if( !getParm( ctx, LW ) ) return false;
        }

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.cmds.push( RTCmds::Paragraph );
        if( !ctx.pushParagraph( LW ) ) return false;
        return true;
    }

    bool handleParagraphEnd( Analyzer& ctx, int stackSize )
    {
        if( !jumpRightAngleBrackets( ctx ) ) return false;

        if( (int)ctx.cmds.size() < stackSize
            || (ctx.cmds.size() && ctx.cmds.top() != RTCmds::Paragraph) )
        {
            ctx.setError_MissEndCommand( _cmdTexts[ (int)ctx.cmds.top() ] );
            return false;
        }
        ctx.cmds.pop();
        ctx.popParagraph();
        return true;
    }

    bool handleUnderLine( Analyzer& ctx, int stackSize )
    {
        Color3B c;
        if( *ctx.s == '>' )                             // �����κβ���, ȡĬ��ֵ
        {
            if( !ctx.getColorByIndex( 0, c ) )
            {
                ctx.setError_GetColorByIndex( 0 );
                return false;
            }
            ++ctx.s;
            --ctx.len;                                  // ���� >
            ctx.cmds.push( RTCmds::UnderLine );
            ctx.pushUnderLine( c, 0, 0 );
            return true;
        }

        if( *ctx.s == '#' )                             // # ��ͷ�� 6 �ֽڳ��� 16������ɫ��
        {
            ++ctx.s;                                    // ���� #
            --ctx.len;
            if( !getParm( ctx, c ) ) return false;
        }
        else if( *ctx.s >= '0' &&  *ctx.s <= '9' )      // ��ɫ���
        {
            int cidx;
            if( !getParm( ctx, cidx ) ) return false;
            if( !ctx.getColorByIndex( cidx, c ) )
            {
                ctx.setError_GetColorByIndex( cidx );
                return false;
            }
        }
        else                                            // ��ɫ����
        {
            string cn;
            if( !getParm( ctx, cn ) ) return false;
            if( !ctx.getColorByName( cn, c ) )
            {
                ctx.setError_GetColorByName( cn );
                return false;
            }
        }

        float thickness = 1;                                // �ߴ�
        jumpSpaces( ctx );
        if( *ctx.s == ',' )
        {
            ++ctx.s;
            --ctx.len;
            jumpSpaces( ctx );
            if( !getParm( ctx, thickness ) ) return false;
        }

        float ypos = 0;                                 // ���µ��ϵ���ʾƫ����
        jumpSpaces( ctx );
        if( *ctx.s == ',' )
        {
            ++ctx.s;
            --ctx.len;
            jumpSpaces( ctx );
            if( !getParm( ctx, ypos ) ) return false;
        }

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.cmds.push( RTCmds::UnderLine );
        ctx.pushUnderLine( c, thickness, ypos );
        return true;
    }

    bool handleUnderLineEnd( Analyzer& ctx, int stackSize )
    {
        if( !jumpRightAngleBrackets( ctx ) ) return false;

        if( (int)ctx.cmds.size() < stackSize
            || (ctx.cmds.size() && ctx.cmds.top() != RTCmds::UnderLine) )
        {
            ctx.setError_MissEndCommand( _cmdTexts[ (int)ctx.cmds.top() ] );
            return false;
        }
        ctx.cmds.pop();
        ctx.popUnderLine();
        return true;
    }

    bool handleButton( Analyzer& ctx, int stackSize )
    {
        string key;             // button click �ص� Ҫ���Ĳ���
        if( !getParm( ctx, key ) ) return false;

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.cmds.push( RTCmds::Button );
        ctx.pushButton( key );
        return true;
    }

    bool handleButtonEnd( Analyzer& ctx, int stackSize )
    {
        if( !jumpRightAngleBrackets( ctx ) ) return false;

        if( (int)ctx.cmds.size() < stackSize
            || (ctx.cmds.size() && ctx.cmds.top() != RTCmds::Button) )
        {
            ctx.setError_MissEndCommand( _cmdTexts[ (int)ctx.cmds.top() ] );
            return false;
        }
        ctx.cmds.pop();
        ctx.popButton();
        return true;
    }

    bool handleFont( Analyzer& ctx, int stackSize )
    {
        int fontIndex = 0;  // ������
        if( !getParm( ctx, fontIndex ) ) return false;

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        if( !ctx.setFont( fontIndex ) )
        {
            ctx.setError_MissFontIndex( fontIndex );
            return false;
        }
        return true;
    }

    bool handleColor( Analyzer& ctx, int stackSize )
    {
        Color3B c;                                      // ��ɫ�� 3 �ֱ�﷽ʽ
        if( *ctx.s == '#' )                             // # ��ͷ�� 6 �ֽڳ��� 16������ɫ��
        {
            ++ctx.s;                                    // ���� #
            --ctx.len;
            if( !getParm( ctx, c ) ) return false;
        }
        else if( *ctx.s >= '0' &&  *ctx.s <= '9' )      // ��ɫ���
        {
            int cidx;
            if( !getParm( ctx, cidx ) ) return false;
            if( !ctx.getColorByIndex( cidx, c ) )
            {
                ctx.setError_GetColorByIndex( cidx );
                return false;
            }
        }
        else                                            // ��ɫ����
        {
            string cn;
            if( !getParm( ctx, cn ) ) return false;
            if( !ctx.getColorByName( cn, c ) )
            {
                ctx.setError_GetColorByName( cn );
                return false;
            }
        }

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.setColor( c );
        return true;
    }

    bool handleOffset( Analyzer& ctx, int stackSize )
    {
        float offset = 0;     // ��ǰ�� ���� x ����
        if( !getParm( ctx, offset ) ) return false;

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.setOffset( offset );
        return true;
    }

    bool handleImage( Analyzer& ctx, int stackSize )
    {
        string fn;                  // sprite frame name �� file name
        bool isFileName = false;
        if( *ctx.s == '#' )         // # ��ͷ���� file name
        {
            isFileName = true;
            ++ctx.s;                // ���� #
            --ctx.len;
            if( !getParm( ctx, fn ) ) return false;
        }
        else                        // ���� sprite frame name
        {
            if( !getParm( ctx, fn ) ) return false;
        }

        float width = 0;                // ��
        jumpSpaces( ctx );
        if( *ctx.s == ',' )
        {
            ++ctx.s;
            --ctx.len;
            jumpSpaces( ctx );
            if( !getParm( ctx, width ) ) return false;
        }

        float height = 0;               // ��
        jumpSpaces( ctx );
        if( *ctx.s == ',' )
        {
            ++ctx.s;
            --ctx.len;
            jumpSpaces( ctx );
            if( !getParm( ctx, height ) ) return false;
        }

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.addImage( fn, isFileName, width, height );
        return true;
    }

    bool handleAlign( Analyzer& ctx, int stackSize )
    {
        string s;
        if( !getParm( ctx, s ) ) return false;

        auto ha = HorizontalAligns::Unknown;
        auto va = VerticalAligns::Unknown;
        auto setVal = [ &]( int i )
        {
            if( s[ i ] == 'l' ) { ha = HorizontalAligns::Left; return true; }
            else if( s[ i ] == 'c' ) { ha = HorizontalAligns::Center;  return true; }
            else if( s[ i ] == 'r' ) { ha = HorizontalAligns::Right;  return true; }
            else if( s[ i ] == 't' ) { va = VerticalAligns::Top; return true; }
            else if( s[ i ] == 'm' ) { va = VerticalAligns::Middle;  return true; }
            else if( s[ i ] == 'b' ) { va = VerticalAligns::Bottom;  return true; }
            else return false;
        };
        toLower( (char*)s.c_str(), (int)s.size() );
        if( ( s.size() == 1 && !setVal( 0 ) ) ||
            ( s.size() == 2 && ( s[ 0 ] == s[ 1 ]
            || (( s[ 0 ] == 'l' || s[ 0 ] == 'c' || s[ 0 ] == 'r' ) && ( s[ 1 ] == 'l' || s[ 1 ] == 'c' || s[ 1 ] == 'r' ))
            || (( s[ 0 ] == 't' || s[ 0 ] == 'm' || s[ 0 ] == 'b' ) && ( s[ 1 ] == 't' || s[ 1 ] == 'm' || s[ 1 ] == 'b' ))
            || !setVal( 0 ) || !setVal( 1 ) ) ) )
        {
            ctx.setError_WrongParameterFormat( "L/R/C, T/M/B" );
            return false;
        }
        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.setAlign( ha, va );
        return true;
    }

    bool handleSpace( Analyzer& ctx, int stackSize )
    {
        float width = 0, height = 0;    // �ռ���
        if( !getParm( ctx, width ) ) return false;
        jumpSpaces( ctx );
        if( *ctx.s == ',' )             // ����е�2������
        {
            ++ctx.s;
            --ctx.len;
            jumpSpaces( ctx );
            if( !getParm( ctx, height ) ) return false;
        }
        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.addSpace( width, height );
        return true;
    }

    bool handleLineHeight( Analyzer& ctx, int stackSize )
    {
        if( *ctx.s == '>' )                             // �����κβ���, Ĭ��ֵΪ 0
        {
            ++ctx.s;
            --ctx.len;                                  // ���� >
            ctx.setLineHeight( 0, false );
            return true;
        }

        bool isRatio = false;       // �Ƿ�Ϊ����
        if( *ctx.s == '*' )         // * ��ͷ���Ǳ���
        {
            isRatio = true;
            ++ctx.s;                // ���� *
            --ctx.len;
        }

        float LH = 0;               // �и�
        if( !getParm( ctx, LH ) ) return false;

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        ctx.setLineHeight( LH, isRatio );
        return true;
    }

    bool handleVariant( Analyzer& ctx, int stackSize )
    {
        string key;
        if( !getParm( ctx, key ) ) return false;

        if( !jumpRightAngleBrackets( ctx ) ) return false;

        // ��ȡ�����ı������ݲ��뵽��ǰ�����( ] ���� ) �Ա��������
        string val;
        if( !ctx.getVariant( key, val ) )
        {
            ctx.setError_GetVariant( key );
            return false;
        }
        // insert ���ܵ����ִ��ڴ��ط��� ���� s ���ٺϷ� ���ȵõ� offset
        if( val.size() )
        {
            auto currIdx = ctx.so();
            ctx.richText.insert( currIdx, val );
            ctx.s = ctx.richText.c_str() + currIdx;
            ctx.len += (int)val.size();
        }
        return true;
    }



    // ����һ��ָ�����飬���㰴 enum ����
    typedef bool( *HandleType )( Analyzer&, int );
    HandleType _handles[] =
    {
        handleParagraph,
        handleParagraphEnd,
        handleUnderLine,
        handleUnderLineEnd,
        handleButton,
        handleButtonEnd,
        handleFont,
        handleColor,
        handleOffset,
        handleImage,
        handleAlign,
        handleSpace,
        handleLineHeight,
        handleVariant
    };











    Analyzer::Analyzer()
    {
        if( !_cmdsMap.size() ) initCmdsMap();
    }

    Analyzer::Analyzer( string const& richText, int maxLen, int maxStackSize )
    {
        if( !_cmdsMap.size() ) initCmdsMap();
        assign( richText, maxLen, maxStackSize );
    }

    Analyzer& Analyzer::assign( string const& richText, int maxLen, int maxStackSize )
    {
        this->richText = richText;
        this->s = this->richText.c_str();
        this->len = ( int )this->richText.size();
        this->maxLen = maxLen;
        this->maxStackSize = maxStackSize;
        while( this->cmds.size() ) this->cmds.pop();
        return *this;
    }

    int Analyzer::so()
    {
        return int( s - richText.c_str() );
    }

    bool Analyzer::analyze()
    {
        string tmpStr, cmdStr;
        tmpStr.resize( len );
        auto outBuf = (char*)tmpStr.c_str();
        int outLen = 0;
        int rtv = 0;

        auto stackSize = (int)cmds.size();                              // ��¼��ջ���, ������������ʱ��Ҫ check, ���м�� pop �����Ҳ����С�ڸ�ֵ
        if( stackSize > maxStackSize )
        {
            setError_StackOverflow();                                   // �������趨�İ�ȫ��ջ���
            return false;
        }
        do
        {
            rtv = scanText( s, len, outBuf, outLen );                   // ɨ�ı� ֱ������ <( << ���� ) �� \n
            if( rtv < 0 )
            {
                s += -rtv + 1;
                len -= -rtv + 1;                                        // ��ָ���Ƶ������ ������ʾɶ��
                setError_MissCommand();
                return false;
            }
            if( outLen )                                                // �ɹ�ɨ���ı���������������
            {
                handleText( *this, outBuf, outLen );
                s += rtv;
                len -= rtv;
            }
            if( !len ) return true;                                     // ��� offset �����β������ ��ȷ�������

            if( *s == '\n' )                                            // ��� scanText ʱ�����˻��з�, ���в�����ɨ �ı�
            {
                ++s;
                --len;                                                  // ���� \n
                addNewLine();
                continue;                                               // ����ɨ�ı�
            }

            ++s;
            --len;                                                      // ���� <
            jumpSpaces( *this );                                        // ���� < ������ܳ��ֵĿո�

            if( !len )
            {
                setError_MissCommand();                                 // ��������ո�֮�������ı���������һ����ȱָ��
                return false;
            }

            rtv = scanCommand( s, len );                                // ɨָ�� ֱ������ �ո��>
            if( rtv == 0 )
            {
                setError_MissCommand();                                 // ɨ�� 0 �� �� ûɨ��ָ��п��������� > ���ָ���ַ�����ͷ
                return false;
            }
            cmdStr.assign( s, rtv );                                    // ��ȡ����ָ���һ�� string ����ȥ map ��
            toLower( (char*)cmdStr.c_str(), (int)cmdStr.size() );       // תСд

            auto it = _cmdsMap.find( cmdStr );
            if( it == _cmdsMap.end() )
            {
                setError_WrongCommandName( cmdStr );                    // �ֵ���û�ҵ�
                return false;
            }
            s += rtv;                                                   // �ƶ�ָ�룬ͬ�����ȣ�ָ��ָ����������
            len -= rtv;

            jumpSpaces( *this );                                        // ���� ָ�� ������ܳ��ֵĿո�
            if( !len )
            {
                setError_MissRightAngleBrackets();                      // ��������ո�֮�������ı���������һ����ȱ >
                return false;
            }

            if( !_handles[ (int)it->second ]( *this, stackSize ) )      // ����ָ��, ���ɹ����� false
            {
                return false;
            }

            if( s > richText.c_str() + maxLen )                         // �� variant replace ��ѭ��
            {
                setError_LengthOverflow();
                return false;
            }

            if( (int)cmds.size() < stackSize )                          // �����ڵݹ�ʱ�������� ���ӡ� ������
            {
                return true;
            }

        } while( len );

        if( (int)cmds.size() > stackSize )
        {
            setError_MissEndCommand( _cmdTexts[ (int)cmds.top() ] );
            return false;
        }
        return true;
    }
}
