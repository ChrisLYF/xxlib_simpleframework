#ifndef __HOSTS_H__
#define __HOSTS_H__


// ƽ̨���, ʱ��, ��Ϸѭ���¼�����
// ��֧���� ִ�� GameLoopEvent ��ʱ�� register �� unregister
// �̳��� Node ��Ϊ�� ÿ frame ����ǰ����ִ��һЩ���� ����ע��
class Hosts : public cocos2d::Node
{
public:
    MAKE_INSTANCE_H( Hosts );

    Hosts();
    ~Hosts();

    // for c2dx schedule register
    virtual void update( float t ) override;

    // ��Ϸѭ���¼�����
    typedef function<void( float t )> GameLoopEventHandler;

    // ��Ϸѭ���¼�����
    typedef function<void()> GenericEventHandler;

    // key �ظ������־, ���Ḳ��
    void registerGameLoopEventHandler( string const &key, GameLoopEventHandler func );

    // key �ظ������־, ���Ḳ��( for lua )
    void registerGameLoopEventHandler( string const &key, LUA_FUNCTION func );

    // key �Ҳ����᷵�� false
    bool unregisterGameLoopEventHandler( string const &key );


    // ע������ɺ�̨���л���ǰ̨ʱִ�еĴ���
    void registerToForegroundEventHandler( GenericEventHandler func );

    // ע�������ǰ̨���л�����̨ʱִ�еĴ���
    void registerToBackgroundEventHandler( GenericEventHandler func );

    // ע������ɺ�̨���л���ǰ̨ʱִ�еĴ���( for lua )
    void registerToForegroundEventHandler( LUA_FUNCTION func );

    // ע�������ǰ̨���л�����̨ʱִ�еĴ���( for lua )
    void registerToBackgroundEventHandler( LUA_FUNCTION func );





    // ���ص�ǰʱ��� ���� ֵ( ��ֵÿ game loop ������һ�� )
    int getTime() const;

    // ����Lua�����Cocos2d-x״̬
    void restart();

    // Ұ����ͨ���������¼�������Լӳ�
    void addRef( Ref* o );
    void removeRef( Ref* o );



    string sdkType();
    string base64Encode(string &s);
    string base64Decode(string &s);

    //**********************************
    // ���к�����ʵ�� λ��ƽ̨������
    //**********************************

    // �����豸 UUID
    string const & getUDID() const;

    // �����豸 MAC Address
    string const & getMAC() const;

    // ���� hosts �汾�� first: minor    second: major
    // LUA ���õ��� { minor = xxxx, major = xxxx } ( �Լ�д lua bind )
    pair<int,int> const & getVersion() const;

    void enabledNotify();

    // ���豸�ڶ�����֮����֪ͨ( ����ظ������δ����֪ͨ������. ���ֻ�� android ���� )
    void notify( int groupId, int seconds, string const & content, string const & title = "", string const & soundFn = "" );

	// ��url
	void openUrl(string url);

    // �� appDelegate ����Ӧ��λ call
    GenericEventHandler _toForegroundHandler;
    GenericEventHandler _toBackgroundHandler;

private:
    //bool _callCBing;                                        // ���� call �ص��ı�־λ
    unordered_map<string, GameLoopEventHandler> _handlers;  // ���лص�
    set<Ref*> _refContainer;                                // ������ԭ��ϣ�� retain �Ķ�����
    EventListenerCustom* _toForegroundListener;
    EventListenerCustom* _toBackgroundListener;
};


#endif
