#ifndef __DESTRUCTOR_H__
#define __DESTRUCTOR_H__

// һ�� lambda ������ for Destructor
class DestructorItem
{
public:
    typedef function<void()> FuncType;

    DestructorItem();
    ~DestructorItem();
    DestructorItem( DestructorItem const & other );
    DestructorItem( DestructorItem && other );

    void add( FuncType f );

    template<typename T>
    T* add( T* p )
    {
        _funcs.push( [ p ] 
        {
            delete p;
        } );
        return p;
    }

    void run();
private:
    stack<FuncType> _funcs;
    DestructorItem & operator=( DestructorItem const & );   // delete
};


// ������ ��ջʽ�������� �� lambda ����( ʾ���� cpp �� )
class Destructor
{
public:
    MAKE_INSTANCE_H( Destructor )

    Destructor();
    ~Destructor();

    void add( DestructorItem::FuncType f );

    template<typename T>
    T* add( T* p )
    {
        if( !_top ) push();
        return _top->add( p );
    }

    void run();

    void push();

    void pop();

private:
    DestructorItem* _top;
    stack<DestructorItem> _destructors;

    Destructor( Destructor const & );
    Destructor & operator=( Destructor const & );
};


#endif
