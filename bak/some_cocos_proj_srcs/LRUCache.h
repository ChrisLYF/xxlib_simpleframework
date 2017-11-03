#ifndef __LRUCACHE_H__
#define __LRUCACHE_H__


/*

Least Recently Used ��������ʹ�� cache ��

ʾ��:

int main()
{
    // �޶� cache ����Ϊ 3
    LRUCache<string, int> c( 3 );   // ����������һ������Ԥ������

    c.add( "11", 123 );             // ��� 11
    c.dump();
    c.add( "22", 321 );             // ��� 11, 22
    c.dump();
    c.add( "33", 1212 );            // ��� 11, 22, 33
    c.dump();
    c.add( "44", 43254 );           // ��� 22, 33, 44    ( 11 ���Ƴ� )
    c.dump();
    c.add( "55", 321 );             // ��� 33, 44, 55    ( 22 ���Ƴ� )
    c.dump();
    c.get( "33" );                  // ������ 33 ����λ
    c.add( "77", 321 );             // ��� 33, 55, 77    ( 44 ���Ƴ� )
    c.dump();



ʾ�� 2:

class Foo
{
public:
    Foo() { cout << "Foo()" << endl; }
    ~Foo() { cout << "~Foo()" << endl; }
    Foo( Foo const & other ) { cout << "Foo( Foo const & other )" << endl; }
    Foo( Foo && other ) { cout << "Foo( Foo && other )" << endl; }
    Foo & operator=( Foo const & other ) { cout << "Foo & operator=( Foo const & other )" << endl; }
    Foo & operator=( Foo && other ) { cout << "Foo & operator=( Foo && other )" << endl; }
};


int main()
{
    LRUCache<string, shared_ptr<Foo>> c( 2 );
    c.add( "11", shared_ptr<Foo>( new Foo() ) );
    c.dump();
    c.add( "22", new Foo() );
    c.dump();
    c.add( "33", new Foo() );
    c.dump();
    cout << "before clear\n\n";
    c.clear();
    cout << "clear()\n\n";
    c.dump();


*/


template<typename KT, typename VT>
class LRUCache;


template<typename KT, typename VT>
class LRUCacheItem
{
public:
    template<typename PKT, typename PVT>
    LRUCacheItem( PKT && key, PVT && value )        // ��������֮���� link. ��Ȼ head �� prev next ָ��ĵ�ַ���ܲ���ȷ
        : _next( nullptr )                          // ������ָ��Ϊ��, ��Ϊ���ƺ�( ����������� ) ��Ҫ link �������������
        , _prev( nullptr )
        , _key( forward<PKT>( key ) )
        , _value( forward<PVT>( value ) )
    {
    }

    LRUCacheItem( LRUCacheItem const & other )
        : _next( nullptr )
        , _prev( nullptr )
        , _key( other._key )
        , _value( other._value )
    {
    }

    LRUCacheItem( LRUCacheItem && other )
        : _next( nullptr )
        , _prev( nullptr )
        , _key( move( other._key ) )
        , _value( move( other._value ) )
    {
    }

    LRUCacheItem & operator=( LRUCacheItem const & other )
    {
        _next = nullptr;
        _prev = nullptr;
        _key = other._key;
        _value = other._value;
    }

    LRUCacheItem & operator=( LRUCacheItem && other )
    {
        _next = nullptr;
        _prev = nullptr;
        _key = move( other._key );
        _value = move( other._value );
    }

    friend LRUCache<KT, VT>;
private:
    LRUCacheItem( LRUCacheItem * next, LRUCacheItem * prev )
        : _next( next )
        , _prev( prev )
        //, _key( KT() )            // �ƺ����س�ʼ��
        //, _value( VT() )
    {
    }

    void link( LRUCacheItem & head )
    {
        _next = &head;
        _prev = head._prev;
        head._prev->_next = this;
        head._prev = this;
    }

    void unlink()
    {
        _prev->_next = _next;
        _next->_prev = _prev;
    }

    void moveTo( LRUCacheItem & head )
    {
        unlink();
        this->_next = &head;
        this->_prev = head._prev;
        head._prev->_next = this;
        head._prev = this;
    }

    LRUCacheItem *  _next;
    LRUCacheItem *  _prev;
    KT              _key;
    VT              _value;
};


template<typename KT, typename VT>
class LRUCache
{
public:
    typedef LRUCacheItem<KT, VT> ITEM_T;

    // limit Ϊ�����޶�. ���޽��Ƴ�
    LRUCache( int limit = 100 )
        : _limit( limit )
        , _head( &_head, &_head )
    {
        _data.reserve( limit + 1 );
    }

    // add. ����ָ�� _data �д洢 value �ĵ�ַ ��  true( add�ɹ� ), false( �Ѵ��� )
    template<typename PKT, typename PVT>
    pair<VT*, bool> add( PKT && key, PVT && value, bool override = true )
    {
        pair<VT*, bool> rtv;
        auto it = _data.insert( make_pair( forward<PKT>( key ), ITEM_T( key, forward<PVT>( value ) ) ) );
        if( it.second )
        {
            it.first->second.link( _head );
            if( (int)_data.size() > _limit )
            {
                evict();
            }
            rtv.second = false;
        }
        else if( override ) // ����
        {
            it.first->second._value = forward<PVT>( value );
            rtv.second = true;
        }
        rtv.first = &it.first->second._value;
        return rtv;
    }

    // get. ˳����������, ���Ƴ�
    VT * get( KT const & key )
    {
        auto it = _data.find( key );
        if( it == _data.end() )
        {
            return nullptr;
        }
        it->second.moveTo( _head );
        return &it->second._value;
    }

    // clear
    void clear()
    {
        _data.clear();
    }

    // dump
    void dump()
    {
        for( auto &o : _data ) cout << o.first << endl;
        cout << endl;
    }

    // todo: ȱ�����Ƴ�? ȱ _data ��¶?

private:
    // �Ƴ����һ��
    void evict()
    {
        auto lastItem = _head._next;
        lastItem->unlink();
        _data.erase( lastItem->_key );
    }

    int                         _limit;
    ITEM_T                      _head;
    unordered_map<KT, ITEM_T>   _data;
};


#endif

