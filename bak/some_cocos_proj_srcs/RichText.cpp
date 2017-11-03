#include "Precompile.h"


RTBaseNode::RTBaseNode()
: _x( 0 )
, _y( 0 )
, _w( 0 )
, _h( 0 )
, _p( nullptr )
{
}


WrapPanel::WrapPanel( float w, float rowSpacing /*= 0*/, RichTextLineVAligns am /*= AlignMode_Bottom */ )
: _rowX( 0 )
, _maxW( 0 )
, _am( am )
, _rowSpacing( rowSpacing )
, _fm( FontManager::getInstance() )
{
    _w = w;
}

WrapPanel::~WrapPanel()
{
    for( auto it = _childs.begin(); it != _childs.end(); ++it )
    {
        delete ( *it );
    }
}

vector<RTBaseNode*> & WrapPanel::Childs()
{
    return _childs;
}

void WrapPanel::NewLine( float rowSpacing /*= 0 */ )
{
    _h += rowSpacing ? rowSpacing : _rowSpacing;
    _currRowChilds.clear();
    _rowX = 0;
}

WrapPanel & WrapPanel::SetOffset( float offset )
{
    _rowX = offset;
    return *this;
}

RTBaseNode * WrapPanel::AddChild( RTBaseNode * n )
{
    assert( !n->_p );
    n->_p = this;
    _childs.push_back( n );
    if( _currRowChilds.size() == 0 )
    {
        _rowH = n->_h;
        _h += n->_h;
    }
    else if( _w >= _rowX + n->_w )
    {   // �����ǰ��������, ��ʣ�������ӿ�, �� check �иߣ��������и߷����仯��ͬ�� _h
        if( n->_h > _rowH )
        {
            // �õ��߶Ȳ�, ͬ����ǰ��������
            auto val = n->_h - _rowH;
            if( _am != RichTextLineVAligns::Top )
            {
                for( auto it = _currRowChilds.begin(); it != _currRowChilds.end(); ++it )
                {
                    ( *it )->_y -= ( _am == RichTextLineVAligns::Middle ? val / 2 : val );
                }
            }
            _h += val;
            _rowH = n->_h;
        }
    }
    else
    {
        _h += n->_h;
        _rowH = n->_h;
        NewLine();
    }
    _currRowChilds.push_back( n );

    auto node = dynamic_cast<RTNode*>( n );   // node ��ê��_x Ϊ 0.5
    if( node )
    {
        n->_x = _x + _rowX + n->_w / 2;
    }
    else
    {
        n->_x = _x + _rowX;
    }
    _rowX += n->_w;
    if( _rowX > _maxW ) _maxW = _rowX;  // ��¼�����

    // ��Ϊ wp ���������֮ǰ������֪���߶ȣ���ֻ����������ݣ��ټ��뵽���������� draw ֮ǰ������������Ҫ�Ӹ���ƫ��
    auto c = dynamic_cast<WrapPanel*>( n );
    if( c )
    {                              // ������ê��������
        switch( _am )
        {
        case RichTextLineVAligns::Top:
            n->_y = _y - _h + _rowH;
            break;
        case RichTextLineVAligns::Middle:
            n->_y = _y - _h + _rowH - ( _rowH - n->_h ) / 2;
            break;
        case RichTextLineVAligns::Bottom:
            n->_y = _y - _h + n->_h;
            break;
        }
        SyncOffset( c, n->_x - _x, n->_y - _y );
    }
    else
    {
        switch( _am )                       // �� ����, �� RTNode ��ê��������
        {
        case RichTextLineVAligns::Top:
            n->_y = _y - _h + _rowH - n->_h / 2;
            break;
        case RichTextLineVAligns::Middle:
            n->_y = _y - _h + _rowH / 2;
            break;
        case RichTextLineVAligns::Bottom:
            n->_y = _y - _h + n->_h / 2;
            break;
        }
    }

    return n;
}

void WrapPanel::SyncOffset( WrapPanel * c, float x, float y )
{
    for( auto it = c->Childs().begin(); it != c->Childs().end(); ++it )
    {
        ( *it )->_x += x;
        ( *it )->_y += y;
        SyncOffset( c, x, y );
    }
}

void WrapPanel::Render( Node * n )
{
    for( auto it = _childs.begin(); it != _childs.end(); ++it )
    {
        ( *it )->Render( n );
    }
}

WrapPanel & WrapPanel::AddSpace( float w )
{
    if( _w >= _rowX + w )
    {
        _rowX += w;
    }
    return *this;
}

