#ifndef _THREADSAFEQUEUE_H_
#define _THREADSAFEQUEUE_H_

template<typename T>
class ThreadSafeQueue
{
public:
    ThreadSafeQueue();
    ThreadSafeQueue( ThreadSafeQueue const & other );
    ThreadSafeQueue( ThreadSafeQueue && other );

    // ѹ��һ�� item
    template<typename TV>
    void push( TV && item );                        

    // ȡ��һ�� item( �����ǰ�� item �� f ���� true �Ļ�. ������� false ����û�ж���ȡ�˻����� f �� outVal ���ܱ���Ⱦ
    bool pop( T & outVal, function<bool(T const& )> f = nullptr );

    // ȡ��һ�� item. �������ֵ == nullptr ����û�ж���ȡ�ˡ�
    shared_ptr<T> pop();

    // �ȴ���ȡ��һ�� item. todo: �賬ʱʱ��
    shared_ptr<T> popWait();

    // �ȵ���Ԫ�ؿ��� pop Ϊֹ
    void popWait( T& outVal );

    // �����Ƿ��ѿ�
    bool empty() const;

    // �����
    void clear();

    // ���ض��г���
    int size() const;

    // �����⼸������ ��С��ʹ�� ��������

    // �����( ÿ�� item �����ô���� function ��һ�� )
    void clear( function<void( T& )> f );

    // foreach ɨ, ɾ�����뺯������ true ��Ԫ��, ����һ��ɾ�˶��ٸ�
    int  erase( function<bool( T& )> f );

    // foreach ���봫�뺯��. ����κ�һ������ true �򷵻� true. ���򷵻� false
    bool exists( function<bool( T const& )> f );

    // �� foreach ( �������� mutex lock )
    void foreach( function<void( T& )> f );

private:
    mutable mutex           _m;
    deque<T>                _q;
    condition_variable      _c;

    ThreadSafeQueue& operator=( const ThreadSafeQueue& );   // delete
};


#endif
