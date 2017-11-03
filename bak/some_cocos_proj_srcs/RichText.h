#ifndef _RICHTEXT_H_
#define _RICHTEXT_H_


/*

RichText ָ��˵��( ע��ָ���ڲ������ܼӿո���Ű�� ):

font config reference:
    @f[xxx]
        xxx Ϊ addFont �� font manager ��������������( ������, �ֺ�, �����Ϣ )

font color: 
    @c[xxx]
        xxx Ϊ 10���� addColor �� font manager ����ɫ��������
        ��� xxx �� �����ִ�ͷ, �� xxx ��������Ϊ ��ɫ���ñ���
    @c[#xxxxxx]
        xxxxxx Ϊ 6 �ֽڶ��� RGB 16������
     
offset:
    @o[xxx]
        ��������ݵ���ʾ x ���꣨ ����������� ��

image:
    @i[xxx]
        xxx Ϊ frame name, ����ԭʼ�ߴ���ʾ
    @I[xxx]
        xxx Ϊ file name, ����ԭʼ�ߴ���ʾ
    @i[xxx,width,height]
    @I[xxx,width,height]
        width height Ϊ������ʾ�Ŀ��, ���ĳ��ֵΪ 0, ���ֵΪ �ȱ�����Ӧ. ����ֵ��Ϊ 0 ��ͬ ԭʼ�ߴ�

*/



class RTBaseNode
{
public:
    float _x, _y, _w, _h;                       // ��ʾ����, ���
    RTBaseNode * _p;                            // ���ڵ�
    virtual void Render( Node * n ) = 0;        // ���� c2dx ������䵽 Node
    virtual Node * GetRenderer() = 0;           // �� Render ֮������������ֱ�����õ���ʾԪ��
    RTBaseNode();
    virtual ~RTBaseNode() {}
};

class WrapPanel : public RTBaseNode
{
    std::vector<RTBaseNode*> _currRowChilds;    // ��ǰ�ж���, ����ʱ, ��������, ���
    std::vector<RTBaseNode*> _childs;           // ���ж���, Container ����ʱ����
    float _rowH;                                // ��ǰ�и�
    RichTextLineVAligns _am;                              // �ӵĶ���ģʽ
public:
    float _rowX, _maxW;                         // ����ʼ x ����, ����п�
    float _rowSpacing;                          // �м��

    WrapPanel( float w, float rowSpacing = 0, RichTextLineVAligns am = RichTextLineVAligns::Bottom );
    ~WrapPanel();
    std::vector<RTBaseNode*> & Childs();
    void NewLine( float rowSpacing = 0 );
    // ���ӽڵ����, ���ýڵ�� x, �ٸ��ݽڵ��������� _rowX ( ��������� )
    RTBaseNode * AddChild( RTBaseNode * n );
    template<typename T>
    T * AddChild( T * n )
    {
        return static_cast<T*>( AddChild( static_cast<RTBaseNode*>( n ) ) );
    }
    static void SyncOffset( WrapPanel * c, float x, float y );
    void Render( Node * n );
    // �ڵ�ǰ�и��� w ��ȵĿհס�����β��ִ��
    WrapPanel & AddSpace( float w );
    // ����ǰ�����x����ֱ����Ϊһ��ƫ����
    WrapPanel & SetOffset( float offset );
    WrapPanel & AddImage( char const * frameName, char const * defaultFrameName = "", float w = 0, float h = 0 );
    WrapPanel & AddPicture( char const * fileName, float w = 0, float h = 0 );
    WrapPanel & AddRichText( std::string const & txt );
    WrapPanel & AddText( std::string const & txt, Color3B c = Color3B::WHITE, int fontIndex = 0 );
    WrapPanel & AddNode( Node * ctrl );
	
private:
    Node * GetRenderer();
    FontManager * _fm;
};


class RTNode : public RTBaseNode
{
public:
    // �ɳ�ʼ��ʵ����ʾ���
    RTNode( Node * node );
    void Render( Node * n );
    Node * GetRenderer();
private:
    Node * _node;
};


class RTImage : public RTBaseNode
{
public:
    // �ɳ�ʼ��ʵ����ʾ���
    RTImage( char const * frameName, char const * defaultFrameName = "", float w = 0, float h = 0 );
    void Render( Node * n );
    Sprite * GetRenderer();
private:
    Sprite * _sprite;
};


class RTPicture : public RTBaseNode
{
public:
    // �ɳ�ʼ��ʵ����ʾ���
    RTPicture( char const * fileName, float w = 0, float h = 0 );
    void Render( Node * n );
    Sprite * GetRenderer();
private:
    Sprite * _sprite;
};


class RTText : public RTBaseNode
{
public:
    // ��ʼ����ʾ�ı�, ��ɫ, ÿ���ֵĿ��
    RTText( std::string const & txt, Color3B color = Color3B::WHITE, int fontIndex = 0 );
    void Render( Node * n );
    Label * GetRenderer();
private:
    std::string _txt;
    int _fontIndex;
    Color3B _color;
    Label * _label;
};







class RichText : public Node
{
public:
    // ���ı� �� ������/button ֮�� �� ����ص�
    typedef function<void( string const & )> RichTextTouchHandlerType;

    // ��ȡ���ı�ƴ�ӽڵ�
    static RichText * create( string const & txt, float maxWidth, float rowSpacing = 0, RichTextLineVAligns am = RichTextLineVAligns::Bottom );

    void registerTouchEventHandler( RichTextTouchHandlerType handler);
    void registerTouchEventHandler( LUA_FUNCTION handler );
	void setText( std::string const & txt );
private:
	float _maxWidth, _rowSpacing;
	RichTextLineVAligns	_am;
    RichTextTouchHandlerType _handler;
};

#endif