Node * WrapPanel::GetRenderer()
{
    return nullptr;
}



RTNode::RTNode( Node * node )
: _node( node )
{
    auto size = node->getContentSize();
    _w = size.width;
    _h = size.height;
}

void RTNode::Render( Node * n )
{
    _node->setAnchorPoint( Vec2( 0.5f, 0.5f ) );
    _node->setPosition( Vec2( _x, _y ) );

    n->addChild( _node );
}

Node * RTNode::GetRenderer()
{
    return _node;
}



RTImage::RTImage( char const * frameName, char const * defaultFrameName, float w, float h )
: _sprite( nullptr )
{
    auto frame = SpriteFrameCache::getInstance()->getSpriteFrameByName( frameName );
    if( !frame ) frame = SpriteFrameCache::getInstance()->getSpriteFrameByName( defaultFrameName );

    _sprite = Sprite::createWithSpriteFrame( frame );
    auto size = _sprite->getContentSize();
    if( w )
    {
        _sprite->setScaleX( w / size.width );
        _w = w;
    }
    else
    {
        _w = size.width;
    }
    if( h )
    {
        _sprite->setScaleY( h / size.height );
        _h = h;
    }
    else
    {
        _h = size.height;
    }
}

void RTImage::Render( Node * n )
{
    _sprite->setAnchorPoint( Vec2( 0, 0.5f ) );
    _sprite->setPosition( Vec2( _x, _y ) );

    n->addChild( _sprite );
}

Sprite * RTImage::GetRenderer()
{
    return _sprite;
}


RTPicture::RTPicture( char const * fileName, float w, float h )
: _sprite( nullptr )
{
    _sprite = Sprite::create( fileName );
    auto size = _sprite->getContentSize();
    if( w )
    {
        _sprite->setScaleX( w / size.width );
        _w = w;
    }
    else
    {
        _w = size.width;
    }
    if( h )
    {
        _sprite->setScaleY( h / size.height );
        _h = h;
    }
    else
    {
        _h = size.height;
    }
}

void RTPicture::Render( Node * n )
{
    _sprite->setAnchorPoint( Vec2( 0, 0.5f ) );
    _sprite->setPosition( Vec2( _x, _y ) );

    n->addChild( _sprite );
}

Sprite * RTPicture::GetRenderer()
{
    return _sprite;
}


RTText::RTText( string const & txt, Color3B color, int fontIndex )
: _txt( txt )
, _fontIndex( fontIndex )
, _color( color )
, _label( nullptr )
{
    _label = FontManager::getInstance()->createLabel( fontIndex, txt );
    _label->setColor( color );
    auto size = _label->getContentSize();
    _w = size.width;
    _h = size.height;
}

void RTText::Render( Node * n )
{
    _label->setAnchorPoint( Vec2( 0, 0.5f ) );
    _label->setPosition( Vec2( _x, _y ) );
    n->addChild( _label );
}

Label * RTText::GetRenderer()
{
    return _label;
}





// �� s[ offset ] λ�ÿ�ʼ�� "[...]" ���ж��� float
float getFloat( char const * s, int & offset )
{
    char buf[ 32 ];
    int i = 0;
    char c;
    ++offset;   // ���� [
    while( ( c = s[ ++offset ] ) != ']' ) buf[ i++ ] = c;
    assert( i < 32 );
    buf[ i ] = 0;
    return std::atof( buf );
}


// �� s[ offset ] λ�ÿ�ʼ�� "[...]" ���ж��� int
float getInteger( char const * s, int & offset )
{
    char buf[ 32 ];
    int i = 0;
    char c;
    ++offset;   // ���� [
    while( ( c = s[ ++offset ] ) != ']' ) buf[ i++ ] = c;
    assert( i < 32 );
    buf[ i ] = 0;
    return std::atoi( buf );
}


// �� 0 ~ 9, a ~ f, A ~ F �� ASCII ӳ�����Ӧ�� int ֵ
int const Hex2IntTable[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    2, 3, 4, 5, 6, 7, 8, 9, 0, 0,
    0, 0, 0, 0, 0, 10, 11, 12, 13, 14,
    15, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 10, 11, 12,
    13, 14, 15
};

