#ifndef __COMMANDQUEUE_H__
#define __COMMANDQUEUE_H__

// push ���� �� lambda ����, �ӳٵ���ǰ��Ϸ֡����ǰ ����ִ��, ִ��ʱ����
class CommandQueue
{
public:
    MAKE_INSTANCE_H( CommandQueue );

    typedef function<void()> FT;

    // frame update ʱ����, ����ִ�� _fs �еĺ���
    void update();

    // ѹ��һ���ص�( ��������� lua bind )
    void push( FT f );

    // ѹ��һ�� lua �����ص�
    void push( LUA_FUNCTION f );

private:
    // ɶ������
    CommandQueue();

    // ɶ������
    ~CommandQueue();

    vector<FT>              _fs;
    mutex                   _m;
};


#endif

