#ifndef __EXTExtScrollView_H__
#define __EXTExtScrollView_H__

 
// ����/����/�����Թٷ��� ScrollView
class ExtScrollView : public Layer, public ActionTweenDelegate
{
public:
    // size Ϊ contentSize �� viewSize. container ������� ���Զ�����
    static ExtScrollView* create( cocos2d::Size size, Node* container = NULL );

    // �����תΪӢ��
    static float convertDistanceFromPointToInch( float pointDis );

    // ��������ƫ����x,y. animated: �Ƿ񶯻��ƶ�
    void setContentOffset( cocos2d::Vec2 offset, bool animated = false );

    // �õ�����ƫ����x,y
    cocos2d::Vec2 getContentOffset();

    // ��������ƫ����x,y. dt Ϊ�����ƶ���ʱ
    void setContentOffsetInDuration( cocos2d::Vec2 offset, float dt );

    // �������ű�
    void setZoomScale( float s );

    // �������ű� animated: �Ƿ񶯻�����
    void setZoomScale( float s, bool animated );

    // �õ����ű�
    float getZoomScale();

    // �������ű� dt: �������ź�ʱ
    void setZoomScaleInDuration( float s, float dt );

    // �õ�����ƫ������С�ߴ�( �Է��� setContentOffsetXxxxx ʱ�綨ֵ�� )
    cocos2d::Vec2 getMinContainerOffset();

    // �õ�����ƫ�������ߴ�( �Է��� setContentOffsetXxxxx ʱ�綨ֵ�� )
    cocos2d::Vec2 getMaxContainerOffset();

    // �ж�һ�� node �Ƿ���ȫ��ʾ������
    bool isNodeVisible( Node * node );

    // ����/���� touch
    void setEnabled( bool enabled );

    // �����Ƿ������� touch( Ĭ��Ϊ true )
    bool isEnabled() const;

    // ��ָ�Ƿ��ڰ�ѹ״̬
    bool isDragging() const;

    // ��ָ�Ƿ񻬶���
    bool isTouchMoved() const;

    // ����/���� ����Ч��
    void setBounceable( bool bBounceable );

    // �����Ƿ����˵���Ч��( Ĭ��Ϊ true )
    bool isBounceable() const;

    // ���� ��ͼ �ߴ�
    void setViewSize( cocos2d::Size size );
    // ��ȡ ��ͼ �ߴ�
    cocos2d::Size getViewSize() const;

    // �� 2 ��ָ zoom �����޶�
    void setZoomScaleLimit( float minScale, float maxScale );

    // �õ� ��������
    Node * getContainer();

    // ���� ��������
    void setContainer( Node * pContainer );

    // �õ� �ƶ�����
    ScrollViewDirection getDirection() const;
    
    // �����ƶ�����
    void setDirection( ScrollViewDirection eDirection );

    // 
    void updateInset();

    // �Ƿ�ü���ʾ( Ĭ��Ϊ true )
    bool isClippingToBounds() { return _clippingToBounds; }

    // �����Ƿ�ü���ʾ
    void setClippingToBounds( bool bClippingToBounds ) { _clippingToBounds = bClippingToBounds; }

    // touch �¼� ������ ���
    // ʾ��: xxxx->registerActionEventHandler( [ this, xxxxxx ]( ExtScrollView * sender, ScrollViewEvents e ) {  ... } );
    typedef function<void( ExtScrollView *, ScrollViewEvents )> ActionEventHandlerType;
    void registerActionEventHandler( LUA_FUNCTION h );
    void registerActionEventHandler( ActionEventHandlerType h );
    void unregisterActionEventHandler();

    // Provided to make scroll view compatible with SWLayer's pause method
    void pause( Ref * sender );

    // Provided to make scroll view compatible with SWLayer's resume method
    void resume( Ref * sender );

    // Overrides
    virtual void setContentSize( const cocos2d::Size & size ) override;
    virtual const cocos2d::Size& getContentSize() const override;
    virtual void visit( Renderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags ) override;
    virtual void addChild( Node * child, int zOrder, int tag ) override;

    // CCActionTweenDelegate
    void updateTweenAction(float value, const std::string& key);

	// �����ж�container�Ƿ���Ա��Ƴ�
	bool _canMovedOut;                              
	inline bool getCanMovedOut() const { return _canMovedOut; }
	inline void setCanMovedOut( bool val ) { _canMovedOut = val; }
	
protected:

    ExtScrollView();
    virtual ~ExtScrollView();
    static ExtScrollView* create();
    bool init();
    bool initWithViewSize( cocos2d::Size size, Node* container = NULL );

    bool touchBegan( Touch * t, Event * e );
    void touchMoved( Touch * t, Event * e );
    void touchEnded( Touch * t, Event * e );
    void touchCancelled( Touch * t, Event * e );

    // Relocates the container at the proper offset, in bounds of max/min offsets.
    void relocateContainer( bool animated );

    // implements auto-scrolling behavior. change SCROLL_DEACCEL_RATE as needed to choose
    // deacceleration speed. it must be less than 1.0f.
    void deaccelerateScrolling( float dt );

	// This method makes sure auto scrolling causes delegate to invoke its method
    void performedAnimatedScroll( float dt );
    // Expire animated scroll delegate calls
    void stoppedAnimatedScroll( Node * node );
    // clip this view so that outside of the visible bounds can be hidden.
    void beforeDraw();
    void onBeforeDraw();

    // retract what's done in beforeDraw so that there's no side effect to other nodes.
    void afterDraw();
    void onAfterDraw();
    
    // Zoom handling
    void handleZoom();

    cocos2d::Rect getViewRect();

    
    float _zoomScale;                               // current zoom scale
    float _minZoomScale;                            // min zoom scale
    float _maxZoomScale;                            // max zoom scale
    ScrollViewDirection _direction;
    bool _dragging;                                 // If YES, the view is being dragged.
    cocos2d::Vec2 _contentOffset;                  // Content offset. Note that left-bottom point is the origin
    Node * _container;                              // Container holds scroll view contents, Sets the scrollable container object of the scroll view
    bool _touchMoved;                               // Determiens whether user touch is moved after begin phase.
    cocos2d::Vec2 _maxInset;                       // max inset point to limit scrolling by touch
    cocos2d::Vec2 _minInset;                       // min inset point to limit scrolling by touch
    bool _bounceable;                               // Determines whether the scroll view is allowed to bounce or not.
    bool _clippingToBounds;
    cocos2d::Vec2 _scrollDistance;                 // scroll speed
    cocos2d::Vec2 _touchPoint;                     // Touch point
    float _touchLength;                             // length between two fingers
    vector<Touch*> _touches;                        // Touch objects to detect multitouch
    cocos2d::Size _viewSize;                        // size to clip. Node boundingBox uses contentSize directly.
    float _minScale, _maxScale;                     // max and min scale
    cocos2d::Rect _parentScissorRect;               // scissor rect for parent, just for restoring GL_SCISSOR_BOX
    bool _scissorRestored;                          // ���� onBeforeDraw �м�¼�Ƿ�ɹ� scissor �Ա��� afterDraw �лָ�
    EventListenerTouchOneByOne * _touchListener;    // Touch listener
	

    CustomCommand _beforeDrawCommand;
    CustomCommand _afterDrawCommand;

    ActionEventHandlerType _actionEventHandler;     // ��Ϊ������
};


#endif