// �� s[ offset ] λ�ÿ�ʼ�� "[...]" ���ж��� #112233 ��ɫ��� ?? ��ɫ����  ������ɫ
Color3B getFontColor( char const * s, int & offset )
{
    // ���� c[
    offset += 2;           
    if( s[ offset ] == '#' )                                    // ��ɫ��
    {
        assert( s[ offset + 7 ] == ']' );
        Color3B c3b( GLubyte( ( Hex2IntTable[ s[ offset + 1 ] ] << 4 ) + Hex2IntTable[ s[ offset + 2 ] ] ),
                     GLubyte( ( Hex2IntTable[ s[ offset + 3 ] ] << 4 ) + Hex2IntTable[ s[ offset + 4 ] ] ),
                     GLubyte( ( Hex2IntTable[ s[ offset + 5 ] ] << 4 ) + Hex2IntTable[ s[ offset + 6 ] ] ) );
        offset += 7;
        return c3b;
    }
    else if( s[ offset ] >= '0' && s[ offset ] <= '9' )         // ��ɫ����
    {
        char c;
        char buf[ 32 ];
        int i = 0;
        --offset; // move ptr to '['
        while( ( c = s[ ++offset ] ) != ']' ) buf[ i++ ] = c;
        assert( i < 32 );
        buf[ i ] = 0;
        return FontManager::getInstance()->getColor( std::atoi( buf ) );
    }
    else                                                        // ��ɫ����
    {
        char c;
        char buf[ 32 ];
        int i = 0;
        --offset; // move ptr to '['
        while( ( c = s[ ++offset ] ) != ']' ) buf[ i++ ] = c;
        assert( i < 32 );
        buf[ i ] = 0;
        return FontManager::getInstance()->getColor( buf );
    }
}

// �� s[ offset ] λ�ÿ�ʼ�� "[...]" ���ж��� frame name[, width, height] ���� image ����
RTImage * getImage( char const * s, int & offset, char const * defaultFrameName = "" )
{
    char buf[ 256 ];
    int i = 0;
    char c;
    ++offset;   // ���� [
    while( ( c = s[ ++offset ] ) != ']' ) buf[ i++ ] = c;
    assert( i < 256 );
    buf[ i ] = 0;
    float w = 0, h = 0;
    // ɨ����, Ҫôһ����û�У�Ҫô��2������λ�ü����������Ŵ���0
    auto pEnd = buf + i;
    auto pDot1 = std::find( buf, pEnd, ',' );
    if( pDot1 != pEnd )
    {
        auto pDot2 = std::find( pDot1 + 1, pEnd, ',' );
        assert( pDot2 != pEnd );    // ��Ȼ�߱�2�����ŲźϷ�
        *pDot1 = 0;
        *pDot2 = 0;
        w = std::atof( pDot1 + 1 );
        h = std::atof( pDot2 + 1 );
    }
    return new RTImage( buf, defaultFrameName, w, h );
}

// �� s[ offset ] λ�ÿ�ʼ�� "[...]" ���ж��� file name[, width, height] ���� picture ����
RTPicture * getPicture( char const * s, int & offset )
{
    char buf[ 256 ];
    int i = 0;
    char c;
    ++offset;   // ���� [
    while( ( c = s[ ++offset ] ) != ']' ) buf[ i++ ] = c;
    assert( i < 256 );
    buf[ i ] = 0;
    float w = 0, h = 0;
    // ɨ����, Ҫôһ����û�У�Ҫô��2������λ�ü����������Ŵ���0
    auto pEnd = buf + i;
    auto pDot1 = std::find( buf, pEnd, ',' );
    if( pDot1 != pEnd )
    {
        auto pDot2 = std::find( pDot1 + 1, pEnd, ',' );
        assert( pDot2 != pEnd );    // ��Ȼ�߱�2�����ŲźϷ�
        *pDot1 = 0;
        *pDot2 = 0;
        w = std::atof( pDot1 + 1 );
        h = std::atof( pDot2 + 1 );
    }
    return new RTPicture( buf, w, h );
}







WrapPanel & WrapPanel::AddImage( char const * frameName, char const * defaultFrameName, float w, float h )
{
    AddChild( new RTImage( frameName, defaultFrameName, w, h ) );
    return *this;
}

WrapPanel & WrapPanel::AddPicture( char const * fileName, float w, float h )
{
    AddChild( new RTPicture( fileName, w, h ) );
    return *this;
}

