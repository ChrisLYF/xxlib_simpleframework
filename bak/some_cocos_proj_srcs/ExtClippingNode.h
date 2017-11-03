#ifndef __MISCNODE_CCCLIPPING_REGION_NODE_H__
#define __MISCNODE_CCCLIPPING_REGION_NODE_H__

// ���βü��ڵ�
class ExtClippingNode : public cocos2d::Node
{
public:
    static ExtClippingNode* create( const cocos2d::Rect& clippingRegion );
    static ExtClippingNode* create( void );

    // ȡ�òü���Χ
    const cocos2d::Rect getClippingRegion( void );

    // ���òü���Χ
    void setClippingRegion( const cocos2d::Rect& clippingRegion );

    // �ڲ���Ⱦ���ú���, �����ɵ� LUA
    virtual void visit( Renderer *renderer, const Mat4 &parentTransform, uint32_t parentFlags ) override;

    // ����ü��ڵ����� ��ק����, touch move ʱ���ø�ֵΪ true ����ֹ������ button �����
    bool isTouchMoved() const;
    void setTouchMoved( bool v );

protected:
    ExtClippingNode( void );

    /**
    * clip this view so that outside of the visible bounds can be hidden.
    */
    void beforeDraw();
    void onBeforeDraw();
    /**
    * retract what's done in beforeDraw so that there's no side effect to
    * other nodes.
    */
    void afterDraw();
    void onAfterDraw();


    cocos2d::Rect _clippingRegion;
    /**
    * scissor rect for parent, just for restoring GL_SCISSOR_BOX
    */
    cocos2d::Rect _parentScissorRect;
    bool _scissorRestored;
    bool _clippingEnabled;

    CustomCommand _beforeDrawCommand;
    CustomCommand _afterDrawCommand;

    bool _isTouchMoved;
};


#endif

