#ifndef __EXTBUTTONEX_H__
#define __EXTBUTTONEX_H__


class ExtButton;

// ʵ�ֵ����޶������
struct hasher
{
    inline size_t operator()( const pair<int, ExtButton*> & v ) const
    {
        return v.first;
    }
};
struct equalto
{
    inline bool operator()( const pair<int, ExtButton*> & _Left, const pair<int, ExtButton*> & _Right ) const
    {
        return ( _Left.first == _Right.first );
    }
};

/*
    �򵥹��ܵ� button.
    ֧�ִ��� ����, ǰ���ڵ�. �����ش�, �� ContentSize ��Ϊ�����Χ
    ֧�� ��/����, ����/��ͨ ����¼� ������ bind
    */
class ExtButton : public Layer
{
public:
    // touch �¼� ������ ���
    // ʾ��: xxxx->registerTouchEventHandler( [ this, xxxxxx ]( ExtButton * sender, ButtonTouchEvents e, float x, float y ) {  ... } );
    typedef function<void( ExtButton *, ButtonTouchEvents, float, float )> TouchEventHandlerType;
    void registerTouchEventHandler( LUA_FUNCTION h );
    void registerTouchEventHandler( TouchEventHandlerType h );
    void unregisterTouchEventHandler();

    // high light ������ ���
    // ʾ��: xxxx->registerHighlightHandler( [ this, xxxxxx ]( ExtButton * sender, bool b ) {  ... } );
    typedef function<void( ExtButton *, bool )> HighlightHandlerType;
    void registerHighlightHandler( LUA_FUNCTION h );
    void registerHighlightHandler( HighlightHandlerType h );
    void unregisterHighlightHandler();

    // enable ������ ���
    // ʾ��: xxxx->registerEnableHandler( [ this, xxxxxx ]( ExtButton * sender, bool b ) {  ... } );
    typedef function<void( ExtButton *, bool )> EnableHandlerType;
    void registerEnableHandler( LUA_FUNCTION h );
    void registerEnableHandler( EnableHandlerType h );
    void unregisterEnableHandler();

    // ����. �������봫��. ������ content size ����Ϊ touch ��Χ����. �� btn ���� scroll view ��ʱ, swallow �봫 false( �ɻ��� )
    // mutexNumber Ϊ������. ��ͬ��ŵ� btn ͬʱ���� touch ʱ, ֻ��һ������Ӧ 
    static ExtButton* create( Node * bg, Node * fg = nullptr, bool swallow = true, int mutexNumber = 1 );

    // ����ʵ��, ��Ҫ���ⲽ��ȫ�ַ� multi touch ��Ƿ��������( ���� touch down �Ļص��� visible false )
    virtual void setVisible( bool visible ) override;
    virtual void onExit() override;
	virtual void onEnter() override;


    // �����ڵ�
protected:
    Node * _bg;
public:
    Node * getBg() const;
    void setBg( Node * v );

    // ǰ���ڵ�
protected:
    Node * _fg;
public:
    Node * getFg() const;
    void setFg( Node * v );

    // ����/���� touch ʱ�Ŵ�Ч��
protected:
    bool _zoomOnTouchDown;
public:
    bool isZoomOnTouchDown() const;
    void setZoomOnTouchDown( bool b );

    // ����/���� touch
public:
    bool isEnabled() const;
    void setEnabled( bool b );

    // ����/�Ǹ���
protected:
    bool _highlighted;
public:
    bool isHighlighted() const;
    void setHighlighted( bool b );

    // �Ƿ��Ѱ���
protected:
    bool _pushed;
public:
    bool isPushed() const;

    // �Ƿ������
protected:
    bool  _swallow;
public:
    bool isSwallow() const;
    void setSwallow( bool b );


    // button state
protected:
    ButtonStates _state;
public:
    ButtonStates const & getState() const;

    friend Scheduler;
protected:
    ExtButton();
    virtual ~ExtButton();
    DELETE_COPY_ASSIGN( ExtButton );

    // for schedule frame callback
    virtual void update( float df ) override;

    // for create
    bool init( Node * bg, Node * fg, bool swallow = true, int mutexNumber = 0 );

    // �ڲ�������غ���
    void needsLayout( void );

    // for EventListenerTouchOneByOne touchListener
    bool handleTouchBegan( Touch * t, Event * e );
    void handleTouchMoved( Touch * t, Event * e );
    void handleTouchEnded( Touch * t, Event * e );
    void handleTouchCancelled( Touch * t, Event * e );

    // ���漸���������� touch handler ���õĹ��ú���
    void callTouchEventHandler( ButtonTouchEvents e, Touch *t );
	
	// �����תΪӢ��
	inline static float convertDistanceFromPointToInch( float pointDis )
	{
		auto glview = Director::getInstance()->getOpenGLView();
		float factor = ( glview->getScaleX() + glview->getScaleY() ) / 2;
		return pointDis * factor / Device::getDPI();
	}

    // �� frame ��ִ�й� touch up inside ����ı�� ( ��˫�� ), ÿ frame �� update ��������
    bool  _currFrameTouched;

    // touch ������ ���
    EventListenerTouchOneByOne * _touchListener;
    EventDispatcher * _dispatcher;

    // ���� handler lambda ����
    TouchEventHandlerType _touchEventHandler;
    HighlightHandlerType _highlightHandler;
    EnableHandlerType _enableHandler;

    // ʵ�ֵ����޶�
    int _mutexNumber;
    static unordered_set<pair<int, ExtButton*>, hasher, equalto> _mutexNumbers;
public:
    void clearTouchLock();


protected:
    // ��¼�� 1 �ε�����������ɷǵ� 1 �ε��
    Touch* _firstTouch;

};

#endif