WrapPanel & WrapPanel::AddRichText( string const & txt )
{
    auto str = txt.c_str();
    string s;
    s.reserve( txt.size() );                    // ƴ��ר�� string

    auto rowLeftWidth = _w - _rowX;             // ��ǰ��ʣ���
    float currTxtWidth = 0;                     // ��ǰ�ı�ƴ�ӿ�
    auto currColor = _fm->getDefaultColor();    // ��ǰ�õ���ɫ
    auto currFontIndex = 0;
    auto maxLineHeight = 0;                     // new line ʱ�� h ����

    // ��֮ǰ�ۻ��� s ����Ϊһ�� RTText, ��������и�
    auto newRTText = [&]
    {
        auto t = new RTText( s, currColor, currFontIndex );
        AddChild( t );
        s.clear();
        if( maxLineHeight < t->_h ) maxLineHeight = t->_h;
    };

    auto pushChar = [&]( int & i )
    {
        auto p = str + i;
        int len;
        auto cw = FontManager::getInstance()->getCharSize( currFontIndex, p, len ).width;    // ȡ wchar ��ʾ���
        i += len - 1;   // for �� + 1 ������
        if( currTxtWidth + cw > rowLeftWidth )                              // ���� check
        {
            rowLeftWidth = _w;
            newRTText();
            currTxtWidth = 0;
        }
        s.append( p, len );
        currTxtWidth += cw;
    };

    for( int i = 0; i < (int)txt.size(); ++i )
    {
        auto c = (byte)str[ i ];
        if( c == '@' )
        {                        // �����ת���
            c = str[ ++i ];
            if( c == '@' )
            {
                pushChar( i );
            }
            else
            {
                switch( c ) // fcoip unb
                {
                case 'f': {
                    auto idx = getInteger( str, i );
                    if( currFontIndex != idx )              // ������巢���ı�, ��ǰ s �е��ı�����Ϊ RTText
                    {
                        if( currTxtWidth )                  // ������Ż����壬���к����壬��ֻ�������壬�������¿ؼ�
                        {
                            rowLeftWidth -= currTxtWidth;
                            newRTText();
                            currTxtWidth = 0;
                        }
                        currFontIndex = idx;
                    }
                } break;
                case 'c': {
                    auto color = getFontColor( str, i );
                    if( currColor != color )
                    {
                        if( currTxtWidth )
                        {
                            rowLeftWidth -= currTxtWidth;
                            newRTText();
                            currTxtWidth = 0;
                        }
                        currColor = color;
                    }
                } break;
                case 'o': {
                    auto x = getFloat( str, i );
                    if( currTxtWidth )
                    {
                        newRTText();
                        currTxtWidth = 0;
                    }
                    SetOffset( x );
                    rowLeftWidth = _w - x;
                } break;
                case 'i': {
                    auto img = getImage( str, i );
                    if( currTxtWidth )
                    {
                        rowLeftWidth -= currTxtWidth;
                        newRTText();
                        currTxtWidth = 0;
                    }
                    AddChild( img );
                    if( rowLeftWidth >= img->_w )
                        rowLeftWidth -= img->_w;
                    else rowLeftWidth = _w - img->_w;
                } break;
                case 'I': {
                    auto pic = getPicture( str, i );
                    if( currTxtWidth )
                    {
                        rowLeftWidth -= currTxtWidth;
                        newRTText();
                        currTxtWidth = 0;
                    }
                    AddChild( pic );
                    if( rowLeftWidth >= pic->_w )
                        rowLeftWidth -= pic->_w;
                    else rowLeftWidth = _w - pic->_w;
                } break;
                default:
                    CCLOG( "unknown command char: %c", c );
                    break;
                }
            }
        }
        else if( c == '\n' )
        {
            bool b = false;
            if( currTxtWidth )
            {
                newRTText();
                currTxtWidth = 0;
                b = true;
            }
            if( rowLeftWidth == _w && !b )
            {
                NewLine( maxLineHeight );
                maxLineHeight = 0;
            }
            else
            {
                NewLine();
                rowLeftWidth = _w;
            }
        }
        else
        {                                   // һ�����֣����ƴ, ����ͻ���
            pushChar( i );
        }
    }
    if( s.size() )
    {
        newRTText();
    }
    return *this;
}

WrapPanel & WrapPanel::AddText( string const & txt, Color3B c, int fontIndex )
{
    AddChild( new RTText( txt, c, fontIndex ) );
    return *this;
}

WrapPanel & WrapPanel::AddNode( Node * node )
{
    AddChild( new RTNode( node ) );
    return *this;
}

RichText * RichText::create( string const & txt, float maxWidth, float rowSpacing, RichTextLineVAligns am )
{
    auto ret = new RichText();
    if( ret && ret->init() )
    {
        ret->autorelease();
		ret->_maxWidth = maxWidth;
		ret->_am = am;
		ret->_rowSpacing = rowSpacing;
        WrapPanel wp( maxWidth, rowSpacing, am );
        wp.AddRichText( txt );
        wp.Render( ret );
        ret->setAnchorPoint( Vec2( 0, 1 ) );
        ret->setContentSize( Size( wp._maxW, wp._h ) );
        ret->setAnchorPoint( Vec2( 0, 0 ) );
    }
    else
    {
        CC_SAFE_DELETE( ret );
    }
    return ret;
}

void RichText::setText(std::string const & txt)
{
	this->removeAllChildrenWithCleanup( true );
	WrapPanel wp( _maxWidth, _rowSpacing, _am );
	wp.AddRichText( txt );
	wp.Render( this );
	this->setAnchorPoint( Vec2( 0, 1 ) );
	this->setContentSize( Size( wp._maxW, wp._h ) );
	this->setAnchorPoint( Vec2( 0, 0 ) );
}


















// ������ֱ���������������Ű�ľ���ʵ��
//
//Node* createNode( string const & richText, int lineWidth )
//{
//    auto fm = FontManager::getInstance();
//    auto container = Node::create();    // ������
//    vector<Node*> currNodes;            // ���ڵ�ǰ�еĽڵ�( ���ڵ�����ǰ�Ķ���ĸ߶�ƫ�� )
//    float maxLineHeight = 0;            // ��������и�
//    Vec2  currCursor( 0, 0 );           // ��ǰ"���"����λ��
//    auto currColorIndex = 0;
//    auto currFontIndex = 0;
//    string currText;                    // �ۻ��ı�( ����״̬�ı�ʱ����ȾΪ���� Label )
//    int currTextWidth = 0;              // �ۻ��ı��Ŀ��
//    currText.reserve( richText.size() );
//    auto str = richText.c_str();
//    auto strEnd = str + richText.size();
//    auto newLabel = [ &]                // ���ۻ��ı�����Ϊһ�� Label
//    {
//        auto lbl = fm->createLabel( currFontIndex, currText );
//        lbl->setAnchorPoint( Vec2( 0, 0 ) );
//        lbl->setPosition( currCursor );
//        lbl->setColor( fm->getColor( currColorIndex ) );
//        auto &sz = lbl->getContentSize();
//        currCursor.x += sz.width;
//        if( maxLineHeight < sz.height ) maxLineHeight = sz.height;
//        currNodes.push_back( lbl );
//        currText.clear();
//        currTextWidth = 0;
//    };
//    auto newLine = [ &]                 // ɨ��ǰ�����пؼ�, �����߶�. �����߶���
//    {
//        for( auto n : currNodes )
//        {
//            n->setPositionY( n->getPositionY() + maxLineHeight );
//            container->addChild( n );
//        }
//        currNodes.clear();
//        currCursor.x = 0;
//        currCursor.y -= maxLineHeight;
//        maxLineHeight = 0;
//    };
//    auto getQuoteOffset = [ &]
//    {
//        for( auto p = str; p < strEnd; ++p )
//        {
//            if( *p == ']' ) return p - str;
//        }
//        return strEnd - str;
//    };
//    while( str < strEnd )
//    {
//        if( *str == '@' )
//        {
//            ++str;
//            switch( *str )
//            {
//            case 'c':
//            {
//                str += 2;                           // ���� c[
//                int len = getQuoteOffset();
//                int colorIndex = std::atoi( str );
//                str += len + 1;                     // ���� xxx]
//                if( currColorIndex != colorIndex )
//                {
//                    newLabel();
//                    currColorIndex = colorIndex;
//                }
//                continue;
//            }
//            }
//        }
//        int utf8len;
//        auto cw = FontManager::getInstance()->getCharSize( 0, str, utf8len ).width;    // ȡ wchar ��ʾ���
//        if( currCursor.x + currTextWidth + cw <= lineWidth )   // ��ǰ�ı� ���� char �� ���û����, �����׷��
//        {
//            currTextWidth += cw;
//            currText.append( str, utf8len );
//            str += utf8len;
//            continue;
//        }
//        newLabel();
//        newLine();
//    }
//    if( currText.size() ) // ����ʣ��
//    {
//        newLabel();
//        newLine();  // ���һ�в���Ҫ����
//    }
//    return container;
//}